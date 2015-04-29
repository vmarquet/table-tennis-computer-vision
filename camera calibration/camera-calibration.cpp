// g++ camera-calibration.cpp -o camera-calibration `pkg-config --cflags --libs opencv`
// see http://docs.opencv.org/doc/tutorials/calib3d/camera_calibration/camera_calibration.html

#include <iostream>
#include <sstream>
#include <time.h>
#include <stdio.h>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>

#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
#endif

using namespace cv;
using namespace std;

static void help()
{
    cout <<  "This is a camera calibration sample." << endl
         <<  "Usage: ./camera-calibration [configurationFile.xml]"  << endl
         <<  "Near the sample file you'll find the configuration file, which has detailed help of "
             "how to edit it.  It may be any OpenCV supported file format XML/YAML." << endl;
}

class Settings
{
public:
    Settings() : goodInput(false) {}

    // possible patterns for the calibration process
    enum Pattern { NOT_EXISTING, CHESSBOARD, CIRCLES_GRID, ASYMMETRIC_CIRCLES_GRID };

    // possible input for the calibration process
    enum InputType {INVALID, CAMERA, VIDEO_FILE, IMAGE_LIST};

    Size boardSize;            // The size of the board -> Number of items by width and height
    Pattern calibrationPattern;// One of the Chessboard, circles, or asymmetric circle pattern
    float squareSize;          // The size of a square in your defined unit (point, millimeter,etc).
    int nrFrames;              // The number of frames to use from the input for calibration
    float aspectRatio;         // The aspect ratio
    int delay;                 // In case of a video input
    bool bwritePoints;         //  Write detected feature points
    bool bwriteExtrinsics;     // Write extrinsic parameters
    bool calibZeroTangentDist; // Assume zero tangential distortion
    bool calibFixPrincipalPoint;// Fix the principal point at the center
    bool flipVertical;          // Flip the captured images around the horizontal axis
    string outputFileName;      // The name of the file where to write
    bool showUndistorsed;       // Show undistorted images after calibration
    string input;               // The input ->

    int cameraID;
    vector<string> imageList;
    int atImageList;
    VideoCapture inputCapture;
    InputType inputType;
    bool goodInput;
    int flag;  // flags for the calibration

private:
    string patternToUse;

public:
    void write(FileStorage& fs) const                        //Write serialization for this class
    {
        fs << "{" << "BoardSize_Width"  << boardSize.width
                  << "BoardSize_Height" << boardSize.height
                  << "Square_Size"         << squareSize
                  << "Calibrate_Pattern" << patternToUse
                  << "Calibrate_NrOfFrameToUse" << nrFrames
                  << "Calibrate_FixAspectRatio" << aspectRatio
                  << "Calibrate_AssumeZeroTangentialDistortion" << calibZeroTangentDist
                  << "Calibrate_FixPrincipalPointAtTheCenter" << calibFixPrincipalPoint

                  << "Write_DetectedFeaturePoints" << bwritePoints
                  << "Write_extrinsicParameters"   << bwriteExtrinsics
                  << "Write_outputFileName"  << outputFileName

                  << "Show_UndistortedImage" << showUndistorsed

                  << "Input_FlipAroundHorizontalAxis" << flipVertical
                  << "Input_Delay" << delay
                  << "Input" << input
           << "}";
    }

    // Read serialization for this class
    // we read the input XML and we fill the attributes of the current Settings object
    void read(const FileNode& node)                          
    {
        node["BoardSize_Width" ] >> boardSize.width;
        node["BoardSize_Height"] >> boardSize.height;
        node["Calibrate_Pattern"] >> patternToUse;
        node["Square_Size"]  >> squareSize;
        node["Calibrate_NrOfFrameToUse"] >> nrFrames;
        node["Calibrate_FixAspectRatio"] >> aspectRatio;
        node["Write_DetectedFeaturePoints"] >> bwritePoints;
        node["Write_extrinsicParameters"] >> bwriteExtrinsics;
        node["Write_outputFileName"] >> outputFileName;
        node["Calibrate_AssumeZeroTangentialDistortion"] >> calibZeroTangentDist;
        node["Calibrate_FixPrincipalPointAtTheCenter"] >> calibFixPrincipalPoint;
        node["Input_FlipAroundHorizontalAxis"] >> flipVertical;
        node["Show_UndistortedImage"] >> showUndistorsed;
        node["Input"] >> input;
        node["Input_Delay"] >> delay;
        interprate(); // to check the input data
    }

