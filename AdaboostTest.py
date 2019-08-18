from sklearn.ensemble import AdaBoostClassifier
from sklearn.metrics import accuracy_score
import tensorflow.examples.tutorials.mnist.input_data as input_data
import time
import numpy as py
from sklearn.tree import DecisionTreeClassifier
from sklearn.externals import joblib

def adaboost_train():
	print('start Adaboost')
	data_dir = './MNIST_data/'
	mnist = input_data.read_data_sets(data_dir, one_hot = False)
	batch_size = 60000
	batch_im, batch_l = mnist.train.next_batch(batch_size)
	test_im = mnist.test.images[:10000]
	test_l = mnist.test.labels[:10000]	
	print('start training')
	clf_rf = AdaBoostClassifier(DecisionTreeClassifier(max_depth=5, min_samples_split=5, min_samples_leaf=5), 
								n_estimators=100, learning_rate=0.05, algorithm='SAMME.R')
	t1 = time.clock()
	clf_rf.fit(batch_im, batch_l)
	t2 = time.clock()
	print('AdaBoost training time:%.2f s' % (t2 - t1))
	joblib.dump(clf_rf, 'model/adaboost.pkl')
	print('start predict')
	y_pred_rf = clf_rf.predict(test_im)
	acc_rf = accuracy_score(test_l, y_pred_rf);
	print('score:%f' % (acc_rf))

def main():
	adaboost_train()

if __name__ == "__main__":
	main()