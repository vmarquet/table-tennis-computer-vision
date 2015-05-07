# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all:
	$(CC) $(CFLAGS) camera-calibration.cpp -o camera-calibration $(OPENCV_FLAGS)

clean:
	rm camera-calibration


