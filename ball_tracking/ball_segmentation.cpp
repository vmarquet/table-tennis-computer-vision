#include "ball_segmentation.h"

using namespace std;
using namespace cv;


/**
    Apply a segmentation with a threshold based on the color of the ball

    @param roi_bgr_blurred The ROI blurred, formatted as BGR
    @param roi_binarized The output binarized frame
*/
void thresholdSegmentation(Mat& roi_bgr_blurred, Mat& roi_binarized) {
    Mat roi_hsv(Size(roi_bgr_blurred.cols, roi_bgr_blurred.rows), CV_8UC3);

    // we convert the frame to HSV
    cvtColor(roi_bgr_blurred, roi_hsv, CV_BGR2HSV);

    // we compute the mask by binarizing the picture with a color threshold
    inRange(roi_hsv, BALL_COLOR_HSV_MIN, BALL_COLOR_HSV_MAX, roi_binarized);

    #ifdef SHOW_WINDOWS
        imshow("threshold segmentation", roi_binarized);
    #endif

    // we apply a closing (dilatation then erosion)
    morphologyEx(roi_binarized, roi_binarized, MORPH_CLOSE,
                getStructuringElement(MORPH_ELLIPSE, Size(CLOSING_KERNEL_LENGTH,CLOSING_KERNEL_LENGTH)));
}


