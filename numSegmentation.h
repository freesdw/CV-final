#ifndef NUM_SEGMENTATION_H
#define NUM_SEGMENTATION_H

#include "CImg.h"
#include "util.h"
#include "Otsu.h"
#include "Sauvola.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <direct.h>
using namespace std;
using namespace cimg_library;

#define YHistogramValleyMaxPixelNumber 0         //求Y方向直方图，谷的最少黑色像素个数
#define SubImgBlackPixelPercentage 0.006          //一张子图内黑色像素超过一定百分比才算有数字
#define XHistogramValleyMaxPixelNumber 0         //求X方向直方图，谷的最少黑色像素个数
#define MIN_NUM_H 15
#define BOUNDARY_GAP 30
#define BOUNDARY_GAP_OF_ROWIMG 8
#define NUM_GAP 5
#define DILATE_TIME 4

/*
* 对于每张图像，将里面的数字分割出来，保存进文件，并处理成MNIST数据集类似的格式
* 首先在Y方向上计算直方图，切割出每行数字
* 然后在X反向上计算直方图，切割出单个数字
* 考虑到，上面切割出来的每张图像中，由于数字重叠等问题，可能不止单个数字
* 故再对每张数字图像，进行膨胀，再根据直方图进行分割。
* 由于二值化后的数字断裂较为严重，故不使用连通阈值的方法进行切割
*/
class numSegmentation
{
public:
	void run(CImg<unsigned char> image, const char* folderName, float scale);
private:
	void divideRowImg(vector<int>& divideLineOfY);
	void incisionInY(CImg<unsigned char>& temp);
	void incisionInX(CImg<unsigned char>& subRowImg);
	void divideColImg(CImg<unsigned char>& subRowImg, vector<int>& divideLineOfX, vector<CImg<unsigned char>>& number);
	CImg<unsigned char> preProcess(CImg<unsigned char>& biImg);
	void saveInFolder(const char* folderName);
	void carve(CImg<unsigned char>& image);
	CImg<unsigned char> processNumber(CImg<unsigned char>& tempNumber);
	CImg<unsigned char> dilate(CImg<unsigned char>& image);
	CImg<unsigned char> padding(CImg<unsigned char>& num);
private:
	vector<CImg<unsigned char>> rowImg;
	CImg<int> histogramY;
	CImg<unsigned char> binaryImg;
	vector<CImg<unsigned char>> number;
};

//对图像进行预处理，将距离边缘BOUNDARY_GAP内的像素均视为背景，用于去除二值化后图像
//非A4纸部分对直方图建立的影响
CImg<unsigned char> numSegmentation::preProcess(CImg<unsigned char>& biImg) {
	CImg<unsigned char> result(biImg);
	int width = biImg.width();
	int height = biImg.height();
	cimg_forXY(result, x, y) {
		if(x <= BOUNDARY_GAP || y <= BOUNDARY_GAP || x >= width - BOUNDARY_GAP || y >= height - BOUNDARY_GAP)
			result(x,y,0,0) = BACKGROUND;
	}
	return result;
}

//将每行的切割出来的数字保存在一个文件夹中
void numSegmentation::saveInFolder(const char* folderName) {
	if(_access(folderName, 0) == -1) {
		char command[50] = {0};
		sprintf(command, "mkdir %s", folderName);
		system(command);
	}
	for(int i = 0; i < number.size(); i++) {
		char fileName[64] = {0};
		sprintf(fileName, "%s\\%d.bmp", folderName, i);
		number[i].save(fileName);
	}
}

/*
* 对数字切割过程的处理
* image : 传进来的A4值图像
* folderName:切割好的数字的存储目录，一般为原图的名字
* scale : 原图缩放的参数
*/
void numSegmentation::run(CImg<unsigned char> image, const char* folderName, float scale = 1.0) {
	Sauvola sal;
	image.resize(image.width()*scale, image.height()*scale);  //为了加快处理速度，可更改原图像的大小，再进行处理
	CImg<unsigned char> gray = RGBtoGray(image);              //转灰度图

	binaryImg = sal.run(gray);                                //获得二值化图像
	CImg<unsigned char> temp = preProcess(binaryImg);         //对temp图像进行预处理，temp图像用于直方图的建立

	incisionInY(temp);                                        //在Y方向上进行分割

	//在X方向上进行分割
	for(int i = 0; i < rowImg.size(); i++) {
		if(i > 9) continue;                              
		incisionInX(rowImg[i]);
		char fileName[50] = {0};
		sprintf(fileName, "%s\\row_%d", folderName, i + 1);
		if(number.size() > 5) saveInFolder(fileName);
		number.clear();
	}
}

