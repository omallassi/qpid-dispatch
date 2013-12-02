#ifndef __dispatch_server_h__
#define __dispatch_server_h__ 1
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

#include <qpid/dispatch/dispatch.h>
#include <proton/engine.h>

/**
 * \defgroup Control Server Control Functions
 * @{
 */

/**
 * \brief Thread Start Handler
 *
 * Callback invoked when a new server thread is started.  The callback is
 * invoked on the newly created thread.
 *
 * This handler can be used to set processor affinity or other thread-specific
 * tuning values.
 *
 * @param context The handler context supplied in qd_server_initialize.
 * @param thread_id The integer thread identifier that uniquely identifies this thread.
 */
typedef void (*qd_thread_start_cb_t)(void* context, int thread_id);


/**
 * \brief Set the optional thread-start handler.
 *
 * This handler is called once on each worker thread at the time the thread is
 * started.  This may be used to set tuning settings like processor affinity,
 * etc.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 * @param start_handler The thread-start handler invoked per thread on thread startup.
 * @param context Opaque context to be passed back in the callback function.
 */
void qd_server_set_start_handler(qd_dispatch_t *qd, qd_thread_start_cb_t start_handler, void *context);


/**
 * \brief Run the server threads until completion - The blocking version.
 *
 * Start the operation of the server, including launching all of the worker
 * threads.  This function does not return until after the server has been
 * stopped.  The thread that calls qd_server_run is used as one of the worker
 * threads.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 */
void qd_server_run(qd_dispatch_t *qd);


/**
 * \brief Start the server threads and return immediately - The non-blocking version.
 *
 * Start the operation of the server, including launching all of the worker
 * threads.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 */
void qd_server_start(qd_dispatch_t *qd);


/**
 * \brief Stop the server
 *
 * Stop the server and join all of its worker threads.  This function may be
 * called from any thread.  When this function returns, all of the other
 * server threads have been closed and joined.  The calling thread will be the
 * only running thread in the process.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 */
void qd_server_stop(qd_dispatch_t *qd);


/**
 * \brief Pause (quiesce) the server.
 *
 * This call blocks until all of the worker threads (except the one calling
 * this function) are finished processing and have been blocked.  When this
 * call returns, the calling thread is the only thread running in the process.
 *
 * If the calling process is *not* one of the server's worker threads, then
 * this function will block all of the worker threads before returning.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 */
void qd_server_pause(qd_dispatch_t *qd);


/**
 * \brief Resume normal operation of a paused server.
 *
 * This call unblocks all of the worker threads so they can resume normal
 * connection processing.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 */
void qd_server_resume(qd_dispatch_t *qd);


/**
 * @}
 * \defgroup Signal Server Signal Handling Functions
 * @{
 */


/**
 * \brief Signal Handler
 *
 * Callback for signal handling.  This handler will be invoked on one of the
 * worker threads in an orderly fashion.  This callback is triggered by a call
 * to qd_server_signal.
 *
 * @param context The handler context supplied in qd_server_initialize.
 * @param signum The signal number that was passed into qd_server_signal.
 */
typedef void (*qd_signal_handler_cb_t)(void* context, int signum);


/**
 * Set the signal handler for the server.  The signal handler is invoked
 * cleanly on a worker thread after a call is made to qd_server_signal.  The
 * signal handler is optional and need not be set.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 * @param signal_handler The signal handler called when a registered signal is caught.
 * @param context Opaque context to be passed back in the callback function.
 */
void qd_server_set_signal_handler(qd_dispatch_t *qd, qd_signal_handler_cb_t signal_handler, void *context);


/**
 * \brief Schedule the invocation of the Server's signal handler.
 *
 * This function is safe to call from any context, including an OS signal
 * handler or an Interrupt Service Routine.  It schedules the orderly
 * invocation of the Server's signal handler on one of the worker threads.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 * @param signum The signal number... TODO
 */
void qd_server_signal(qd_dispatch_t *qd, int signum);


/**
 * @}
 * \defgroup Connection Server AMQP Connection Handling Functions
 * @{
 */

/**
 * \brief Listener objects represent the desire to accept incoming transport connections.
 */
