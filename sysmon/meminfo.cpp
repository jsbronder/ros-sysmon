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
#include <list>
#include <boost/algorithm/string.hpp>

#include "meminfo.hpp"

namespace sysmon {

MemInfo::MemInfo()
{}

void MemInfo::ros_update(diagnostic_updater::DiagnosticStatusWrapper &dsw)
{
    if (update()) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Update failed");
        return;
    }

    dsw.summary(diagnostic_msgs::DiagnosticStatus::OK, "OK");
    for (meminfoIter it = m_values.begin(); it != m_values.end(); ++it)
        dsw.add((*it).first, (*it).second);
}

int MemInfo::update()
{
    std::ifstream fp("/proc/meminfo");

    if (!fp.is_open()) {
        ROS_ERROR("%s:  Failed to open /proc/meminfo", __func__);
        return EIO;
    }

    std::string line;
    while (fp.good()) {
        getline(fp, line);

        std::vector<std::string> res;
        boost::algorithm::split(res, line, boost::is_any_of(":"));

        if (res.size() < 2)
            continue;

        std::string value = res.back();
        boost::trim(value);

        res.pop_back();
        std::string key = res.back();
        boost::trim(key);

        if (m_values.find(key) != m_values.end())
            m_values[key] = value;
        else
            m_values.insert(std::pair<std::string, std::string>(key, value));
    }

    fp.close();

    return 0;
}

} // namespace sysmon
