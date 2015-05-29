# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# OpenCV compilation / linker flags
OPENCV_CFLAGS  = `pkg-config --cflags opencv`
OPENCV_LDFLAGS = `pkg-config --libs   opencv`


all: main

main: ball_segmentation.o ball_detection.o ball_tracking.o main.o
	$(CC) $(CFLAGS) -o main ball_segmentation.o ball_detection.o ball_tracking.o main.o $(OPENCV_LDFLAGS)

main.o:
	$(CC) $(CFLAGS) -c main.cpp $(OPENCV_CFLAGS)

ball_segmentation.o:
	$(CC) $(CFLAGS) -c ball_segmentation.cpp $(OPENCV_CFLAGS)

ball_detection.o:
	$(CC) $(CFLAGS) -c ball_detection.cpp $(OPENCV_CFLAGS)

ball_tracking.o:
	$(CC) $(CFLAGS) -c ball_tracking.cpp $(OPENCV_CFLAGS)

clean:
	rm main main.o ball_segmentation.o ball_detection.o ball_tracking.o
