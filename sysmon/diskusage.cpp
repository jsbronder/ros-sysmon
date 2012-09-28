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
#include <cstdio>
#include <mntent.h>
#include <sys/statvfs.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <boost/algorithm/string.hpp>

#include "diskusage.hpp"

namespace sysmon {

DiskUsage::DiskUsage()
{
    /* Kernel pseudo? filesystems */
    m_fs_blacklist.insert("sysfs");
    m_fs_blacklist.insert("rootfs");
    m_fs_blacklist.insert("bdev");
    m_fs_blacklist.insert("proc");
    m_fs_blacklist.insert("cgroup");
    m_fs_blacklist.insert("cpuset");
    m_fs_blacklist.insert("tmpfs");
    m_fs_blacklist.insert("binfmt_misc");
    m_fs_blacklist.insert("debugfs");
    m_fs_blacklist.insert("securityfs");
    m_fs_blacklist.insert("sockfs");
    m_fs_blacklist.insert("usbfs");
    m_fs_blacklist.insert("pipefs");
    m_fs_blacklist.insert("anon_inodefs");
    m_fs_blacklist.insert("futexfs");
    m_fs_blacklist.insert("inotifyfs");
    m_fs_blacklist.insert("devpts");
    m_fs_blacklist.insert("ramfs");
    m_fs_blacklist.insert("hugetlbfs");
    m_fs_blacklist.insert("mqueue");
    m_fs_blacklist.insert("fuse");
    m_fs_blacklist.insert("fusectl");
    m_fs_blacklist.insert("selinuxfs");
    m_fs_blacklist.insert("encryptfs");
    m_fs_blacklist.insert("rpc_pipefs");

    /* Network filesystems */
    m_fs_blacklist.insert("nfs");
    m_fs_blacklist.insert("nfs4");
    m_fs_blacklist.insert("autofs");
    m_fs_blacklist.insert("nfsd");
}

std::vector<std::string> DiskUsage::disks()
{
    update();

    std::vector<std::string> ret;
    for (std::map<std::string, diskusage>::const_iterator it = m_values.begin();
            it != m_values.end();
            ++it) {
        ret.push_back((*it).first);
    }
    return ret;
}

void DiskUsage::ros_update(const std::string &disk, diagnostic_updater::DiagnosticStatusWrapper &dsw)
{
    if (update()) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Update failed");
        return;
    }

    if (m_values.find(disk) == m_values.end()) {
        dsw.summary(diagnostic_msgs::DiagnosticStatus::ERROR, "Invalid disk name");
        return;
    }

    dsw.summary(diagnostic_msgs::DiagnosticStatus::OK, "OK");

    for (diskusageIter it = m_values[disk].begin(); it != m_values[disk].end(); ++it)
        dsw.add((*it).first, (*it).second);
}

int DiskUsage::update()
{
    FILE * mtab = NULL;
    struct mntent * mnt;
    char strings[4096];

    mtab = setmntent("/etc/mtab", "r");
    if (!mtab) {
        int r = errno;
        ROS_ERROR("%s:  Failed to open /etc/mtab, errno %d", __func__, r);
        return r;
    }

    while ((mnt = getmntent(mtab))) {
        struct statvfs fs;
        int r;
        fsblkcnt_t size;
        fsblkcnt_t avail;
        float usage;

        if (!mnt->mnt_dir)
            continue;

        if (std::find(m_fs_blacklist.begin(), m_fs_blacklist.end(), mnt->mnt_type) != m_fs_blacklist.end())
            continue;

        r = statvfs(mnt->mnt_dir, &fs);
        if (r) {
            ROS_ERROR("%s:  statfs failed on %s, errno %d", __func__, mnt->mnt_dir, r);
            continue;
        }

        size = (fs.f_blocks / 1024) * fs.f_bsize;
        avail = (fs.f_bavail / 1024) * fs.f_bsize;
        usage = (float)(fs.f_blocks - fs.f_bavail) / fs.f_blocks;

        diskusage i;
        i["size"] = boost::lexical_cast<std::string>(size);
        i["avail"] = boost::lexical_cast<std::string>(avail);
        i["usage"] = boost::lexical_cast<std::string>(100 * usage);

        if (m_values.find(mnt->mnt_dir) != m_values.end())
            m_values[mnt->mnt_dir] = i;
        else
            m_values.insert(std::pair<std::string, diskusage>(mnt->mnt_dir, i));

    }
    endmntent(mtab);
    return 0;
}

} // namespace sysmon
