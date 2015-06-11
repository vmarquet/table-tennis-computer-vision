#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>

using namespace cv;
using namespace std;


static mutex mutexRequestNewFrame1;
static mutex mutexRequestNewFrame2;
static bool requestNewFrame1 = true;  // set to true when we need a new frame from webcam 1
static bool requestNewFrame2 = true;  // set to true when we need a new frame from webcam 2
static auto request_time = chrono::high_resolution_clock::now(); // set each time we request a new frame

static int number_grab_1 = 0;
static int number_grab_2 = 0;


void getNewFrame(int camNum, VideoCapture* captureCam, Mat* frame,
                    mutex *mutexRequestNewFrame, bool* requestNewFrame);


int main(int argc, char const *argv[])
{
    // we open the webcam streams
    VideoCapture captureCam1(1);
    VideoCapture captureCam2(2);

    if (!captureCam1.isOpened() || !captureCam2.isOpened()) {
        cerr << "Error: can't access webcam stream" << endl;
        exit(1);
    }

    Mat frameBufferCam1, frameBufferCam2, frameCam1, frameCam2;
    bool mutex1Locked = false, mutex2Locked = false;

    // we create two threads to get the pictures, one for each camera
    thread threadFrame1(getNewFrame, 1, &captureCam1, &frameBufferCam1, &mutexRequestNewFrame1, &requestNewFrame1);
    thread threadFrame2(getNewFrame, 2, &captureCam2, &frameBufferCam2, &mutexRequestNewFrame2, &requestNewFrame2);

    auto start = chrono::high_resolution_clock::now();

    float i = 0;
    while(true) {
        int duration;
        chrono::high_resolution_clock::time_point start_time, end_time;

        // we try to get new frames from the buffer, if not possible, we wait 2ms and try again

        // first, for each mutex, we try to lock is
        if (! mutex1Locked) { // because trying to lock a mutex already locked results in UNDEFINED BEHAVIOUR
            if (! mutexRequestNewFrame1.try_lock()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            // and we check that the frame buffer has been updated to a new one
            else if (requestNewFrame1) {
                mutexRequestNewFrame1.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            else
                mutex1Locked = true;
        }

        // same for frame 2
        if (! mutex2Locked) { // because trying to lock a mutex already locked results in UNDEFINED BEHAVIOUR
            if (! mutexRequestNewFrame2.try_lock()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            else if (requestNewFrame2) {
                mutexRequestNewFrame2.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                continue;
            }
            else
                mutex2Locked = true;
        }

        cout << "main: processing new frames  ";

        // so now, if we reach here, we have the 2 mutex locked by us, with 2 new frames
        // so we make a copy of them, and then we release the mutexes,
        // so that the other threads can get new frames while were making some processing
        frameBufferCam1.copyTo(frameCam1);
        frameBufferCam2.copyTo(frameCam2);
        requestNewFrame1 = true;
        requestNewFrame2 = true;
        request_time = chrono::high_resolution_clock::now();
        mutexRequestNewFrame1.unlock();
        mutexRequestNewFrame2.unlock();
        mutex1Locked = false;
        mutex2Locked = false;

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
        // end_time = chrono::high_resolution_clock::now();
        // duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();        
        // cout << "waitKey: " << std::setfill(' ') << std::setw(3) << duration << "ms  ";
        // if (key == 'q')
        //     break;

        cout << endl;
        i++;

        if (i > 200)
            break;
    }

    auto end = chrono::high_resolution_clock::now();
    int duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "\n\n" << i / (duration / 1000) << " FPS" << endl;

    cout << "grabbed from cam 1: " << number_grab_1 << "   grabbed from cam 2: " << number_grab_2 << endl;

    return 0;
}


// void getNewFrame(int camNum, VideoCapture* capture, Mat* frame) {
void getNewFrame(int camNum, VideoCapture* capture, Mat* frame,
                    mutex* mutexRequestNewFrame, bool* requestNewFrame) {
    while (true) {
        // if the flag requestNewFrame is false or locked by a mutex,
        // we wait a few ms and then try again
        if (! mutexRequestNewFrame->try_lock()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        if (! *requestNewFrame) {
            mutexRequestNewFrame->unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        // we grab and retrieve the frame, while recording timing
        auto start = chrono::high_resolution_clock::now();
        bool grab = capture->grab(); // WARNING: we should handle correctly the case that the grab FAILS
        if (!grab)
            cerr << endl << "WARNING: grab failed !" << endl;
        auto end_grab = chrono::high_resolution_clock::now();
        capture->retrieve(*frame);
        auto end_retrieve = chrono::high_resolution_clock::now();

        // we compute grab and retrieve duration
        int duration_grab     = chrono::duration_cast<chrono::milliseconds>(end_grab - start).count();
        int duration_retrieve = chrono::duration_cast<chrono::milliseconds>(end_retrieve - end_grab).count();

        // we keep the number of frames retrieved
        if (camNum == 1)
            number_grab_1++;
        if (camNum == 2)
            number_grab_2++;

        auto request_completed_time = chrono::high_resolution_clock::now();
        int duration_complete_request = chrono::duration_cast<chrono::milliseconds>
                                        (request_completed_time - request_time).count();

        cout << "getNewFrame: camera " << camNum << ": grab: " 
             << duration_grab << " retrieve: " << duration_retrieve
             << "  frame request: " << duration_complete_request << "ms ago " << endl;

        *requestNewFrame = false;
        mutexRequestNewFrame->unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}



