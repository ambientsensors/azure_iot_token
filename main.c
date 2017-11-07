/*
 * ZentriOS SDK LICENSE AGREEMENT | Zentri.com, 2015.
 *
 * Use of source code and/or libraries contained in the ZentriOS SDK is
 * subject to the Zentri Operating System SDK license agreement and
 * applicable open source license agreements.
 *
 */

#include "zos.h"
#include "common.h"
#include "mesh_control.h"

/** @file
 *
 * Send/Receive MQTT Application
 *
 * This application snippet demonstrates how to use the MQTT client library.
 *
 * Features demonstrated
 *  - MQTT Client initiation
 *  - MQTT Client publish (sending / receiving)
 *  - MQTT Client subscribing
 *  - MQTT Client unsubscribing
 *
 * Before running the application, Make sure an MQTT daemon that supports
 * version 3.1.1 (eg: mosqitto-server) is running on your broker machine.
 *
 * Once application started, the terminal displays the supported commands:
 *  - List all variables                        : get mqtt");
 *  - Set variable <mqtt.var> to value <val>    : set mqtt.var val");
 *  - Connect to broker <mqtt.host:port>        : mqtt_connect");
 *  - Subscribe to topic                        : mqtt_subscribe <topic>");
 *  - Publish to topic                          : mqtt_publish <topic> <message>");
 *  - Unsubscribe from topic                    : mqtt_unsubscribe <topic>");
 *  - Disconnect from broker <mqtt.host>        : mqtt_disconnect");
 *
 * NOTES:
 *
 *  - Troubleshooting: If you are having difficulty connecting the MQTT broker,
 *    Make sure your server IP address is configured correctly
 *    Make sure the proposed MQTT version (v3.1.1) is supported by the broker.
 *
 *  - Retransmission: MQTT provides two types of retransmission:
 *    > Session continue retransmission: When a session is not set as clean, retransmission
 *      of any queued packets from previous session is done once a connect ACK is received.
 *      Session retransmission is part of the MQTT standard.
 *    > In session retransmission: This basically means trying to resend un ACKed packets.
 *      MQTT left this as an application specific and didn't indicate if/when retransmissions
 *      should occur.  MQTT does retransmissions for unACKed packets on the reception
 *      of a PINGRES (ping response) from the daemon. If keep_alive is disabled (not recommended),
 *      sessions retransmissions will be disabled.
 */
/******************************************************
 *                      Macros
 ******************************************************/
#define MAX_SIS 8

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/
static zos_result_t mqtt_connection_event_cb( mqtt_event_info_t *event );

/******************************************************
 *               Variable Definitions
 ******************************************************/
mqtt_settings_t *settings;
mqtt_connection_t* mqtt_connection;
char topic[MAX_TOPIC_STRING_SIZE+1];
char message[MAX_MESSAGE_STRING_SIZE+1];
uint8_t username[MAX_USERNAME_STRING_SIZE+1];
uint8_t password[MAX_PASSWORD_STRING_SIZE+1];

static mqtt_callback_t callback = mqtt_connection_event_cb;

/******************************************************
 *               Function Definitions
 ******************************************************/
