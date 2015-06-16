# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11 -stdlib=libc++

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all: main

main:
	$(CC) $(CFLAGS) main.cpp -o main $(OPENCV_FLAGS)

clean:
	rm main
