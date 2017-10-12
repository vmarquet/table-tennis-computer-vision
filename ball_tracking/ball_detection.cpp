#include "ball_detection.h"

using namespace std;
using namespace cv;


/**
    Detect the ball position with a Hough transform on a binarized picture

    @param roi_binarized The binarized picture of the ROI
    @param roi_rect The Rect with the coordinates of the ROI (needed to compute center of circles)
    @param circles The output list of circles
*/
void detectBallWithHough(Mat& roi_binarized, Rect roi_rect, vector<Vec3f>& circles) {
    // we apply a Canny edge detector
    Canny(roi_binarized, roi_binarized, 50, 200, 3);

    // we apply the circle Hough transform
    HoughCircles(roi_binarized, circles, CV_HOUGH_GRADIENT, 1, 10, 200, 15, 0, 0);

    // for each circle, we correct the center position
    // (the HoughCircles function gives us the position inside the ROI,
    //  whereas we want the position inside the full frame)
    for (size_t i=0; i<circles.size(); i++) {
        Vec3f circle = circles.at(i);
        circle[0] = circle[0] + roi_rect.x;
        circle[1] = circle[1] + roi_rect.y;
        circles[i] = circle;
    }
}

/**
    Detect the ball position with contours on a binarized picture

    @param roi_binarized The binarized picture of the ROI
    @param roi_rect The Rect with the coordinates of the ROI (needed to compute center of circles)
    @param circles The output list of possible ball positions
*/
void detectBallWithContours(Mat& roi_binarized, Rect roi_rect, vector<Point>& positions) {
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy; // unused

    // first, we search for contours around shapes in the binarized ROI
    findContours(roi_binarized, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    // we compute the centroid of each shape
    for (size_t i=0; i<contours.size(); i++) {
        Moments m = moments(contours.at(i));
        Point centroid;
        centroid.x = (int)(m.m10/m.m00);
        centroid.y = (int)(m.m01/m.m00);

        // sometimes the results are impossible values, so we ignore them
        if (centroid.x < 0 || centroid.x >= roi_binarized.cols 
                || centroid.y < 0 || centroid.y > roi_binarized.rows)
            continue;

        // we correct the center position (we computed the position in the ROI,
        // we want the position in the full frame)
        centroid.x += roi_rect.x;
        centroid.y += roi_rect.y;

        positions.push_back(centroid);
    }
}