//在Y方向上进行分割
void numSegmentation::incisionInY(CImg<unsigned char>& biImg) {
	histogramY = CImg<int>(biImg.width(), biImg.height(), 1, 1, 0);
	histogramY.fill(BACKGROUND);
	vector<int> inflectionPointSet;
	vector<int> divideLineOfY;
	//建立直方图
	for(int i = 1; i < biImg.height(); i++) {
		int numOfBlack = 0;
		for(int j = 0; j < biImg.width(); j++) {
			if(biImg(j, i, 0, 0) == FOREGROUND) numOfBlack++;
		}
		for(int j = 0; j < numOfBlack; j++) {
			histogramY(j, i, 0, 0) = FOREGROUND;
		}

		//判断直方图的拐点
		if(numOfBlack <= YHistogramValleyMaxPixelNumber && histogramY(0, i - 1, 0, 0) == FOREGROUND) {
			inflectionPointSet.push_back(i);
		} else if(numOfBlack > YHistogramValleyMaxPixelNumber && histogramY(0, i - 1, 0, 0) == BACKGROUND) {
			inflectionPointSet.push_back(i - 1);
		}
	}
	//计算Y方向上的切割线
	int first = inflectionPointSet[0] - NUM_GAP <= 0 ? 0 : inflectionPointSet[0] - NUM_GAP;
	divideLineOfY.push_back(first);
	if(inflectionPointSet.size() > 2) {
		for(int i = 1; i < inflectionPointSet.size() - 1; i = i + 2) {
			if(inflectionPointSet[i+1] - inflectionPointSet[i] < MIN_NUM_H) continue;
			int temp = (inflectionPointSet[i] + inflectionPointSet[i+1]) / 2;
			divideLineOfY.push_back(temp);
		}
	}
	int end = inflectionPointSet[inflectionPointSet.size()-1];
	end = end + NUM_GAP >= biImg.height() ? biImg.height() - 1 : end + NUM_GAP;
	divideLineOfY.push_back(end);
	// for(int i = 0; i < divideLineOfY.size(); i++) {
	// 	cimg_forX(histogramY, x) {
	// 		histogramY(x, divideLineOfY[i], 0, 0) = FOREGROUND;
	// 	}
	// }
	// histogramY.display();
	divideRowImg(divideLineOfY);                  //在Y方向上进行分割
}

//根据得出的Y方向上的分割线，对图像在Y方向上进行分割
void numSegmentation::divideRowImg(vector<int>& divideLineOfY) {
	for(int i = 1; i < divideLineOfY.size(); i++) {
		int subHeight = divideLineOfY[i] - divideLineOfY[i-1];      //分割后图像的高度
		CImg<unsigned char> subImg = CImg<unsigned char>(binaryImg.width(), subHeight, 1, 1, 0);
		int numOfBlack = 0;
		//进行分割
		cimg_forXY(subImg, x, y) {
			subImg(x, y, 0, 0) = binaryImg(x, divideLineOfY[i-1]+y+1, 0, 0);
			if(subImg(x, y, 0, 0) == FOREGROUND) numOfBlack++;
		}
		//计算分割后图像中，前景像素个数的占比，如果占比太小，则视为噪声
		double rate = (double)numOfBlack / (double)(binaryImg.width()*subHeight);
		if(rate > SubImgBlackPixelPercentage) {
			cimg_forXY(subImg, x, y) {
				if(x <= BOUNDARY_GAP_OF_ROWIMG || x >= subImg.width() - BOUNDARY_GAP_OF_ROWIMG)
				subImg(x,y,0,0) = BACKGROUND;
			}
			rowImg.push_back(subImg);
		}
	}
}

