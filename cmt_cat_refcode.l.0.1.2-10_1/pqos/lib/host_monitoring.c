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
 * @brief Implementation of PQoS monitoring API.
 *
 * CPUID and MSR operations are done on 'local' system.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "pqos.h"

#include "host_cap.h"
#include "host_monitoring.h"

#include "machine.h"
#include "types.h"
#include "log.h"

/**
 * ---------------------------------------
 * Local macros
 * ---------------------------------------
 */

/**
 * Allocation & Monitoring association MSR register
 *
 * [63..<QE COS>..32][31..<RESERVED>..10][9..<RMID>..0]
 */
#define PQOS_MSR_ASSOC             0xC8F
#define PQOS_MSR_ASSOC_QECOS_SHIFT 32
#define PQOS_MSR_ASSOC_QECOS_MASK  0xffffffff00000000ULL
#define PQOS_MSR_ASSOC_RMID_MASK   ((1ULL<<10)-1ULL)

/**
 * Monitoring data read MSR register
 */
#define PQOS_MSR_MON_QMC             0xC8E
#define PQOS_MSR_MON_QMC_DATA_MASK   ((1ULL<<62)-1ULL)
#define PQOS_MSR_MON_QMC_ERROR       (1ULL<<63)
#define PQOS_MSR_MON_QMC_UNAVAILABLE (1ULL<<62)

/**
 * Monitoring event selection MSR register
 * [63..<RESERVED>..42][41..<RMID>..32][31..<RESERVED>..8][7..<EVENTID>..0]
 */
#define PQOS_MSR_MON_EVTSEL            0xC8D
#define PQOS_MSR_MON_EVTSEL_RMID_SHIFT 32
#define PQOS_MSR_MON_EVTSEL_RMID_MASK  ((1ULL<<10)-1ULL)
#define PQOS_MSR_MON_EVTSEL_EVTID_MASK ((1ULL<<8)-1ULL)

/**
 * Allocation class of service (COS) MSR registers
 */
#define PQOS_MSR_L3CA_MASK_START 0xC90
#define PQOS_MSR_L3CA_MASK_END   0xD8F
#define PQOS_MSR_L3CA_MASK_NUMOF (PQOS_MSR_L3CA_MASK_END-PQOS_MSR_L3CA_MASK_START+1)

/**
 * Special RMID - after reset all cores are associated with it.
 *
 * The assumption is that if core is not assigned to it
 * then it is subject of monitoring activity by a different process.
 */
#define RMID0 (0)

/**
 * ---------------------------------------
 * Local data types
 * ---------------------------------------
 */

/**
 * List of RMID states
 */
enum rmid_state {
        RMID_STATE_FREE = 0,                            /**< RMID is currently unused and can
                                                           be used by the library */
        RMID_STATE_ALLOCATED,                           /**< RMID was free at start but now it
                                                           is used for monitoring */
        RMID_STATE_UNAVAILABLE                          /**< RMID was associated to some core
                                                           at start-up. It may be used by
                                                           another process for monitoring */
};

/**
 * Per logical core entry to track monitoring processes
 */
struct mon_entry {
        pqos_rmid_t rmid;                               /**< current RMID association */
        int unavailable;                                /**< if true then core is subject of
                                                           monitoring by another process */
        struct pqos_mon_data *grp;                      /**< monitoring group the core belongs to */
};

/**
 * ---------------------------------------
 * Local data structures
 * ---------------------------------------
 */
static const struct pqos_cap *m_cap = NULL;             /**< capabilites structure passed from host_cap */
static const struct pqos_cpuinfo *m_cpu = NULL;         /**< cpu topology passed from host_cap */

static enum rmid_state **m_rmid_cluster_map = NULL;     /**< 32 is max supported monitoring clusters */
static unsigned m_num_clusters = 0;                     /**< number of clusters in the topology */
static unsigned m_rmid_max = 0;                         /**< max RMID */
static unsigned m_dim_cores = 0;                        /**< max coreid in the topology */