typedef struct qd_listener_t qd_listener_t;

/**
 * \brief Connector objects represent the desire to create and maintain an outgoing transport connection.
 */
typedef struct qd_connector_t qd_connector_t;

/**
 * \brief Connection objects wrap Proton connection objects.
 */
typedef struct qd_connection_t qd_connection_t;

/**
 * \brief Event type for the connection callback.
 */
typedef enum {
    /// The connection just opened via a listener (inbound).
    QD_CONN_EVENT_LISTENER_OPEN,

    /// The connection just opened via a connector (outbound).
    QD_CONN_EVENT_CONNECTOR_OPEN,

    /// The connection was closed at the transport level (not cleanly).
    QD_CONN_EVENT_CLOSE,

    /// The connection requires processing.
    QD_CONN_EVENT_PROCESS
} qd_conn_event_t;


/**
 * \brief Configuration block for a connector or a listener.
 */
typedef struct qd_server_config_t {
    /**
     * Host name or network address to bind to a listener or use in the connector.
     */
    const char *host;

    /**
     * Port name or number to bind to a listener or use in the connector.
     */
    const char *port;

    /**
     * Space-separated list of SASL mechanisms to be accepted for the connection.
     */
    const char *sasl_mechanisms;

    /**
     * If appropriate for the mechanism, the username for authentication
     * (connector only)
     */
    const char *sasl_username;

    /**
     * If appropriate for the mechanism, the password for authentication
     * (connector only)
     */
    const char *sasl_password;

    /**
     * If appropriate for the mechanism, the minimum acceptable security strength factor
     */
    int sasl_minssf;

    /**
     * If appropriate for the mechanism, the maximum acceptable security strength factor
     */
    int sasl_maxssf;

    /**
     * SSL is enabled for this connection iff non-zero.
     */
    int ssl_enabled;

    /**
     * Connection will take on the role of SSL server iff non-zero.
     */
    int ssl_server;

    /**
     * Iff non-zero AND ssl_enabled is non-zero, this listener will detect the client's use
     * of SSL or non-SSL and conform to the client's protocol.
     * (listener only)
     */
    int ssl_allow_unsecured_client;

    /**
     * Path to the file containing the PEM-formatted public certificate for the local end
     * of the connection.
     */
    const char *ssl_certificate_file;

    /**
     * Path to the file containing the PEM-formatted private key for the local end of the
     * connection.
     */
    const char *ssl_private_key_file;

    /**
     * The password used to sign the private key, or NULL if the key is not protected.
     */
    const char *ssl_password;

    /**
     * Path to the file containing the PEM-formatted set of certificates of trusted CAs.
     */
    const char *ssl_trusted_certificate_db;

    /**
     * Iff non-zero, require that the peer's certificate be supplied and that it be authentic
     * according to the set of trusted CAs.
     */
    int ssl_require_peer_authentication;

    /**
     * Allow the connection to be redirected by the peer (via CLOSE->Redirect).  This is
     * meaningful for outgoing (connector) connections only.
     */
    int allow_redirect;

    /**
     * The specified role of the connection.  This can be used to control the behavior and
     * capabilities of the connections.
     */
    const char *role;
} qd_server_config_t;


/**
 * \brief Connection Event Handler
 *
 * Callback invoked when processing is needed on a proton connection.  This
 * callback shall be invoked on one of the server's worker threads.  The
 * server guarantees that no two threads shall be allowed to process a single
 * connection concurrently.  The implementation of this handler may assume
 * that it has exclusive access to the connection and its subservient
 * components (sessions, links, deliveries, etc.).
 *
 * @param handler_context The handler context supplied in qd_server_set_conn_handler.
 * @param conn_context The handler context supplied in qd_server_{connect,listen}.
 * @param event The event/reason for the invocation of the handler.
 * @param conn The connection that requires processing by the handler.
 * @return A value greater than zero if the handler did any proton processing for
 *         the connection.  If no work was done, zero is returned.
 */
typedef int (*qd_conn_handler_cb_t)(void *handler_context, void* conn_context, qd_conn_event_t event, qd_connection_t *conn);