    // we check that the data in the input XML is correct and that there is no inconsistency
    void interprate()
    {
        goodInput = true;
        if (boardSize.width <= 0 || boardSize.height <= 0)
        {
            cerr << "Invalid Board size: " << boardSize.width << " " << boardSize.height << endl;
            goodInput = false;
        }
        if (squareSize <= 10e-6)
        {
            cerr << "Invalid square size " << squareSize << endl;
            goodInput = false;
        }
        if (nrFrames <= 0)
        {
            cerr << "Invalid number of frames " << nrFrames << endl;
            goodInput = false;
        }

        if (input.empty()) {    // Check for valid input
            inputType = INVALID;
            cerr << " Inexistent input: " << input;
            goodInput = false;
        }
        else
        {
            // if the first character of "Input" field in the XML is a number
            // we parse this field as a number
            if (input[0] >= '0' && input[0] <= '9')
            {
                stringstream ss(input); // convert string to stringstream...
                ss >> cameraID;         // ...in order to extract the number
                inputType = CAMERA;
            }
            else
            {
                // we check if the value of the "Input" field is a filename (XML file,
                // and if we can open that XML file and read the list of pictures to use
                if (readStringList(input, imageList))
                {
                    inputType = IMAGE_LIST;
                    nrFrames = (nrFrames < (int)imageList.size()) ? nrFrames : (int)imageList.size();
                }
                // if the value of the "Input" field it is not an XML file,
                // it's the filename of the video file to use for the calibration
                else
                    inputType = VIDEO_FILE;
            }
            // we open the input file(s) / stream, depending on the input type
            if (inputType == CAMERA)        inputCapture.open(cameraID);
            if (inputType == VIDEO_FILE)    inputCapture.open(input);

            // if input is a video file or webcam stream, we check that opening it succeeded
            if (inputType != IMAGE_LIST && !inputCapture.isOpened())
                inputType = INVALID;
        }

        // NB: |= is bitwise OR (only add bits)
        flag = 0;
        if(calibFixPrincipalPoint) flag |= CV_CALIB_FIX_PRINCIPAL_POINT;
        if(calibZeroTangentDist)   flag |= CV_CALIB_ZERO_TANGENT_DIST;
        if(aspectRatio)            flag |= CV_CALIB_FIX_ASPECT_RATIO;

        // we check that the pattern used for the calibration is a valid one
        calibrationPattern = NOT_EXISTING;
        if (!patternToUse.compare("CHESSBOARD"))              calibrationPattern = CHESSBOARD;
        if (!patternToUse.compare("CIRCLES_GRID"))            calibrationPattern = CIRCLES_GRID;
        if (!patternToUse.compare("ASYMMETRIC_CIRCLES_GRID")) calibrationPattern = ASYMMETRIC_CIRCLES_GRID;
        if (calibrationPattern == NOT_EXISTING)
        {
            cerr << " Inexistent camera calibration mode: " << patternToUse << endl;
            goodInput = false;
        }
        atImageList = 0;

    }

    // we get the next image from the input
    // NB: this shouldn't be in a "Settings" class !?
    Mat nextImage()
    {
        Mat result;
        if( inputCapture.isOpened() )
        {
            Mat view0;
            inputCapture >> view0;
            view0.copyTo(result);
        }
        else if( atImageList < (int)imageList.size() )
            result = imread(imageList[atImageList++], CV_LOAD_IMAGE_COLOR);

        return result;
    }

