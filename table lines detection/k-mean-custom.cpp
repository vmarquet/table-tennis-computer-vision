#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <iostream>
#include <math.h>

using namespace cv;
using namespace std;

// number of clusters to split the set by
// we select 4 because it is likely to have 4 main colors:
// - the main table color (blue / green)
// - the color of the lines (white)
// - the color of the ground
// - the color of the black edges of the pictures, which come from undistortion algorithm
const int K = 4;

// we predefine the centroids (if possible) for better results of the k-mean algorithm
// format: {R,G,B}
#define CENTROID_LINE {192,188,158} // white line
#define CENTROID_EDGE {0,0,0}       // black edges
#define CENTROID_TABLE {35,43,46}      // green / blue

// we give a number to some k-mean class
#define LINE_CLASS_NUMBER  0
#define TABLE_CLASS_NUMBER 1
#define EDGE_CLASS_NUMBER  2

// blurring parameters:
const int BLUR_KERNEL_LENGTH = 15;  // must be an odd number
const int OPENING_KERNEL_LENGTH = 15;  // must be an odd number
const int CLOSING_KERNEL_LENGTH = 101;  // must be an odd number

// Nota bene: a pixel can be seen as a point in a 3-dimensional space
typedef struct {
    int R;
    int G;
    int B;
    bool fixed; // used only for centroids
} point;

uint64_t rdtsc(); // random number provider
void initialize_centroid_with_rand(point* c); // initialize centroid with random values
void initialize_fixed_centroid_with_tab(point* c, point* c_old, int* values);
float distance(point vect1, point vect2); // compute distance between 3D vectors


