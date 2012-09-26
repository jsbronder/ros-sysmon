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

#include <cstdio>
#include <cerrno>

#include "cpuinfo.hpp"

namespace sysmon {

CpuInfo::CpuInfo()
{}

unsigned int CpuInfo::nproc() {
    update();
    return m_values.size();
}

void CpuInfo::ros_update(unsigned int proc, diagnostic_updater::DiagnosticStatusWrapper &dsw)
{
    if (proc > m_values.size()) {
        err("Request for unknown processor id %d\n", proc);
        return;
    }

    update();

    dsw.summary(diagnostic_msgs::DiagnosticStatus::OK, "OK");

    for (cpuinfoIter it = m_values[proc].begin(); it != m_values[proc].end(); ++it)
        dsw.add((*it).first, (*it).second);
}

int CpuInfo::update() {
    FILE * fp = std::fopen("/proc/cpuinfo", "r");
    if (!fp)
        return errno;

    char * line = NULL;
    size_t len = 0;
    int lineno = 0;
    int processor = 0;

    while (true) {
        ssize_t read = getline(&line, &len, fp);
        char * key = NULL;
        char * clear_whitespace = NULL;
        char * value = NULL;

        lineno += 1;

        if (read == -1)
            break;

        if (read <= 1)
            continue;

        key = strsep(&line, ":");

        if (!key) {
            err("Failed to find first key: line %d\n", lineno);
            continue;
        }

        if (!line || *line == '\0' || *line == '\n') {
            value = NULL;
        } else {
            clear_whitespace = line - 2;
            while (*clear_whitespace == ' ' || *clear_whitespace == '\t')
            {
                *clear_whitespace = '\0';
                clear_whitespace--;
            }

            value = line + 1;
            if (*value == '\n' || *value == '\0') {
                err("Failed to parse key for '%s': line %d\n", key, lineno);
                continue;
            }

            clear_whitespace = value;
            while (*clear_whitespace != '\n' && *clear_whitespace != '\0')
                clear_whitespace++;
            *clear_whitespace = '\0';

        }

        if (value && !strcmp(key, "processor"))
            processor = atoi(value);

        if (processor >= m_values.size())
            m_values.push_back(std::map<std::string, std::string>());

        cpuinfoIter it = m_values[processor].find(std::string(key));
        if (it != m_values[processor].end())
            m_values[processor].erase(key);

        m_values[processor].insert(
            std::pair<std::string, std::string>(
                std::string(key),
                std::string(value ? value : "")));

        free(key);
        line = NULL;
    }

    if (line)
        free(line);

    std::fclose(fp);

    return 0;
}

} // namespace sysmon
