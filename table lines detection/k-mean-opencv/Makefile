# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all: k_mean_opencv

k_mean_opencv:
	$(CC) $(CFLAGS) k-mean-opencv.cpp -o k-mean-opencv $(OPENCV_FLAGS)

clean:
	rm k-mean-opencv