int main(int argc, char** argv) {
    // source image
    const char* filename = argc >= 2 ? argv[1] : "table.jpg";

    Mat src = imread(filename, CV_LOAD_IMAGE_COLOR);
    if( src.empty() )
    {
        cout << "Couldn't load " << filename << endl;
        exit(1);
    }

    char window_title[270];
    sprintf(window_title, "k-mean %s", filename);

    // we blur the source picture to remove noise
    GaussianBlur(src, src, Size(BLUR_KERNEL_LENGTH, BLUR_KERNEL_LENGTH), 0, 0);
    imshow(window_title, src);
    cout << "blur kernel size: " << BLUR_KERNEL_LENGTH << endl;
    waitKey();
    
    // variables used for the algorithm
    int iteration_number = 1;
    point centroids[K];      // the centroids of current  iteration (3 dimensional vectors: B, G, R)
    point centroids_old[K];  // the centroids of previous iteration (3 dimensional vectors: B, G, R)
    point point_current;  // variable to store temporarily a point
    int gap;  // the termination condition for the k-mean algorithm
    vector<Mat> BGR; // to split the source picture in 3 layers (B, G, R)
    Mat kmean_class_number(src.size(), CV_8U); // to store the class number of each pixel
    int nb_pixel_class[K];  // to store the number of pixel in each k-mean class
    point pixel_value_sum[K];   // to store the total value of all pixel in a k-mean class, for each color
    Mat mat_display(src.size(), CV_8U);  // a matrice for the diplay image


    // ===== k-mean first step =====
    // we initialize the centroids

    // first, we initialize everything with random value, but it will be overriden
    // by specific values if user defines specific color values for the centroids

    for (int i=0; i<K; i++)
        initialize_centroid_with_rand(&centroids[i]);


    #ifdef CENTROID_LINE
        int centroid_0_tmp[3] = CENTROID_LINE;
        initialize_fixed_centroid_with_tab(&centroids[LINE_CLASS_NUMBER], &centroids_old[LINE_CLASS_NUMBER],
                                            centroid_0_tmp);
    #endif

    #ifdef CENTROID_TABLE
        int centroid_1_tmp[3] = CENTROID_TABLE;
        initialize_fixed_centroid_with_tab(&centroids[TABLE_CLASS_NUMBER], &centroids_old[TABLE_CLASS_NUMBER],
                                            centroid_1_tmp);
    #endif

    #ifdef CENTROID_EDGE
        int centroid_3_tmp[3] = CENTROID_EDGE;
        initialize_fixed_centroid_with_tab(&centroids[EDGE_CLASS_NUMBER], &centroids_old[EDGE_CLASS_NUMBER],
                                            centroid_3_tmp);
    #endif

    
    // ===== k-mean algorithm loop =====
    do {
        // ===== k-mean 2nd step =====
        // for each pixel in the picture, we search the nearest centroid

        // we split the picture in 3 layers (B, G, R)
        split(src, BGR);

        // we cover all pixels
        for (int i=0; i<src.rows; i++) {
            for (int j=0; j<src.cols; j++) {
                // we get the pixel at current position
                point_current.B = (int) BGR[0].at<uchar>(i,j);
                point_current.G = (int) BGR[1].at<uchar>(i,j);
                point_current.R = (int) BGR[2].at<uchar>(i,j);

                // we search the closest centroid:
                // we compute the distance to the first centroid
                int dist_min = distance(point_current, centroids[0]);
                int candidate = 0;
                // then we search if there is a closer other centroid
                int dist_current;
                for (int n=1; n<K; n++){
                    if( (dist_current = distance(point_current, centroids[n])) < dist_min ) {
                        // if the centroid is closer
                        dist_min = dist_current;
                        candidate = n;
                    }
                }
                // so now we have the closest centroid, for this picture pixel

                // we store the number of the closest centroid (= k-mean class) for this pixel
                kmean_class_number.at<unsigned char>(i,j) = (unsigned char)candidate;
            }
        }

        // we display the picture with the k-mean class for each pixel
        // we multiply each class number by a big value, else we won't see the difference
        for (int i=0; i<src.rows; i++) {
            for (int j=0; j<src.cols; j++) {
                mat_display.at<unsigned char>(i,j) = kmean_class_number.at<unsigned char>(i,j) * 50;
            }
        }
        imshow(window_title, mat_display);
        waitKey();

        // ===== k-mean 3rd step =====
        // we compute the new value for each centroid
        // it's the mean of the values of all the pixels that belongs to that class (see 2nd step)

        // we reset counters that will be used later
        for(int i=0; i<K; i++) {
            nb_pixel_class[i] = 0;  // to store the number of pixel in each k-mean class
            pixel_value_sum[i].B = 0;   // total sum of all pixel in k-mean class i, color B
            pixel_value_sum[i].G = 0;   // total sum of all pixel in k-mean class i, color R
            pixel_value_sum[i].R = 0;   // total sum of all pixel in k-mean class i, color G
        }

        // we loop through the picture to count the number of pixels in each k-mean class,
        // and the sum of the color values for each color in each class
        for (int i=0; i<src.rows; i++) {
            for (int j=0; j<src.cols; j++) {
                int class_number = (int)kmean_class_number.at<unsigned char>(i,j);
                if (class_number < 0 || class_number >= K) {
                    cerr << "ERROR: class_number not valid." << endl;
                    abort();
                }
                nb_pixel_class[class_number] = nb_pixel_class[class_number] + 1;
                pixel_value_sum[class_number].G += src.at<Vec3b>(i,j)[0];
                pixel_value_sum[class_number].B += src.at<Vec3b>(i,j)[1];
                pixel_value_sum[class_number].R += src.at<Vec3b>(i,j)[2];
            }
        }

        // we compute the new positions of centroids,
        // unless centroids are fixed (if they have been defined by the user)
        for (int i=0; i<K; i++) {
            if (centroids[i].fixed == false) {
                // we record old centroids value (used in the termination condition)
                centroids_old[i].B = centroids[i].B;
                centroids_old[i].G = centroids[i].G;
                centroids_old[i].R = centroids[i].R;

                // for each class, we make the mean pixel value on each color
                // and we replace the centroid of that class with the new values
                if (nb_pixel_class[i] != 0) {
                    centroids[i].B = pixel_value_sum[i].B / nb_pixel_class[i];
                    centroids[i].G = pixel_value_sum[i].G / nb_pixel_class[i];
                    centroids[i].R = pixel_value_sum[i].R / nb_pixel_class[i];
                } else {
                    // we choose a random value for the centroid
                    initialize_centroid_with_rand(&centroids[i]);
                }
            }
        }

        // ===== k-mean 4th step =====
        // termination condition: we compute the mean of the difference between the centroid
        // we computed during the current iteration, and the previous centroid
        gap = 0;
        for (int i=0; i<K; i++) {
            // we don't compute the distance between current and previous iteration
            // for fixed centroids, because it's always 0
            if (centroids[i].fixed == false)
                gap += distance(centroids[i], centroids_old[i]);
        }
        gap = gap / K;

        cout << "iteration " << iteration_number++ << " finished, gap " << gap << endl;

    } while (gap > 1);  // termination condition for k-mean

    // we display the final result: a binarized image with the pixels of k-mean class
    // of the table lines white, and all other classes' pixels black
    Mat mat_binarized(src.size(), CV_8U);
    for (int i=0; i<src.rows; i++) {
        for (int j=0; j<src.cols; j++) {
            if (kmean_class_number.at<unsigned char>(i,j) == LINE_CLASS_NUMBER)
                mat_binarized.at<unsigned char>(i,j) = 255;
            else
                mat_binarized.at<unsigned char>(i,j) = 0;
        }
    }
    cout << "displaying binarized picture" << endl;
    imshow(window_title, mat_binarized);
    waitKey();

    // we perform an opening, to remove the lines of the table and keep only the big blobs
    // (like reflection on the table) that we want to eliminate
    Mat mat_opening(src.size(), CV_8U);
    morphologyEx(mat_binarized, mat_opening, MORPH_OPEN,
                 getStructuringElement(MORPH_ELLIPSE, Size(OPENING_KERNEL_LENGTH,OPENING_KERNEL_LENGTH)));
    cout << "dislaying binarized picture after opening" << endl;
    imshow(window_title, mat_opening);
    waitKey();

    // now we subtract the opening to the binarized picture, to remove all big blobs
    for (int i=0; i<src.rows; i++) {
        for (int j=0; j<src.cols; j++) {
            if (mat_binarized.at<unsigned char>(i,j) == 255 && mat_opening.at<unsigned char>(i,j) == 255)
                mat_binarized.at<unsigned char>(i,j) = 0;
        }
    }
    cout << "displaying binarized picture after removing big blobs" << endl;
    imshow(window_title, mat_binarized);
    waitKey();

    // we apply a closing, to have line appear more straight
    Mat mat_closing(src.size(), CV_8U);
    morphologyEx(mat_binarized, mat_closing, MORPH_CLOSE,
                 getStructuringElement(MORPH_ELLIPSE, Size(CLOSING_KERNEL_LENGTH,CLOSING_KERNEL_LENGTH)));
    printf("dislaying binarized picture after closing \n");
    imshow(window_title, mat_closing);
    waitKey();

    return 0;
}

// random number provider
uint64_t rdtsc() {
   uint32_t hi, lo;
   __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
   return ( (uint64_t)lo | ((uint64_t)hi)<<32 );
}

// initialize centroid with random value
void initialize_centroid_with_rand(point* c) {
    srand(rdtsc());
    c->R = rand()%255;
    c->G = rand()%255;
    c->B = rand()%255;
    c->fixed = false;
}

// initialize fixed centroid with tab
void initialize_fixed_centroid_with_tab(point* c, point* c_old, int* values) {
    c->R = values[0];  c_old->R = values[0];
    c->G = values[1];  c_old->G = values[1];
    c->B = values[2];  c_old->B = values[2];
    c->fixed = true;
}

// compute the distance between two points
float distance(point vect1, point vect2) {
    int x = 0, y = 0, z = 0;
    x = vect2.R - vect1.R;
    y = vect2.G - vect1.G;
    z = vect2.B - vect1.B;
    return sqrt(pow(x,2) + pow(y,2) + pow(z,2));
}


