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

bool intersection_point(Point2f ori1, Point2f end1, Point2f ori2, Point2f end2, Point2f &res);

int main(int argc, char** argv)
{
    const char* filename = argc >= 2 ? argv[1] : "table.jpg";
    const char* window_name = "table line detection";

    Mat src = imread(filename, CV_LOAD_IMAGE_GRAYSCALE);
    if(src.empty())
    {
        help();
        cout << "can not open " << filename << endl;
        return -1;
    }

    Mat canny, dst_color;
    vector<Vec4i> lines;

    // we apply a canny edge detector, it produces a picture with shades of gray
    Canny(src, canny, 50, 200, 3);

    // the result of Canny edge detector is a gray picture, so we convert from grey to BGR
    cvtColor(canny, dst_color, CV_GRAY2BGR);

    // Basic Hough transform
    #ifdef HOUGH_BASIC
        vector<Vec2f> lines_basic;
        HoughLines(canny, lines_basic, 1, CV_PI/180, 100, 0, 0 );

        for( size_t i = 0; i < lines_basic.size(); i++ )
        {
            float rho = lines_basic[i][0], theta = lines_basic[i][1];
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
        HoughLinesP(canny, lines, 1, CV_PI/180, 50, 200, 100);

        // we display all lines on the resulting picture
        for( size_t i = 0; i < lines.size(); i++ )
        {
            Vec4i l = lines[i];
            line(dst_color, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 2, CV_AA);
        }
        cout << "Number of lines found: " << lines.size() << endl;
    #endif

    // we display original and final pictures
    imshow(window_name, src);
    waitKey();
    imshow(window_name, dst_color);
    waitKey();

    #ifdef HOUGH_BASIC
        return 0;
    #endif


    // we separate the lines found with Probabilistic Hough Transform
    // in four sets, based on the coordinates of the end of the lines,
    // and the line angle
    vector<Vec4i> lines_horizontal_up;
    vector<Vec4i> lines_horizontal_down;
    vector<Vec4i> lines_vertical_left;
    vector<Vec4i> lines_vertical_right;
    
    for (size_t i = 0; i < lines.size(); i++) {
        Vec4i line = lines[i];

        // we convert get the angle of the line, to know
        // if it's vertical or horizontal

        // input for cartToPolar
        std::vector<float> x;
        x.push_back((float) (line[0] - line[2]));
        std::vector<float> y;
        y.push_back((float) (line[1] - line[3]));
        // output from cartToPolar
        std::vector<float> magnitude;
        std::vector<float> angle;

        cartToPolar(x, y, magnitude, angle, true);

        // check if it's a horizontal line
        if (((int)angle[0] % 180) < 5 || ((int)angle[0] % 180) > 175) {
            // check if it's the upper line or the lower line
            // we use the coordinates of the ends of the line
            // WARNING: OpenCV y axis is growing when going down
            if (line[1] < (src.rows/2) && line[3] < (src.rows/2))
                lines_horizontal_up.push_back(line);
            else if (line[1] > (src.rows/2) && line[3] > (src.rows/2))
                lines_horizontal_down.push_back(line);
        }

        // check if it's a vertical line
        if (((int)angle[0] % 180) > 85 && ((int)angle[0] % 180) < 95) {
            if (line[0] < (src.cols/2) && line[2] < (src.cols/2))
                lines_vertical_left.push_back(line);
            else if (line[0] > (src.cols/2) && line[2] > (src.cols/2))
                lines_vertical_right.push_back(line);
        }
    }

    cout << "horizontal upper lines: " << lines_horizontal_up.size()   << endl;
    cout << "horizontal lower lines: " << lines_horizontal_down.size() << endl;
    cout << "vertical left lines: "    << lines_vertical_left.size()   << endl;
    cout << "vertical right lines: "   << lines_vertical_right.size()  << endl;

    // we find the points at the intersections of the lines, at each corner
    vector<Point2f> points_left_up_corner;
    vector<Point2f> points_right_up_corner;
    vector<Point2f> points_right_down_corner;
    vector<Point2f> points_left_down_corner;

    // left up corner:
    for (size_t i = 0; i < lines_horizontal_up.size(); i++) {
        for (size_t j = 0; j < lines_vertical_left.size(); j++) {
            Point2f ori1((float)(lines_horizontal_up[i][0]), (float)(lines_horizontal_up[i][1]));
            Point2f end1((float)(lines_horizontal_up[i][2]), (float)(lines_horizontal_up[i][3]));
            Point2f ori2((float)(lines_vertical_left[j][0]), (float)(lines_vertical_left[j][1]));
            Point2f end2((float)(lines_vertical_left[j][2]), (float)(lines_vertical_left[j][3]));
            Point2f res;

            if (intersection_point(ori1, end1, ori2, end2, res) != false)
                points_left_up_corner.push_back(res);
        }
    }

    // right up corner:
    for (size_t i = 0; i < lines_horizontal_up.size(); i++) {
        for (size_t j = 0; j < lines_vertical_right.size(); j++) {
            Point2f ori1((float)(lines_horizontal_up[i][0]), (float)(lines_horizontal_up[i][1]));
            Point2f end1((float)(lines_horizontal_up[i][2]), (float)(lines_horizontal_up[i][3]));
            Point2f ori2((float)(lines_vertical_right[j][0]), (float)(lines_vertical_right[j][1]));
            Point2f end2((float)(lines_vertical_right[j][2]), (float)(lines_vertical_right[j][3]));
            Point2f res;

            if (intersection_point(ori1, end1, ori2, end2, res) != false)
                points_right_up_corner.push_back(res);
        }
    }

    // right down corner:
    for (size_t i = 0; i < lines_horizontal_down.size(); i++) {
        for (size_t j = 0; j < lines_vertical_right.size(); j++) {
            Point2f ori1((float)(lines_horizontal_down[i][0]), (float)(lines_horizontal_down[i][1]));
            Point2f end1((float)(lines_horizontal_down[i][2]), (float)(lines_horizontal_down[i][3]));
            Point2f ori2((float)(lines_vertical_right[j][0]), (float)(lines_vertical_right[j][1]));
            Point2f end2((float)(lines_vertical_right[j][2]), (float)(lines_vertical_right[j][3]));
            Point2f res;

            if (intersection_point(ori1, end1, ori2, end2, res) != false)
                points_right_down_corner.push_back(res);
        }
    }

    // left down corner:
    for (size_t i = 0; i < lines_horizontal_down.size(); i++) {
        for (size_t j = 0; j < lines_vertical_left.size(); j++) {
            Point2f ori1((float)(lines_horizontal_down[i][0]), (float)(lines_horizontal_down[i][1]));
            Point2f end1((float)(lines_horizontal_down[i][2]), (float)(lines_horizontal_down[i][3]));
            Point2f ori2((float)(lines_vertical_left[j][0]), (float)(lines_vertical_left[j][1]));
            Point2f end2((float)(lines_vertical_left[j][2]), (float)(lines_vertical_left[j][3]));
            Point2f res;

            if (intersection_point(ori1, end1, ori2, end2, res) != false)
                points_left_down_corner.push_back(res);
        }
    }

    cout << "left up corner points: "    << points_left_up_corner.size()    << endl;
    cout << "right up corner points: "   << points_right_up_corner.size()   << endl;
    cout << "right down corner points: " << points_right_down_corner.size() << endl;
    cout << "left down corner points: "  << points_left_down_corner.size()  << endl;

    // we draw all the points on the picture
    for (size_t i = 0; i < points_left_up_corner.size(); i++)
        circle(dst_color, points_left_up_corner[i], 2, Scalar(255,0,0), 2);
    for (size_t i = 0; i < points_right_up_corner.size(); i++)
        circle(dst_color, points_right_up_corner[i], 2, Scalar(255,0,0), 2);
    for (size_t i = 0; i < points_right_down_corner.size(); i++)
        circle(dst_color, points_right_down_corner[i], 2, Scalar(255,0,0), 2);
    for (size_t i = 0; i < points_left_down_corner.size(); i++)
        circle(dst_color, points_left_down_corner[i], 2, Scalar(255,0,0), 2);
    
    imshow(window_name, dst_color);
    waitKey();


    // for each of the group of points,we select a point that will be used
    // as the coordinate of the corner of the table

    // we choose the more "extrem" point in each group, because the points which are closer
    // to the center of the table correspond to the intersection of lines that are
    // the lines between the table lines and the inner part of the table, whereas we want the
    // intersection of the lines between the table line and the ground

    // N.B.: this algorithm suppose that the anti-distortion filter was successful and effective
    // If there is some barrel distortion on the input picture, this algorithm
    // will give false corner coordinates that are farther than the actual real corners

    Point2f table_center(src.cols / 2, src.rows / 2);

    // left up corner
    Point2f left_up_corner;
    if (points_left_up_corner.size() > 0)
        left_up_corner = points_left_up_corner[0];
    for (size_t i = 1; i < points_left_up_corner.size(); i++)
        if (norm(points_left_up_corner[i] - table_center) > norm(left_up_corner - table_center))
            left_up_corner = points_left_up_corner[i];

    // right up corner
    Point2f right_up_corner;
    if (points_right_up_corner.size() > 0)
        right_up_corner = points_right_up_corner[0];
    for (size_t i = 1; i < points_right_up_corner.size(); i++)
        if (norm(points_right_up_corner[i] - table_center) > norm(right_up_corner - table_center))
            right_up_corner = points_right_up_corner[i];

    // right down corner
    Point2f right_down_corner;
    if (points_right_down_corner.size() > 0)
        right_down_corner = points_right_down_corner[0];
    for (size_t i = 1; i < points_right_down_corner.size(); i++)
        if (norm(points_right_down_corner[i] - table_center) > norm(right_down_corner - table_center))
            right_down_corner = points_right_down_corner[i];

    // left down corner
    Point2f left_down_corner;
    if (points_left_down_corner.size() > 0)
        left_down_corner = points_left_down_corner[0];
    for (size_t i = 1; i < points_left_down_corner.size(); i++)
        if (norm(points_left_down_corner[i] - table_center) > norm(left_down_corner - table_center))
            left_down_corner = points_left_down_corner[i];

    // we draw the four corners, and the lines between them
    cvtColor(src, src, CV_GRAY2BGR);
    line(src, left_up_corner, right_up_corner, Scalar(0,0,255), 2, CV_AA);
    line(src, right_up_corner, right_down_corner, Scalar(0,0,255), 2, CV_AA);
    line(src, right_down_corner, left_down_corner, Scalar(0,0,255), 2, CV_AA);
    line(src, left_down_corner, left_up_corner, Scalar(0,0,255), 2, CV_AA);
    circle(src, left_up_corner, 2, Scalar(255,0,0), 2);
    circle(src, right_up_corner, 2, Scalar(255,0,0), 2);
    circle(src, right_down_corner, 2, Scalar(255,0,0), 2);
    circle(src, left_down_corner, 2, Scalar(255,0,0), 2);

    // we display the final result
    imshow(window_name, src);
    waitKey();

    return 0;
}


// Finds the intersection of two lines, or returns false.
// Each line is defined by two points, (ori1, end1) for the first line
// and (ori2, end2) for the second line
bool intersection_point(Point2f ori1, Point2f end1, Point2f ori2, Point2f end2, Point2f &res)
{
    Point2f x    = ori2 - ori1;  // vector from ori1 to ori2
    Point2f dir1 = end1 - ori1;  // direction vector of line 1
    Point2f dir2 = end2 - ori2;  // direction vector of line 2

    float cross = dir1.x*dir2.y - dir1.y*dir2.x;  // cross product
    if (abs(cross) < /*EPS*/1e-8)  // if the two lines are parallel
        return false;

    double t1 = (x.x * dir2.y - x.y * dir2.x) / cross;
    res = ori1 + dir1 * t1;
    return true;
}

