Camera calibration
------------------
With my gear, I am able to put the camera at 2.5 meters above the table. The camera I use is a GoPro Hero 3+. In order to see the whole table from above, I have to set the range to "medium" or "wide".

The problem is that the GoPro image in "medium" or "wide" mode is very distorted, so in order to make further processing, it is needed to correct the distortion.

In order to correct the distortion with OpenCV, you must first find the camera parameters. There is an OpenCV program to do this. The process is described [here](http://docs.opencv.org/doc/tutorials/calib3d/camera_calibration/camera_calibration.html).


### Using the program

Unfortunately, some parts of the OpenCV program are not very well commented, and contain some bugs, so here are some advices:

* the chessboard used for the calibration program **must** be assymetric (width and height must be different)

* in the XML settings file, `BoardSize_Width` and `BoardSize_Height` are the number of **inner** corner in the chessboard, not the number of squares. So for a chessboard with dimension of 10x7 (squares), the XML will be:

```
<BoardSize_Width>9</BoardSize_Width>
<BoardSize_Height>6</BoardSize_Height>
```

* if your input is a video, you must press `g` to start the calibration process

* to have a good calibration, you must take at least 20 pictures, and in different positions, so move around the table while holding the chessboard above the table, and check that in the pictures selected by the program to make the calibration, the chessboard is at different positions around the table, else the antidistorsion filter will work only on some parts of the images but not on the whole images. To make sure that the calibration program don't take several consecutive pictures, set the field `<Input_Delay>` in the XML to something appropriate given the length of your video

* if you use a video instead of a list of pictures to do the processing, you may encounter a bug with the program (`Too long string or a last string w/o newline in function icvXMLSkipSpaces`), check [here](http://stackoverflow.com/questions/23267658/opencv-camera-calibration)

You can download the chessboard picture I used [here](../images/checkerboard.png).

For an example image of the results, see [here](../distortion correction).


Links
-----
* tutorials
    * http://www.theeminentcodfish.com/gopro-calibration/
    * http://www.aishack.in/tutorials/calibrating-undistorting-with-opencv-in-c-oh-yeah/
* stack overflow discussions
    * http://stackoverflow.com/questions/7902895/opencv-transform-using-chessboard/7906523#7906523
    * http://stackoverflow.com/questions/20706719/calibrate-camera-with-opencv-how-does-it-work-and-how-do-i-have-to-move-my-ches
    * http://stackoverflow.com/questions/17665912/findchessboardcorners-fails-for-calibration-image


