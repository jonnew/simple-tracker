//******************************************************************************
//* File:   FrameMasker.h
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
//******************************************************************************

#ifndef OAT_FRAMEMASKER_H
#define	OAT_FRAMEMASKER_H

#include "FrameFilter.h"

namespace oat {

class FrameMasker : public FrameFilter {

public:
    /**
     * @brief A frame masker to isolate one or more regions of interest in a
     * frame stream using a mask frame. Pixels of the input frames that
     * correspond to non-zero pixels in the mask frame will be unchanged. All
     * other pixels will be set to 0.
     *
     * @param frame_source_address raw frame source address
     * @param frame_sink_address filtered frame sink address
     */
    FrameMasker(const std::string &frame_souce_address,
                const std::string &frame_sink_address);

private:
    // Configurable Interface
    po::options_description options() const override;
    void applyConfiguration(const po::variables_map &vm,
                            const config::OptionTable &config_table) override;

    void filter(cv::Mat& frame) override;

    // Mask frames with an arbitrary ROI
    bool mask_set_ = false;
    cv::Mat roi_mask_;
};

}      /* namespace oat */
#endif /* OAT_FRAMEMASKER_H */
