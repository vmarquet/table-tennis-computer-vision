# C++ compiler to use
CC = g++

# compilation flags
CFLAGS = -g -Wall -std=c++11

# link to OpenCV
OPENCV_FLAGS = `pkg-config --cflags --libs opencv`


all: convex_hull

convex_hull:
	$(CC) $(CFLAGS) convex-hull.cpp -o convex-hull $(OPENCV_FLAGS)

clean:
	rm convex_hull
