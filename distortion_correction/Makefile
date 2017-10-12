# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all:
	$(CC) $(CFLAGS) distortion-correction.cpp -o distortion-correction $(OPENCV_FLAGS)

clean:
	rm distortion-correction


