// modified OpenCV example

#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

using namespace cv;
using namespace std;

static int print_help()
{
    cout <<
            " Given a list of chessboard images, the number of corners (nx, ny) \n"
            " on the chessboards, and a flag: useCalibrated with possible values: \n"
            "   0 : calibrated or \n"
            "   1 : uncalibrated, use cvStereoCalibrate() or \n"
            "   2 : uncalibrated, compute fundamental matrix separately \n"
            " Calibrate the cameras and display the rectified results along with the \n"
            " computed disparity images. \n\n"
            " Parameters: \n"
            "   -w <chesboard_width> : chessboard width (needed) \n"
            "   -h <chesboard_height> : chessboard height (needed) \n"
            "   --norect : if used, don't compute and display rectification \n"
            "   --webcam <w1> <w2> : use input images from webcams, with interactive \n"
            "       program to get the images when the chessboard pattern is found. \n"
            "       with w1: the number of the first webcam to use \n"
            "            w2: the number of the second webcam to use \n"
            "   --image-list <file>: the XML/YML file containing the list of images, \n"
            "        if we don't use webcams \n" << endl;
    return 0;
}

enum INPUT_MODE {
    NONE,       // program will fail
    WEBCAM,     // use webcam to get images
    IMAGE_LIST  // use XML/YML image list file
};

class Settings
{
public:
    Size boardSize;  // the size of the chessboard
    bool showRectified = true;  // display rectification ?
    bool useCalibrated = true;
    INPUT_MODE inputMode = NONE;  // input mode for images
    string imageListFilename;  // only if input mode is an image list file
    int webcam[2];  // the number of the webcams (only if input mode is webcam)
    VideoCapture capture[2];  // the VideoCapture to access the webcam streams
    int imageNumber = 10; // number of images to get (for each cam) if input mode is webcam
};

static bool readStringList( const string& filename, vector<string>& l );
static void getImagesFromWebcams(Settings settings, vector<Mat>& images);
static void StereoCalib(const vector<Mat>& images, Settings settings);


int main(int argc, char** argv)
{
    Settings settings;

    // we parse the arguments to get board size and image list XML/YML file
    for( int i = 1; i < argc; i++ )
    {
        // chessboard width parameter
        if( string(argv[i]) == "-w" )
        {
            if ((i+1) == argc) {
                cout << "missing board width after -w" << endl;
                return print_help();
            }
            if( sscanf(argv[++i], "%d", &settings.boardSize.width) != 1 || settings.boardSize.width <= 0 )
            {
                cout << "invalid board width" << endl;
                return print_help();
            }
        }
        // chessboard height parameter
        else if( string(argv[i]) == "-h" )
        {
            if ((i+1) == argc) {
                cout << "missing board height after -h" << endl;
                return print_help();
            }
            else if( sscanf(argv[++i], "%d", &settings.boardSize.height) != 1 || settings.boardSize.height <= 0 )
            {
                cout << "invalid board height" << endl;
                return print_help();
            }
        }
        // display rectification ?
        else if( string(argv[i]) == "--norect" )
            settings.showRectified = false;
        // input mode: webcam or image list
        else if (string(argv[i]) == "--image-list" ) {
            settings.imageListFilename = argv[++i];
            settings.inputMode = IMAGE_LIST;
        }
        else if (string(argv[i]) == "--webcam" ) {
            if ((i+2) >= argc) {
                cout << "missing webcam numbers after --webcam" << endl;
                return print_help();
            }
            else if( sscanf(argv[++i], "%d", &settings.webcam[0]) != 1 || settings.webcam[0] < 0 ||
                sscanf(argv[++i], "%d", &settings.webcam[1]) != 1 || settings.webcam[1] < 0)
            {
                cout << "invalid webcam number" << endl;
                return print_help();
            }
            settings.inputMode = WEBCAM;
        }
        // invalid options or parameters
        else if( string(argv[i]) == "-h" || string(argv[i]) == "--help" )
            return print_help();
        else if( argv[i][0] == '-' )
        {
            cout << "invalid option: " << argv[i] << endl;
            return 0;
        }
        else
        {
            cout << "unknown parameter: " << argv[i] << endl;
            return 0;
        }
    }

    // check that an input mode was selected
    if (settings.inputMode == NONE) {
        cout << "ERROR: please precise an input mode (--webcam or --image-list options)\n" << endl;
        return print_help();
    }

    // check that the chessboard size was entered by the user
    if( settings.boardSize.width <= 0 || settings.boardSize.height <= 0 )
    {
        cout << "ERROR: please precise chessboard width and height (-w and -h options)\n" << endl;
        return print_help();
    }

    vector<Mat> images; // the images

    // if the input mode is an image list (XML/YML)
    if (settings.inputMode == IMAGE_LIST) {
        vector<string> imageFilenames;
        // we get the list of images from the file
        bool ok = readStringList(settings.imageListFilename, imageFilenames);
        if(!ok || imageFilenames.empty())
        {
            cout << "can not open " << settings.imageListFilename << " or the string list is empty" << endl;
            return print_help();
        }
        // we load all the images
        for (size_t i=0; i<imageFilenames.size(); i++)
            images.push_back(imread(imageFilenames[i], CV_LOAD_IMAGE_GRAYSCALE));
    }
    else if (settings.inputMode == WEBCAM) {
        getImagesFromWebcams(settings, images);
    }

    

    // we finally start the calibration process
    StereoCalib(images, settings);
    return 0;
}

