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

#pragma once

#include <string>
#include <diagnostic_updater/diagnostic_updater.h>

namespace sysmon {

class DiskUsage {
    /*
     * Monitors disk usage and publishes via ROS diagnostics
     */
    public:
        typedef std::map<std::string, std::string> diskusage;
        typedef std::map<std::string, std::string>::const_iterator diskusageIter;

        /*
         * Constructor
         */
        DiskUsage();

        /*
         * Return list of disks that are being monitored.
         */
        std::vector<std::string> disks();

        /*
         * Update the ROS diagnostics.
         *
         * @param disk  - disk to publish statistics for.
         *
         */
        void ros_update(const std::string &disk, diagnostic_updater::DiagnosticStatusWrapper &dsw);

    private:
        /*
         * Poll the current disk usage.
         *
         * @return  - 0 on success, appropriate errno otherwise.
         */
        int update();

        /*
         * List of filesystem types we ignore.
         */
        std::set<std::string> m_fs_blacklist;

        std::map<std::string, diskusage> m_values;
};

} // namespace sysmon