static struct mon_entry *m_core_map = NULL;             /**< map of core states */

/**
 * ---------------------------------------
 * Local Functions
 * ---------------------------------------
 */

static unsigned
cpu_get_num_clusters(const struct pqos_cpuinfo *cpu);

static unsigned
cpu_get_num_cores(const struct pqos_cpuinfo *cpu);

static int
mon_assoc_set_nocheck(const unsigned lcore,
                      const pqos_rmid_t rmid );

static int
mon_assoc_set(const unsigned lcore,
              const unsigned cluster,
              const pqos_rmid_t rmid );

static int
mon_assoc_get(const unsigned lcore,
              pqos_rmid_t *rmid );

static int
mon_read( const unsigned lcore,
          const pqos_rmid_t rmid,
          const enum pqos_mon_event event,
          uint64_t *value );

static int
rmid_alloc( const unsigned cluster,
            const enum pqos_mon_event event,
            pqos_rmid_t *rmid );

static int
rmid_free( const unsigned cluster,
           const pqos_rmid_t rmid );

/**
 * =======================================
 * =======================================
 *
 * initialize and shutdown
 *
 * =======================================
 * =======================================
 */


int
pqos_mon_init(const struct pqos_cpuinfo *cpu,
              const struct pqos_cap *cap,
              const struct pqos_config *cfg)
{
        const struct pqos_capability *item = NULL;
        unsigned i=0, fails=0;
        int ret = PQOS_RETVAL_OK;

        m_cpu = cpu;
        m_cap = cap;

        /**
         * If monitoring capability has been discovered
         * then get max RMID supported by a CPU socket
         * and allocate memory for RMID table
         */
        ret = pqos_cap_get_type(cap,PQOS_CAP_TYPE_MON,&item);
        if (ret!=PQOS_RETVAL_OK)
                return ret;

        m_num_clusters = cpu_get_num_clusters(m_cpu);
        ASSERT(m_num_clusters>=1);

        ASSERT(item!=NULL);
        m_rmid_max = item->u.mon->max_rmid;
        if (m_rmid_max==0) {
                pqos_mon_fini();
                return PQOS_RETVAL_PARAM;
        }

        LOG_INFO("Max RMID per monitoring cluster is %u\n",m_rmid_max);

        ASSERT(m_cpu!=NULL);

        m_dim_cores = cpu_get_num_cores(m_cpu);
        m_core_map = (struct mon_entry*) malloc(m_dim_cores*sizeof(m_core_map[0]));
        ASSERT(m_core_map!=NULL);
        if (m_core_map==NULL) {
                pqos_mon_fini();
                return PQOS_RETVAL_ERROR;
        }
        memset(m_core_map, 0, m_dim_cores*sizeof(m_core_map[0]));

        m_rmid_cluster_map = (enum rmid_state**) malloc(m_num_clusters*sizeof(m_rmid_cluster_map[0]));
        ASSERT(m_rmid_cluster_map!=NULL);
        if (m_rmid_cluster_map==NULL) {
                pqos_mon_fini();
                return PQOS_RETVAL_ERROR;
        }

        for (i=0;i<m_num_clusters;i++) {
                const size_t size = m_rmid_max * sizeof(enum rmid_state);
                enum rmid_state *st = (enum rmid_state*) malloc(size);
                if (st==NULL) {
                        pqos_mon_fini();
                        return PQOS_RETVAL_RESOURCE;
                }
                m_rmid_cluster_map[i] = st;
                memset(st, 0, size);
                st[RMID0] = RMID_STATE_UNAVAILABLE; /** RMID0 has a special meaning */
        }

        LOG_INFO("RMID internal tables allocated\n");

        /**
         * Read current core<=>RMID associations
         */
        for (i=0;i<m_cpu->num_cores;i++) {
                pqos_rmid_t rmid = 0;
                int ret = PQOS_RETVAL_OK;
                unsigned coreid = m_cpu->cores[i].lcore;
                unsigned clusterid = m_cpu->cores[i].cluster;

                ret = mon_assoc_get(coreid, &rmid);
                if (ret != PQOS_RETVAL_OK) {
                        LOG_ERROR("Failed to read RMID association of lcore %u!\n",
                                  coreid);
                        fails++;
                } else {
                        ASSERT(rmid<m_rmid_max);

                        m_core_map[coreid].rmid = rmid;
                        m_core_map[coreid].unavailable = 0;
                        m_core_map[coreid].grp = NULL;

                        if (rmid==RMID0)
                                continue;

                        /**
                         * At this stage we know core is assigned to non-zero RMID
                         * This means it may be used by another instance of the program
                         * for monitoring.
                         * The other option is that previosly ran program dies and
                         * it didn't revert RMID association.
                         */

                        if (!cfg->free_in_use_rmid) {
                                enum rmid_state *pstate = NULL;
                                LOG_INFO("Detected RMID%u is associated with core %u. "
                                         "Marking RMID & core unavailable.\n",
                                         rmid, coreid );

                                ASSERT(cluster<idm_num_clusters);
                                pstate = m_rmid_cluster_map[clusterid];
                                pstate[rmid] = RMID_STATE_UNAVAILABLE;

                                m_core_map[coreid].unavailable = 1;
                        } else {
                                LOG_INFO("Detected RMID%u is associated with core %u. "
                                         "Freeing the RMID and associateing core with RMID0.\n",
                                         rmid, coreid );
                                ret = mon_assoc_set_nocheck(coreid, RMID0);
                                if (ret != PQOS_RETVAL_OK) {
                                        LOG_ERROR("Failed to associate core %u with RMID0!\n",
                                                  coreid);
                                        fails++;
                                }
                        }
                }
        }

        ret = (fails==0) ? PQOS_RETVAL_OK : PQOS_RETVAL_ERROR;

        if (ret!=PQOS_RETVAL_OK)
                pqos_mon_fini();

        return ret;
}

