/*
 * ZentriOS SDK LICENSE AGREEMENT | Zentri.com, 2015.
 *
 * Use of source code and/or libraries contained in the ZentriOS SDK is
 * subject to the Zentri Operating System SDK license agreement and
 * applicable open source license agreements.
 *
 */
#pragma once


#include "mqtt_api.h"


#define SETTINGS_MAGIC_NUMBER       0xD5A8A3AAUL
#define MQTT_HOST                   "ambient-hub.azure-devices.net"
#define MQTT_DEVICE_ID              "007"
#define MQTT_TOKEN_EXPIRY           "1540935986"
#define MQTT_TOKEN_SIG              "FIf1UT0wV7vbvKybRcMOyMH4%2B3BPqBPJEO0cOI%2FwFXk%3D"
#define MQTT_PORT                   8883
#define MQTT_QOS                    MQTT_QOS_DELIVER_AT_MOST_ONCE
#define MQTT_SECURITY               ZOS_TRUE
#define MQTT_KEEPALIVE              120

#define MAX_TOPIC_STRING_SIZE       100
#define MAX_MESSAGE_STRING_SIZE     100
#define MAX_USERNAME_STRING_SIZE    100
#define MAX_PASSWORD_STRING_SIZE    200

#define MAX_TOKEN_SIG_SIZE          60
#define MAX_DEVICE_STRING_SIZE      50
#define MAX_HOST_STRING_SIZE        40
#define MAX_TOKEN_EXPIRY_SIZE       20


extern mqtt_connection_t* mqtt_connection;
extern char topic[MAX_TOPIC_STRING_SIZE+1];
extern char message[MAX_MESSAGE_STRING_SIZE+1];

typedef struct
{
    uint32_t magic;
    uint8_t device[MAX_DEVICE_STRING_SIZE];
    uint8_t host[MAX_HOST_STRING_SIZE];
    uint8_t token_expiry[MAX_TOKEN_EXPIRY_SIZE];
    uint8_t token_sig[MAX_TOKEN_SIG_SIZE];
    uint16_t port;
    uint16_t keepalive;
    uint8_t qos;
    zos_bool_t security;
} mqtt_settings_t;

void commands_init(void);
void commands_deinit(void);

void mqtt_app_connect( void *arg );
void mqtt_app_disconnect( void *arg );
void mqtt_app_subscribe( void *arg );
void mqtt_app_unsubscribe( void *arg );
void mqtt_app_publish( void *arg );
