#ifndef __python_embedded_h__
#define __python_embedded_h__ 1
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

#include <Python.h>
#include <qpid/dispatch/dispatch.h>
#include <qpid/dispatch/compose.h>
#include <qpid/dispatch/parse.h>
#include <qpid/dispatch/iterator.h>

/**
 * Initialize the embedded-python subsystem.  This must be called before
 * any other call into this module is invoked.
 */
void qd_python_initialize(qd_dispatch_t *qd,
                          const char    *python_pkgdir);

/**
 * Finalize the embedded-python subsystem.  After this is called, there
 * must be no further invocation of qd_python methods.
 */
void qd_python_finalize();

/**
 * Start using embedded python.  This is called once by each module that plans
 * to use embedded python capabilities.  It must call qd_python_start before
 * using any python components.
 */
void qd_python_start();

/**
 * Stop using embedded python.  This is called once by each module after it is
 * finished using embedded python capabilities.
 */
void qd_python_stop();

/**
 * Get the Python top level "dispatch" module.
 */
PyObject *qd_python_module();

/**
 * Convert a Python object to AMQP format and append to a composed_field.
 *
 * @param value A Python Object
 * @param field A composed field
 */
void qd_py_to_composed(PyObject *value, qd_composed_field_t *field);

/**
 * Convert a parsed field to a Python object
 *
 * @param field A parsed field
 * @return A generated Python object
 */
PyObject *qd_field_to_py(qd_parsed_field_t *field);

/**
 * These are temporary and will eventually be replaced by having an internal python
 * work queue that feeds a dedicated embedded-python thread.
 */
void qd_python_lock();
void qd_python_unlock();

#endif