int
pqos_mon_fini(void)
{
        int retval = PQOS_RETVAL_OK;

        if (m_core_map!=NULL && m_cpu!=NULL) {
                /**
                 * Assign monitored cores back to RMID0
                 */
                unsigned i;
                for (i=0;i<m_cpu->num_cores;i++) {
                        if (m_core_map[i].rmid != RMID0 &&
                            m_core_map[i].unavailable==0) {
                                int ret = PQOS_RETVAL_OK;
                                ret = mon_assoc_set_nocheck(m_cpu->cores[i].lcore, RMID0);
                                if (ret != PQOS_RETVAL_OK) {
                                        LOG_ERROR("Failed to associate core %u with  RMID0!\n",
                                                  m_cpu->cores[i].lcore);
                                }
                        }
                }
        }

        /**
         * Free up allocated cluster structures for tracking
         * RMID allocations.
         */
        if (m_rmid_cluster_map!=NULL) {
                unsigned i;
                for (i=0;i<m_num_clusters;i++) {
                        if (m_rmid_cluster_map[i]!=NULL) {
                                free(m_rmid_cluster_map[i]);
                                m_rmid_cluster_map[i] = NULL;
                        }
                }
                free(m_rmid_cluster_map);
                m_rmid_cluster_map = NULL;
        }
        m_rmid_max = 0;
        m_num_clusters = 0;

        /**
         * Free up allocated core map used to track
         * core <=> RMID assignment
         */
        if (m_core_map!=NULL) {
                free(m_core_map);
                m_core_map = NULL;
        }
        m_dim_cores = 0;

        m_cpu = NULL;
        m_cap = NULL;

        return retval;
}

/**
 * =======================================
 * =======================================
 *
 * RMID allocation
 *
 * =======================================
 * =======================================
 */

/** 
 * @brief Validates cluster id paramter for RMID allocation operation
 * 
 * @param cluster cluster id on which rmid is to allocated from
 * @param p_table place to store pointer to cluster map of RMIDs
 * 
 * @return Operation status
 * @retval PQOS_RETVAL_OK success
 */