    // if the input is several images, we open the separate XML file containing
    // the list of the images to use
    // if the input is an video file (and not a XML file containing picture list),
    // this function will return false
    static bool readStringList( const string& filename, vector<string>& l )
    {
        l.clear();
        FileStorage fs;
        try {
            fs.open(filename, FileStorage::READ);
        }
        catch (...) {
            return false;
        }

        if( !fs.isOpened() )
            return false;

        FileNode n = fs.getFirstTopLevelNode();
        if( n.type() != FileNode::SEQ )
            return false;

        // we read all the picture filenames
        FileNodeIterator it = n.begin(), it_end = n.end();
        for( ; it != it_end; ++it )
            l.push_back((string)*it);
        
        return true;
    }
};

static void read(const FileNode& node, Settings& x, const Settings& default_value = Settings())
{
    if(node.empty())
        x = default_value;
    else
        x.read(node);
}

// 3 possible modes
enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };
// DETECTION mode:
// - starting mode when using vide input
// - the pattern recognition is run in this mode, but the data is not used (nothing happens),
//   the user must press 'g' to start capturing mode
// CAPTURING mode:
// - the program stores data when the pattern recognition succeeds in a picture,
//   and when enough data has been stored (pattern recognized in enough pictures),
//   the calibration algorithm is run
// - it is the starting mode for pictures input
// CALIBRATED mode:
// - when the calibration is successful, we switch to this mode


bool runCalibrationAndSave(Settings& s, Size imageSize, Mat&  cameraMatrix, Mat& distCoeffs,
                           vector<vector<Point2f> > imagePoints );


