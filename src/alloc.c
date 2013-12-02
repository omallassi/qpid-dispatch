/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <qpid/dispatch/alloc.h>
#include <qpid/dispatch/ctools.h>
#include <qpid/dispatch/log.h>
#include <qpid/dispatch/agent.h>
#include <memory.h>
#include <stdio.h>

#define QD_MEMORY_DEBUG 1

typedef struct qd_alloc_type_t qd_alloc_type_t;
typedef struct qd_alloc_item_t qd_alloc_item_t;

struct qd_alloc_type_t {
    DEQ_LINKS(qd_alloc_type_t);
    qd_alloc_type_desc_t *desc;
};

DEQ_DECLARE(qd_alloc_type_t, qd_alloc_type_list_t);

#define PATTERN_FRONT 0xdeadbeef
#define PATTERN_BACK  0xbabecafe

struct qd_alloc_item_t {
    DEQ_LINKS(qd_alloc_item_t);
#ifdef QD_MEMORY_DEBUG
    qd_alloc_type_desc_t *desc;
    uint32_t              header;
#endif
};

DEQ_DECLARE(qd_alloc_item_t, qd_alloc_item_list_t);


struct qd_alloc_pool_t {
    qd_alloc_item_list_t free_list;
};

qd_alloc_config_t qd_alloc_default_config_big   = {16,  32, 0};
qd_alloc_config_t qd_alloc_default_config_small = {64, 128, 0};
#define BIG_THRESHOLD 256

static sys_mutex_t          *init_lock = 0;
static qd_alloc_type_list_t  type_list;

static void qd_alloc_init(qd_alloc_type_desc_t *desc)
{
    sys_mutex_lock(init_lock);

    //qd_log("ALLOC", LOG_TRACE, "Initialized Allocator - type=%s type-size=%d total-size=%d",
    //       desc->type_name, desc->type_size, desc->total_size);

    if (!desc->global_pool) {
        desc->total_size = desc->type_size;
        if (desc->additional_size)
            desc->total_size += *desc->additional_size;

        if (desc->config == 0)
            desc->config = desc->total_size > BIG_THRESHOLD ?
                &qd_alloc_default_config_big : &qd_alloc_default_config_small;

        assert (desc->config->local_free_list_max >= desc->config->transfer_batch_size);

        desc->global_pool = NEW(qd_alloc_pool_t);
        DEQ_INIT(desc->global_pool->free_list);
        desc->lock = sys_mutex();
        desc->stats = NEW(qd_alloc_stats_t);
        memset(desc->stats, 0, sizeof(qd_alloc_stats_t));

        qd_alloc_type_t *type_item = NEW(qd_alloc_type_t);
        DEQ_ITEM_INIT(type_item);
        type_item->desc = desc;
        DEQ_INSERT_TAIL(type_list, type_item);

        desc->header  = PATTERN_FRONT;
        desc->trailer = PATTERN_BACK;
    }

    sys_mutex_unlock(init_lock);
}


void *qd_alloc(qd_alloc_type_desc_t *desc, qd_alloc_pool_t **tpool)
{
    int idx;

    //
    // If the descriptor is not initialized, set it up now.
    //
    if (desc->trailer != PATTERN_BACK)
        qd_alloc_init(desc);

    //
    // If this is the thread's first pass through here, allocate the
    // thread-local pool for this type.
    //
    if (*tpool == 0) {
        *tpool = NEW(qd_alloc_pool_t);
        DEQ_INIT((*tpool)->free_list);
    }

    qd_alloc_pool_t *pool = *tpool;

    //
    // Fast case: If there's an item on the local free list, take it off the
    // list and return it.  Since everything we've touched is thread-local,
    // there is no need to acquire a lock.
    //
    qd_alloc_item_t *item = DEQ_HEAD(pool->free_list);
    if (item) {
        DEQ_REMOVE_HEAD(pool->free_list);
#ifdef QD_MEMORY_DEBUG
        item->desc = desc;
        item->header = PATTERN_FRONT;
        *((uint32_t*) ((void*) &item[1] + desc->total_size))= PATTERN_BACK;
#endif
        return &item[1];
    }

    //
    // The local free list is empty, we need to either rebalance a batch
    // of items from the global list or go to the heap to get new memory.
    //
    sys_mutex_lock(desc->lock);
    if (DEQ_SIZE(desc->global_pool->free_list) >= desc->config->transfer_batch_size) {
        //
        // Rebalance a full batch from the global free list to the thread list.
        //
        desc->stats->batches_rebalanced_to_threads++;
        desc->stats->held_by_threads += desc->config->transfer_batch_size;
        for (idx = 0; idx < desc->config->transfer_batch_size; idx++) {
            item = DEQ_HEAD(desc->global_pool->free_list);
            DEQ_REMOVE_HEAD(desc->global_pool->free_list);
            DEQ_INSERT_TAIL(pool->free_list, item);
        }
    } else {
        //
        // Allocate a full batch from the heap and put it on the thread list.
        //
        for (idx = 0; idx < desc->config->transfer_batch_size; idx++) {
            item = (qd_alloc_item_t*) malloc(sizeof(qd_alloc_item_t) + desc->total_size
#ifdef QD_MEMORY_DEBUG
                                             + sizeof(uint32_t)
#endif
                                             );
            if (item == 0)
                break;
            DEQ_ITEM_INIT(item);
            DEQ_INSERT_TAIL(pool->free_list, item);
            desc->stats->held_by_threads++;
            desc->stats->total_alloc_from_heap++;
        }
    }
    sys_mutex_unlock(desc->lock);

    item = DEQ_HEAD(pool->free_list);
    if (item) {
        DEQ_REMOVE_HEAD(pool->free_list);
#ifdef QD_MEMORY_DEBUG
        item->desc = desc;
        item->header = PATTERN_FRONT;
        *((uint32_t*) ((void*) &item[1] + desc->total_size))= PATTERN_BACK;
#endif
        return &item[1];
    }

    return 0;
}