static int
mon_rmid_alloc_param_check(const unsigned cluster,
                           enum rmid_state **p_table)
{
        int ret;

        if (cluster>=m_num_clusters)
                return PQOS_RETVAL_PARAM;

        ret = _pqos_check_init(1);
        if (ret!=PQOS_RETVAL_OK)
                return ret;

        if (m_rmid_cluster_map[cluster]==NULL) {
                LOG_WARN("Monitoring capability not detected for cluster id %u\n",
                          cluster);
                return PQOS_RETVAL_PARAM;
        }

        (*p_table) = m_rmid_cluster_map[cluster];

        return PQOS_RETVAL_OK;
}

/** 
 * @brief Allocates RMID for given \a event
 * 
 * @param [in] cluster CPU cluster id
 * @param [in] event Monitoring event type
 * @param [out] rmid resource monitoring id
 * 
 * @return Operations status
 */
static int
rmid_alloc( const unsigned cluster,
            const enum pqos_mon_event event,
            pqos_rmid_t *rmid )
{
        enum rmid_state *rmid_table = NULL;
        const struct pqos_capability *item = NULL;
        const struct pqos_cap_mon *mon = NULL;
        int ret = PQOS_RETVAL_OK;
        unsigned max_rmid = 0;
        unsigned i;
        int j;

        if (rmid==NULL)
                return PQOS_RETVAL_PARAM;

        ret = mon_rmid_alloc_param_check(cluster, &rmid_table);
        if (ret!=PQOS_RETVAL_OK) {
                return ret;
        }
        ASSERT(rmid_table!=NULL);

        /**
         * This is not so straight forward as it appears to be.
         * We first have to figure out max RMID
         * for given event type. In order to do so we need:
         * - go through capabilities structure
         * - find monitoring capability
         * - look for the \a event in the event list
         * - find max RMID matching the \a event
         */
        ASSERT(m_cap!=NULL);
        ret = pqos_cap_get_type(m_cap, PQOS_CAP_TYPE_MON, &item);
        if (ret!=PQOS_RETVAL_OK) {
                return ret;
        }

        ASSERT(item!=NULL);
        mon = item->u.mon;

        for (i=0;i<mon->num_events;i++) {
                if (event!=mon->events[i].type)
                        continue;
                max_rmid = mon->events[i].max_rmid;
                break;
        }

        if (i>=mon->num_events || max_rmid==0) {
                return PQOS_RETVAL_ERROR;               /**< no such event found */
        }

        ASSERT(m_rmid_max>=max_rmid);

        /**
         * Check for free RMID in the table
         * Do it backwards (from max to 0) in order to preserve low RMID values
         * for overlapping RMID ranges for future events.
         */
        ret = PQOS_RETVAL_ERROR;
        for (j=(int)max_rmid-1;j>=0;j--) {
                if (rmid_table[j]!=RMID_STATE_FREE)
                        continue;
                rmid_table[j] = RMID_STATE_ALLOCATED;
                *rmid = (pqos_rmid_t) j;
                ret = PQOS_RETVAL_OK;
                break;
        }

        return ret;
}

/** 
 * @brief Frees previously allocated \a rmid
 * 
 * @param [in] cluster CPU cluster id
 * @param [in] rmid resource monitoring id
 *
 * @return Operations status
 */
static int
rmid_free( const unsigned cluster,
           const pqos_rmid_t rmid )
{
        enum rmid_state *rmid_table = NULL;
        int ret = PQOS_RETVAL_OK;

        ret = mon_rmid_alloc_param_check(cluster, &rmid_table);
        if (ret!=PQOS_RETVAL_OK) {
                return ret;
        }
        ASSERT(rmid_table!=NULL);

        if (rmid>=m_rmid_max) {
                return PQOS_RETVAL_PARAM;
        }

        if (rmid_table[rmid]!=RMID_STATE_ALLOCATED) {
                return PQOS_RETVAL_ERROR;
        }

        rmid_table[rmid] = RMID_STATE_FREE;

        return ret;
}

