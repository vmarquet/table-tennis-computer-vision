#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>

#include "constants.h"
#include "ball_segmentation.h"
#include "ball_detection.h"
#include "ball_tracking.h"

using namespace cv;
using namespace std;


void drawHoughCircles(Mat& frame, vector<Vec3f> circles);
void drawTrackingInfo(Mat& frame, vector<Point> positions);
void drawTrackingInfo(Mat& frame, vector<Point> positions, Rect roi_rect);


int main(int argc, char const *argv[])
{
    // we get the filename of the video file to use
    if (argc == 1) {
        cerr << "Please give the video filename as an argument" << endl;
        exit(1);
    }
    const string videofilename = argv[1];

    // we open the video file
    VideoCapture capture(videofilename);
    if (!capture.isOpened()) {
        cerr << "Error when reading video file" << endl;
        exit(1);
    }


    Mat frame, roi_blurred, roi_binarized;
    Rect roi;
    vector<Vec3f> circles;

    // we save info from previous iterations
    Mat frame_previous;  // the previous frame
    vector<Point> positions;  // the history of all detected positions (0 or 1 per frame)
    bool ball_found_prev = false;  // whether we found ball during previous iteration

    int i = 0;
    while(true)
    {
        if (i<60) {  // TEMP: we skip the first 60 frames (bad test video)
            capture >> frame;
            i++;
            continue;
        }

        bool ball_found = false;
        Point ball_position;

        // we grab a new frame
        capture >> frame;

        // we check that we didn't reached the end of the video
        if(frame.empty())
            break;

        // TEMPORARY: we reduce frame size
        Size temp_size(640, 480);
        resize(frame, frame, temp_size, 0, 0, INTER_CUBIC);

        // if we found the ball during previous iteration, we use these coordinates
        // as centre of a reduced ROI, else the ROI is the full frame
        Mat roi;
        Rect roi_rect;
        if (ball_found_prev) {
            getRoiRect(positions.back(), temp_size, roi_rect);
            roi = Mat(frame, roi_rect);
        }
        else
            roi = frame;

        // we blur the picture to remove the noise
        GaussianBlur(roi, roi_blurred, Size(BLUR_KERNEL_LENGTH, BLUR_KERNEL_LENGTH), 0, 0);

        // ===== first try =====
        // with a threshold segmentation and a Hough transform
        thresholdSegmentation(roi_blurred, roi_binarized);
        detectBallWithHough(roi_binarized, roi_rect, circles);

        // if the number of circles found by the Hough transform is exactly 1,
        // we accept that circle as the correct ball position
        if (circles.size() == 1) {
            ball_found = true;
            ball_position = Point(circles[0][0], circles[0][1]);
            #ifdef SHOW_WINDOWS
                drawHoughCircles(frame, circles);
            #endif
        }

        if (ball_found == false) {
            // ===== second try =====
            // we use OpenCV convex hull algorithm to detect shapes
            // (the ball is not detected by Hough transform if it is not round enough)
            vector<Point> possible_positions;
            detectBallWithContours(roi_binarized, roi_rect, possible_positions);

            // if the number of positions found by the Hough transform is exactly 1,
            // we accept that position as the correct ball position
            if (possible_positions.size() == 1) {
                ball_found = true;
                ball_position = possible_positions[0];
            }
        }


        // if we have found the ball position, we save it in the history
        if (ball_found == true)
            positions.push_back(ball_position);

        #ifdef SHOW_WINDOWS
            // we display the image
            if (ball_found_prev)
                drawTrackingInfo(frame, positions, roi_rect);
            else
                drawTrackingInfo(frame, positions);
            imshow(videofilename, frame);
        #endif

        // we save the previous frame
        frame_previous = frame;
        ball_found_prev = ball_found;

        // press 'q' to quit
        char key = waitKey(1);
        if (key == 'q')
            break;
    }

    return 0;
}


// draws the circles that found with Hough transform
void drawHoughCircles(Mat& frame, vector<Vec3f> circles) {
    for(size_t i = 0; i < circles.size(); i++)
    {
        Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
        int radius = cvRound(circles[i][2]);

        circle(frame, center, radius, Scalar(0,0,255), 3, 8, 0);
    }
}

// draws the trajectory
void drawTrackingInfo(Mat& frame, vector<Point> positions) {
    // we draw the trajectory lines between the positions detected
    for (int j=0; j<((int)positions.size()-1); j++)
        line(frame, positions.at(j), positions.at(j+1), Scalar(0,0,255), 2, CV_AA);

    // we draw a circle for each position detected
    for (size_t j=0; j<positions.size(); j++)
        circle(frame, positions.at(j), 2, Scalar(255,0,0), 2);
}

// draws the trajectory and the rectangle of the ROI
void drawTrackingInfo(Mat& frame, vector<Point> positions, Rect roi_rect) {
    drawTrackingInfo(frame, positions);
    rectangle(frame, roi_rect, Scalar(0,255,0));
}

