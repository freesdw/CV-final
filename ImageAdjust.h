#ifndef IMAGE_ADJUST_H
#define IMAGE_ADJUST_H

#include "Otsu.h"
#include "util.h"
#include <vector>
#include <cmath>
#include <fstream>
#include <utility>
using namespace std;

//封装A4纸矫正过程的类
class ImageAdjust
{
public:
	ImageAdjust(const char* fileName, float sigma, float scale);
	~ImageAdjust();
	CImg<unsigned char> run();
  void getIntersections(ofstream& out);
private:
	void houghLinesDetect();
	CImg<unsigned char> Morphing();
	unsigned char* bilinearInterpolation(CImg<unsigned char>& src, Vector3& p);
	CImg<unsigned char> image;           //原图像
	vector<Vector3> intersections;	     //原图像中，A4值的四个角点
	float sigma;                         //高斯模糊的参数
	float *sin_value;                    //sin的值
  	float *cos_value;                    //cos的值
};

/**
* 构造函数
* fileName：原图像的文件路径
* sigma：高斯模糊参数
* scale：处理图像的尺度变换（为加快处理速度，使scale < 1
*/
ImageAdjust::ImageAdjust(const char* fileName, float sigma, float scale = 1.0) {
	this->image.load(fileName);
	this->image.resize(int(image.width() * scale), int(image.height() * scale));
	this->sigma = sigma;
	sin_value = new float[TWO_PI];
   	cos_value = new float[TWO_PI]; //提前计算sin与cos的值，节省后面的计算时间
	for(int i = 0; i < TWO_PI; i++) {
     	sin_value[i] = sin(2*cimg_library::cimg::PI*i/TWO_PI);
      	cos_value[i] = cos(2*cimg_library::cimg::PI*i/TWO_PI);
   	}
}

void ImageAdjust::getIntersections(ofstream& out) {
  for(int i = 0; i < intersections.size() - 1; i++) {
    out << '(' << intersections[i].getX() << ',' << intersections[i].getY() << ')' << ' ';
  }
  out << '(' << intersections[3].getX() << ',' << intersections[3].getY() << ')' << endl;
}

//析构函数
ImageAdjust::~ImageAdjust() {
	delete sin_value;
   	delete cos_value;
}

//外部调用接口
CImg<unsigned char> ImageAdjust::run() {
	//检测出原图中A4纸的四条边及角点
	houghLinesDetect();
	//进行矫正
	return Morphing();
}

// CImg<unsigned char> ImageAdjust::homomorphicFiltering(CImg<unsigned char>& gray) {
//   CImg<double> temp = CImg<double>(gray.width(), gray.height(), 1, 1, 0);
//   CImg<unsigned char> result(gray);
//   cimg_forXY(temp, x, y) {
//     temp(x, y, 0, 0) = log(gray(x, y, 0, 0));
//   }
//   for(int x = 1; x < temp.width() - 1; x++) {
//     for(int y = 1; y < temp.height() - 1; y++) {
//       std::vector<double> v;
//       v.push_back(temp(x, y-1, 0, 0));
//       v.push_back(temp(x-1, y, 0, 0));
//       v.push_back(temp(x,y,0,0));
//       v.push_back(temp(x+1, y, 0,0));
//       v.push_back(temp(x, y+1, 0,0));
//       sort(v.begin(), v.end());
//       result(x,y,0,0) = (unsigned char)exp(v[2]);
//     }
//   }
//   return result;
// }