/**
 * =======================================
 * =======================================
 *
 * Monitoring
 *
 * =======================================
 * =======================================
 */

/** 
 * @brief Checks logical core parameter for core association get operation
 * 
 * @param lcore logical core id
 * @param p_cluster place to store pointer to cluster map of RMIDs
 * 
 * @return Operation status
 * @retval PQOS_RETVAL_OK success
 */
static int
mon_assoc_param_check(const unsigned lcore,
                      unsigned *p_cluster)
{
        int ret = PQOS_RETVAL_OK;

        ret = _pqos_check_init(1);
        if (ret!=PQOS_RETVAL_OK)
                return ret;

        ASSERT(m_cpu!=NULL);
        ret = pqos_cpu_check_core(m_cpu, lcore);
        if (ret!=PQOS_RETVAL_OK) {
                return PQOS_RETVAL_PARAM;
        }

        ASSERT(p_cluster!=NULL);
        ret = pqos_cpu_get_clusterid(m_cpu,lcore,p_cluster);
        if (ret!=PQOS_RETVAL_OK) {
                return PQOS_RETVAL_PARAM;
        }

        if ((*p_cluster)>=m_num_clusters) {
                return PQOS_RETVAL_PARAM;
        }

        if (m_rmid_cluster_map[(*p_cluster)]==NULL) {
                LOG_WARN("Monitoring capability not detected\n");
                return PQOS_RETVAL_PARAM;
        }

        return PQOS_RETVAL_OK;
}

/** 
 * @brief Associates core with RMID at register level
 * 
 * This function doesn't acquire API lock
 * and can be used internally when lock is already taken.
 *
 * @param lcore logical core id
 * @param rmid resource monitoring ID
 * 
 * @return Operation status
 * @retval PQOS_RETVAL_OK on success
 */
static int
mon_assoc_set_nocheck(const unsigned lcore,
                      const pqos_rmid_t rmid )
{
        int ret = 0;
        uint32_t reg = 0;
        uint64_t val = 0;

        reg = PQOS_MSR_ASSOC;
        ret = msr_read(lcore,reg,&val);
        if (ret!=MACHINE_RETVAL_OK) {
                return PQOS_RETVAL_ERROR;
        }

        val &= PQOS_MSR_ASSOC_QECOS_MASK;
        val |= (uint64_t)(rmid & PQOS_MSR_ASSOC_RMID_MASK);

        ret = msr_write(lcore,reg,val);
        if (ret!=MACHINE_RETVAL_OK) {
                return PQOS_RETVAL_ERROR;
        }

        return PQOS_RETVAL_OK;
}

/** 
 * @brief Associates core with RMID
 * 
 * This function doesn't acquire API lock
 * and can be used internally when lock is already taken.
 *
 * @param lcore logical core id
 * @param cluster cluster that core belongs to
 * @param rmid resource monitoring ID
 * 
 * @return Operation status
 * @retval PQOS_RETVAL_OK on success
 */
static int
mon_assoc_set(const unsigned lcore,
              const unsigned cluster,
              const pqos_rmid_t rmid )
{
        enum rmid_state *rmid_table = NULL;
        int ret = 0;

        if (cluster>=m_num_clusters)
                return PQOS_RETVAL_PARAM;

        rmid_table = m_rmid_cluster_map[cluster];
        if (rmid_table[rmid]!=RMID_STATE_ALLOCATED)
                return PQOS_RETVAL_PARAM;

        ret = mon_assoc_set_nocheck(lcore, rmid);
        if (ret!=PQOS_RETVAL_OK) {
                return ret;
        }

        m_core_map[lcore].rmid = rmid;
        
        return PQOS_RETVAL_OK;
}

