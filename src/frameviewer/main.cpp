//******************************************************************************
//* File:   oat view main.cpp
//* Author: Jon Newman <jpnewman snail mit dot edu>
//
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

#include <csignal>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <opencv2/core.hpp>

#include "../../lib/utility/IOFormat.h"

#include "Viewer.h"

namespace po = boost::program_options;
namespace bfs = boost::filesystem;

volatile sig_atomic_t quit = 0;
volatile sig_atomic_t source_eof = 0;

// Signal handler to ensure shared resources are cleaned on exit due to ctrl-c
void sigHandler(int) {
    quit = 1;
}

void run(std::shared_ptr<oat::Viewer>& viewer) {

    try {

        viewer->connectToNode();

        while (!quit && !source_eof) {
            source_eof = viewer->showImage();
        }

    } catch (const boost::interprocess::interprocess_exception &ex) {

        // Error code 1 indicates a SIGNINT during a call to wait(), which
        // is normal behavior
        if (ex.get_error_code() != 1)
            throw;
    }
}

void printUsage(po::options_description options) {
    std::cout << "Usage: view [INFO]\n"
              << "   or: view SOURCE [CONFIGURATION]\n"
              << "Display frame SOURCE on a monitor.\n\n"
              << "SOURCE:\n"
              << "  User-supplied name of the memory segment to receive frames "
              << "from (e.g. raw).\n\n"
              << options << "\n";
}

int main(int argc, char *argv[]) {

    std::signal(SIGINT, sigHandler);

    std::string source;
    std::string snapshot_path;
    po::options_description visible_options("OPTIONS");

    try {

        po::options_description options("INFO");
        options.add_options()
                ("help", "Produce help message.")
                ("version,v", "Print version information.")
                ;

        po::options_description config("CONFIGURATION");
        config.add_options()
                ("snapshot-path,f", po::value<std::string>(&snapshot_path),
                "The path to which in which snapshots will be saved. "
                "If a folder is designated, the base file name will be SOURCE. "
                "The timestamp of the snapshot will be prepended to the file name. "
                "Defaults to the current directory.")
                ;

        po::options_description hidden("HIDDEN OPTIONS");
        hidden.add_options()
                ("source", po::value<std::string>(&source),
                "The name of the frame SOURCE that supplies frames to view.\n")
                ;

        po::positional_options_description positional_options;
        positional_options.add("source", -1);

        po::options_description all_options("ALL OPTIONS");
        all_options.add(options).add(config).add(hidden);

        visible_options.add(options).add(config);

        po::variables_map variable_map;
        po::store(po::command_line_parser(argc, argv)
                .options(all_options)
                .positional(positional_options)
                .run(),
                variable_map);
        po::notify(variable_map);

        // Use the parsed options
        if (variable_map.count("help")) {
            printUsage(visible_options);
            return 0;
        }

        if (variable_map.count("version")) {
            std::cout << "Oat Frame Viewer version "
                      << Oat_VERSION_MAJOR
                      << "."
                      << Oat_VERSION_MINOR
                      << "\n";
            std::cout << "Written by Jonathan P. Newman in the MWL@MIT.\n";
            std::cout << "Licensed under the GPL3.0.\n";
            return 0;
        }

        if (!variable_map.count("source")) {
            printUsage(visible_options);
            std::cerr << oat::Error("A SOURCE must be specified. Exiting.\n");
            return -1;
        }

        if (!variable_map.count("snapshot-path")) {
            snapshot_path = bfs::current_path().string();
        }

    } catch (std::exception& e) {
        std::cerr << oat::Error(e.what()) << "\n";
        return -1;
    } catch (...) {
        std::cerr << oat::Error("Exception of unknown type.\n");
        return -1;
    }

    // Create component
    std::shared_ptr<oat::Viewer> viewer =
            std::make_shared<oat::Viewer>(source);

    try {

        // Create a path to save snapshots
        viewer->storeSnapshotPath(snapshot_path);

        // Tell user
        std::cout << oat::whoMessage(viewer->name(),
                  "Listening to source " + oat::sourceText(source) + ".\n")
                  << oat::whoMessage(viewer->name(),
                  "Press 's' on the viewer window to take a snapshot.\n")
                  << oat::whoMessage(viewer->name(),
                  "Press CTRL+C to exit.\n");

        // Infinite loop until ctrl-c or end of stream signal
        run(viewer);

        // Tell user
        std::cout << oat::whoMessage(viewer->name(), "Exiting.\n");

        // Exit
        return 0;

    } catch (const std::runtime_error &ex) {
        std::cerr << oat::whoError(viewer->name(), ex.what()) << "\n";
    } catch (const cv::Exception &ex) {
        std::cerr << oat::whoError(viewer->name(), ex.msg) << "\n";
    } catch (const boost::interprocess::interprocess_exception &ex) {
        std::cerr << oat::whoError(viewer->name(), ex.what()) << "\n";
    } catch (...) {
        std::cerr << oat::whoError(viewer->name(), "Unknown exception.\n");
    }

    // Exit failure
    return -1;
}
