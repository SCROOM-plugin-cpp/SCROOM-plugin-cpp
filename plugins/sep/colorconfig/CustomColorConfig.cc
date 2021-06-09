//
// Created by developer on 01-06-21.
//
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "CustomColorConfig.hh"
#include <list>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace pt = boost::property_tree;
ColorConfig::Ptr ColorConfig::create() { return Ptr(new ColorConfig()); }

ColorConfig::ColorConfig() {
    loadFile();
}

std::vector<CustomColor>* ColorConfig::colors = {};


void ColorConfig::loadFile() {
    pt::ptree root;
    boost::filesystem::path full_path(boost::filesystem::current_path());
    full_path.append("colours.json");

    pt::read_json(full_path.c_str(), root);
    std::cout<<"It worked!";
    for(pt::ptree::value_type& v : root.get_child("colours")){
        std::string name = v.second.get<std::string>("colourName");
        float c = v.second.get<float>("cValue");
        float m = v.second.get<float>("mValue");
        float y = v.second.get<float>("yValue");
        float k = v.second.get<float>("kValue");
        CustomColor newColour = CustomColor(name, c, m, y, k);

        colors->push_back(newColour);
    }

    for(CustomColor color : *colors){
        std::cout << color.getName()<<"\n";

    }

}

CustomColor* ColorConfig::getColorByNameOrAlias(std::string name) {
    boost::algorithm::to_lower(name);
    auto colors = getDefinedColors();
    for (auto & color : *colors)
    {
        if (color.getName() == name){
            return &color;
        }
        for (auto const alias : color.getAliasses()){
            if (alias == name) {
                return &color;
            }
        }

    }
    return nullptr;
}

std::vector<CustomColor>* ColorConfig::getDefinedColors() {
    return colors;
}


