#define cimg_display 0
#include "CImg.h"
#include <Eigen/Dense>

using namespace cimg_library;
using namespace std;
using namespace Eigen;

int getIndex(int col, int row, int width, int height); //Function declaration for all the functions       
Vector3d * delseam(Vector3d *image, int width, int height);
Vector3d * rotate(Vector3d *image, int width, int height);
double cal_energy(Vector3d *image, int col, int row, int width, int height);

int main(int argc, char * argv[]) {

    char * input_image = argv[1]; //storing input image name      
    char * output_image = argv[2]; //storing output image name
    int output_width = atoi(argv[3]); //storing output image width    
    int output_height = atoi(argv[4]); //Storing output image height

    CImg < double > input(argv[1]); //Stub code for converting input image to LAB color
    CImg < double > lab = input.RGBtoLab();
    Vector3d *image = new Vector3d[input.width() * input.height()];
    for (unsigned int i = 0; i < input.width(); i++) {
        for (unsigned int j = 0; j < input.height(); j++) {
            image[i * input.height() + j][0] = lab(i, j, 0);
            image[i * input.height() + j][1] = lab(i, j, 1);
            image[i * input.height() + j][2] = lab(i, j, 2);
        }
    }

    int width = input.width();
    int height = input.height();

    for (; width > output_width; width--) { // delete seam vertically
        image = delseam(image, width, input.height());
    }

    image = rotate(image, width, height); // transposose / rotate to remove horizontal seam

    for (; height > output_height; height--) {
        image = delseam(image, height, width);
    }

    image = rotate(image, height, width); // rotating back to get noraml image

    CImg < double > output(atoi(argv[3]), atoi(argv[4]), input.depth(), input.spectrum(), 0); // Stub code for output
    for (unsigned int i = 0; i < output.width(); i++) {
        for (unsigned int j = 0; j < output.height(); j++) {
            output(i, j, 0) = image[i * output.height() + j][0];
            output(i, j, 1) = image[i * output.height() + j][1];
            output(i, j, 2) = image[i * output.height() + j][2];
        }
    }
    CImg < double > rgb = output.LabtoRGB();
    if (strstr(argv[2], "png"))
        rgb.save_png(argv[2]);
    else if (strstr(argv[2], "jpg"))
        rgb.save_jpeg(argv[2]);

    delete[] image;
    return 0;
}

Vector3d * delseam(Vector3d *image, int width, int height) { // for deleting a verical seam

    double * energy = new double[width * height];
    double crest = 0.0; // initalizing max and energy value

    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int col = 0; col < width; col++) {
            int index = col * height + row; // calling calculate enegry on each pixel
            energy[index] = cal_energy(image, col, row, width, height);
            crest = max < double > (crest, energy[index]);
        }
    }

    double * M = new double[width * height];

    for (unsigned int col = 0; col < width; col++) {
        int index = getIndex(col, 0, width, height);
        M[index] = energy[index]; //get cumilative min energy, cost at each pixel
    }

    for (unsigned int row = 1; row < height; row++) {
        for (unsigned int col = 0; col < width; col++) {
            int index = getIndex(col, row, width, height);

            double C[3] = {
                std::numeric_limits < double > ::max(), // getting cost of all possible paths up,left,right
                std::numeric_limits < double > ::max(),
                std::numeric_limits < double > ::max()
            };
            if (0 != col) {
                C[0] = M[getIndex(col - 1, row - 1, width, height)];
            }

            C[1] = M[getIndex(col, row - 1, width, height)];

            if (width - 1 != col) {
                C[2] = M[getIndex(col + 1, row - 1, width, height)];
            }

            M[index] = energy[index] + min(min(C[0], C[1]), C[2]); // stroing  total min energy of pixel 
        }
    }

    int * seam = new int[height]; //Finding least cost seam to remove 
    int leftCol = 0, rightCol = width - 1;
    for (int row = height - 1; row >= 0; row--) {
        seam[row] = leftCol; //columin with min M value
        for (unsigned int col = leftCol; col <= rightCol; col++) {
            int index = getIndex(col, row, width, height);
            if (M[index] < M[getIndex(seam[row], row, width, height)])
                seam[row] = col;
        }

        leftCol = std::max(0, seam[row] - 1); // searching left and right col for next row
        rightCol = std::min(width - 1, seam[row] + 1);
    }

    string fname = std::to_string(width) + ".jpg";
    char * outName = & fname[0u];

    Vector3d * carvedimage = new Vector3d[(width - 1) * height]; // Creating a new image for ouput
    for (unsigned int row = 0; row < height; row++) {
        double minCost = std::numeric_limits < double > ::max();
        int offset = 0;
        for (unsigned int col = 0; col < width - 1; col++) {
            if (col == seam[row]) {

                offset++;
            }
            for (unsigned int k = 0; k < 3; k++) {
                carvedimage[getIndex(col, row, width, height)][k] = image[getIndex(col + offset, row, width, height)][k];
            }
        }
    }
    delete[] image;
    delete[] energy;
    delete[] M;
    delete[] seam;

    return carvedimage;         // returning iamge after deleting the seam
}

int getIndex(int col, int row, int width, int height) {
    return col * height + row;
}

Vector3d * rotate(Vector3d *image, int width, int height) {

    Vector3d * carvedimage = new Vector3d[width * height]; //Transpose the image so that we dont have to write another seam removal for horizontal seams and just transpose the image and use vertical seam removal
    int curindex, newindex;

    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int col = 0; col < width; col++) {
            curindex = getIndex(col, row, width, height);
            newindex = getIndex(row, col, height, width);

            carvedimage[newindex] = image[curindex];
        }
    }
    delete[] image;
    return carvedimage;
}

double getDx(Vector3d *image, int col, int row, int width, int height) { // X derivative

    int divide = 1, index1, index2;
    if (0 == col) {
        index1 = getIndex(col, row, width, height);
        index2 = getIndex(col + 1, row, width, height);
    } else if (width - 1 == col) {
        index1 = getIndex(col - 1, row, width, height);
        index2 = getIndex(col, row, width, height);
    } else {
        index1 = getIndex(col - 1, row, width, height);
        index2 = getIndex(col + 1, row, width, height);
        divide = 2.0;
    }

    return abs(image[index2][0] - image[index1][0]) / (double) divide; // Lightness value is 0 
}

double getDy(Vector3d *image, int col, int row, int width, int height) { //  Y derivative

    int divide = 1, index1, index2;
    if (0 == row) {
        index1 = getIndex(col, row, width, height);
        index2 = getIndex(col, row + 1, width, height);
    } else if (height - 1 == row) {
        index1 = getIndex(col, row - 1, width, height);
        index2 = getIndex(col, row, width, height);
    } else {
        index1 = getIndex(col, row - 1, width, height);
        index2 = getIndex(col, row + 1, width, height);
        divide = 2.0;
    }

    return abs(image[index2][0] - image[index1][0]) / (double) divide;
}



double cal_energy(Vector3d *image, int col, int row, int width, int height) { // function to calculate energy
    return getDx(image, col, row, width, height) + getDy(image, col, row, width, height);

}

