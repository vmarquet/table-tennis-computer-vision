#ifndef BALL_SEGMENTATION_H
#define BALL_SEGMENTATION_H

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>

#include "constants.h"

#define COLOR_WHITE 255
#define COLOR_BLACK 0


void thresholdSegmentation(Mat& roi_bgr_blurred, Mat& roi_binarized);


#endif