//在X方向上对每行数字的图像进行分割
void numSegmentation::incisionInX(CImg<unsigned char>& subRowImg) {
	CImg<unsigned char> histogramX = CImg<unsigned char>(subRowImg.width(), subRowImg.height(), 1, 1, 0);
	histogramX.fill(BACKGROUND);
	vector<int> inflectionPointSet;
	vector<int> divideLineOfX;
	//在X方向上建立直方图
	for(int x = 1; x < subRowImg.width(); x++) {
		int numOfBlack = 0;
		for(int y = 0; y < subRowImg.height(); y++) {
			if(subRowImg(x, y, 0, 0) == FOREGROUND) numOfBlack++;
		}
		for(int y = 0; y < numOfBlack; y++) {
			histogramX(x, y, 0, 0) = FOREGROUND;
		} 
		//判断拐点
		if(numOfBlack <= XHistogramValleyMaxPixelNumber && histogramX(x - 1, 0, 0, 0) == FOREGROUND) {
			inflectionPointSet.push_back(x);
		} else if(numOfBlack > XHistogramValleyMaxPixelNumber && histogramX(x - 1, 0, 0, 0) == BACKGROUND) {
			inflectionPointSet.push_back(x - 1);
		}
	}
	//寻找切割点
	divideLineOfX.push_back(inflectionPointSet[0] - 5);
	if(inflectionPointSet.size() > 2) {
		for(int i = 1; i < inflectionPointSet.size() - 1; i = i + 2) {
			int temp = (inflectionPointSet[i] + inflectionPointSet[i+1]) / 2;
			divideLineOfX.push_back(temp);
		}
	}
	divideLineOfX.push_back(inflectionPointSet[inflectionPointSet.size()-1]+5);
	//进行切割
	divideColImg(subRowImg,inflectionPointSet, number);
}

//根据切割点，在X方向上进行切割
void numSegmentation::divideColImg(CImg<unsigned char>& subRowImg, vector<int>& divideLineOfX, vector<CImg<unsigned char>>& number) {
	for(int i = 1; i < divideLineOfX.size(); i++) {
		int subWidth = divideLineOfX[i] - divideLineOfX[i-1];

		CImg<unsigned char> subImg = CImg<unsigned char>(subWidth, subRowImg.height(), 1, 1, 0);
		int numOfBlack = 0;
		cimg_forXY(subImg, x, y) {
			subImg(x, y, 0, 0) = subRowImg(divideLineOfX[i-1]+1+x, y, 0, 0);
			if(subImg(x, y, 0, 0) == FOREGROUND) numOfBlack++;
		}
		double rate = (double)numOfBlack / (double)(subRowImg.height()*subWidth);
		if(rate > SubImgBlackPixelPercentage && numOfBlack > 10) {
			carve(subImg);
		}
	}
}

//对每个数字的图像进行处理。包括框出数字，去除噪声，格式转换为MNIST数据类型等
void numSegmentation::carve(CImg<unsigned char>& num) {
	CImg<unsigned char> temp(num);
	for(int i = 0; i < DILATE_TIME; i++) {          //膨胀
		temp = dilate(temp);
	}

	CImg<unsigned char> his_x = CImg<unsigned char>(temp.width(), temp.height(), 1, 1, 0);
	his_x.fill(BACKGROUND);
	CImg<unsigned char> his_y(his_x);

	vector<int> inflectionPointSet_Y;
	vector<int> inflectionPointSet_X;
	//计算膨胀后Y方向的直方图
	for(int i = 1; i < num.height(); i++) {
		int numOfBlack = 0;
		for(int j = 0; j < num.width(); j++) {
			if(temp(j, i, 0, 0) == FOREGROUND) numOfBlack++;
		}
		for(int j = 0; j < numOfBlack; j++) {
			his_y(j, i, 0, 0) = FOREGROUND;
		}
		//判断是否为拐点
		if(numOfBlack <= 0 && his_y(0, i - 1, 0, 0) == FOREGROUND) {
			inflectionPointSet_Y.push_back(i);
		} else if(numOfBlack > 0 && his_y(0, i - 1, 0, 0) == BACKGROUND) {
			inflectionPointSet_Y.push_back(i - 1);
		}
	}
	//计算膨胀后X方向的直方图
	for(int x = 1; x < num.width(); x++) {
		int numOfBlack = 0;
		for(int y = 0; y < num.height(); y++) {
			if(temp(x, y, 0, 0) == FOREGROUND) numOfBlack++;
		}
		for(int y = 0; y < numOfBlack; y++) {
			his_x(x, y, 0, 0) = FOREGROUND;
		}
		if(numOfBlack <= 0 && his_x(x - 1, 0, 0, 0) == FOREGROUND) {
			inflectionPointSet_X.push_back(x);
		} else if(numOfBlack > 0 && his_x(x - 1, 0, 0, 0) == BACKGROUND) {
			inflectionPointSet_X.push_back(x - 1);
		}
	}
	//出错处理，上下边缘的噪声容易出现此错误
	if(inflectionPointSet_Y.size() < 2 || inflectionPointSet_X.size() < 2) {
		printf("Error in carve\n");
		return;
	}
	//找出每个数字的边框（前面切割的每个数字图像中，实际中可能含有多个数字，故作此处理）
	for(int i = 0; i < inflectionPointSet_Y.size(); i += 2) {
		for(int j = 0; j < inflectionPointSet_X.size(); j += 2) {
			int xMin = inflectionPointSet_X[j], 
				xMax = inflectionPointSet_X[j + 1], 
				yMin = inflectionPointSet_Y[i], 
				yMax = inflectionPointSet_Y[i + 1];

			if(xMax <= xMin || yMax <= yMin) continue;
			CImg<unsigned char> tempNumber = CImg<unsigned char>(xMax - xMin + 1, yMax - yMin + 1, 1, 1, 0);
			cimg_forXY(tempNumber, x, y) {
				tempNumber(x, y, 0, 0) = num(xMin + x, yMin + y, 0, 0);
			}
			tempNumber = processNumber(tempNumber);
			int numberOfWhite = 0;
			cimg_forXY(tempNumber, x, y) {
				if(tempNumber(x, y, 0, 0) > 0) numberOfWhite++;
			}
			if(numberOfWhite >= 40) {
				number.push_back(tempNumber);
			}
		}
	}
}

