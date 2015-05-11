// Source: example Hough transform program from OpenCV

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>

using namespace cv;
using namespace std;

// to use "basic" Hough transform instead of "probabilistic" one, define "HOUGH_BASIC"
// #define HOUGH_BASIC

void help()
{
 cout << "This program demonstrates line finding with the Hough transform.\n"
         "Usage:  ./hough-transform <image_name>" << endl;
}

int main(int argc, char** argv)
{
    const char* filename = argc >= 2 ? argv[1] : "table.jpg";

    Mat src = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    if(src.empty())
    {
        help();
        cout << "can not open " << filename << endl;
        return -1;
    }

    Mat dst, dst_color;

    // we apply a canny edge detector, it produces a picture with shades of gray
    Canny(src, dst, 50, 200, 3);

    // the result of Canny edge detector is a gray picture, so we convert from grey to BGR
    cvtColor(dst, dst_color, CV_GRAY2BGR);

    // Basic Hough transform
    #ifdef HOUGH_BASIC
        vector<Vec2f> lines;
        HoughLines(dst, lines, 1, CV_PI/180, 100, 0, 0 );

        for( size_t i = 0; i < lines.size(); i++ )
        {
            float rho = lines[i][0], theta = lines[i][1];
            Point pt1, pt2;
            double a = cos(theta), b = sin(theta);
            double x0 = a*rho, y0 = b*rho;
            pt1.x = cvRound(x0 + 1000*(-b));
            pt1.y = cvRound(y0 + 1000*(a));
            pt2.x = cvRound(x0 - 1000*(-b));
            pt2.y = cvRound(y0 - 1000*(a));
            line(dst_color, pt1, pt2, Scalar(0,0,255), 3, CV_AA);
        }
    // Probabilistic Hough transform
    #else
        // we apply Hough transform to the image, after canny edge detection, to get all lines
        vector<Vec4i> lines;
        HoughLinesP(dst, lines, 1, CV_PI/180, 50, 50, 10 );

        // we display all lines on the resulting picture
        for( size_t i = 0; i < lines.size(); i++ )
        {
            Vec4i l = lines[i];
            line(dst_color, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
        }
    #endif

    // we display original and final pictures
    imshow("source", src);
    imshow("detected lines", dst_color);

    waitKey();

    return 0;
}