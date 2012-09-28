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

#include <unistd.h>

#include "cpuinfo.hpp"
#include "cputime.hpp"
#include "diskusage.hpp"
#include "loadavg.hpp"
#include "meminfo.hpp"

int main(int argc, char **argv)
{
    ros::init(argc, argv, "sysmon");
    ros::NodeHandle nh;
    diagnostic_updater::Updater updater;

    char hostname[HOST_NAME_MAX];
    int r = gethostname(hostname, HOST_NAME_MAX-1);
    if (r)
        updater.setHardwareID("unknown");
    else
        updater.setHardwareID(hostname);

    sysmon::CpuInfo cpuinfo;
    unsigned int nproc = cpuinfo.nproc();

    for (unsigned int i = 0; i < nproc; ++i) {
        char buf[16];
        snprintf(buf, 15, "Processor %d", i);
        updater.add(buf, boost::bind(&sysmon::CpuInfo::ros_update, cpuinfo, i, _1));
    }

    sysmon::LoadAvg loadavg;
    updater.add("Load Average", &loadavg, &sysmon::LoadAvg::ros_update);

    sysmon::MemInfo meminfo;
    updater.add("Memory", &meminfo, &sysmon::MemInfo::ros_update);

    sysmon::CpuTime cputime;
    updater.add("CPU time - Total", boost::bind(&sysmon::CpuTime::ros_update, cputime, -1, _1));

    nproc = cputime.nproc();
    for (unsigned int i = 0; i < nproc; ++i) {
        char buf[32];
        snprintf(buf, 31, "Cpu Time - Processor %d", i);
        updater.add(buf, boost::bind(&sysmon::CpuTime::ros_update, cputime, i, _1));
    }

    sysmon::DiskUsage diskusage;
    std::vector<std::string> disks = diskusage.disks();
    for (std::vector<std::string>::const_iterator it = disks.begin(); it != disks.end(); ++it) {
        char buf[64];
        snprintf(buf, 63, "Disk Usage - %s", (*it).c_str());
        updater.add(buf, boost::bind(&sysmon::DiskUsage::ros_update, diskusage, *it, _1));
    }

    while (nh.ok()) {
        ros::Duration(1).sleep();
        updater.update();
    }

    return 0;
}

