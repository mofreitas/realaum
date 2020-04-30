#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace std;

struct Resolution{
        int width;
        int height;

        Resolution(){}

        Resolution(int w, int h){
            width = w;
            height = h;
        }
    };

Resolution camera;
vector<Resolution> available_resolutions;

void getAvailableResolutions(){
        std::ifstream file("../charuco/output_formats.txt");
        if (file.is_open()) {
            std::string line;
            int w, h;
            char x;
            while (file >> w >> x >> h) {
                available_resolutions.push_back(Resolution(w, h));
            }
            file.close();
        }
    }

    
    void setBestInputResolution(int screen_width, int screen_height){
        //Size of half screen
        int h_scr_height = screen_height;
        int h_scr_width = screen_width/2;

        //Weight (w) of each scenario
        int w_overheight = 7;
        int w_overwidth = 5;
        int w_underheight = 8;
        int w_underwidth = 10;

        //Best resolution
        int best_resolution_id = 0;
        int best_resolution_points = INT32_MAX;

        for(int i = 0; i < available_resolutions.size(); i++){
            int points = 0;
            Resolution r = available_resolutions.at(i);

            if(r.width > h_scr_width){
                points += w_overwidth*(r.width-h_scr_width);
            }
            else if(r.width < h_scr_width){
                points -= w_underwidth*(r.width-h_scr_width);
            }

            if(r.height > h_scr_height){
                points += w_overheight*(r.height-h_scr_height);
            }
            else if(r.height < h_scr_height){
                points -= w_underheight*(r.height-h_scr_height);
            }

            if(points < best_resolution_points){
                best_resolution_points = points;
                best_resolution_id = i;
            }
        }

        camera = available_resolutions.at(best_resolution_id);
    }

int main(int argc, char** argv){
    getAvailableResolutions();
    int screen_width = stoi(std::string(argv[1]));
    int screen_height = stoi(std::string(argv[2]));

    //cout << "vetor de resoluções" << endl;
    //for(int i = 0; i < available_resolutions.size(); i++ ){
    //    cout << available_resolutions.at(i).width << "x" << available_resolutions.at(i).height << endl;
    //}

    cout << "best resoltion" << endl;
    setBestInputResolution(screen_width, screen_height);

    cout << camera.width << "x" << camera.height << endl;
}