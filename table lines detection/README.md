Description
===========
Example programs to detect the lines of the edges of the table on a picture.


First attempt
-------------
Using [Hough transform](http://docs.opencv.org/doc/tutorials/imgproc/imgtrans/hough_lines/hough_lines.html).

![hough](https://drive.google.com/uc?export=view&id=0B31-CIvNW1LdM3loc3lYWlg2SE0)


Second attempt
--------------
Using [rectangle detection](https://opencv-code.com/tutorials/detecting-simple-shapes-in-an-image/).

![rectangle](https://drive.google.com/uc?export=view&id=0B31-CIvNW1LdMVMzdzV6WER5TFE)


k-mean
------
We can see on the above pictures that some other lines are detected whereas we don't want them to be, so we apply the k-mean algorithm (with k = 4) to segment the image in 4 parts:

* the table (which color we know: either green or blue)
* the table lines (white)
* the ground
* the black edges of the pictures, which come from undistortion algorithm (always black)

Then, we binarize the picture: the pixels belonging to the k-mean class corresponding to the table lines are set to white, and the pixels of the 3 other classes are set to black.

![binarized](https://drive.google.com/uc?export=view&id=0B31-CIvNW1LdeFhMYmplOTM2LVk)

We apply then a few [morphological operations](http://docs.opencv.org/doc/tutorials/imgproc/opening_closing_hats/opening_closing_hats.html). First, an opening to find the big blobs in the image.

![opening](https://drive.google.com/uc?export=view&id=0B31-CIvNW1LdMGlmRy1BejJkVkk)

Then, we substract the opening from the binarized image, to remove all big blobs and keep only the lines of the table.

![remove blobs](https://drive.google.com/uc?export=view&id=0B31-CIvNW1LdWHBrMXdaQlNqenc)

Then, we can apply a closing to join the lines of the table and fill gaps, but it may make some blobs appear again.

![closing](https://drive.google.com/uc?export=view&id=0B31-CIvNW1LdM091OU8zb0F2azQ)








