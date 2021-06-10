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

ColorConfig::ColorConfig() {
    colors = new std::vector<CustomColor>();
}



void ColorConfig::loadFile() {
    colors->clear();
    pt::ptree root;
    boost::filesystem::path full_path(boost::filesystem::current_path());
    full_path.append("colours.json");
    std::cout << full_path.c_str();
    pt::read_json(full_path.c_str(), root);
    std::cout<<"It worked!";

    std::vector<std::string> allAliasses = {};

    for(pt::ptree::value_type& v : root.get_child("colours")){
        std::string name = v.second.get<std::string>("name");
        float c = v.second.get<float>("cMultiplier");
        float m = v.second.get<float>("mMultiplier");
        float y = v.second.get<float>("yMultiplier");
        float k = v.second.get<float>("kMultiplier");
        CustomColor newColour = CustomColor(name, c, m, y, k);

        //Try to load aliasses, if the field exists
        try{
            //Get the aliasses array
            pt::ptree array = v.second.get_child("aliasses");
            //Initialise an iterator over the aliasses array
            pt::ptree::iterator iterator = array.begin();

            //Initialise aliasses vector
            std::vector<std::string> aliasses = {};

            //Store aliasses in vector
            for(; iterator != array.end(); iterator++){
                std::string alias = iterator->second.get_value<std::string>();

                //Test if an alias already exists in a different colour
                if(std::find(allAliasses.begin(), allAliasses.end(), alias) != allAliasses.end()){
                    //It exists
                    std::cout << "ERROR: Duplicate alias: " << alias << "!\n";
                } else {
                    //It is a new alias
                    std::cout << "New alias: " << alias << "\n";
                    allAliasses.push_back(alias);
                }

                aliasses.push_back(alias);
            }

            //Set aliassses for newColour
            newColour.setAliasses(aliasses);
        } catch(const std::exception& e){
            //When no aliases exist, ignore exception
            std::cout << "No aliasses found." << "\n";
        }

        colors->push_back(newColour);
    }

    for(CustomColor color : *colors){
        std::cout << color.getName()<<"\n";

    }

}

CustomColor* ColorConfig::getColorByNameOrAlias(std::string name) {
    boost::algorithm::to_lower(name); //TODO Check for version of to_lower algorithm that copies instead of changing the variable
    auto colors = getDefinedColors();
    for (auto & color : *colors)
    {
        if (color.getName() == name){ //TODO Use found algorithm above to change color.getName() to lowercase
            return &color;
        }
        for (auto const alias : color.getAliasses()){
            if (alias == name) { //TODO Same for aliasses
                return &color;
            }
        }

    }
    return nullptr;
}

std::vector<CustomColor>* ColorConfig::getDefinedColors() {
    return colors;
}



