#ifndef __dispatch_bitmask_h__
#define __dispatch_bitmask_h__ 1
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

typedef struct qd_bitmask_t qd_bitmask_t;

int qd_bitmask_width();
qd_bitmask_t *qd_bitmask(int initial);
void qd_bitmask_free(qd_bitmask_t *b);
void qd_bitmask_set_all(qd_bitmask_t *b);
void qd_bitmask_clear_all(qd_bitmask_t *b);
void qd_bitmask_set_bit(qd_bitmask_t *b, int bitnum);
void qd_bitmask_clear_bit(qd_bitmask_t *b, int bitnum);
int qd_bitmask_value(qd_bitmask_t *b, int bitnum);
int qd_bitmask_first_set(qd_bitmask_t *b, int *bitnum);



#endif