static void StereoCalib(const vector<Mat>& images, Settings settings)
{
    if( images.size() % 2 != 0 )
    {
        cout << "Error: the image list contains odd (non-even) number of elements\n" << endl;
        exit(1);
    }

    bool displayCorners = true;  // display GUI with the chessboard corners drawn on the window
    const int maxScale = 2;
    const float squareSize = 1.f;  // Set this to your actual square size
    // ARRAY AND VECTOR STORAGE:

    vector<vector<Point2f> > imagePoints[2];
    vector<vector<Point3f> > objectPoints;

    // we get the size of the first image, and for each image we will check that it is the same size
    Size imageSize = images[0].size();

    int i, j, k, nimages = (int)images.size()/2;

    imagePoints[0].resize(nimages);
    imagePoints[1].resize(nimages);
    vector<Mat> goodImages;

    // loop on all the pictures
    // (outer loop iterates only on the pictures from one of the two cameras)
    for( i = j = 0; i < nimages; i++ )
    {
        // for the picture of the left and the right camera
        for( k = 0; k < 2; k++ )
        {
            // we get a new image
            Mat img = images[i*2+k];

            // we check that the image is not empty and that it has the same size than other images
            if(img.empty())
                break;
            else if( img.size() != imageSize )
            {
                cout << "The image number " << i*2+k << " has the size different "
                        "from the first image size. Skipping the pair\n";
                break;
            }

            bool found = false;
            vector<Point2f>& corners = imagePoints[k][j];
            for( int scale = 1; scale <= maxScale; scale++ )
            {
                Mat timg;
                // we resize the picture if necessary
                if( scale == 1 )
                    timg = img;
                else
                    resize(img, timg, Size(), scale, scale);

                // we search for the chessboard corners
                found = findChessboardCorners(timg, settings.boardSize, corners,
                                                CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);
                if( found )
                {
                    // if the picture was scaled, we scale back the corner coordinates
                    if( scale > 1 )
                    {
                        Mat cornersMat(corners);
                        cornersMat *= 1./scale;
                    }
                    break;
                }
            }

            // if user want to display the corners in a GUI
            if( displayCorners )
            {
                Mat cimg, cimg1;
                cvtColor(img, cimg, COLOR_GRAY2BGR);
                drawChessboardCorners(cimg, settings.boardSize, corners, found);
                double sf = 640./MAX(img.rows, img.cols); // scale factor stuff
                resize(cimg, cimg1, Size(), sf, sf);      // scale factor stuff
                imshow("corners", cimg1);
                char c = (char)waitKey(500);
                if( c == 27 || c == 'q' || c == 'Q' ) //Allow ESC to quit
                    exit(-1);
            }
            else
                putchar('.');

            if( !found )
                break;

            // if we found the corners, we refine the corner locations
            cornerSubPix(img, corners, Size(11,11), Size(-1,-1),
                         TermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 30, 0.01));
        }
        // if we found the corners in each of the two pictures,
        // we add them to the good images list
        if( k == 2 )
        {
            goodImages.push_back(images[i*2]);
            goodImages.push_back(images[i*2+1]);
            j++;
        }
    } // end of the loop on all the pictures

    cout << j << " pairs have been successfully detected.\n";
    nimages = j;
    if( nimages < 2 )
    {
        cout << "Error: too little pairs to run the calibration\n";
        return;
    }

    // we remove unitialized image points
    // (which appear when there is a picture on which we don't find the chessboard pattern)
    imagePoints[0].resize(nimages);
    imagePoints[1].resize(nimages);

    // we create a list containing the positions of the chessboard corner points in the real world
    // in a coordinate system where the chessboard plane is in the plane z = 0
    objectPoints.resize(nimages);
    for( i = 0; i < nimages; i++ )
        for( j = 0; j < settings.boardSize.width; j++ )
            for( k = 0; k < settings.boardSize.height; k++ )
                objectPoints[i].push_back(Point3f(j*squareSize, k*squareSize, 0));


    cout << "Running stereo calibration ...\n";

    Mat cameraMatrix[2], distCoeffs[2];
    cameraMatrix[0] = Mat::eye(3, 3, CV_64F);
    cameraMatrix[1] = Mat::eye(3, 3, CV_64F);
    Mat R, T, E, F;

    double rms = stereoCalibrate(objectPoints, imagePoints[0], imagePoints[1],
                    cameraMatrix[0], distCoeffs[0],
                    cameraMatrix[1], distCoeffs[1],
                    imageSize, R, T, E, F,
                    TermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 100, 1e-5),
                    CV_CALIB_FIX_ASPECT_RATIO +
                    CV_CALIB_ZERO_TANGENT_DIST +
                    CV_CALIB_SAME_FOCAL_LENGTH +
                    CV_CALIB_RATIONAL_MODEL +
                    CV_CALIB_FIX_K3 + CV_CALIB_FIX_K4 + CV_CALIB_FIX_K5);
    cout << "done with RMS error=" << rms << endl; // RMS = Root Mean Square error

    // CALIBRATION QUALITY CHECK
    // because the output fundamental matrix implicitly
    // includes all the output information,
    // we can check the quality of calibration using the
    // epipolar geometry constraint: m2^t*F*m1=0
    double err = 0;
    int npoints = 0;
    vector<Vec3f> lines[2];
    for( i = 0; i < nimages; i++ )
    {
        int npt = (int)imagePoints[0][i].size();
        Mat imgpt[2];
        for( k = 0; k < 2; k++ )
        {
            imgpt[k] = Mat(imagePoints[k][i]);
            undistortPoints(imgpt[k], imgpt[k], cameraMatrix[k], distCoeffs[k], Mat(), cameraMatrix[k]);
            computeCorrespondEpilines(imgpt[k], k+1, F, lines[k]);
        }
        for( j = 0; j < npt; j++ )
        {
            double errij = fabs(imagePoints[0][i][j].x*lines[1][j][0] +
                                imagePoints[0][i][j].y*lines[1][j][1] + lines[1][j][2]) +
                           fabs(imagePoints[1][i][j].x*lines[0][j][0] +
                                imagePoints[1][i][j].y*lines[0][j][1] + lines[0][j][2]);
            err += errij;
        }
        npoints += npt;
    }
    cout << "average reprojection err = " <<  err/npoints << endl;

    // save intrinsic parameters
    FileStorage fs("intrinsics.yml", CV_STORAGE_WRITE);
    if( fs.isOpened() )
    {
        fs << "M1" << cameraMatrix[0] << "D1" << distCoeffs[0] <<
              "M2" << cameraMatrix[1] << "D2" << distCoeffs[1];
        fs.release();
    }
    else
        cout << "Error: can not save the intrinsic parameters\n";

    // stereo rectification
    Mat R1, R2, P1, P2, Q;
    Rect validRoi[2]; // rectangles inside the rectified images where all the pixels are valid
                      // (smaller than full frame)

    stereoRectify(cameraMatrix[0], distCoeffs[0],
                  cameraMatrix[1], distCoeffs[1],
                  imageSize, R, T, R1, R2, P1, P2, Q,
                  CALIB_ZERO_DISPARITY, 1, imageSize, &validRoi[0], &validRoi[1]);

    // save extrinsic parameters
    fs.open("extrinsics.yml", CV_STORAGE_WRITE);
    if( fs.isOpened() )
    {
        fs << "R" << R << "T" << T << "R1" << R1 << "R2" << R2 << "P1" << P1 << "P2" << P2 << "Q" << Q;
        fs.release();
    }
    else
        cout << "Error: can not save the extrinsic parameters\n";

    // OpenCV can handle left-right
    // or up-down camera arrangements
    bool isVerticalStereo = fabs(P2.at<double>(1, 3)) > fabs(P2.at<double>(0, 3));

    // COMPUTE AND DISPLAY RECTIFICATION
    if( !settings.showRectified )
        return;

    Mat rmap[2][2];  // rectify maps
    // IF BY CALIBRATED (BOUGUET'S METHOD)
    if( settings.useCalibrated )
    {
        // we already computed everything
    }
    // OR ELSE HARTLEY'S METHOD
    // use intrinsic parameters of each camera, but
    // compute the rectification transformation directly
    // from the fundamental matrix
    else
    {
        vector<Point2f> allimgpt[2];
        for( k = 0; k < 2; k++ )
        {
            for( i = 0; i < nimages; i++ )
                std::copy(imagePoints[k][i].begin(), imagePoints[k][i].end(), back_inserter(allimgpt[k]));
        }
        F = findFundamentalMat(Mat(allimgpt[0]), Mat(allimgpt[1]), FM_8POINT, 0, 0);
        Mat H1, H2;
        stereoRectifyUncalibrated(Mat(allimgpt[0]), Mat(allimgpt[1]), F, imageSize, H1, H2, 3);

        R1 = cameraMatrix[0].inv()*H1*cameraMatrix[0];
        R2 = cameraMatrix[1].inv()*H2*cameraMatrix[1];
        P1 = cameraMatrix[0];
        P2 = cameraMatrix[1];
    }

    //Precompute maps for cv::remap()
    initUndistortRectifyMap(cameraMatrix[0], distCoeffs[0], R1, P1, imageSize, CV_16SC2, rmap[0][0], rmap[0][1]);
    initUndistortRectifyMap(cameraMatrix[1], distCoeffs[1], R2, P2, imageSize, CV_16SC2, rmap[1][0], rmap[1][1]);

    Mat canvas;
    double sf;  // some ratio, to set canvas to pre-determined size 
    int w, h;
    if( !isVerticalStereo )
    {
        // if the width is bigger than the height (which is most likely)
        // the canvas width for an image will be 600 and the height will be scaled acordingly
        // to keep the original ratip, and in the canvas we will put two images,
        // one for each camera (hence the w*2)
        sf = 600./MAX(imageSize.width, imageSize.height);
        w = cvRound(imageSize.width*sf);
        h = cvRound(imageSize.height*sf);
        canvas.create(h, w*2, CV_8UC3);
    }
    else
    {
        // same principle than above, but we limit the size to 300 instead of 600
        sf = 300./MAX(imageSize.width, imageSize.height);
        w = cvRound(imageSize.width*sf);
        h = cvRound(imageSize.height*sf);
        canvas.create(h*2, w, CV_8UC3);
    }

    for( i = 0; i < nimages; i++ )
    {
        for( k = 0; k < 2; k++ )
        {
            Mat img = goodImages[i*2+k], rimg, cimg; // rimg = rectified img

            // we remap the images with the rectify maps found by initUndistortRectifyMap
            remap(img, rimg, rmap[k][0], rmap[k][1], CV_INTER_LINEAR);

            // we make a copy of the rectified img in grey shades
            cvtColor(rimg, cimg, COLOR_GRAY2BGR);
            // we get the part of canvas which contains the picture related to the current camera
            Mat canvasPart = !isVerticalStereo ? canvas(Rect(w*k, 0, w, h)) : canvas(Rect(0, h*k, w, h));
            // we resize the grey img to fill in the canvasPart
            resize(cimg, canvasPart, canvasPart.size(), 0, 0, CV_INTER_AREA);
            if( settings.useCalibrated )
            {
                // we draw the rectangle of the valid ROI on the canvasPart
                Rect vroi(cvRound(validRoi[k].x*sf), cvRound(validRoi[k].y*sf),
                          cvRound(validRoi[k].width*sf), cvRound(validRoi[k].height*sf));
                rectangle(canvasPart, vroi, Scalar(0,0,255), 3, 8);
            }
        }

        if( !isVerticalStereo )
            for( j = 0; j < canvas.rows; j += 16 )
                // horizontal line every 16 pixels
                line(canvas, Point(0, j), Point(canvas.cols, j), Scalar(0, 255, 0), 1, 8);
        else
            for( j = 0; j < canvas.cols; j += 16 )
                // vertical line every 16 pixels
                line(canvas, Point(j, 0), Point(j, canvas.rows), Scalar(0, 255, 0), 1, 8);

        imshow("rectified", canvas);
        char c = (char)waitKey();
        if( c == 27 || c == 'q' || c == 'Q' )
            break;
    } // end loop in each image
}

