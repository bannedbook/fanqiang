/**
 * @file
 * MQTT client
 */

/*
 * Copyright (c) 2016 Erik Andersson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Erik Andersson
 *
 */
#ifndef LWIP_HDR_APPS_MQTT_CLIENT_H
#define LWIP_HDR_APPS_MQTT_CLIENT_H

#include "lwip/apps/mqtt_opts.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/prot/iana.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mqtt_client_s mqtt_client_t;

#if LWIP_ALTCP && LWIP_ALTCP_TLS
struct altcp_tls_config;
#endif

/** @ingroup mqtt
 * Default MQTT port (non-TLS) */
#define MQTT_PORT     LWIP_IANA_PORT_MQTT
/** @ingroup mqtt
 * Default MQTT TLS port */
#define MQTT_TLS_PORT LWIP_IANA_PORT_SEQURE_MQTT

/*---------------------------------------------------------------------------------------------- */
/* Connection with server */

/**
 * @ingroup mqtt
 * Client information and connection parameters */
struct mqtt_connect_client_info_t {
  /** Client identifier, must be set by caller */
  const char *client_id;
  /** User name, set to NULL if not used */
  const char* client_user;
  /** Password, set to NULL if not used */
  const char* client_pass;
  /** keep alive time in seconds, 0 to disable keep alive functionality*/
  u16_t keep_alive;
  /** will topic, set to NULL if will is not to be used,
      will_msg, will_qos and will retain are then ignored */
  const char* will_topic;
  /** will_msg, see will_topic */
  const char* will_msg;
  /** will_qos, see will_topic */
  u8_t will_qos;
  /** will_retain, see will_topic */
  u8_t will_retain;
#if LWIP_ALTCP && LWIP_ALTCP_TLS
  /** TLS configuration for secure connections */
  struct altcp_tls_config *tls_config;
#endif
};

/**
 * @ingroup mqtt
 * Connection status codes */
typedef enum
{
  /** Accepted */
  MQTT_CONNECT_ACCEPTED                 = 0,
  /** Refused protocol version */
  MQTT_CONNECT_REFUSED_PROTOCOL_VERSION = 1,
  /** Refused identifier */
  MQTT_CONNECT_REFUSED_IDENTIFIER       = 2,
  /** Refused server */
  MQTT_CONNECT_REFUSED_SERVER           = 3,
  /** Refused user credentials */
  MQTT_CONNECT_REFUSED_USERNAME_PASS    = 4,
  /** Refused not authorized */
  MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_  = 5,
  /** Disconnected */
  MQTT_CONNECT_DISCONNECTED             = 256,
  /** Timeout */
  MQTT_CONNECT_TIMEOUT                  = 257
} mqtt_connection_status_t;

/**
 * @ingroup mqtt
 * Function prototype for mqtt connection status callback. Called when
 * client has connected to the server after initiating a mqtt connection attempt by
 * calling mqtt_connect() or when connection is closed by server or an error
 *
 * @param client MQTT client itself
 * @param arg Additional argument to pass to the callback function
 * @param status Connect result code or disconnection notification @see mqtt_connection_status_t
 *
 */
typedef void (*mqtt_connection_cb_t)(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);


/**
 * @ingroup mqtt
 * Data callback flags */
enum {
  /** Flag set when last fragment of data arrives in data callback */
  MQTT_DATA_FLAG_LAST = 1
};

/** 
 * @ingroup mqtt
 * Function prototype for MQTT incoming publish data callback function. Called when data
 * arrives to a subscribed topic @see mqtt_subscribe
 *
 * @param arg Additional argument to pass to the callback function
 * @param data User data, pointed object, data may not be referenced after callback return,
          NULL is passed when all publish data are delivered
 * @param len Length of publish data fragment
 * @param flags MQTT_DATA_FLAG_LAST set when this call contains the last part of data from publish message
 *
 */
typedef void (*mqtt_incoming_data_cb_t)(void *arg, const u8_t *data, u16_t len, u8_t flags);


/** 
 * @ingroup mqtt
 * Function prototype for MQTT incoming publish function. Called when an incoming publish
 * arrives to a subscribed topic @see mqtt_subscribe
 *
 * @param arg Additional argument to pass to the callback function
 * @param topic Zero terminated Topic text string, topic may not be referenced after callback return
 * @param tot_len Total length of publish data, if set to 0 (no publish payload) data callback will not be invoked
 */
typedef void (*mqtt_incoming_publish_cb_t)(void *arg, const char *topic, u32_t tot_len);


/**
 * @ingroup mqtt
 * Function prototype for mqtt request callback. Called when a subscribe, unsubscribe
 * or publish request has completed
 * @param arg Pointer to user data supplied when invoking request
 * @param err ERR_OK on success
 *            ERR_TIMEOUT if no response was received within timeout,
 *            ERR_ABRT if (un)subscribe was denied
 */
typedef void (*mqtt_request_cb_t)(void *arg, err_t err);


/** Connect to server */
err_t mqtt_client_connect(mqtt_client_t *client, const ip_addr_t *ipaddr, u16_t port, mqtt_connection_cb_t cb, void *arg,
                   const struct mqtt_connect_client_info_t *client_info);

/** Disconnect from server */
void mqtt_disconnect(mqtt_client_t *client);

mqtt_client_t *mqtt_client_new(void);
void mqtt_client_free(mqtt_client_t* client);

/** Check connection status */
u8_t mqtt_client_is_connected(mqtt_client_t *client);

/** Set callback to call for incoming publish */
void mqtt_set_inpub_callback(mqtt_client_t *client, mqtt_incoming_publish_cb_t,
                             mqtt_incoming_data_cb_t data_cb, void *arg);

/** Common function for subscribe and unsubscribe */
err_t mqtt_sub_unsub(mqtt_client_t *client, const char *topic, u8_t qos, mqtt_request_cb_t cb, void *arg, u8_t sub);

/** @ingroup mqtt
 *Subscribe to topic */
#define mqtt_subscribe(client, topic, qos, cb, arg) mqtt_sub_unsub(client, topic, qos, cb, arg, 1)
/** @ingroup mqtt
 *  Unsubscribe to topic */
#define mqtt_unsubscribe(client, topic, cb, arg) mqtt_sub_unsub(client, topic, 0, cb, arg, 0)


/** Publish data to topic */
err_t mqtt_publish(mqtt_client_t *client, const char *topic, const void *payload, u16_t payload_length, u8_t qos, u8_t retain,
                                    mqtt_request_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_MQTT_CLIENT_H */