void zn_app_init(void)
{
    zos_result_t result;
    char systemindicator_string[MAX_SIS];

    ZOS_LOG("Calling the uart config now!!!!");
    setup_serial_port();

    /* Bring up the network interface */
    if (!zn_network_is_up(ZOS_WLAN))
    {
        ZOS_LOG("Network is down, restarting...");
        if(ZOS_FAILED(result, zn_network_restart(ZOS_WLAN)))
        {
            ZOS_LOG("Failed to restart network: %d", result);
            return;
        }
    }

    /* Memory allocated for MQTT object*/
    zn_malloc((uint8_t**)&mqtt_connection, sizeof(mqtt_connection_t));
    if ( mqtt_connection == NULL )
    {
        ZOS_LOG("Failed to allocate MQTT object...\n");
        return;
    }

    result = mqtt_init( mqtt_connection );
    if(result != ZOS_SUCCESS)
    {
        ZOS_LOG("Error initializing");
        zn_free(mqtt_connection);
        mqtt_connection = NULL;
        return;
    }

    commands_init();
    ZOS_NVM_GET_REF(settings);

    ZOS_LOG("MQTT Demo Application Started:");
    ZOS_LOG("  - List all variables                        : get mqtt");
    ZOS_LOG("  - Set variable <mqtt.var> to value <val>    : set mqtt.var val");
    ZOS_LOG("  - Connect to broker <mqtt.host:port>        : mqtt_connect");
    ZOS_LOG("  - Subscribe to topic                        : mqtt_subscribe <topic>");
    ZOS_LOG("  - Publish to topic                          : mqtt_publish <topic> <message>");
    ZOS_LOG("  - Unsubscribe from topic                    : mqtt_unsubscribe <topic>");
    ZOS_LOG("  - Disconnect from broker <mqtt.host>        : mqtt_disconnect");
    ZOS_LOG("  - send cmd to mesh (\"cmd - -\" for usage)    : cmd");

    if(zn_load_app_settings("settings.ini") != ZOS_SUCCESS)
    {
        ZOS_LOG("Failed to load settings");
        return;
    }

    /// make sure that GPIO 23 wasn't set to wlan - if so, change it to -1 and save it
    /// to NVRAM.  This is to prevent the OS from blinking the WLAN LED by using GPIO 23
    /// (which we need to use for UART RTS).  If we get -1 back, we don't have to override.
    zn_settings_get_print_str("system.indicator.gpio wlan", systemindicator_string, MAX_SIS);
    if (strncmp(systemindicator_string, "-1", 2) != 0)
    {
        zn_settings_set_int32("system.indicator.gpio wlan", -1);
        zn_settings_save(NULL);
    }

    zn_settings_get_str("system.uuid", (char *) settings->device, sizeof(settings->device));
#if 0
    // override the UUID if you need to pretend to be the original demo zentri board
    strcpy((char *) settings->device, "0641304100000000220068001851343438333231");
#endif

    zn_event_issue(mqtt_app_connect, NULL, 0);
}


/*************************************************************************************************/
void zn_app_deinit(void)
{
    mqtt_deinit( mqtt_connection );
    zn_free(mqtt_connection);
    mqtt_connection = NULL;
}


/*************************************************************************************************/
zos_bool_t zn_app_idle(void)
{
    return ZOS_TRUE; // don't kill the app
    // if not initialized then kill the app
//    return mqtt_connection->session_init;
}

/*************************************************************************************************/
/*
 * Connect to broker (sent connect frame)
 */
void mqtt_app_connect( void *arg )
{
    mqtt_pkt_connect_t conninfo;
    zos_result_t ret = ZOS_SUCCESS;

    sprintf((char*)username, "%s/%s/api-version=2016-11-14", settings->host, settings->device);
    sprintf((char*)password, "SharedAccessSignature sr=%s%%2Fdevices%%2F%s&sig=%s&se=%s",
            settings->host, settings->device, settings->token_sig, settings->token_expiry);

    ZOS_LOG("Opening connection with broker %s:%u", settings->host, settings->port);
    ret = mqtt_open( mqtt_connection, (const char*)settings->host, settings->port, ZOS_WLAN, callback, settings->security );
    if ( ret != ZOS_SUCCESS )
    {
        ZOS_LOG("Error opening connection (keys and certificates are set properly?)");
        return;
    }
    ZOS_LOG("Connection established");

    /* Now, after socket is connected we can send the CONNECT frame safely */
    ZOS_LOG("Connecting...");
    memset( &conninfo, 0, sizeof( conninfo ) );

    conninfo.mqtt_version = MQTT_PROTOCOL_VER4;
    conninfo.clean_session = 1; /// Azure, don't give me old messages
    conninfo.client_id = settings->device;

    conninfo.keep_alive = settings->keepalive;
    conninfo.username = username;
    conninfo.password = password;

    ZOS_LOG("Client ID: %s", conninfo.client_id);
    ZOS_LOG("Username: %s", conninfo.username);
    ZOS_LOG("Password(token): %s", conninfo.password);

    ret = mqtt_connect( mqtt_connection, &conninfo );
    if ( ret != ZOS_SUCCESS )
    {
        ZOS_LOG("Error connecting");
    }
    else
    {
        /// now that we are successfully connected, subscribe to our topic
        snprintf(topic, MAX_TOPIC_STRING_SIZE,
                 "devices/%s/messages/devicebound/#", settings->device);
        zn_event_issue(mqtt_app_subscribe, NULL, 0); /// uses the topic string
    }
}

