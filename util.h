#ifndef FUNCTION_H
#define FUNCTION_H

#include "CImg.h"
#include <vector>

using namespace std;
using namespace cimg_library;


#define EDGE 255            //前景值
#define A4_WIDTH 1240       //A4纸的宽度
#define A4_HEIGHT 1754      //A4纸的高度
#define TWO_PI 360       
#define widSize 30          //非最大化抑制的窗口大小
#define PorVthreahood 10    //判断直线平行或垂直的阈值


//三维向量
class Vector3
{
public:
	Vector3() {}
	Vector3(int tx, int ty, int tz): x(tx), y(ty), z(tz){}
	//差
	Vector3 operator-(const Vector3& other) {
		return Vector3(x - other.x, y - other.y, z - other.z);
	}
	//点乘
	double dot(const Vector3& other) {
		return x*other.x + y*other.y + z*other.z;
	}
	int getX() {return x;}
	int getY() {return y;}
	int getZ() {return z;}
private:
	int x, y, z;
};

/**
* 用于记录每条线的信息
*/
struct Line
{
	double k, b;
	Line(double _k, double _b):k(_k), b(_b) {}
	int index;
	double distance;
	Line(int _index, double _distance):index(_index), distance(_distance) {}
};

//用于辅助点的排序
bool sortPoint(Vector3 a, Vector3 b) {
	return a.getY() < b.getY();
}

//用于辅助线的排序
bool sortLine(Line a, Line b) {
	return a.distance < b.distance;
}

/*
* 判断两条直线是否平行
* theta1、theta2: 两条直线的theta值
* p1、p2：两条直线的p值
* return：若两条直线平行，返回true，否则返回false 
*/
bool isParallel(int theta1, int theta2, int p1, int p2) { 
	//判断两条直线是否重合
	if(abs(theta1 - theta2) < PorVthreahood && abs(p1 - p2) < PorVthreahood) 
		return false;
	if(abs(theta1 - theta2) < PorVthreahood || abs(abs(theta1 - theta2) - 180) < PorVthreahood)
		return true;
	else return false;
}

/*
* 判断两条直线是否垂直
* theta1、theta2: 两条直线的theta值
* return：若两条直线垂直，返回true，否则返回false 
*/
bool isVertical(int theta1, int theta2) {
	if(abs(abs(theta1 - theta2) - 90) < PorVthreahood || abs(abs(theta1 - theta2) - 270) < PorVthreahood)
		return true;
	else return false;
}

/*
* 判断四条直线是否能组成个矩形，如果不能，返回出错的那条直线的下标
*/
int findUnvalidLine(vector<pair<int, int>>& result) {
	bool hasWrong = false;
	int weight[4] = {0};
	for(int i = 0; i < result.size(); i++) {
		for(int j = 0; j < result.size(); j++) {
			if(i != j) {
				int theta_1 = result[i].second, theta_2 = result[j].second, p1 = result[i].first, p2 = result[j].first;
				if(!(isVertical(theta_1, theta_2) || isParallel(theta_1, theta_2, p1, p2))) {
					hasWrong = true;
					weight[i]++;
				}
			}
		}
	}
	if(!hasWrong) return -1;
	int index = -1;
	int max = 0;
	for(int i = 0; i < result.size(); i++) {
		if(weight[i] > max) {
			max = weight[i];
			index = i;
		}
	}
	return index;
}


/*
* getMatrix
* 获取一个r*c大小的矩阵
*/
double** getMatrix(int r, int c) {
	double** matrix = new double*[r];
	for(int i = 0; i < r; i++) {
		matrix[i] = new double[c];
		memset(matrix[i], 0, c*sizeof(double));
	}
	return matrix;
}

/*
* createMatrix
* 以p1,p2,p3三点创建一个3*3的矩阵
*/
double** createMatrix(Vector3& p1, Vector3& p2, Vector3& p3) {
	double** matrix = getMatrix(3, 3);
	matrix[0][0] = p1.getX();
	matrix[0][1] = p2.getX();
	matrix[0][2] = p3.getX();
	matrix[1][0] = p1.getY();
	matrix[1][1] = p2.getY();
	matrix[1][2] = p3.getY();
	matrix[2][0] = 1;
	matrix[2][1] = 1;
	matrix[2][2] = 1;
	return matrix;
}

/*
* deleteMatrix
* 释放矩阵matrix的内存
*/
void deleteMatrix(double** matrix) {
	for(int i = 0; i < 3; i++) {
		delete[] matrix[i];
	}
	delete[] matrix;
	matrix = nullptr;
}

