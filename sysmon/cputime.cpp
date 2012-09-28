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

#include "cputime.hpp"

namespace sysmon {

CpuTime::CpuTime()
{}

unsigned int CpuTime::nproc() {
    update();
    return m_values.size();
}

void CpuTime::ros_update(int proc, diagnostic_updater::DiagnosticStatusWrapper &dsw)
{
    if (proc > m_values.size() && proc < -1) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Unknown processor id");
        return;
    }

    if (update()) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Update failed");
        return;
    }

    dsw.summary(diagnostic_msgs::DiagnosticStatus::OK, "OK");

    if (proc == -1) {
        for (cputimeIter it = m_totals.begin(); it != m_totals.end(); ++it)
            dsw.add((*it).first, (*it).second);
    } else {
        for (cputimeIter it = m_values[proc].begin(); it != m_values[proc].end(); ++it)
            dsw.add((*it).first, (*it).second);
    }
}

int CpuTime::update() {
    std::ifstream fp("/proc/stat");

    if (!fp.is_open()) {
        ROS_ERROR("%s:  Failed to open /proc/cputime", __func__);
        return EIO;
    }

    std::string line;
    unsigned int processor = -1;
    while (fp.good()) {
        getline(fp, line);

        if (!boost::algorithm::starts_with(line, "cpu"))
            break;

        std::vector<std::string> res;
        boost::algorithm::split(res, line, boost::is_any_of(" \t"), boost::token_compress_on);

        if (res.size() < 11)
            continue;

        std::string key = res.front();
        boost::trim(key);

        if (key.length() > 3) {
            std::string str_processor = key.substr(3, key.length());
            processor = boost::lexical_cast<unsigned int>(str_processor);
        }

        static const char * names[] = { NULL,   "user",     "nice", "system",
                                        "idle", "iowait",   "irq",  "softirq",
                                        "steal","guest",    "guest_nice" };

        cputime *dest = &m_totals;
        if (processor != -1) {
            if (processor >= m_values.size())
                m_values.push_back(std::map<std::string, std::string>());
            dest = &m_values[processor];
        }

        unsigned int total = 0;

        for (unsigned int i = 1; i < 11; ++i)
        {
            total += boost::lexical_cast<unsigned int>(res[i]);

            if (dest->find(key) != dest->end())
                (*dest)[names[i]] = res[i];
            else
                dest->insert(std::pair<std::string, std::string>(names[i], res[i]));
        }

        std::string str_total = boost::lexical_cast<std::string>(total);

        if (dest->find("total") != dest->end())
            (*dest)["total"] = str_total;
        else
            dest->insert(std::pair<std::string, std::string>("total", str_total));
    }

    fp.close();

    return 0;
}

} // namespace sysmon