void qd_dealloc(qd_alloc_type_desc_t *desc, qd_alloc_pool_t **tpool, void *p)
{
    qd_alloc_item_t *item = ((qd_alloc_item_t*) p) - 1;
    int              idx;

#ifdef QD_MEMORY_DEBUG
    assert (desc->header  == PATTERN_FRONT);
    assert (desc->trailer == PATTERN_BACK);
    assert (item->header  == PATTERN_FRONT);
    assert (*((uint32_t*) (p + desc->total_size)) == PATTERN_BACK);
    assert (item->desc == desc);
    item->desc = 0;
#endif

    //
    // If this is the thread's first pass through here, allocate the
    // thread-local pool for this type.
    //
    if (*tpool == 0) {
        *tpool = NEW(qd_alloc_pool_t);
        DEQ_INIT((*tpool)->free_list);
    }

    qd_alloc_pool_t *pool = *tpool;

    DEQ_INSERT_TAIL(pool->free_list, item);

    if (DEQ_SIZE(pool->free_list) <= desc->config->local_free_list_max)
        return;

    //
    // We've exceeded the maximum size of the local free list.  A batch must be
    // rebalanced back to the global list.
    //
    sys_mutex_lock(desc->lock);
    desc->stats->batches_rebalanced_to_global++;
    desc->stats->held_by_threads -= desc->config->transfer_batch_size;
    for (idx = 0; idx < desc->config->transfer_batch_size; idx++) {
        item = DEQ_HEAD(pool->free_list);
        DEQ_REMOVE_HEAD(pool->free_list);
        DEQ_INSERT_TAIL(desc->global_pool->free_list, item);
    }

    //
    // If there's a global_free_list size limit, remove items until the limit is
    // not exceeded.
    //
    if (desc->config->global_free_list_max != 0) {
        while (DEQ_SIZE(desc->global_pool->free_list) > desc->config->global_free_list_max) {
            item = DEQ_HEAD(desc->global_pool->free_list);
            DEQ_REMOVE_HEAD(desc->global_pool->free_list);
            free(item);
            desc->stats->total_free_to_heap++;
        }
    }

    sys_mutex_unlock(desc->lock);
}


void qd_alloc_initialize(void)
{
    init_lock = sys_mutex();
    DEQ_INIT(type_list);
}


static void alloc_schema_handler(void *context, void *correlator)
{
}


static void alloc_query_handler(void* context, const char *id, void *cor)
{
    qd_alloc_type_t *item = DEQ_HEAD(type_list);

    while (item) {
        qd_agent_value_string(cor, "name", item->desc->type_name);
        qd_agent_value_uint(cor, "type_size", item->desc->total_size);
        qd_agent_value_uint(cor, "transfer_batch_size", item->desc->config->transfer_batch_size);
        qd_agent_value_uint(cor, "local_free_list_max", item->desc->config->local_free_list_max);
        qd_agent_value_uint(cor, "global_free_list_max", item->desc->config->global_free_list_max);
        qd_agent_value_uint(cor, "total_alloc_from_heap", item->desc->stats->total_alloc_from_heap);
        qd_agent_value_uint(cor, "total_free_to_heap", item->desc->stats->total_free_to_heap);
        qd_agent_value_uint(cor, "held_by_threads", item->desc->stats->held_by_threads);
        qd_agent_value_uint(cor, "batches_rebalanced_to_threads", item->desc->stats->batches_rebalanced_to_threads);
        qd_agent_value_uint(cor, "batches_rebalanced_to_global", item->desc->stats->batches_rebalanced_to_global);

        item = DEQ_NEXT(item);
        qd_agent_value_complete(cor, item != 0);
    }
}


void qd_alloc_setup_agent(qd_dispatch_t *qd)
{
    qd_agent_register_class(qd, "org.apache.qpid.dispatch.allocator", 0, alloc_schema_handler, alloc_query_handler);
}