/*
* getDet
* 计算矩阵arcs的行列式，n为矩阵的维度
*/
double getDet(double** arcs, int n) {
	if(n == 1) return arcs[0][0];
	double ans = 0;
	double** temp = getMatrix(3, 3);
	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n - 1; j++) {
			for(int k = 0; k < n - 1; ++k) {
				temp[j][k] = arcs[j+1][k>=i?k+1:k];
			}
		}
		double t = getDet(temp, n - 1);
		if(i % 2 == 0) {
			ans += arcs[0][i] * t;
		} else {
			ans -= arcs[0][i] * t;
		}
	}
	deleteMatrix(temp);
	return ans;
}

/*
* getA*
* 计算矩阵arcs的伴随矩阵，n为矩阵维度，ans为结果矩阵
*/
void getAStart(double** arcs, int n, double** ans) {
	if(n == 1) {
		ans[0][0] = 1;
		return;
	}
	double** temp = getMatrix(3, 3);
	for(int i = 0; i < n; i++) {
		for(int j = 0; j < n; j++) {
			//计算余子式
			for(int k = 0; k < n - 1; ++k) {
				for(int t = 0; t < n - 1; ++t) {
					temp[k][t]=arcs[k>=i?k+1:k][t>=j?t+1:t];
				}
			}
			//计算代数余子式
			ans[j][i] = getDet(temp, n - 1);
			if((i + j)%2 == 1) {
				ans[j][i] *= -1;
			}
		}
	}
	deleteMatrix(temp);
}

/*
* getInverse
* 计算orignal_m矩阵的逆矩阵并返回
*/
double** getInverse(double** orignal_m) {
	double** target_m = getMatrix(3, 3);
	double det = getDet(orignal_m, 3);
	if(det == 0) {
		printf("原矩阵行列式为0，无法求逆！！\n");
		return nullptr;
	}
	double** t = getMatrix(3, 3);
	getAStart(orignal_m, 3, t);
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			target_m[i][j] = t[i][j]/det;
		}
	}
	deleteMatrix(t);
	return target_m;
}

/*
* getMatrixProduct
* 计算m1和m2两个矩阵的点乘并返回结果
*/
double** getMatrixProduct(double** m1, double** m2) {
	double** ans = getMatrix(3, 3);
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			for(int k = 0; k < 3; k++) {
				ans[i][j] += m1[i][k] * m2[k][j];
			}
		}
	}
	return ans;
}

// Determine whether point P in triangle ABC
// u = ((v1•v1)(v2•v0)-(v1•v0)(v2•v1)) / ((v0•v0)(v1•v1) - (v0•v1)(v1•v0))
// v = ((v0•v0)(v2•v1)-(v0•v1)(v2•v0)) / ((v0•v0)(v1•v1) - (v0•v1)(v1•v0))
/*
* pointInTriangle
* 判断点P是否在以A，B，C三点为顶点的三角形内
*/
bool pointInTriangle(Vector3 A, Vector3 B, Vector3 C, Vector3 P)
{
    Vector3 v0 = C - A ;
    Vector3 v1 = B - A ;
    Vector3 v2 = P - A ;

    float dot00 = v0.dot(v0) ;
    float dot01 = v0.dot(v1) ;
    float dot02 = v0.dot(v2) ;
    float dot11 = v1.dot(v1) ;
    float dot12 = v1.dot(v2) ;

    float inverDeno = 1 / (dot00 * dot11 - dot01 * dot01) ;

    float u = (dot11 * dot02 - dot01 * dot12) * inverDeno ;
    float v = (dot00 * dot12 - dot01 * dot02) * inverDeno ;

    return (u >= 0 && v >= 0 && u + v <= 1);
}

/*
* 从文件夹folder_path中读取所有文件，并将文件名及路径存入file_paths中
*/
void getFilesInFolder(string folder_path, vector<string> &file_paths) {
	long hFile = 0; //文件句柄
	struct _finddata_t fileinfo; //文件夹信息
	string p;
	if ((hFile = _findfirst(p.assign(folder_path).append("\\*").c_str(), &fileinfo)) != -1) {
		do {
			if ((fileinfo.attrib & _A_SUBDIR)) {
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
				{
					file_paths.push_back(p.assign(folder_path).append("\\").append(fileinfo.name));
					getFilesInFolder(p.assign(folder_path).append("\\").append(fileinfo.name), file_paths);
				}
			}
			else {
				file_paths.push_back(p.assign(folder_path).append("\\").append(fileinfo.name));
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}


/*
* 将RGB图像转化为灰度图像
*/
CImg<unsigned char> RGBtoGray(const CImg<unsigned char>& image) {
	CImg<unsigned char> gray = CImg<unsigned char>(image.width(), image.height(), 1, 1, 0);
	cimg_forXY(gray, x, y) {
		gray(x, y, 0, 0) = (unsigned char)(image(x, y, 0, 0) * 0.299 + image(x, y, 0, 1) * 0.587 + image(x, y, 0, 2) * 0.114);
	}
	return gray;
}

#endif