//******************************************************************************
//* File:   Viewer.cpp
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
//****************************************************************************

#include "OatConfig.h" // Generated by CMake

#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <opencv2/cvconfig.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "../../lib/utility/IOFormat.h"
#include "../../lib/utility/FileFormat.h"
#include "../../lib/shmemdf/Source.h"
#include "../../lib/shmemdf/SharedFrameHeader.h"

#include "Viewer.h"

namespace oat {

namespace bfs = boost::filesystem;
using namespace boost::interprocess;

// Constant definitions
constexpr Viewer::Milliseconds Viewer::MIN_UPDATE_PERIOD_MS;
constexpr int Viewer::COMPRESSION_LEVEL;

Viewer::Viewer(const std::string& frame_source_address) :
  name_("viewer[" + frame_source_address + "]")
, frame_source_address_(frame_source_address)
{

    // Initialize GUI update timers
    tick_ = Clock::now();
    tock_ = Clock::now();

    // Snapshot encoding
    compression_params_.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params_.push_back(COMPRESSION_LEVEL);

#ifdef HAVE_OPENGL
    try {
        cv::namedWindow(name_, cv::WINDOW_OPENGL & cv::WINDOW_KEEPRATIO);
    } catch (cv::Exception& ex) {
        oat::whoWarn(name_, "OpenCV not compiled with OpenGL support. "
                           "Falling back to OpenCV's display driver.\n");
        cv::namedWindow(name_, cv::WINDOW_NORMAL & cv::WINDOW_KEEPRATIO);
    }
#else
    cv::namedWindow(name_, cv::WINDOW_NORMAL & cv::WINDOW_KEEPRATIO);
#endif
}

void Viewer::connectToNode() {

    // Establish our a slot in the node
    frame_source_.touch(frame_source_address_);

    // Wait for sychronous start with sink when it binds the node
    frame_source_.connect();
}

bool Viewer::showImage() {

    // START CRITICAL SECTION //
    ////////////////////////////

    // Wait for sink to write to node
    node_state_ = frame_source_.wait();
    if (node_state_ == oat::NodeState::END)
        return true;

    // Clone the shared frame
    //internal_frame_ = frame_source_.clone();
    frame_source_.copyTo(internal_frame_);

    // Tell sink it can continue
    frame_source_.post();

    ////////////////////////////
    //  END CRITICAL SECTION  //

    // Get current time
    tick_ = Clock::now();

    // Figure out the time since we last updated the viewer
    Milliseconds duration = std::chrono::duration_cast<Milliseconds>(tick_ - tock_);

    // If the minimum update period has passed, show frame.
    if (duration > MIN_UPDATE_PERIOD_MS) {

        cv::imshow(name_, internal_frame_);
        tock_ = Clock::now();

        char command = cv::waitKey(1);

        if (command == 's') {
            std::string fid = makeFileName();
            cv::imwrite(makeFileName(), internal_frame_, compression_params_);
            std::cout << "Snapshot saved to " << fid << "\n";
        }
    }

    // Sink was not at END state
    return false;
}

void Viewer::storeSnapshotPath(const std::string &snapshot_path) {

    bfs::path path(snapshot_path.c_str());

    // Check that the snapshot save folder is valid
    if (!bfs::exists(path.parent_path())) {
        throw (std::runtime_error ("Requested snapshot save directory "
                                   "does not exist.\n"));
    }

    // Get folder from path
    if (bfs::is_directory(path)) {
        snapshot_folder_ = path.string();
        snapshot_base_file_ = frame_source_address_;
    } else {
        snapshot_folder_ = path.parent_path().string();

        // Generate base file name
        snapshot_base_file_ = path.stem().string();
        if (snapshot_base_file_.empty() || snapshot_base_file_ == ".")
            snapshot_base_file_ = frame_source_address_;
    }
}

std::string Viewer::makeFileName() {

    // Generate current snapshot save path
    std::string fid;
    std::string timestamp = oat::createTimeStamp();

    int err = oat::createSavePath(fid,
                                 snapshot_folder_,
                                 snapshot_base_file_ + ".png",
                                 timestamp + "_" ,
                                 true);

    if (err) {
        std::cerr << oat::Error("Snapshop file creation exited "
                                "with error " + std::to_string(err) + "\n");
    }

    return fid;
}

} /* namespace oat */
