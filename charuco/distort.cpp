#include <opencv2/calib3d.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace cv;
using namespace std;

double distorcao(vector<double>& c, const double x, const double y, double& nx, double& ny){
    
    if(c.size() != 12){
        for(int i = c.size(); i < c.size(); i++){
            c.push_back(0);
        }
    }

    double r2 = pow(x, 2) + pow(y, 2);
    double r4 = pow(r2, 2);
    double r6 = pow(r2, 3);

    nx = x*(1.0 + c[0]*r2 + c[1]*r4 + c[4]*r6)/(1.0 + c[5]*r2 + c[6]*r4 + c[7]*r6);   //radial
    nx += 2.0*c[2]*x*y + c[3]*(r2 + 2.0*x*x);                                         //Tangencial
    nx += c[8]*r2 + c[9]*r4;                                                          //thin prism

    ny = y*(1.0 + c[0]*r2 + c[1]*r4 + c[4]*r6)/(1.0 + c[5]*r2 + c[6]*r4 + c[7]*r6);   //radial
    ny += 2.0*c[3]*x*y + c[2]*(r2 + 2.0*y*y);                                         //Tangencial
    ny += c[10]*r2 + c[11]*r4;                                                        //thin prism
}

int main(){
    Mat image = imread("quadriculado.jpg", IMREAD_GRAYSCALE);
    Mat image2 = Mat(image.size(), image.type());
    vector<Point2f> pontos;

    //https://docs.opencv.org/4.3.0/d9/d0c/group__calib3d.html
    vector<double> coeff = {0, 0, 0, 0, 0, 0, 0, 0, 0.15};

    cout << image.rows << " ," << image.cols  << endl;

    double dx = 1.0/image.cols;
    double dy = 1.0/image.rows;
    double nx, ny;

    for(int i = 0; i < image.rows; i++){
        for(int j = 0; j < image.cols; j++){
            distorcao(coeff, dx*j-0.5, dy*i-0.5, nx, ny);
            
            if(nx < 0.5 && nx > -0.5 &&
               ny < 0.5 && ny > -0.5){
                image2.at<uchar>((ny+0.5)/dy, (nx+0.5)/dx) = image.at<uchar>(i, j);
            }
        }
    }

    imshow("image", image2);
    int key = waitKey();
    
    if(key == 32){
        //Para remover ruído, ou pixels que não tiveram valor definido na transformação
        dilate(image2, image2, Mat());
        erode(image2, image2, Mat());
        imshow("corrected image", image2);
        key = waitKey();
    }
    
    if(key == 13){
        imwrite("prism_s1_015.png", image2);
    }
    
    
    return 0;
}
