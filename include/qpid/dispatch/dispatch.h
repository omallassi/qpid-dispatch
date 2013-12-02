#ifndef __dispatch_dispatch_h__
#define __dispatch_dispatch_h__ 1
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

/**
 * \defgroup General Dispatch Definitions
 * @{
 */

typedef struct qd_dispatch_t qd_dispatch_t;

/**
 * \brief Initialize the Dispatch library and prepare it for operation.
 *
 * @param config_path The path to the configuration file.
 * @param python_pkgdir The path to the Python files.
 * @return A handle to be used in API calls for this instance.
 */
qd_dispatch_t *qd_dispatch(const char *config_path,
                           const char *python_pkgdir);


/**
 * \brief Finalize the Dispatch library after it has stopped running.
 *
 * @param dispatch The dispatch handle returned by qd_dispatch
 */
void qd_dispatch_free(qd_dispatch_t *dispatch);

void qd_dispatch_configure(qd_dispatch_t *dispatch);


/**
 * @}
 */

#endif