int main(int argc, char* argv[])
{
    help(); // display the help message

    // we read settings (which pattern is used, ...) from an XML file
    Settings s;
    const string inputSettingsFile = argc > 1 ? argv[1] : "default.xml";
    FileStorage fs(inputSettingsFile, FileStorage::READ); // open the file and read the settings

    if (!fs.isOpened())
    {
        cout << "Could not open the configuration file: \"" << inputSettingsFile << "\"" << endl;
        return -1;
    }

    fs["Settings"] >> s;
    fs.release();   // close Settings file

    if (!s.goodInput)
    {
        cout << "Invalid input detected. Application stopping." << endl;
        return -1;
    }


    // we prepare variables for the calibration algorithm

    // the inner vector is the list of points detected on the pattern on a single image
    // the outer vector is the list of points for all images on which the pattern detection was successful
    vector<vector<Point2f> > imagePoints;
    
    Mat cameraMatrix, distCoeffs;
    Size imageSize;  // Size object has 2 attributes: width and height
    // CAPTURING mode for images, DETECTION mode for video
    int mode = (s.inputType == Settings::IMAGE_LIST) ? CAPTURING : DETECTION;
    clock_t prevTimestamp = 0;
    const Scalar RED(0,0,255), GREEN(0,255,0);
    const char ESC_KEY = 27; // press ESCAPE to exit


    // loop on the images
    for(int i = 0;;++i)
    {
        Mat view = s.nextImage(); // we get the next image
        bool blinkOutput = false;

        // if we have collected enough images on which the pattern detection was successful,
        // we run the calibration algorithm
        // NB: nrFrames is the number of frames to use for the calibration (given in the XML)
        if( mode == CAPTURING && imagePoints.size() >= (unsigned)s.nrFrames )
        {
            // if calibration algorithm is successful, we switch to CALIBRATED mode
            if( runCalibrationAndSave(s, imageSize,  cameraMatrix, distCoeffs, imagePoints)) {
                mode = CALIBRATED;
                cout << "calibration successful, switching from CAPTURING mode to CALIBRATED mode" << endl;
            }
            // if calibration algorithm is not successful, we go back to DETECTION mode
            else {
                mode = DETECTION;
                cout << "calibration failed, switching from CAPTURING mode to DETECTION mode" << endl;
            }
        }

        // if no more images and we have not run the calibration algorithm yet,
        // we run it and we stop the loop
        if(view.empty() && mode != CALIBRATED)
        {
            if( imagePoints.size() > 0 ) {
                cout << "no more pictures, running calibration algorithm before exiting" << endl;
                runCalibrationAndSave(s, imageSize,  cameraMatrix, distCoeffs, imagePoints);
            }
            break;
        }


        // we get the size (width and height) of input image
        imageSize = view.size();
        // if in the XML parameters, it was specified that image must be flipped, we flip it
        if( s.flipVertical )
            flip( view, view, 0 );


        // we try to find in the pictures the calibration pattern we chose
        // it will give us a list of points (inner corners positions in the case of the checkerboard)
        // which will be later used by the calibration algorithm when we will have done this
        // for enough pictures
        bool found;
        vector<Point2f> pointBuf;
        switch( s.calibrationPattern ) // Find feature points on the input format
        {
            case Settings::CHESSBOARD:
                found = findChessboardCorners( view, s.boardSize, pointBuf,
                    CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FAST_CHECK | CV_CALIB_CB_NORMALIZE_IMAGE);
                break;
            case Settings::CIRCLES_GRID:
                found = findCirclesGrid( view, s.boardSize, pointBuf );
                break;
            case Settings::ASYMMETRIC_CIRCLES_GRID:
                found = findCirclesGrid( view, s.boardSize, pointBuf, CALIB_CB_ASYMMETRIC_GRID );
                break;
            default:
                found = false;
                break;
        }

        if (found)  // if we detected the pattern (chess / circle / asymetric circle)
        {
            cout << "picture " << i << ": pattern found" << endl;

            // we try to improve the found corners' coordinate accuracy for chessboard
            // by converting the image to gray, and by applying an algorithm which
            // give corner positions more precise than the integer number of the pixel
            if( s.calibrationPattern == Settings::CHESSBOARD)
            {
                Mat viewGray;
                cvtColor(view, viewGray, COLOR_BGR2GRAY);
                cornerSubPix( viewGray, pointBuf, Size(11,11),
                    Size(-1,-1), TermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 ));
            }

            if( mode == CAPTURING &&  // For camera only take new samples after delay time
                (!s.inputCapture.isOpened() || clock() - prevTimestamp > s.delay*1e-3*CLOCKS_PER_SEC) )
            {
                imagePoints.push_back(pointBuf);
                prevTimestamp = clock();
                blinkOutput = s.inputCapture.isOpened();
            }

            // Draw the corners.
            drawChessboardCorners( view, s.boardSize, Mat(pointBuf), found );
        }
        else
            cout << "picture " << i << ": pattern not found" << endl;


        // we display some info messages on the video
        string msg = (mode == CAPTURING) ? "100/100" :
                      mode == CALIBRATED ? "Calibrated" : "Press 'g' to start";
        int baseLine = 0;
        Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(view.cols - 2*textSize.width - 10, view.rows - 2*baseLine - 10);

        if( mode == CAPTURING )
        {
            if(s.showUndistorsed)
                msg = format( "%d/%d Undist", (int)imagePoints.size(), s.nrFrames );
            else
                msg = format( "%d/%d", (int)imagePoints.size(), s.nrFrames );
        }

        putText( view, msg, textOrigin, 1, 1, mode == CALIBRATED ?  GREEN : RED);

        if( blinkOutput )
            bitwise_not(view, view);


        // if the calibration was achieved successfully, the user can toggle the video display
        // undistortion by pressing 'u'. The following lines compute the undistorsed image
        if( mode == CALIBRATED && s.showUndistorsed )
        {
            Mat temp = view.clone();
            undistort(temp, view, cameraMatrix, distCoeffs);
        }

        // we display the image
        imshow("Image View", view);


        // we check for input by the user
        char key = (char)waitKey(s.inputCapture.isOpened() ? 50 : s.delay);

        // press ESCAPE to exit
        if( key  == ESC_KEY )
            break;

        // press 'u' to switch between original image and undistorted image
        if( key == 'u' && mode == CALIBRATED )
           s.showUndistorsed = !s.showUndistorsed;

        // press 'g' to switch to CAPTURING mode
        if( s.inputCapture.isOpened() && key == 'g' )
        {
            mode = CAPTURING;
            cout << "switching to CAPTURING mode" << endl;
            imagePoints.clear();
        }
    } // end loop on the images


    // if the input was an image list, and if the user enabled undistorsion
    // we display all images undistorsed
    if( s.inputType == Settings::IMAGE_LIST && s.showUndistorsed )
    {
        Mat view, rview, map1, map2;
        initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(),
            getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 1, imageSize, 0),
            imageSize, CV_16SC2, map1, map2);

        for(int i = 0; i < (int)s.imageList.size(); i++ )
        {
            view = imread(s.imageList[i], 1);
            if(view.empty())
                continue;
            remap(view, rview, map1, map2, INTER_LINEAR);
            imshow("Image View", rview);
            char c = (char)waitKey();
            if( c  == ESC_KEY || c == 'q' || c == 'Q' )
                break;
        }
    }

    return 0;
}



