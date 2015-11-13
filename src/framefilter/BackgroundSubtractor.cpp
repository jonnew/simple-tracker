//******************************************************************************
//* File:   BackgroundSubtractor.cpp
//* Author: Jon Newman <jpnewman snail mit dot edu>
//*
//* Copyright (c) Jon Newman (jpnewman snail mit dot edu)
//* All right reserved.
//* This file is part of the Oat project.
//* This is free software: you can redistribute it and/or modify
//* it under the terms of the GNU General Public License as published by
//* the Free Software Foundation, either version 3 of the License, or
//* (at your option) any later version.
//* This software is distributed in the hope that it will be useful,
//* but WITHOUT ANY WARRANTY; without even the implied warranty of
//* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//* GNU General Public License for more details.
//* You should have received a copy of the GNU General Public License
//* along with this source code.  If not, see <http://www.gnu.org/licenses/>.
//******************************************************************************

#include "OatConfig.h" // Generated by CMake

#include <string>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include <cpptoml.h>
#include "../../lib/utility/OatTOMLSanitize.h"
#include "../../lib/utility/IOFormat.h"

#include "BackgroundSubtractor.h"

namespace oat {

BackgroundSubtractor::BackgroundSubtractor(
            const std::string &frame_source_address,
            const std::string &frame_sink_address) :
  FrameFilter(frame_source_address, frame_sink_address)
{
    // Nothing
}

void BackgroundSubtractor::configure(const std::string& config_file, const std::string& config_key) {

    // Available options
    std::vector<std::string> options {"background"};

    // This will throw cpptoml::parse_exception if a file
    // with invalid TOML is provided
    auto config = cpptoml::parse_file(config_file);

    // See if a camera configuration was provided
    if (config->contains(config_key)) {

        // Get this components configuration table
        auto this_config = config->get_table(config_key);

        // Check for unknown options in the table and throw if you find them
        oat::config::checkKeys(options, this_config);

        std::string background_img_path;
        if (oat::config::getValue(this_config, "background", background_img_path)) {
            background_frame = cv::imread(background_img_path, CV_LOAD_IMAGE_COLOR);

            if (background_frame.data == NULL) {
                throw (std::runtime_error("File \"" + background_img_path + "\" could not be read."));
            }

            background_set = true;
        }

    } else {
        throw (std::runtime_error(oat::configNoTableError(config_key, config_file)));
    }
}

void BackgroundSubtractor::setBackgroundImage(const cv::Mat& frame) {

    background_frame = frame.clone();
    background_set = true;
}

void BackgroundSubtractor::filter(cv::Mat& frame) {
    // Throws cv::Exception if there is a size mismatch between frames,
    // or in any case where cv assertions fail.

    // Only proceed with processing if we are getting a valid frame
    if (background_set)
        frame = frame - background_frame;
    else
        // First image is always used as the default background image if one is
        // not provided in a configuration file
        setBackgroundImage(frame);
}

} /* namespace oat */