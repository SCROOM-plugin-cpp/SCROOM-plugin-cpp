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
#include <unordered_set>

namespace pt = boost::property_tree;

ColorConfig::ColorConfig() {
    colors = new std::vector<CustomColor>();
}



void ColorConfig::loadFile() {
    colors->clear();
    pt::ptree root;
    boost::filesystem::path full_path(boost::filesystem::current_path());
    full_path.append("colours.json");
    try {
        pt::read_json(full_path.c_str(), root);
    }
    catch (const std::exception& e){
        // Loading didnt work
        std::cout << "Loading colours file failed. Are you sure there is a file at: " + full_path.string() + "?\n";
        std::cout << "Loading default CMYK \n";
        addNonExistantDefaultColors();
        return;
    }

    std::unordered_set<std::string> seenNamesAndAliasses = {};
    seenNamesAndAliasses.insert("v"); // Insert placeholder for varnish
    std::cout << "Loading colour config file. NOTE: v is reserved for varnish, so should not be defined as name or alias!!\n";
    for(pt::ptree::value_type& v : root.get_child("colours")){
        auto name = v.second.get<std::string>("name");
        boost::algorithm::to_lower(name); // Convert the name to lowercase

        // Check if this name has not yet been seen before
        if (seenNamesAndAliasses.find(name) != seenNamesAndAliasses.end())
        {
            // It exists
            std::cout << "ERROR: Duplicate name or alias: " << name << "!\n";
            // Color already exists, so it should not be added to the colors
            continue;
        }
        // Color is new, so we can add it to the defined colors
        // First add the name to the seen names and aliasses
        seenNamesAndAliasses.insert(name);

        float c = v.second.get<float>("cMultiplier");
        float m = v.second.get<float>("mMultiplier");
        float y = v.second.get<float>("yMultiplier");
        float k = v.second.get<float>("kMultiplier");
        CustomColor newColour = CustomColor(name, c, m, y, k);




        //Initialise aliasses vector
        std::vector<std::string> aliasses = {};
        //Try to load aliasses, if the field exists
        try{
            //Get the aliasses array
            pt::ptree array = v.second.get_child("aliasses");
            //Initialise an iterator over the aliasses array
            pt::ptree::iterator iterator = array.begin();



            //Store aliasses in vector
            for(; iterator != array.end(); iterator++) {
                //Load alias with uppercase included
                auto alias = iterator->second.get_value<std::string>();
                //Convert alias to all lowercase
                boost::algorithm::to_lower(alias);

                //Test if an alias already exists in a different colour
                if (seenNamesAndAliasses.find(alias) != seenNamesAndAliasses.end()) {
                    //It exists
                    std::cout << "ERROR: Duplicate alias: " << alias << "!\n";
                } else {
                    //It is a new alias
                    std::cout << "New alias: " << alias << "\n";
                    seenNamesAndAliasses.insert(alias);
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
    addNonExistantDefaultColors();

}

void ColorConfig::addNonExistantDefaultColors() {//Initialise an array to check whether default colours exist
    bool defaultExist[4] = {false, false, false, false};
    if (getColorByNameOrAlias("c") != nullptr) defaultExist[0] = true;
    if (getColorByNameOrAlias("m") != nullptr) defaultExist[1] = true;
    if (getColorByNameOrAlias("y") != nullptr) defaultExist[2] = true;
    if (getColorByNameOrAlias("k") != nullptr) defaultExist[3] = true;
    //If no cyan configuration exists, add the default configuration
    if(!defaultExist[0]){
        CustomColor newColour = CustomColor("c", 1, 0, 0, 0);

        colors->push_back(newColour);
    }

    //If no magenta configuration exists, add the default configuration
    if(!defaultExist[1]){
        CustomColor newColour = CustomColor("m", 0, 1, 0, 0);

        colors->push_back(newColour);
    }

    //If no yellow configuration exists, add the default configuration
    if(!defaultExist[2]){
        CustomColor newColour = CustomColor("y", 0, 0, 1, 0);

        colors->push_back(newColour);
    }

    //If no key configuration exists, add the default configuration
    if(!defaultExist[3]){
        CustomColor newColour = CustomColor("k", 0, 0, 0, 1);

        colors->push_back(newColour);
    }
}

CustomColor* ColorConfig::getColorByNameOrAlias(std::string name) {
    boost::algorithm::to_lower(name);
    auto colors = getDefinedColors();
    std::string lowerName;
    std::string lowerAlias;
    for (auto & color : *colors)
    {
        lowerName = boost::algorithm::to_lower_copy(color.getName());
        if (lowerName == name){
            return &color;
        }

        for (auto const alias : color.getAliasses()){
            lowerAlias = boost::algorithm::to_lower_copy(alias);
            if (lowerAlias == name) {
                return &color;
            }
        }

    }
    return nullptr;
}

std::vector<CustomColor>* ColorConfig::getDefinedColors() {
    return colors;
}