static double computeReprojectionErrors( const vector<vector<Point3f> >& objectPoints,
                                         const vector<vector<Point2f> >& imagePoints,
                                         const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                                         const Mat& cameraMatrix , const Mat& distCoeffs,
                                         vector<float>& perViewErrors)
{
    vector<Point2f> imagePoints2;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for( i = 0; i < (int)objectPoints.size(); ++i )
    {
        projectPoints( Mat(objectPoints[i]), rvecs[i], tvecs[i], cameraMatrix,
                       distCoeffs, imagePoints2);
        err = norm(Mat(imagePoints[i]), Mat(imagePoints2), CV_L2);

        int n = (int)objectPoints[i].size();
        perViewErrors[i] = (float) std::sqrt(err*err/n);
        totalErr        += err*err;
        totalPoints     += n;
    }

    return std::sqrt(totalErr/totalPoints);
}

static void calcBoardCornerPositions(Size boardSize, float squareSize, vector<Point3f>& corners,
                                     Settings::Pattern patternType /*= Settings::CHESSBOARD*/)
{
    corners.clear();

    switch(patternType)
    {
        case Settings::CHESSBOARD:
        case Settings::CIRCLES_GRID:
            for( int i = 0; i < boardSize.height; ++i )
                for( int j = 0; j < boardSize.width; ++j )
                    corners.push_back(Point3f(float( j*squareSize ), float( i*squareSize ), 0));
            break;

        case Settings::ASYMMETRIC_CIRCLES_GRID:
            for( int i = 0; i < boardSize.height; i++ )
                for( int j = 0; j < boardSize.width; j++ )
                    corners.push_back(Point3f(float((2*j + i % 2)*squareSize), float(i*squareSize), 0));
            break;
        default:
            break;
    }
}

// return value: if the calibration was successful or not
static bool runCalibration( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
                            vector<vector<Point2f> > imagePoints, vector<Mat>& rvecs, vector<Mat>& tvecs,
                            vector<float>& reprojErrs,  double& totalAvgErr)
{

    cameraMatrix = Mat::eye(3, 3, CV_64F);
    if( s.flag & CV_CALIB_FIX_ASPECT_RATIO )
        cameraMatrix.at<double>(0,0) = 1.0;

    distCoeffs = Mat::zeros(8, 1, CV_64F);

    vector<vector<Point3f> > objectPoints(1);
    calcBoardCornerPositions(s.boardSize, s.squareSize, objectPoints[0], s.calibrationPattern);

    objectPoints.resize(imagePoints.size(),objectPoints[0]);

    //Find intrinsic and extrinsic camera parameters
    double rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
                                 distCoeffs, rvecs, tvecs, s.flag|CV_CALIB_FIX_K4|CV_CALIB_FIX_K5);

    cout << "Re-projection error reported by calibrateCamera: "<< rms << endl;

    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints,
                                             rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs);

    return ok;
}

