# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all:
	$(CC) $(CFLAGS) play-video.cpp -o play-video $(OPENCV_FLAGS)

clean:
	rm play-video