/*************************************************************************************************/
/*
 * Disconnect from broker (sent disconnect frame)
 */
void mqtt_app_disconnect( void *arg )
{
    ZOS_LOG("mqtt_app_disconnect");
    if ( mqtt_disconnect( mqtt_connection ) != ZOS_SUCCESS )
    {
        ZOS_LOG("Error disconnecting");
    }
}

/*************************************************************************************************/
/*
 * Subscribe to topic
 */
void mqtt_app_subscribe( void *arg )
{
    mqtt_msgid_t pktid;
    ZOS_LOG("Subscribing to topic '%s'", topic);
    pktid = mqtt_subscribe( mqtt_connection, (uint8_t*)topic, settings->qos );
    if ( pktid == 0 )
    {
        ZOS_LOG("Error subscribing: packet ID = 0");
    }
}

/*************************************************************************************************/
/*
 * Unsubscribe from topic
 */
void mqtt_app_unsubscribe( void *arg )
{
    mqtt_msgid_t pktid;
    ZOS_LOG("Unsubscribing from topic '%s'", topic);
    pktid = mqtt_unsubscribe( mqtt_connection, (uint8_t*)topic );

    if ( pktid == 0 )
    {
        ZOS_LOG("Error unsubscribing: packet ID = 0");
    }
}

/*************************************************************************************************/
/*
 * Publish (send) message to topic
 */
void mqtt_app_publish( void *arg )
{
    mqtt_msgid_t pktid;
    ZOS_LOG("Publishing to topic: '%s', message: '%s'", topic, message);
    pktid = mqtt_publish( mqtt_connection, (uint8_t*)topic, (uint8_t*)message, (uint32_t)strlen(message), settings->qos );

    if ( pktid == 0 )
    {
        ZOS_LOG("Error publishing: packet ID = 0");
    }
}

/******************************************************
 *               Static Function Definitions
 ******************************************************/
/*
 * Call back function to handle connection events.
 */
static zos_result_t mqtt_connection_event_cb( mqtt_event_info_t *event )
{
    switch ( event->type )
    {
        case MQTT_EVENT_TYPE_CONNECTED:
            ZOS_LOG("CONNECTED" );
            break;
        case MQTT_EVENT_TYPE_DISCONNECTED:
            ZOS_LOG("DISCONNECTED - issue event to connect" );
            zn_event_issue(mqtt_app_connect, NULL, 0);
            break;
        case MQTT_EVENT_TYPE_PUBLISHED:
            ZOS_LOG("MESSAGE PUBLISHED" );
            break;
        case MQTT_EVENT_TYPE_SUBCRIBED:
            ZOS_LOG("TOPIC SUBSCRIBED" );
            break;
        case MQTT_EVENT_TYPE_UNSUBSCRIBED:
            ZOS_LOG("TOPIC UNSUBSCRIBED" );
            break;
        case MQTT_EVENT_TYPE_PUBLISH_MSG_RECEIVED:
        {
            mqtt_topic_msg_t msg = event->data.pub_recvd;
            ZOS_LOG("MESSAGE RECEIVED");

            ZOS_LOG("----------------------------");
            ZOS_LOG("Topic  : %.*s", msg.topic_len, msg.topic );
            ZOS_LOG("Message: %.*s", msg.data_len, msg.data);
            ZOS_LOG("----------------------------");

            parse_received_request((char *) msg.data, msg.data_len);
        }
            break;
        default:
            ZOS_LOG("recevied unknown connection event - WHAT EVENT TYPE IS %d?", event->type);
            break;
    }
    return ZOS_SUCCESS;
}