//对数字的图像进行处理，包括膨胀，padding为正方形和处理为MNIST数据格式
CImg<unsigned char> numSegmentation::processNumber(CImg<unsigned char>& tempNumber) {
	CImg<unsigned char> result = CImg<unsigned char>(tempNumber.width() + 8, tempNumber.height() + 8);
	result.fill(BACKGROUND);
	for(int x = 4; x < result.width() - 4; x++) {
		for(int y = 4; y < result.height() - 4; y++) {
			result(x,y,0,0) = tempNumber(x-4,y-4,0,0);
		}
	}
	result = dilate(result);
	result = dilate(result);
	//通过对数字进行膨胀，然后对边缘进行腐蚀的形式，形成前景和背景的过度
	CImg<unsigned char> temp(result);
	cimg_forXY(temp, x, y) {
		if(temp(x, y, 0, 0) == BACKGROUND) result(x, y, 0, 0) = 0;
		else {
			int sum = temp(x-1, y, 0, 0) + temp(x, y - 1, 0, 0) + temp(x, y + 1, 0, 0) + temp(x + 1, y, 0, 0)
				+ temp(x-1,y-1,0,0) + temp(x-1,y+1,0,0) + temp(x+1,y-1,0,0) + temp(x+1,y+1, 0,0);
			if(sum > FOREGROUND) result(x, y, 0, 0) = 128;
			else result(x, y, 0, 0) = 255;
		}
	}

	result = padding(result);
	result.resize(28,28);
	return result;
}

//对数字图像进行扩充，即扩充为正方形。
CImg<unsigned char> numSegmentation::padding(CImg<unsigned char>& num) {
	int width = num.width(), height = num.height();
	//对宽度和高度，选择较大的作为扩充后的结果
	if(width > height) {
		CImg<unsigned char> result = CImg<unsigned char>(width, width, 1, 1, 0);
		result.fill(0);
		int padding_size = (width - height) / 2;
		for(int y = padding_size; y < width - padding_size; y++) {
			for(int x = 0; x < width; x++) {
				result(x,y,0,0) = num(x, y - padding_size, 0, 0);
			}
		}	
		return result;
	} else {
		CImg<unsigned char> result = CImg<unsigned char>(height, height, 1, 1, 0);
		result.fill(0);
		int padding_size = (height - width) / 2;
		for(int x = padding_size; x < height - padding_size; x++) {
			for(int y = 0; y < height; y++) {
				result(x, y, 0, 0) = num(x - padding_size, y, 0, 0);
			}
		}
		return result;
	}
}

//对图像进行膨胀
CImg<unsigned char> numSegmentation::dilate(CImg<unsigned char>& image) {
	CImg<unsigned char> result(image);
	for(int x = 1; x < image.width() - 1; x++) {
		for(int y = 1; y < image.height() - 1; y++) {
			if(image(x,y,0,0) == FOREGROUND) continue;
			int sum = image(x-1,y, 0,0) + image(x+1,y,0,0) + image(x-1,y-1,0,0)+image(x-1,y+1,0,0)
				+ image(x,y-1,0,0) + image(x,y+1,0,0)+image(x+1,y+1,0,0)+image(x+1,y-1,0,0);
			if(sum < 8*BACKGROUND) result(x,y,0,0) = FOREGROUND;
		}
	}
	return result;
}

#endif