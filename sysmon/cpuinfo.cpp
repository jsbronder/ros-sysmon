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

#include "cpuinfo.hpp"

namespace sysmon {

CpuInfo::CpuInfo()
{}

unsigned int CpuInfo::nproc()
{
    update();
    return m_values.size();
}

void CpuInfo::ros_update(unsigned int proc, diagnostic_updater::DiagnosticStatusWrapper &dsw)
{
    if (proc > m_values.size()) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Unknown processor id");
        return;
    }

    if (update()) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Update failed");
        return;
    }

    dsw.summary(diagnostic_msgs::DiagnosticStatus::OK, "OK");
    for (cpuinfoIter it = m_values[proc].begin(); it != m_values[proc].end(); ++it)
        dsw.add((*it).first, (*it).second);
}

int CpuInfo::update()
{
    std::ifstream fp("/proc/cpuinfo");

    if (!fp.is_open()) {
        ROS_ERROR("%s:  Failed to open /proc/cpuinfo", __func__);
        return EIO;
    }

    std::string line;
    unsigned int processor = 0;
    while (fp.good()) {
        getline(fp, line);

        std::list<std::string> res;
        boost::algorithm::split(res, line, boost::is_any_of(":"));

        if (res.size() < 2)
            continue;

        std::string key = res.front();
        boost::trim(key);

        res.pop_front();
        std::string value = boost::algorithm::join(res, " ");
        boost::trim(value);

        if (key == "processor")
            processor = boost::lexical_cast<unsigned int>(value);

        if (processor >= m_values.size())
            m_values.push_back(std::map<std::string, std::string>());

        if (m_values[processor].find(key) != m_values[processor].end())
            m_values[processor][key] = value;
        else
            m_values[processor].insert(std::pair<std::string, std::string>(key, value));
    }

    fp.close();

    return 0;
}

} // namespace sysmon
