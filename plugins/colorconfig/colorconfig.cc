//
// Created by developer on 01-06-21.
//
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "colorconfig.hh"
#include <scroom/scroominterface.hh>
#include <list>

namespace pt = boost::property_tree;
ColorConfig::Ptr ColorConfig::create() { return Ptr(new ColorConfig()); }

std::string ColorConfig::getPluginName() { return "ColorConfig"; }

std::string ColorConfig::getPluginVersion() { return "0.0"; }

void    ColorConfig::registerCapabilities(ScroomPluginInterface::Ptr host)
{
//  host->registerOpenInterface("Color configuration reader", shared_from_this<ColorConfig>());
}

ColorConfig::ColorConfig() {
    loadFile();
}

ColorConfig& ColorConfig::getInstance()
{
    static ColorConfig instance;
    return instance;
}
void ColorConfig::loadFile() {
    pt::ptree root;

    std::string filePath = "/home/developer/Desktop";
    std::string name = "colours";
    pt::read_json(filePath + "/" + name + ".json", root);
    std::cout<<"It worked!";
    for(pt::ptree::value_type& v : root.get_child("colours")){
        std::string name = v.second.get<std::string>("colourName");
        float c = v.second.get<float>("cValue");
        float m = v.second.get<float>("mValue");
        float y = v.second.get<float>("yValue");
        float k = v.second.get<float>("kValue");
        Colour newColour = Colour(name, c, m, y, k);

        colours.push_back(newColour);
    }

    for(Colour colour : colours){
        cout << colour.getName()<<"\n";

    }

}