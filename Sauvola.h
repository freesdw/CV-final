#ifndef SAUVOLA_H
#define SAUVOLA_H
#include "CImg.h"
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;
using namespace cimg_library;
#define WIN_SIZE 31          //求自适应阈值时窗口大小
#define RATE 0.9             //阈值设置参数
#define BACKGROUND 255       //背景像素值
#define FOREGROUND 0         //前景像素值
#define MEDIAN_MASK 3        //中值滤波窗口大小

/*
* 使用自适应阈值方法，对图像进行二值化
*/
class Sauvola
{
public:
	CImg<unsigned char> run(CImg<unsigned char>& image);
private:
	CImg<unsigned char> binarize(const CImg<unsigned char>& image);
	CImg<unsigned char> filter(CImg<unsigned char>& image);
	CImg<unsigned char> medianFilter(CImg<unsigned char>& image);
	CImg<unsigned char> filterA(CImg<unsigned char>& image);
	CImg<unsigned char> filterB(CImg<unsigned char>& image);
};

//封装二值化的过程
CImg<unsigned char> Sauvola::run(CImg<unsigned char>& image) {
//	image.display();
	CImg<unsigned char> biImg = binarize(image);
	biImg = filter(biImg);
//	biImg.display();
	return biImg;
}

//用3*3模板对图像进行腐蚀
CImg<unsigned char> Sauvola::filterA(CImg<unsigned char>& image) {
	CImg<unsigned char> result(image);
	for(int x = 1; x < image.width() - 1; x++) {
		for(int y = 1; y < image.height() - 1; y++) {
			if(image(x,y,0,0) == BACKGROUND) continue;
			int sum = image(x, y-1,0,0) + image(x,y+1, 0, 0) + image(x-1,y-1,0,0)+image(x-1,y+1,0,0)
				+ image(x-1,y,0,0) + image(x+1,y,0,0) + image(x+1,y+1,0,0)+image(x+1,y-1,0,0);
			if(sum > 0) result(x,y, 0,0) = BACKGROUND;
		}		
	}
	return result;
}
//用3*3模板对图像进行膨胀
CImg<unsigned char> Sauvola::filterB(CImg<unsigned char>& image) {
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
//以MEDIAN_MASK大小的窗口对图像进行中值滤波
CImg<unsigned char> Sauvola::medianFilter(CImg<unsigned char>& image) {
	CImg<unsigned char> result = CImg<unsigned char>(image.width(), image.height(), 1, 1, 0);
	cimg_forXY(image, x, y) {
		int tempx_l = x - MEDIAN_MASK / 2 < 0 ? 0 : x - MEDIAN_MASK / 2;
		int tempx_r = x + MEDIAN_MASK / 2 >= image.width() ? image.width() - 1 : x + MEDIAN_MASK / 2;
		int tempy_b = y + MEDIAN_MASK / 2 >= image.height() ? image.height() - 1 : y + MEDIAN_MASK / 2;
		int tempy_u = y - MEDIAN_MASK / 2 < 0 ? 0 :  y - MEDIAN_MASK / 2;
		vector<unsigned char> pixels;
		for(int i = tempx_l; i <= tempx_r; i++) {
			for(int j = tempy_u; j <= tempy_b; j++) {
				pixels.push_back(image(i,j, 0, 0));
			}
		}
		sort(pixels.begin(), pixels.end());
		result(x,y,0,0) = pixels[(int)(pixels.size()/2)];
	}
	return result;
}
//对图像进行滤波。通过几个滤波器的结合，得到去噪结果。此处进行形态学闭运算
CImg<unsigned char> Sauvola::filter(CImg<unsigned char>& image) {
	CImg<unsigned char> result(image);
	//result = medianFilter(result);
	result = filterB(result);
	result = filterA(result);

	return result;
}
//对图像进行二值化，使用积分图减少运算。对Sauvola算法进行更改，设阈值T = RATE * MEAN
CImg<unsigned char> Sauvola::binarize(const CImg<unsigned char>& image) {
	CImg<unsigned char> result = CImg<unsigned char>(image.width(), image.height(), 1, 1, 0);
	//计算积分图像
	CImg<int> integralImg = CImg<int>(image.width(), image.height(), 1, 1, 0);
	for(int y = 0; y < image.height(); ++y) {
		int sum = 0;
		for(int x = 0; x < image.width(); x++) {
			sum += image(x,y,0,0);
			if(y == 0) integralImg(x,y,0,0) = sum;
			else integralImg(x,y,0,0) = integralImg(x,y-1,0,0) + sum;
		}
	}
	//利用积分图像计算阈值
	cimg_forXY(image, x, y) {
		float tempx_l = (x - WIN_SIZE / 2) < 0 ? 0 : x - WIN_SIZE / 2;
		float tempx_r = (x + WIN_SIZE / 2) >= image.width() ? image.width() - 1 : x + WIN_SIZE / 2;
		float tempy_b = (y + WIN_SIZE / 2) >= image.height() ? image.height() - 1 : y + WIN_SIZE / 2;
		float tempy_u = (y - WIN_SIZE / 2) < 0 ? 0 : y - WIN_SIZE / 2;
		int count = (int)(tempx_r - tempx_l + 1) * (tempy_b - tempy_u + 1);
		if(tempx_l < 1) tempx_l = 1;
		if(tempy_u < 1) tempy_u = 1;
		if(tempx_r < 1) tempx_r = 1;
		double sum = integralImg((int)tempx_r, (int)tempy_b, 0, 0) - integralImg((int)tempx_r, (int)tempy_u-1, 0, 0)
			- integralImg((int)tempx_l - 1, (int)tempy_b, 0, 0) + integralImg((int)tempx_l-1, (int)tempy_u-1, 0, 0);
		double mean =  sum / count;

		result(x, y, 0, 0) = image(x, y, 0, 0) >= RATE*mean ? BACKGROUND : FOREGROUND;
	}		
	return result;
}

#endif