// read a XML/YML image list file to put all the filenames in a vector
// @param filename -> the input XML / YML file
// @param l -> the output list of filenames
static bool readStringList( const string& filename, vector<string>& l )
{
    l.resize(0);
    FileStorage fs(filename, FileStorage::READ);
    if( !fs.isOpened() )
        return false;
    FileNode n = fs.getFirstTopLevelNode();
    if( n.type() != FileNode::SEQ )
        return false;
    FileNodeIterator it = n.begin(), it_end = n.end();
    for( ; it != it_end; ++it )
        l.push_back((string)*it);
    return true;
}

// get images from the webcams
// @param settings -> the settings
// @param images -> the output list of images
static void getImagesFromWebcams(Settings settings, vector<Mat>& images) {
    // we initialize the webcams
    settings.capture[0] = VideoCapture(settings.webcam[0]);
    settings.capture[1] = VideoCapture(settings.webcam[1]);

    if (!settings.capture[0].isOpened() || !settings.capture[1].isOpened()) {
        cerr << "Error: can't access webcam stream" << endl;
        exit(1);
    }

    vector<Point2f> corners1; // content unused, but needed as argument for findChessboardCorners
    vector<Point2f> corners2; // content unused, but needed as argument for findChessboardCorners

    bool detectionMode = false; // press 'd' to start detection mode (take a new image)
    // this boolean is set to false again each time a new picture on which the pattern is found is taken.
    // this allows for a better quality of results, because:
    // 1. you can control when you want to start taking a new picture, so you have time to change
    //    position of the chessboard in the webcam pictures. Having pictures with the chessboard
    //    located in different positions results in better calibration results.
    // 2. we keep the picture only when the chessboard if found, if not, we try again to find it
    //    in the next picture 

    Mat frameCam1, frameCam2;
    while (images.size()/2 < settings.imageNumber) {
        // we get new frames
        settings.capture[0].grab();
        settings.capture[1].grab();
        settings.capture[0].retrieve(frameCam1);
        settings.capture[1].retrieve(frameCam2);

        if (detectionMode) {
            bool found1 = findChessboardCorners(frameCam1, settings.boardSize, corners1,
                                                CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);
            bool found2 = findChessboardCorners(frameCam2, settings.boardSize, corners2,
                                                CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE);

            if (found1 && found2) {
                // we keep those images
                Mat frameCam1_save, frameCam2_save;
                cvtColor(frameCam1, frameCam1_save, CV_BGR2GRAY);
                cvtColor(frameCam2, frameCam2_save, CV_BGR2GRAY);
                images.push_back(frameCam1_save);
                images.push_back(frameCam2_save);

                // we draw the chessboard corners, and we apply special effect to the image
                // so the user can notice that we have registered this image
                drawChessboardCorners(frameCam1, settings.boardSize, corners1, found1);
                drawChessboardCorners(frameCam2, settings.boardSize, corners2, found2);
                bitwise_not(frameCam1, frameCam1);
                bitwise_not(frameCam2, frameCam2);

                cout << "images: " << images.size()/2 << " / " << settings.imageNumber << endl;
                detectionMode = false;
            }
        }

        // we resize picture for a more convenient GUI display
        resize(frameCam1, frameCam1, Size(frameCam1.cols/2, frameCam1.rows/2));
        resize(frameCam2, frameCam2, Size(frameCam2.cols/2, frameCam2.rows/2));

        // GUI
        imshow("cam 1", frameCam1);
        imshow("cam 2", frameCam2);

        char key = waitKey(1); // wait before next frame
        if (key == 'd') // start detection mode
            detectionMode = true;
        if (key == 'q') // exit program
            exit(0);
    }
}