/**
 * \brief Set the connection event handler callback.
 *
 * Set the connection handler callback for the server.  This callback is
 * mandatory and must be set prior to the invocation of qd_server_run.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 * @param conn_hander The handler for processing connection-related events.
 */
void qd_server_set_conn_handler(qd_dispatch_t *qd, qd_conn_handler_cb_t conn_handler, void *handler_context);


/**
 * \brief Set the user context for a connection.
 *
 * @param conn Connection object supplied in QD_CONN_EVENT_{LISTENER,CONNETOR}_OPEN
 * @param context User context to be stored with the connection.
 */
void qd_connection_set_context(qd_connection_t *conn, void *context);


/**
 * \brief Get the user context from a connection.
 *
 * @param conn Connection object supplied in QD_CONN_EVENT_{LISTENER,CONNETOR}_OPEN
 * @return The user context stored with the connection.
 */
void *qd_connection_get_context(qd_connection_t *conn);


/**
 * \brief Set the link context for a connection.
 *
 * @param conn Connection object supplied in QD_CONN_EVENT_{LISTENER,CONNETOR}_OPEN
 * @param context Link context to be stored with the connection.
 */
void qd_connection_set_link_context(qd_connection_t *conn, void *context);


/**
 * \brief Get the link context from a connection.
 *
 * @param conn Connection object supplied in QD_CONN_EVENT_{LISTENER,CONNETOR}_OPEN
 * @return The link context stored with the connection.
 */
void *qd_connection_get_link_context(qd_connection_t *conn);


/**
 * \brief Activate a connection for output.
 *
 * This function is used to request that the server activate the indicated
 * connection.  It is assumed that the connection is one that the caller does
 * not have permission to access (i.e. it may be owned by another thread
 * currently).  An activated connection will, when writable, appear in the
 * internal work list and be invoked for processing by a worker thread.
 *
 * @param conn The connection over which the application wishes to send data
 */
void qd_server_activate(qd_connection_t *conn);


/**
 * \brief Get the wrapped proton-engine connection object.
 *
 * @param conn Connection object supplied in QD_CONN_EVENT_{LISTENER,CONNETOR}_OPEN
 * @return The proton connection object.
 */
pn_connection_t *qd_connection_pn(qd_connection_t *conn);


/**
 * \brief Get the configuration that was used in the setup of this connection.
 *
 * @param conn Connection object supplied in QD_CONN_EVENT_{LISTENER,CONNETOR}_OPEN
 * @return A pointer to the server configuration used in the establishment of this connection.
 */
const qd_server_config_t *qd_connection_config(const qd_connection_t *conn);


/**
 * \brief Create a listener for incoming connections.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 * @param config Pointer to a configuration block for this listener.  This block will be
 *               referenced by the server, not copied.  The referenced record must remain
 *               in-scope for the life of the listener.
 * @param context User context passed back in the connection handler.
 * @return A pointer to the new listener, or NULL in case of failure.
 */
qd_listener_t *qd_server_listen(qd_dispatch_t *qd, const qd_server_config_t *config, void *context);


/**
 * \brief Free the resources associated with a listener.
 *
 * @param li A listener pointer returned by qd_listen.
 */
void qd_listener_free(qd_listener_t* li);


/**
 * \brief Close a listener so it will accept no more connections.
 *
 * @param li A listener pointer returned by qd_listen.
 */
void qd_listener_close(qd_listener_t* li);


/**
 * \brief Create a connector for an outgoing connection.
 *
 * @param qd The dispatch handle returned by qd_dispatch.
 * @param config Pointer to a configuration block for this connector.  This block will be
 *               referenced by the server, not copied.  The referenced record must remain
 *               in-scope for the life of the connector..
 * @param context User context passed back in the connection handler.
 * @return A pointer to the new connector, or NULL in case of failure.
 */
qd_connector_t *qd_server_connect(qd_dispatch_t *qd, const qd_server_config_t *config, void *context);


/**
 * \brief Free the resources associated with a connector.
 *
 * @param ct A connector pointer returned by qd_connect.
 */
void qd_connector_free(qd_connector_t* ct);

/**
 * @}
 */

#endif