//对A4纸进行矫正，并将校正后结果返回
CImg<unsigned char> ImageAdjust::Morphing() {
	CImg<unsigned char> A4(A4_WIDTH, A4_HEIGHT, 1, 3, 0);  //实例化A4纸大小的图像
  // CImg<unsigned char> A4(A4_WIDTH, A4_HEIGHT, 1, 1, 0);
  // CImg<unsigned char> gray = RGBtoGray(image);
  // gray = homomorphicFiltering(gray);

	vector<Vector3> destPoints;                           //矫正后A4纸四个角点坐标，记四个角点为A、B、C、D
	destPoints.push_back(Vector3(0, 0, 0));
	destPoints.push_back(Vector3(A4_WIDTH - 1, 0, 0));
	destPoints.push_back(Vector3(0, A4_HEIGHT - 1, 0));
	destPoints.push_back(Vector3(A4_WIDTH - 1, A4_HEIGHT - 1, 0));
	//建立三角形ABC的映射
	double** A_1 = createMatrix(destPoints[0], destPoints[1], destPoints[2]);
	double** A_1_inverse = getInverse(A_1);
	double** M_1 = createMatrix(intersections[0], intersections[1], intersections[2]);
	double** pro_M_1 = getMatrixProduct(M_1, A_1_inverse);
	//建立三角形BCD的映射
	double** A_2 = createMatrix(destPoints[1], destPoints[2], destPoints[3]);
	double** A_2_inverse = getInverse(A_2);
	double** M_2 = createMatrix(intersections[1], intersections[2], intersections[3]);
	double** pro_M_2 = getMatrixProduct(M_2, A_2_inverse);
	//根据映射矩阵，将原图像中的点映射到A4纸图像中，采用双线性插值操作填补空缺像素
	cimg_forXY(A4, x, y) {
		Vector3 P(x, y, 0);
		bool in_1 = pointInTriangle(destPoints[0], destPoints[1], destPoints[2], P);
		if(in_1) {
			double u = pro_M_1[0][0]*x + pro_M_1[0][1]*y + pro_M_1[0][2];
			double v = pro_M_1[1][0]*x + pro_M_1[1][1]*y + pro_M_1[1][2];
			Vector3 p(u, v, 0);

			unsigned char* rgb = bilinearInterpolation(image, p);
			for(int k = 0; k < 3; k++) {
				A4(x, y, 0, k) = rgb[k];
			}
			delete[] rgb;
		} else {
			double u = pro_M_2[0][0]*x + pro_M_2[0][1]*y + pro_M_2[0][2];
			double v = pro_M_2[1][0]*x + pro_M_2[1][1]*y + pro_M_2[1][2];
			Vector3 p(u, v, 0);

			unsigned char* rgb = bilinearInterpolation(image, p);
			for(int k = 0; k < 3; k++) {
				A4(x, y, 0, k) = rgb[k];
			}
			delete[] rgb;
		}
	}

	deleteMatrix(A_1);
	deleteMatrix(A_1_inverse);
	deleteMatrix(M_1);
	deleteMatrix(pro_M_1);
	deleteMatrix(A_2);
	deleteMatrix(A_2_inverse);
	deleteMatrix(M_2);
	deleteMatrix(pro_M_2);
	return A4;
}

/*
* bilinearInterpolation
* 进行双线性插值操作。
* src为源图像，p为要计算的像素点位置，返回该像素点的rgb值
*/
unsigned char* ImageAdjust::bilinearInterpolation(CImg<unsigned char>& src, Vector3& p) {
	int x = static_cast<int>(p.getX());
	int y = static_cast<int>(p.getY());
	int x1 = x + 1 >= src.width() ? x : x + 1;
	int y1 = y + 1 >= src.height() ? y : y + 1;

	double a = p.getX() - x;
	double b = p.getY() - y;
	double Si, Sj;
  // Si = (1 - a)*src(x, y, 0, 0) + a*src(x1, y, 0, 0);
  // Sj = (1 - a)*src(x1, y, 0, 0) + a*src(x1, y1, 0, 0);
  // return static_cast<unsigned char>(Si + b*(Sj-Si));
	unsigned char* rgb = new unsigned char[3];
	for(int i = 0; i < 3; ++i) {
		Si = (1 - a)*src(x, y, 0, i) + a*src(x1, y, 0, i);
		Sj = (1 - a)*src(x1, y, 0, i) + a*src(x1, y1, 0, i);
		rgb[i] = static_cast<unsigned char>(Si + b*(Sj-Si));
	}
	return rgb;
}