/** 
 * @brief Reads \a lcore to RMID association
 * 
 * @param lcore logical core id
 * @param rmid place to store RMID \a lcore is assigned to
 * 
 * @return Operation status
 * @retval PQOS_RETVAL_OK success
 */
static int
mon_assoc_get( const unsigned lcore,
               pqos_rmid_t *rmid )
{
        int ret = 0;
        uint32_t reg = PQOS_MSR_ASSOC;
        uint64_t val = 0;

        ASSERT(rmid!=NULL);

        ret = msr_read(lcore,reg,&val);
        if (ret!=MACHINE_RETVAL_OK) {
                return PQOS_RETVAL_ERROR;
        }

        val &= PQOS_MSR_ASSOC_RMID_MASK;
        *rmid = (pqos_rmid_t) val;

        return PQOS_RETVAL_OK;
}

int
pqos_mon_assoc_get( const unsigned lcore,
                    pqos_rmid_t *rmid )
{
        int ret = PQOS_RETVAL_OK;
        unsigned cluster = 0;

        _pqos_api_lock();

        ret = mon_assoc_param_check(lcore, &cluster);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return ret;
        }

        if (rmid==NULL) {
                _pqos_api_unlock();
                return PQOS_RETVAL_PARAM;
        }

        ret = mon_assoc_get(lcore,rmid);

        _pqos_api_unlock();
        return ret;
}

/** 
 * @brief Reads monitoring event data from given core
 * 
 * This function doesn't acquire API lock.
 *
 * @param lcore logical core id
 * @param rmid RMID to be read
 * @param event monitoring event
 * @param value place to store read value
 * 
 * @return Operation status
 * @retval PQOS_RETVAL_OK on success
 */
static int
mon_read( const unsigned lcore,
          const pqos_rmid_t rmid,
          const enum pqos_mon_event event,
          uint64_t *value )
{
        int retries = 3, retval = PQOS_RETVAL_OK;
        uint32_t reg = 0;
        uint64_t val = 0;

        /**
         * Set event selection register (RMID + event id)
         */
        reg = PQOS_MSR_MON_EVTSEL;
        val = ((uint64_t)rmid) & PQOS_MSR_MON_EVTSEL_RMID_MASK;
        val <<= PQOS_MSR_MON_EVTSEL_RMID_SHIFT;
        val |= ((uint64_t)event) & PQOS_MSR_MON_EVTSEL_EVTID_MASK;
        if (msr_write(lcore,reg,val)!=MACHINE_RETVAL_OK)
                return PQOS_RETVAL_ERROR;

        /**
         * read selected data associated with previously selected RMID+event
         */
        reg = PQOS_MSR_MON_QMC;
        do {
                if (msr_read(lcore,reg,&val)!=MACHINE_RETVAL_OK) {
                        retval = PQOS_RETVAL_ERROR;
                        break;
                }

                if ((val&(PQOS_MSR_MON_QMC_ERROR))!=0ULL) {
                        retval = PQOS_RETVAL_ERROR;       /**< unsupported event id or RMID
                                                          selected */
                        break;
                }

                retries--;
        } while ( (val&PQOS_MSR_MON_QMC_UNAVAILABLE)!=0ULL && retries>0);

        /**
         * Store event value
         */
        if (retval==PQOS_RETVAL_OK)
                *value = (val & PQOS_MSR_MON_QMC_DATA_MASK);

        return retval;
}

int
pqos_mon_start( const unsigned num_cores,
                const unsigned *cores,
                const enum pqos_mon_event event,
                void *context,
                struct pqos_mon_data *group)
{
        unsigned cluster = 0, socket = 0;
        unsigned i = 0;
        int ret = PQOS_RETVAL_OK;
        pqos_rmid_t rmid = 0;

        if (group==NULL || cores==NULL || num_cores==0)
                return PQOS_RETVAL_PARAM;

        _pqos_api_lock();

