from sklearn.metrics import accuracy_score
import tensorflow.examples.tutorials.mnist.input_data as input_data
import time
import numpy as py
from sklearn.neural_network import MLPClassifier
from sklearn.externals import joblib

def MLP_training():
	data_dir = './MNIST_data/'
	mnist = input_data.read_data_sets(data_dir, one_hot = False)
	batch_size = 60000
	batch_im, batch_l = mnist.train.next_batch(batch_size)
	test_im = mnist.test.images[:10000]
	test_l = mnist.test.labels[:10000]
	print('start MLP')
	model = MLPClassifier(hidden_layer_sizes=(400, 200), activation='logistic', 
				solver='sgd', learning_rate_init=0.01, max_iter=200, verbose = True)
	print('start training')
	t1 = time.clock()
	model.fit(batch_im, batch_l)
	t2 = time.clock()
	print('MLP training time:%.2f s' % (t2 - t1));
	joblib.dump(model, 'model/mlp.pkl')
	print('start test')
	y_hat = model.predict(test_im)
	print('score:', accuracy_score(test_l, y_hat))

def main():
	MLP_training()

if __name__ == '__main__':
	main()