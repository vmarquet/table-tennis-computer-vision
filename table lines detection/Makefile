# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all: hough_transform detect_rectangles k_mean_opencv k_mean_custom

hough_transform:
	$(CC) $(CFLAGS) hough-transform.cpp -o hough-transform $(OPENCV_FLAGS)

detect_rectangles:
	$(CC) $(CFLAGS) detect-rectangles.cpp -o detect-rectangles $(OPENCV_FLAGS)

k_mean_opencv:
	$(CC) $(CFLAGS) k-mean-opencv.cpp -o k-mean-opencv $(OPENCV_FLAGS)

k_mean_custom:
	$(CC) $(CFLAGS) k-mean-custom.cpp -o k-mean-custom $(OPENCV_FLAGS)

clean:
	rm hough-transform detect-rectangles k-mean-opencv k-mean-custom
