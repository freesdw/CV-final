说明：
1、为减少存储空间，data中的数据已全部转换为jpg格式，运行程序时需将所有转换为bmp格式
2、为了减少运行时间，在A4纸矫正过程中，已经原本图像缩放为原来的四分之一再处理，故得出的
   四个角点的坐标可能与用原图像进行矫正不一致。
3、model文件夹中位已经训练好的模型

4、文件说明：
CImg.h : CImg类库文件
ImageAdjust.h : A4纸矫正过程的类
numSegmentation.h : 切割出A4纸中数字的类
Otsu.h : 使用Otsu算法进行边缘检测的类
Sauvola.h : 使用自适应阈值算法对A4纸进行二值化的类
util.h ： 辅助函数库
numberClassify.cpp : 实现整个分割过程的文件
AdaboostTest.py : Adaboost模型训练文件
MLPTest.py : MLP模型训练文件
classify.py : 对data中所有图像进行识别，且将结果保存为Excel文件
adaboost_result.xls : 使用adaboost模型识别结果
mlp_result.xls : 使用MLP模型识别结果
label.txt : 对图像中数字的标识

5、有些图像上，重复了多次学号、手机号、身份证号等信息，在这里，处理时当作所有信息均是有效的，即如果
   一张图像上有三组信息，即使是同一个人的，这里也当作是有三个学生的信息来处理。
