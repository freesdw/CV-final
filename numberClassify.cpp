#include "ImageAdjust.h"
#include <string>
#include <fstream>
#include "numSegmentation.h"
#include <vector>
#include "util.h"

using namespace std;

#define SIGMA 1.0
#define SCALE 0.25

void numberClassify(string path, const char* msgFile) {	
	ofstream file(msgFile);
	if(file == nullptr) {
		printf("Failed to open %s\n", msgFile);
		exit(1);
	}

	vector<string> filenames;
	getFilesInFolder(path, filenames);
	for(int i = 0; i < filenames.size(); i++) {
		int folderLength = filenames[i].length()-path.length() - 5;
		string imageName = filenames[i].substr(path.length() + 1);
		file << imageName << ' ';
		string folderName = filenames[i].substr(path.length() + 1, folderLength);
		if(_access(folderName.c_str(), 0) == -1) {
			char command[50] = {0};
			sprintf(command, "mkdir %s", folderName.c_str());
			system(command);
		}
		file << folderName << ' ';
		ImageAdjust adjust(filenames[i].c_str(), SIGMA, SCALE);
		CImg<unsigned char> A4 = adjust.run();
		adjust.getIntersections(file);
		char A4_name[50] = {0};
		sprintf(A4_name, "A4\\%s", imageName.c_str());
		A4.save(A4_name);
		numSegmentation seg;
		seg.run(A4, folderName.c_str());
	}
}

int main(int argc, char const *argv[])
{
	string path = "data\\";
	numberClassify(path, "Dir.txt");
	return 0;
}