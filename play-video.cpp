// g++ -std=c++11 play-video.cpp -o play-video `pkg-config --cflags --libs opencv`
// ./play-video [video filename]

// a program to open a video file, and to display it
// it also print the mean time to grab frames and display them
// useful to check if your computer is fast enough to process the video in real-time

#include <iostream>
#include <chrono>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
    // video filename should be given as an argument
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

    // we compute the frame duration
    int FPS = capture.get(CV_CAP_PROP_FPS);
    cout << "FPS: " << FPS << endl;

    int frame_duration = 1000 / FPS;  // frame duration in milliseconds
    cout << "frame duration: " << frame_duration << " ms" << endl;

    // we read and display the video file, image after image
    Mat frame;
    namedWindow(videofilename, 1);

    // counters for computing means
    int frame_number = 1;
    int total_grab_frame_time = 0;  // milliseconds
    int total_imshow_time = 0;  // milliseconds
    int total_remaining_time_to_wait = 0;  // milliseconds

    while(true)
    {
        // time counter for speed analysis purposes
        auto start_time = chrono::high_resolution_clock::now();

        // we grab a new image
        capture >> frame;
        if(frame.empty())
            break;

        // we compute and display the time needed to grab a new image, and the mean time
        auto end_time = chrono::high_resolution_clock::now();
        int grab_frame_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
        total_grab_frame_time += grab_frame_time;
        cout << "grab " << grab_frame_time << " (" << total_grab_frame_time/frame_number << ")";

        // reset time counter
        start_time = chrono::high_resolution_clock::now();

        // we display the image
        imshow(videofilename, frame);

        // we compute and display the time needed to display a new image, and the mean time
        end_time = chrono::high_resolution_clock::now();
        int imshow_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
        total_imshow_time += imshow_time;
        cout << "  imshow " << imshow_time << " (" << total_imshow_time/frame_number << ")";

        // we compute and display the time left before we need to display the next picture
        int remaining_time_to_wait = frame_duration - (grab_frame_time + imshow_time);
        if (remaining_time_to_wait >= 0)
            cout << "  ahead " << remaining_time_to_wait;
        else
            cout << "  behind " << -remaining_time_to_wait;

        // we display the mean time left before we need to display the next picture
        total_remaining_time_to_wait += remaining_time_to_wait;
        if (total_remaining_time_to_wait >= 0)
            cout << "  (mean: ahead of schedule by " << total_remaining_time_to_wait/frame_number << "ms)" << endl;
        else
            cout << "  (mean: behind of schedule by " << -total_remaining_time_to_wait/frame_number << "ms)" << endl;

        // if computing time was longer than normal time between frames,
        // we wait only 1ms
        remaining_time_to_wait = max(1, remaining_time_to_wait);

        // press 'q' to quit
        char key = waitKey(remaining_time_to_wait); // waits to display frame
        if (key == 'q')
            break;

        frame_number++;  // counter for computing means
    }

    // releases and window destroy are automatic in C++ interface
}

