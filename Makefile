all:numberClassify.cpp CImg.h
	rm -f numberClassify.exe
	g++ -g -std=c++11 -o numberClassify.exe Otsu.h numberClassify.cpp CImg.h ImageAdjust.h util.h numSegmentation.h Sauvola.h -O2 -lgdi32
