#ifndef BALL_DETECTION_H
#define BALL_DETECTION_H

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>

#include "constants.h"
#include "ball_segmentation.h"


void detectBallWithHough(Mat& roi_binarized, Rect roi_rect, vector<Vec3f>& circles);
void detectBallWithContours(Mat& roi_binarized, Rect roi_rect, vector<Point>& positions);


#endif