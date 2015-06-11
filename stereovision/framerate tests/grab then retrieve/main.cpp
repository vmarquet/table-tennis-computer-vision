#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include <chrono>

using namespace cv;
using namespace std;


void myGrab(int camNum, VideoCapture& captureCam);
void myRetrieve(int camNum, VideoCapture& captureCam, Mat& frameCam);


int main(int argc, char const *argv[])
{
    // we open the webcam streams
    VideoCapture captureCam1(1);
    VideoCapture captureCam2(2);

    if (!captureCam1.isOpened() || !captureCam2.isOpened()) {
        cerr << "Error: can't access webcam stream" << endl;
        exit(1);
    }

    Mat frameCam1, frameCam2;

    auto start = chrono::high_resolution_clock::now();

    float i = 0;
    while(true) {
        int duration;
        chrono::high_resolution_clock::time_point start_time, end_time;
        
        myGrab(1, captureCam1);
        myGrab(2, captureCam2);
        
        myRetrieve(1, captureCam1, frameCam1);
        myRetrieve(2, captureCam2, frameCam2);

        // uncomment to display the streams in a GUI
        // start_time = chrono::high_resolution_clock::now();
        // imshow("cam 1", frameCam1);
        // imshow("cam 2", frameCam2);
        // end_time = chrono::high_resolution_clock::now();
        // duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();        
        // cout << "imshow: " << std::setfill(' ') << std::setw(3) << duration << "ms  ";

        // uncomment to use waitKey (needed if displaying the streams)
        // start_time = chrono::high_resolution_clock::now();
        // char key = waitKey(1); // wait before next frame
        // if (key == 'q')
        //     break;
        // end_time = chrono::high_resolution_clock::now();
        // duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();        
        // cout << "waitKey: " << std::setfill(' ') << std::setw(3) << duration << "ms  ";

        cout << endl;
        i++;

        if (i > 200)
            break;
    }

    auto end = chrono::high_resolution_clock::now();
    cout << "\n\n" << i / chrono::duration_cast<chrono::milliseconds>(end - start).count()*1000 << " FPS" << endl;

    return 0;
}


void myGrab(int camNum, VideoCapture& capture) {
    auto start_time = chrono::high_resolution_clock::now();
    bool grab = capture.grab();
    auto end_time = chrono::high_resolution_clock::now();
    int duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
    cout << "g" << camNum << ": " << std::setfill(' ') << std::setw(3) << duration << "ms  ";
    if (! grab)
        cout << endl << "GRAB FAILED cam number " << camNum << endl;
}

void myRetrieve(int camNum, VideoCapture& capture, Mat& frame) {
    auto start_time = chrono::high_resolution_clock::now();
    capture.retrieve(frame);
    auto end_time = chrono::high_resolution_clock::now();
    int duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();        
    cout << "r" << camNum << ": " << std::setfill(' ') << std::setw(3) << duration << "ms  ";
}




