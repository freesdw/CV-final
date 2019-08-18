import csv
import struct
import os
import numpy as np
from PIL import Image
from sklearn.metrics import accuracy_score
from sklearn.neural_network import MLPClassifier
from sklearn.externals import joblib
import pandas as pd
import xlwt

#从索引文件中获得所有需要处理的文件夹名字及一些处理过程中的信息（图像名，四个角点坐标）
def getImageDir():
	ImageDir = []
	index = open('./Dir.txt')
	msg = index.readline()
	while msg:
		msg = msg.strip('\n')
		ImageDir.append(msg)
		msg = index.readline()
	index.close()
	return ImageDir

#根据图像路径，装载图像
def get_image(imgPath):
	img = Image.open(imgPath)
	img = img.convert('L').resize((28, 28))
	img_np = np.array(img)
	img_np = img_np.reshape(1, 784)
	return img_np

#使用识别器，对folderName目录中数字进行识别并返回
def classify(folderName, classifier):
	numberOfSeq = []
	for roots, dirs, files in os.walk(folderName):
		for folder in dirs:
			numSequence = ''
			#遍历图像目录里面的行图像目录，得出每个数字并识别
			for root, folders, imgs in os.walk(os.path.join(roots, folder)):
				for img in imgs:
					imgPath = os.path.join(root, img)
					image = get_image(imgPath)
					res = str(int(classifier.predict(image)))
					numSequence = numSequence + res
			numberOfSeq.append(numSequence)
	return numberOfSeq

#使用adaboost算法识别
def adaboost_recognition(model_path):
	if(os.path.isfile(model_path)): #装载模型
		adaboost_classifier = joblib.load(model_path)
		f = xlwt.Workbook()
		sheet1 = f.add_sheet(u'sheet1', cell_overwrite_ok=True)  #建立Excel文件用于存放结果
		row0 = [u'文件名',u'角点1',u'角点2',u'角点3',u'角点4',u'学号',u'手机号',u'身份证号']
		for i in range(0, len(row0)):
			sheet1.write(0,i,row0[i])
		images = getImageDir()
		numOfRow = 0 
		for i in range(0, len(images)):        #进行识别和存放结果
			str = images[i]
			msgs = str.split(' ')
			folderName = msgs[1]
			newRow = []
			newRow.append(msgs[0])
			newRow.append(msgs[2])
			newRow.append(msgs[3])
			newRow.append(msgs[4])
			newRow.append(msgs[5])
			numSequences = classify(folderName, adaboost_classifier)
			for j in range(0, len(numSequences)):
				if(j % 3 == 0):
					numOfRow = numOfRow + 1;
					for x in range(0, len(newRow)):
						sheet1.write(numOfRow, x, newRow[x])
				sheet1.write(numOfRow, (j % 3) + 5, numSequences[j])
		f.save('adaboost_result.xls')
	else:
		print('No trained model of adaboost')

#使用MLP算法进行识别		
def MLP_recognition(model_path):
	if(os.path.isfile(model_path)):
		mlp_classifier = joblib.load(model_path)
		f = xlwt.Workbook()
		sheet1 = f.add_sheet(u'sheet1', cell_overwrite_ok=True)
		row0 = [u'文件名',u'角点1',u'角点2',u'角点3',u'角点4',u'学号',u'手机号',u'身份证号']
		for i in range(0, len(row0)):
			sheet1.write(0,i,row0[i])
		images = getImageDir()
		numOfRow = 0
		for i in range(0, len(images)):
			str = images[i]
			msgs = str.split(' ')
			folderName = msgs[1]
			newRow = []
			newRow.append(msgs[0])
			newRow.append(msgs[2])
			newRow.append(msgs[3])
			newRow.append(msgs[4])
			newRow.append(msgs[5])
			numSequences = classify(folderName, mlp_classifier)
			for j in range(0, len(numSequences)):
				if(j % 3 == 0):
					numOfRow = numOfRow + 1;
					for x in range(0, len(newRow)):
						sheet1.write(numOfRow, x, newRow[x])
				sheet1.write(numOfRow, (j % 3) + 5, numSequences[j])
		f.save('mlp_result.xls')
	else:
		print('No trained model of mlp')

#main函数
def main():
	print('classiy...');
	print('classifying by adaboost...');
	adaboost_recognition('./model/adaboost.pkl');
	print('classifying by mlp...');
	MLP_recognition('./model/mlp.pkl');
	
if __name__ == '__main__':
	main()