/*******************************************************************************
* PROCEDURE: houghLinesDetec
* PURPOSE: 进行霍夫直线变换
*******************************************************************************/
void ImageAdjust::houghLinesDetect() {
	Otsu otsu;
	CImg<unsigned char> edgeIm = otsu.run(image, sigma);
//	edgeIm = otsu.edge(edgeIm);
//	edgeIm.display("edge");

	int width = image.width();
   int height = image.height();

   	int p_max = sqrt(pow(width/2, 2) + pow(height/2, 2));
   	CImg<float> hough_img = CImg<float>(p_max, TWO_PI); //初始化霍夫空间图像
   	hough_img.fill(0);
   	//将边缘图像映射到霍夫空间
   	for(int x = 0; x < width; x++) {
      	for(int y = 0; y < height; y++) {
         	int pix_value = edgeIm(x, y);
         	if(pix_value == EDGE) {
            	int p = 0;
            	int x0 = x - width/2, y0 = y - height/2;
            	for(int i = 0; i < TWO_PI; i++) {
               		p = x0 * cos_value[i] + y0*sin_value[i];
               		if(p >= 0 && p < p_max) {
                  		hough_img(p, i)++;
               		}
            	}
         	}
      	}
   	}
 //  	hough_img.display("hough_img");
   //寻找极大值，并对非极大值进行抑制
   	int size = widSize;
   	for(int i = 0; i < TWO_PI; i += size/2) {
      	for(int j = 0; j < p_max; j += size/2) {
         	int max = 0;
         	int _width = (j + size > p_max) ? p_max : j + size;
         	int _height = (i + size > TWO_PI) ? TWO_PI : i + size;
         	for(int x = j; x < _width; x++) {
            	for(int y = i; y < _height; y++) {
               		if(max > hough_img(x, y)) hough_img(x, y) = 0;
               		else max = hough_img(x, y);
            	}
         	}

      	}
   	}
   	//获取所有可能的直线
	vector<pair<int, int>> lines;
	vector<int> lineWeight;
   	cimg_forXY(hough_img, x, y) {
      	if(hough_img(x, y) != 0) {
         	lines.push_back(make_pair(x, y));
         	lineWeight.push_back(hough_img(x, y));
      	}
   	}

   	CImg<unsigned char> edge(edgeIm.width(), edgeIm.height(), 1, 3, 0);
   	CImg<unsigned char> temp(width, height, image.depth(), 1, 0);
 //  	temp.fill(0);
	//对极大值进行排序
   	vector<int> sortLineWeight = lineWeight;
   	sort(sortLineWeight.begin(), sortLineWeight.end(), greater<int>());
   	//寻找出四条边
   	vector<pair<int, int>> result;
   	int line_number = -1;
   	for(int i = 0; i < sortLineWeight.size(); i++) {
      	int weight = sortLineWeight[i], index;
      	auto iter = find(lineWeight.begin(), lineWeight.end(), weight);
      	index = iter - lineWeight.begin();
      	result.push_back(lines[index]);
      	line_number++;
      	int p = result[line_number].first, theta = result[line_number].second;
//      	printf("the formalu is -> %d = x*cos(%d) + y*sin(%d).\n", p, theta, theta);
      	if(line_number == 3) { //找出不符合条件（平行、垂直）的线
      		int index = findUnvalidLine(result);
      		if(index == -1) break; //如果所找的四条线互相垂直或平行，则认为找到了四条边
      		else {
      			result.erase(result.begin() + index);
      			--line_number;
      		}
      	}
   	}
   	//在图像上画出四条边
   	unsigned char blue[3] = {0, 0, 255};
   	unsigned char red[3] = {255, 0, 0};

   	for(int i = 0; i < result.size(); i++) {
      	int p = result[i].first, theta = result[i].second;
      	cimg_forXY(edge, x, y) {
         	int x0 = x - width/2, y0 = y - height/2;
         	if(p == (int)(x0 * cos_value[theta] + y0* sin_value[theta])) {
 //           	temp(x, y)++;
            	edge.draw_circle(x, y, 1, blue);
         	}
      	}
   	}
//   	edge.display("5");

   	vector<Vector3> tempIntersections;
  // 画出角点
   // 	for(int y = 0; y < image.height() - 2; y++) {
   //    	for(int x = 0; x < image.width() - 2; x++) {
   //       	if(temp(x, y) > 1) {
   //          	tempIntersections.push_back(Vector3(x, y, 0));
   //       	}
   //    	}
  	// }
  	
  	// //去除同一角点的重复点
  	// for(int i = 0; i < tempIntersections.size() - 1; i++) {
  	// 	for(int j = i + 1; j < tempIntersections.size(); j++) {
  	// 		double temp_x = tempIntersections[i].getX() - tempIntersections[j].getX();
			// double temp_y = tempIntersections[i].getY() - tempIntersections[j].getY();
			// double dis = sqrt(temp_x*temp_x + temp_y*temp_y);
			// if(dis < 10) {
			// 	tempIntersections.erase(tempIntersections.begin() + j--);
			// }
  	// 	}
  	// }

  	// if(tempIntersections.size() !=4) {
  	// 	tempIntersections.clear();
//利用公式计算出四个角点
   	for(int i = 0; i < result.size() - 1; i++) {
   		int p1 = result[i].first, theta1 = result[i].second;
   		for(int j = i + 1; j < result.size(); j++) {
   			int p2 = result[j].first, theta2 = result[j].second;
   			if(isVertical(theta1, theta2)) {
   				int x = int((p2 * sin_value[theta1] - p1 * sin_value[theta2]) / sin(2*cimg_library::cimg::PI*(theta1 - theta2)/TWO_PI) + width / 2);
   				int y = int((p1 * cos_value[theta2] - p2 * cos_value[theta1]) / sin(2*cimg_library::cimg::PI*(theta1 - theta2)/TWO_PI) + height / 2);
   				tempIntersections.push_back(Vector3(x, y, 0));
 //  				printf("(%d, %d)\n", x, y);
   				// if(theta1 < 1) {
   				// 	int x = p1 + width / 2;
   				// 	int y = p2 / sin_value[theta2] - p1 * cos_value[theta2] / sin_value[theta2] + height / 2;
   				// 	tempIntersections.push_back(Vector3(x, y, 0));
   				// } else if(theta2 < 1) {
   				// 	int x = p2 + width / 2;
   				// 	int y = p1 / sin_value[theta1] - p2 * cos_value[theta1] / sin_value[theta1] + height / 2;
   				// 	tempIntersections.push_back(Vector3(x, y, 0));
   				// } else {
   				// 	int x = (p1 * sin_value[theta2] - p2 * sin_value[theta1]) 
   				// 		/ (cos_value[theta1]*sin_value[theta2] - cos_value[theta2]*sin_value[theta1]);
   				// 	int y = p1 / sin_value[theta1] - cos_value[theta1]/sin_value[theta1]*x + height / 2;
   				// 	x += width / 2;
   				// 	tempIntersections.push_back(Vector3(x, y, 0));
   				// }
   			}
   		}
   	}
 //
 // 	}

  	// CImg<unsigned char> show_temp(image);
   // 	for(int i = 0; i < tempIntersections.size(); i++) {
   // 		show_temp.draw_circle(tempIntersections[i].getX(), tempIntersections[i].getY(), 5, red);
   // 	}
   // 	show_temp.display("7");
  	sort(tempIntersections.begin(), tempIntersections.end(), sortPoint);

  	vector<Line> sort_line;
	for(int i = 0; i < tempIntersections.size(); i++) {
		double temp_x = tempIntersections[i].getX() - tempIntersections[0].getX();
		double temp_y = tempIntersections[i].getY() - tempIntersections[0].getY();
		double dis = sqrt(temp_x*temp_x + temp_y*temp_y);
		Line temp(i, dis);
		sort_line.push_back(temp);
	}
	sort(sort_line.begin(), sort_line.end(), sortLine);

	//确定A、B、C、D点
	vector<Vector3> tempIntersections_2;
	for(int i = 0; i < sort_line.size(); i++) {
		tempIntersections_2.push_back(tempIntersections[sort_line[i].index]);
	}
	int A = tempIntersections_2[0].getX() >= tempIntersections_2[1].getX() ? 1 : 0;
	int B = A == 0 ? 1 : 0;
	int C = A == 0 ? 2 : 3;
	int D = A == 0 ? 3 : 2;

	intersections.push_back(tempIntersections_2[A]);
	intersections.push_back(tempIntersections_2[B]);
	intersections.push_back(tempIntersections_2[C]);
	intersections.push_back(tempIntersections_2[D]);
	// printf("intersections:\n");
	// for(int i = 0; i < intersections.size(); i++) {
	// 	printf("(%d, %d)\n", intersections[i].getX(), intersections[i].getY());
	// }
}


#endif