        ret = _pqos_check_init(1);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return ret;
        }

        ASSERT(m_cpu!=NULL);
        for (i=0;i<num_cores;i++) {
                /**
                 * Check if all requested cores are valid
                 * and not used by other monitoring processes.
                 */
                unsigned lcore = cores[i];
                ret = pqos_cpu_check_core(m_cpu, lcore);
                if (ret!=PQOS_RETVAL_OK) {
                        _pqos_api_unlock();
                        return PQOS_RETVAL_PARAM;
                }
                if (m_core_map[lcore].unavailable) {
                        _pqos_api_unlock();
                        return PQOS_RETVAL_RESOURCE;
                }
        }

        if (num_cores>1) {
                /**
                 * Checks if requested cores belong 
                 * to the same cluster
                 */
                unsigned cluster_0 = 0, cluster_i = 0;
                for (i=0;i<num_cores;i++) {
                        ret = pqos_cpu_get_clusterid(m_cpu, cores[i],
                                                     (i==0) ? &cluster_0 : &cluster_i );
                        if (ret!=PQOS_RETVAL_OK) {
                                _pqos_api_unlock();
                                return PQOS_RETVAL_PARAM;
                        }
                        if (i==0)
                                continue;

                        if (cluster_i!=cluster_0) {
                                /**
                                 * Restrict usage to cores from a single cluster
                                 */
                                _pqos_api_unlock();
                                return PQOS_RETVAL_PARAM;
                        }
                }
        }

        ret = pqos_cpu_get_clusterid(m_cpu, cores[0], &cluster);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return PQOS_RETVAL_PARAM;
        }

        ret = pqos_cpu_get_socketid(m_cpu, cores[0], &socket);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return PQOS_RETVAL_PARAM;
        }

        for (i=0;i<num_cores;i++) {
                /**
                 * Check if any of requested cores is already subject to monitoring
                 * within this process
                 */
                unsigned lcore = cores[i];
                if (m_core_map[lcore].grp!=NULL) {
                        _pqos_api_unlock();
                        return PQOS_RETVAL_RESOURCE;
                }
        }

        /**
         * Validate event parameter
         */
        switch (event) {
        case PQOS_MON_EVENT_L3_OCCUP:
                break;
        default:
                _pqos_api_unlock();
                return PQOS_RETVAL_PARAM;
                break;
        }

        /**
         * Fill in the monitoring group structure
         */
        memset(group, 0, sizeof(*group));
        group->cores = (unsigned *) malloc(sizeof(group->cores[0])*num_cores);
        if (group->cores==NULL) {
                _pqos_api_unlock();
                return PQOS_RETVAL_RESOURCE;
        }

        ret = rmid_alloc(cluster, event, &rmid);
        if (ret!=PQOS_RETVAL_OK) {
                free(group->cores);
                _pqos_api_unlock();
                return PQOS_RETVAL_RESOURCE;
        }

        /**
         * Associate requested cores with
         * the allocated RMID
         */
        group->num_cores = num_cores;
        for (i=0;i<num_cores;i++) {
                group->cores[i] = cores[i];
                ret = mon_assoc_set(cores[i], cluster, rmid);
                if (ret!=PQOS_RETVAL_OK) {
                        free(group->cores);
                        _pqos_api_unlock();
                        return ret;
                }
        }

        group->event = event;
        group->rmid = rmid;
        group->cluster = cluster;
        group->socket = socket;
        group->context = context;
        
        for (i=0;i<num_cores;i++) {
                /**
                 * Mark monitoring activity in the core map
                 */
                unsigned lcore = cores[i];
                m_core_map[lcore].grp = group;
        }

        _pqos_api_unlock();
        return ret;
}

