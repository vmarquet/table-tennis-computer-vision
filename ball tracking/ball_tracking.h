#ifndef BALL_TRACKING_H
#define BALL_TRACKING_H

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>

#include "constants.h"


void getRoiRect(Point position, Size frame_size, Rect& roi);


#endif