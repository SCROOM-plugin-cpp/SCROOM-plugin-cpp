//
// Created by jelle on 02/06/2021.
//

#ifndef SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
#define SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
#include <string>

class CustomColor {
public:
    std::string name;
    std::vector<std::string> aliasses;
    float cMultiplier;
    float mMultiplier;
    float yMultiplier;
    float kMultiplier;
    CustomColor(std::string colorName, float c, float m, float y, float k) {
        name = colorName;
        cMultiplier = c;
        mMultiplier = m;
        yMultiplier = y;
        kMultiplier = k;
    }

    std::string getName(){
        return name;
    }

    std::vector<std::string> getAliasses() {
        return aliasses;
    }


    float * getColor(){
        float color[4] = {cMultiplier, mMultiplier, yMultiplier, kMultiplier};

        return color;
    }
};


#endif //SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
