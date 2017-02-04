/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *    * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <dlfcn.h>

#define LOG_TAG "QCOM PowerHAL"
#include <utils/Log.h>

//#define LOG_NDEBUG 0

#define CPUFREQ_ROOT_PATH "/sys/devices/system/cpu/cpu"
#define CPUFREQ_TAIL_PATH "/cpufreq/"

// what is the max frequency (for using max value)
#define MAX_FREQ_PATH "cpuinfo_max_freq"
// how much cpus ara available (for using max value) e.g. 0-3
#define CPUS_MAX_PATH "/sys/devices/system/cpu/present"
#define ONLINE_PATH "/online"

#define PROFILE_MAX_TAG "max"

void sysfs_write(const char *path, const char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

int sysfs_write_silent(const char *path, const char *s)
{
    int len;
    int rc = 0;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        return -1;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        rc = -1;
    }
    close(fd);
    return rc;
}

int sysfs_read(const char *path, char *s, int num_bytes)
{
    char buf[80];
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);

        return -1;
    }

    if ((count = read(fd, s, num_bytes - 1)) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading %s: %s\n", path, buf);

        ret = -1;
    } else {
        s[count] = '\0';
    }

    close(fd);

    return ret;
}

int sysfs_read_buf(const char* path, char *buf, int size) {
    if (sysfs_read(path, buf, size) == -1) {
        return -1;
    } else {
        // Strip newline at the end.
        int len = strlen(buf);

        len--;

        while (len >= 0 && (buf[len] == '\n' || buf[len] == '\r'))
            buf[len--] = '\0';
    }

    return 0;
}

int get_max_freq(int cpu, char *freq, int size) {
    char cpufreq_path[256];

    sprintf(cpufreq_path, "%s%d%s%s", CPUFREQ_ROOT_PATH, cpu, CPUFREQ_TAIL_PATH, MAX_FREQ_PATH);
    return sysfs_read_buf(cpufreq_path, freq, size);
}

int set_online_state(int cpu, const char* value) {
    char online_path[256];

    sprintf(online_path, "%s%d%s", CPUFREQ_ROOT_PATH, cpu, ONLINE_PATH);
    int rc = sysfs_write_silent(online_path, value);
    if (!rc) {
        ALOGI("write %s -> %s", online_path, value);
    }
    return rc;
}

int get_max_cpus(char *cpus, int size) {
    return sysfs_read_buf(CPUS_MAX_PATH, cpus, size);
}

int write_cpufreq_value(int cpu, const char* key, const char* value) {
    char cpufreq_path[256];

    sprintf(cpufreq_path, "%s%d%s%s", CPUFREQ_ROOT_PATH, cpu, CPUFREQ_TAIL_PATH, key);
    int rc = sysfs_write_silent(cpufreq_path, value);
    if (!rc) {
        ALOGI("write %s -> %s", cpufreq_path, value);
    }
    return rc;
}