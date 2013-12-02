#ifndef __dispatch_alloc_h__
#define __dispatch_alloc_h__ 1
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

#include <stdlib.h>
#include <stdint.h>
#include <qpid/dispatch/threading.h>

typedef struct qd_alloc_pool_t qd_alloc_pool_t;

typedef struct {
    int  transfer_batch_size;
    int  local_free_list_max;
    int  global_free_list_max;
} qd_alloc_config_t;

typedef struct {
    uint64_t total_alloc_from_heap;
    uint64_t total_free_to_heap;
    uint64_t held_by_threads;
    uint64_t batches_rebalanced_to_threads;
    uint64_t batches_rebalanced_to_global;
} qd_alloc_stats_t;

typedef struct {
    uint32_t           header;
    char              *type_name;
    size_t             type_size;
    size_t            *additional_size;
    size_t             total_size;
    qd_alloc_config_t *config;
    qd_alloc_stats_t  *stats;
    qd_alloc_pool_t   *global_pool;
    sys_mutex_t       *lock;
    uint32_t           trailer;
} qd_alloc_type_desc_t;


void *qd_alloc(qd_alloc_type_desc_t *desc, qd_alloc_pool_t **tpool);
void qd_dealloc(qd_alloc_type_desc_t *desc, qd_alloc_pool_t **tpool, void *p);


#define ALLOC_DECLARE(T) \
    T *new_##T();        \
    void free_##T(T *p)

#define ALLOC_DEFINE_CONFIG(T,S,A,C)                                \
    qd_alloc_type_desc_t __desc_##T = {0, #T, S, A, 0, C, 0, 0, 0, 0};    \
    __thread qd_alloc_pool_t *__local_pool_##T = 0;                 \
    T *new_##T() { return (T*) qd_alloc(&__desc_##T, &__local_pool_##T); }  \
    void free_##T(T *p) { qd_dealloc(&__desc_##T, &__local_pool_##T, (void*) p); } \
    qd_alloc_stats_t *alloc_stats_##T() { return __desc_##T.stats; }

#define ALLOC_DEFINE(T) ALLOC_DEFINE_CONFIG(T, sizeof(T), 0, 0)


#endif