#include <opencv2/aruco/charuco.hpp>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(){
    Ptr<aruco::Dictionary> dictionary =
        aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME(aruco::DICT_4X4_250));

    cv::Ptr<cv::aruco::CharucoBoard> board = cv::aruco::CharucoBoard::create(5, 7, 0.07, 0.05, dictionary);
    cv::Mat boardImage;
    board->draw( cv::Size(600, 500), boardImage, 10, 1 );

    imwrite("saida.png", boardImage);

    return 0;
}
