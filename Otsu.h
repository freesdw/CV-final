#ifndef OTSU_H
#define OTSU_H

#include "CImg.h"
#include "util.h"
#include <iostream>
#include <algorithm>
#include <cmath>
using namespace std;
using namespace cimg_library;

#define L 256           //灰度级
#define EDGE 255        //前景的灰度值

//Otsu算法实现
class Otsu
{
public:
	CImg<unsigned char> run(const CImg<unsigned char>& image, float sigma);
private:
	int getThreshold(const CImg<unsigned char>& gray);
	CImg<unsigned char> binarize(const CImg<unsigned char>& gray, int threshold);
	CImg<unsigned char> homomorphicFiltering(CImg<unsigned char>& gray);
	CImg<unsigned char> edge(const CImg<unsigned char>& biImage);
};

/**
* 封装使用Otsu方法得出边缘的过程
* image：原图像
* sigma：对原图像进行高斯模糊的参数
* return: 边缘图像
*/
CImg<unsigned char> Otsu::run(const CImg<unsigned char>& image, float sigma = 1.0) {
	CImg<unsigned char> gray = RGBtoGray(image); //转灰度图
	gray = gray.get_blur(sigma);                 //高斯模糊，去噪
	int threshold = getThreshold(gray);          //获取二值化阈值
	CImg<unsigned char> result = binarize(gray, threshold); //图像二值化
	result = edge(result);                      //提取图像边缘
	return result;
}

/**
* 提取二值图像的边缘
* biImage：二值图像
* return：提取的边缘图像
*/
CImg<unsigned char> Otsu::edge(const CImg<unsigned char>& biImage) {
	CImg<unsigned char> edgeImage(biImage);

	for(int row = 1; row < biImage.height() - 1; ++row) {
		for(int col = 1; col < biImage.width() - 1; ++col) {
			int temp = biImage(col - 1, row, 0, 0) + biImage(col + 1, row, 0, 0)
				+ biImage(col, row - 1, 0, 0) + biImage(col, row + 1, 0, 0) + biImage(col - 1, row - 1, 0, 0)
				+ biImage(col + 1, row - 1, 0, 0) + biImage(col - 1, row + 1, 0, 0) + biImage(col + 1, row + 1, 0, 0);
			if(temp == 8 * EDGE) {
				edgeImage(col, row, 0, 0) = 0;
			}
		}
	}
	return edgeImage;
}

//对图像进行同态滤波
CImg<unsigned char> Otsu::homomorphicFiltering(CImg<unsigned char>& gray) {
	CImg<double> temp = CImg<double>(gray.width(), gray.height(), 1, 1, 0);
	CImg<unsigned char> result(gray);
	cimg_forXY(temp, x, y) {
		temp(x, y, 0, 0) = log(gray(x, y, 0, 0));
	}
	for(int x = 1; x < temp.width() - 1; x++) {
		for(int y = 1; y < temp.height() - 1; y++) {
			std::vector<double> v;
			v.push_back(temp(x, y-1, 0, 0));
			v.push_back(temp(x-1, y, 0, 0));
			v.push_back(temp(x,y,0,0));
			v.push_back(temp(x+1, y, 0,0));
			v.push_back(temp(x, y+1, 0,0));
			sort(v.begin(), v.end());
			result(x,y,0,0) = (unsigned char)exp(v[2]);
		}
	}
	return result;
}

/**
* 获取图像的二值化阈值
* gray:输入的灰度图像
* return：图像的二值化阈值
*/
int Otsu::getThreshold(const CImg<unsigned char>& gray) {
	double variance = 0;
	int threshold = 0;
	double histogram[L] = {0};
	double MN = gray.width() * gray.height();
	//计算灰度直方图
	cimg_forXY(gray, x, y) {
		histogram[(int)(gray(x, y, 0, 0))]++;
	}
	//直方图归一化
	for(int i = 0; i < L; i++) {
		histogram[i] = histogram[i]/MN;
	}
	for(int k = 1; k < L; k++) {
		double P1 = 0;                //灰度值小于k的像素比例
		double P2 = 0;                //灰度值大于k的像素比例
		double m1 = 0;				  //灰度值小于k的像素集合的平均值
		double m2 = 0;				  //灰度值大于k的像素集合的平均值
		for(int i = 0; i <= k; i++) {
			P1 += histogram[i];
		}
		P2 = 1 - P1;
		if(P1 == 0 || P2 == 0) continue;
		for(int i = 0; i <= k; i++) {
			m1 += i * histogram[i];
		}
		m1 = (1/P1) * m1;
		for(int i = k + 1; i < L; i++) {
			m2 += i * histogram[i];
		}
		m2 = (1/P2) * m2;
		double temp = P1 * P2 * (m1 - m2) * (m1 - m2);
		if(variance < temp) {
			variance = temp;
			threshold = k;
		}
	}
	return threshold;
}

/**
* 图像二值化
* gray:输入的灰度图像
* threshold: 阈值，小于该值设为背景，否则设为前景
* return：图像的二值化阈值
*/
CImg<unsigned char> Otsu::binarize(const CImg<unsigned char>& gray, int threshold) {
	CImg<unsigned char> biImage = CImg<unsigned char>(gray.width(), gray.height(), 1, 1, 0);
	cimg_forXY(biImage, x, y) {
		if(gray(x, y, 0, 0) > threshold) {
			biImage(x, y, 0, 0) = EDGE;
		} else {
			biImage(x, y, 0, 0) = 0;
		}
	}
	return biImage;
}

#endif