int
pqos_mon_stop( struct pqos_mon_data *group )
{
        int ret = PQOS_RETVAL_OK;
        unsigned i = 0;
        unsigned cluster = 0;

        if (group==NULL)
                return PQOS_RETVAL_PARAM;

        if (group->num_cores==0 || group->cores==NULL)
                return PQOS_RETVAL_PARAM;

        _pqos_api_lock();

        ret = _pqos_check_init(1);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return ret;
        }

        ASSERT(m_cpu!=NULL);
        for (i=0;i<group->num_cores;i++) {
                /**
                 * Validate core list in the group structure is correct
                 */
                unsigned lcore = group->cores[i];
                ret = pqos_cpu_check_core(m_cpu, lcore);
                if (ret!=PQOS_RETVAL_OK) {
                        _pqos_api_unlock();
                        return PQOS_RETVAL_PARAM;
                }
                if (m_core_map[lcore].grp==NULL) {
                        _pqos_api_unlock();
                        return PQOS_RETVAL_RESOURCE;
                }
        }

        /**
         * add check for the same cluster id
         */
        ret = pqos_cpu_get_clusterid(m_cpu,group->cores[0],&cluster);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return PQOS_RETVAL_PARAM;
        }

        for (i=0;i<group->num_cores;i++) {
                /**
                 * Associate cores from the group back with RMID0
                 */
                unsigned lcore = group->cores[i];
                m_core_map[lcore].grp = NULL;
                m_core_map[lcore].rmid = 0;
                ret = mon_assoc_set_nocheck(lcore,RMID0);
                if (ret!=PQOS_RETVAL_OK) {
                        _pqos_api_unlock();
                        return PQOS_RETVAL_RESOURCE;
                }
        }

        /**
         * Free previously allocated RMID
         */
        ret = rmid_free(cluster,group->rmid);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return PQOS_RETVAL_RESOURCE;
        }

        /**
         * Free the core list and clear the group structure
         */
        free(group->cores);
        memset(group,0,sizeof(*group));

        _pqos_api_unlock();
        return ret;
}

int
pqos_mon_poll(struct pqos_mon_data *groups,
              const unsigned num_groups)
{
        int ret = PQOS_RETVAL_OK;
        unsigned i = 0;

        ASSERT(groups!=NULL);
        ASSERT(num_groups>0);
        if (groups==NULL || num_groups==0)
                return PQOS_RETVAL_PARAM;

        _pqos_api_lock();

        ret = _pqos_check_init(1);
        if (ret!=PQOS_RETVAL_OK) {
                _pqos_api_unlock();
                return ret;
        }

        for (i=0;i<num_groups;i++) {
                ret = mon_read(groups[i].cores[0], groups[i].rmid,
                               groups[i].event, &groups[i].value);

                if (ret != PQOS_RETVAL_OK) {
                        LOG_WARN("Failed to read monitoring data for event %u on core %u (RMID%u)\n",
                                 groups[i].event, groups[i].cores[0], groups[i].rmid);
                        continue;
                }
        }

        _pqos_api_unlock();
        return PQOS_RETVAL_OK;
}

/**
 * =======================================
 * =======================================
 *
 * Small utils
 *
 * =======================================
 * =======================================
 */

/** 
 * @brief Finds maximum number of clusters in the topology
 *
 * @param cpu cpu topology structure
 * 
 * @return Max cluster id (plus one) in the topology.
 *         This indicates how big the look up table has to be if cluster_id is an index.
 */
static unsigned
cpu_get_num_clusters(const struct pqos_cpuinfo *cpu)
{
        unsigned i=0, n=0;

        ASSERT(cpu!=NULL);

        for (i=0;i<cpu->num_cores;i++)
                if (cpu->cores[i].cluster>n)
                        n = cpu->cores[i].cluster;

        return n+1;
}

/** 
 * @brief Finds maximum number of logical cores in the topology
 *
 * @param cpu cpu topology structure
 * 
 * @return Max core id (plus one) in the topology.
 *         This indicates how big the look up table has to be if core id is an index.
 */
static unsigned
cpu_get_num_cores(const struct pqos_cpuinfo *cpu)
{
        unsigned i=0, n=0;

        ASSERT(cpu!=NULL);

        for (i=0;i<cpu->num_cores;i++)
                if (cpu->cores[i].lcore>n)
                        n = cpu->cores[i].lcore;

        return n+1;
}
