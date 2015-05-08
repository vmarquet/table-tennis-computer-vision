# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all: hough_transform detect_rectangles

hough_transform:
	$(CC) $(CFLAGS) hough-transform.cpp -o hough-transform $(OPENCV_FLAGS)

detect_rectangles:
	$(CC) $(CFLAGS) detect-rectangles.cpp -o detect-rectangles $(OPENCV_FLAGS)

clean:
	rm hough-transform detect-rectangles
