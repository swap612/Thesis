/*
 * BSD LICENSE
 * 
 * Copyright(c) 2014-2015 Intel Corporation. All rights reserved.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.O
 * 
 *  version: CMT_CAT_Refcode.L.0.1.2-10
 */ 

/**
 * @brief Provides access to machine operations (CPUID, MSR read & write)
 */

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "machine.h"
#include "log.h"

static int *m_msr_fd = NULL;                            /**< MSR driver file descriptors table */
static unsigned m_maxcores = 0;                         /**< max number of cores (size of the
                                                           table above too) */

int
machine_init(const unsigned max_core_id)
{
        unsigned i;

        if (max_core_id==0)
                return MACHINE_RETVAL_PARAM;

        m_maxcores = max_core_id + 1;

        /**
         * Allocate table to hold MSR driver file descriptors
         * Each file descriptor is for a different core.
         * Core id is an index to the table.
         */
        m_msr_fd = (int *)malloc(m_maxcores * sizeof(m_msr_fd[0]));
        if (m_msr_fd==NULL) {
                m_maxcores = 0;
                return MACHINE_RETVAL_ERROR;
        }

        for (i=0;i<m_maxcores;i++)
                m_msr_fd[i] = -1;

        return MACHINE_RETVAL_OK;
}

int
machine_fini(void)
{
        unsigned i;

        ASSERT(m_msr_fd!=NULL);
        if (m_msr_fd==NULL)
                return MACHINE_RETVAL_ERROR;

        /**
         * Close open file descriptors and free up table memory.
         */
        for (i=0;i<m_maxcores;i++)
                if (m_msr_fd[i] != -1) {
                        close(m_msr_fd[i]);
                        m_msr_fd[i] = -1;
                }

        free(m_msr_fd);
        m_msr_fd = NULL;
        m_maxcores = 0;

        return MACHINE_RETVAL_OK;
}

int
cpuid(const unsigned lcore,
      const unsigned leaf,
      const unsigned subleaf,
      struct cpuid_out *out)
{
        char fname[32];
        off_t offset = ((off_t)leaf) + ((off_t) subleaf << 32);
        ssize_t read_ret = 0;
        int ret = MACHINE_RETVAL_OK;
        int fd = -1;

        ASSERT(out!=NULL);
        if (out==NULL)
                return MACHINE_RETVAL_PARAM;

        memset(fname,0,sizeof(fname));
        snprintf( fname, sizeof(fname)-1,
                  "/dev/cpu/%u/cpuid", lcore );
        fd = open(fname,O_RDONLY);
        if (fd<0) {
                LOG_ERROR("Error opening file '%s'!\n",fname);
                return MACHINE_RETVAL_ERROR;
        }

        read_ret = pread(fd, out, sizeof(*out), offset);
        if (read_ret!=sizeof(*out))
                ret = MACHINE_RETVAL_ERROR;

        close(fd);
        return ret;
}

/** 
 * @brief Returns MSR driver file descriptor for given core id
 *
 * File descriptor could be previously open and comes from
 * m_msr_fd table or is open (& cached) during the call.
 * 
 * @param lcore logical core id
 * 
 * @return MSR driver file descriptor corresponding \a lcore
 */
static int
msr_file_open(const unsigned lcore)
{
        int fd = -1;

        ASSERT(lcore<m_maxcores);
        ASSERT(m_msr_fd!=NULL);
        fd = m_msr_fd[lcore];

        if (fd<0) {
                char fname[32];
                memset(fname,0,sizeof(fname));
                snprintf( fname, sizeof(fname)-1,
                          "/dev/cpu/%u/msr", lcore );
                fd = open(fname,O_RDWR);
                if (fd<0) {
                        LOG_WARN("Error opening file '%s'!\n",fname);
                } else {
                        m_msr_fd[lcore] = fd;
                }
        }

        return fd;
}

int
msr_read(const unsigned lcore,
         const uint32_t reg,
         uint64_t *value)
{
        int ret = MACHINE_RETVAL_OK;
        int fd = -1;
        ssize_t read_ret = 0;

        ASSERT(value!=NULL);
        if (value==NULL)
                return MACHINE_RETVAL_PARAM;

        ASSERT(lcore<m_maxcores);
        if(lcore>=m_maxcores)
                return MACHINE_RETVAL_PARAM;

        ASSERT(m_msr_fd!=NULL);
        if(m_msr_fd==NULL)
                return MACHINE_RETVAL_ERROR;

        fd = msr_file_open(lcore);
        if (fd<0)
                return MACHINE_RETVAL_ERROR;

        read_ret = pread(fd, value, sizeof(value[0]), (off_t)reg);
        if (read_ret!=sizeof(value[0])) {
                LOG_ERROR("RDMSR failed for reg[0x%x] on lcore %u\n",
                          (unsigned) reg, lcore );
                ret = MACHINE_RETVAL_ERROR;
        }

        return ret;
}

int
msr_write(const unsigned lcore,
          const uint32_t reg,
          const uint64_t value)
{
        int ret = MACHINE_RETVAL_OK;
        int fd = -1;
        ssize_t write_ret = 0;

        ASSERT(lcore<m_maxcores);
        if(lcore>=m_maxcores)
                return MACHINE_RETVAL_PARAM;

        ASSERT(m_msr_fd!=NULL);
        if(m_msr_fd==NULL)
                return MACHINE_RETVAL_ERROR;

        fd = msr_file_open(lcore);
        if (fd<0)
                return MACHINE_RETVAL_ERROR;

        write_ret = pwrite(fd, &value, sizeof(value), (off_t)reg);
        if (write_ret!=sizeof(value)) {
                LOG_ERROR("WRMSR failed for reg[0x%x] <- value[0x%llx] on lcore %u\n",
                          (unsigned) reg, (long long unsigned) value, lcore ); 
                ret = MACHINE_RETVAL_ERROR;
        }

        return ret;
}

