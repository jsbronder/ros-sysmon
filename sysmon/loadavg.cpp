/*
 * Copyright (c) 2012, Justin Bronder
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the organization nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <cerrno>

#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include "loadavg.hpp"

namespace sysmon {

LoadAvg::LoadAvg()
{}

void LoadAvg::ros_update(diagnostic_updater::DiagnosticStatusWrapper &dsw)
{
    if (update() || m_load.size() < 3) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Update failed");
        return;
    }

    dsw.summary(diagnostic_msgs::DiagnosticStatus::OK, "OK");
    static const char * names[] = {"1 minute", "5 minute", "15 minute"};

    for (unsigned int i = 0; i < 3; ++i)
        dsw.add(names[i], m_load[i]);
}

int LoadAvg::update()
{
    std::ifstream fp("/proc/loadavg");

    if (!fp.is_open()) {
        ROS_ERROR("%s:  Failed to open /proc/loadavg", __func__);
        return EIO;
    }

    std::string line;
    while (fp.good()) {
        getline(fp, line);

        std::list<std::string> res;
        boost::algorithm::split(res, line, boost::is_any_of(" \t"));

        if (res.size() < 3)
            continue;

        while (res.size() > 3)
            res.pop_back();

        m_load.clear();
        for (std::list<std::string>::const_iterator it = res.begin(); it != res.end(); ++it)
            m_load.push_back(*it);
    }

    fp.close();

    return 0;
}

} // namespace sysmon
