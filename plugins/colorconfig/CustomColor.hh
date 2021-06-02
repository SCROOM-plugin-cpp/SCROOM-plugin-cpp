//
// Created by jelle on 02/06/2021.
//

#ifndef SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
#define SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
#include <string>

class CustomColor {
public:
    std::string name;
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
};


#endif //SCROOMCPPPLUGINS_CUSTOMCOLOR_HH
