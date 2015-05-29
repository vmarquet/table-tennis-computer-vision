#include "ball_tracking.h"

using namespace std;
using namespace cv;


/**
    Compute the ROI where to search the ball, given a ball position estimation

    @param position The estimated position of the ball
    @param frame_size The total frame size
    @param roi The output rectangle
*/
void getRoiRect(Point position, Size frame_size, Rect& roi) {
    int x_min = position.x - (ROI_WIDTH / 2);
    if (x_min < 0)
        x_min = 0;

    int x_max = position.x + (ROI_WIDTH / 2);
    if (x_max > frame_size.width - 1)
        x_max = frame_size.width - 1;

    int y_min = position.y - (ROI_HEIGHT / 2);
    if (y_min < 0)
        y_min = 0;

    int y_max = position.y + (ROI_HEIGHT / 2);
    if (y_max > frame_size.height - 1)
        y_max = frame_size.height - 1;

    roi.x = x_min;
    roi.y = y_min;
    roi.width  = x_max - x_min;
    roi.height = y_max - y_min;
}