// Print camera parameters to the XML output file
static void saveCameraParams( Settings& s, Size& imageSize, Mat& cameraMatrix, Mat& distCoeffs,
                              const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                              const vector<float>& reprojErrs, const vector<vector<Point2f> >& imagePoints,
                              double totalAvgErr )
{
    FileStorage fs( s.outputFileName, FileStorage::WRITE );

    time_t tm;
    time( &tm );
    struct tm *t2 = localtime( &tm );
    char buf[1024];
    strftime( buf, sizeof(buf)-1, "%c", t2 );

    fs << "calibration_Time" << buf;

    if( !rvecs.empty() || !reprojErrs.empty() )
        fs << "nrOfFrames" << (int)std::max(rvecs.size(), reprojErrs.size());
    fs << "image_Width" << imageSize.width;
    fs << "image_Height" << imageSize.height;
    fs << "board_Width" << s.boardSize.width;
    fs << "board_Height" << s.boardSize.height;
    fs << "square_Size" << s.squareSize;

    if( s.flag & CV_CALIB_FIX_ASPECT_RATIO )
        fs << "FixAspectRatio" << s.aspectRatio;

    if( s.flag )
    {
        sprintf( buf, "flags: %s%s%s%s",
            s.flag & CV_CALIB_USE_INTRINSIC_GUESS ? " +use_intrinsic_guess" : "",
            s.flag & CV_CALIB_FIX_ASPECT_RATIO ? " +fix_aspectRatio" : "",
            s.flag & CV_CALIB_FIX_PRINCIPAL_POINT ? " +fix_principal_point" : "",
            s.flag & CV_CALIB_ZERO_TANGENT_DIST ? " +zero_tangent_dist" : "" );
        cvWriteComment( *fs, buf, 0 );

    }

    fs << "flagValue" << s.flag;

    fs << "Camera_Matrix" << cameraMatrix;
    fs << "Distortion_Coefficients" << distCoeffs;

    fs << "Avg_Reprojection_Error" << totalAvgErr;
    if( !reprojErrs.empty() )
        fs << "Per_View_Reprojection_Errors" << Mat(reprojErrs);

    if( !rvecs.empty() && !tvecs.empty() )
    {
        CV_Assert(rvecs[0].type() == tvecs[0].type());
        Mat bigmat((int)rvecs.size(), 6, rvecs[0].type());
        for( int i = 0; i < (int)rvecs.size(); i++ )
        {
            Mat r = bigmat(Range(i, i+1), Range(0,3));
            Mat t = bigmat(Range(i, i+1), Range(3,6));

            CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
            CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
            //*.t() is MatExpr (not Mat) so we can use assignment operator
            r = rvecs[i].t();
            t = tvecs[i].t();
        }
        cvWriteComment( *fs, "a set of 6-tuples (rotation vector + translation vector) for each view", 0 );
        fs << "Extrinsic_Parameters" << bigmat;
    }

    if( !imagePoints.empty() )
    {
        Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
        for( int i = 0; i < (int)imagePoints.size(); i++ )
        {
            Mat r = imagePtMat.row(i).reshape(2, imagePtMat.cols);
            Mat imgpti(imagePoints[i]);
            imgpti.copyTo(r);
        }
        fs << "Image_points" << imagePtMat;
    }
}

// return value: if the calibration was successful or not
bool runCalibrationAndSave(Settings& s, Size imageSize, Mat& cameraMatrix, Mat& distCoeffs, 
                           vector<vector<Point2f> > imagePoints )
{
    vector<Mat> rvecs, tvecs;
    vector<float> reprojErrs;
    double totalAvgErr = 0;

    bool ok = runCalibration(s,imageSize, cameraMatrix, distCoeffs, imagePoints, rvecs, tvecs,
                             reprojErrs, totalAvgErr);
    cout << (ok ? "Calibration succeeded" : "Calibration failed")
        << ". avg re projection error = "  << totalAvgErr ;

    if( ok )
        saveCameraParams( s, imageSize, cameraMatrix, distCoeffs, rvecs ,tvecs, reprojErrs,
                            imagePoints, totalAvgErr);
    return ok;
}
