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


/*************************************************
 * Default Settings
 *
 * These settings are stored in the 'Read-Only Memory'
 * This memory does NOT get loaded into RAM and resides
 * on the serial flash.
 *
 * To retrieve these settings, use the zn_load_ro_memory() API.
 */
RO_MEM mqtt_settings_t default_settings =
{
        .magic          = SETTINGS_MAGIC_NUMBER,
        .host           = MQTT_HOST,
        .port           = MQTT_PORT,
        .device         = MQTT_DEVICE_ID,
        .token_expiry   = MQTT_TOKEN_EXPIRY,
        .token_sig      = MQTT_TOKEN_SIG,
        .qos            = MQTT_QOS,
        .security       = MQTT_SECURITY,
        .keepalive      = MQTT_KEEPALIVE,
};

/*************************************************************************************************
 * Getters List
 *************************************************************************************************/
ZOS_GETTERS_START(mqtt)
    ZOS_ADD_GETTER("mqtt.host",         mqtt_host),
    ZOS_ADD_GETTER("mqtt.port",         mqtt_port),
    ZOS_ADD_GETTER("mqtt.device",       mqtt_device),
    ZOS_ADD_GETTER("mqtt.token_expiry", mqtt_token_expiry),
    ZOS_ADD_GETTER("mqtt.token_sig",    mqtt_token_sig),
    ZOS_ADD_GETTER("mqtt.qos",          mqtt_qos),
    ZOS_ADD_GETTER("mqtt.security",     mqtt_security),
    ZOS_ADD_GETTER("mqtt.keepalive",    mqtt_keepalive),
ZOS_GETTERS_END

/*************************************************************************************************
 * Setters List
 *************************************************************************************************/
ZOS_SETTERS_START(mqtt)
    ZOS_ADD_SETTER("mqtt.host",         mqtt_host),
    ZOS_ADD_SETTER("mqtt.port",         mqtt_port),
    ZOS_ADD_SETTER("mqtt.device",       mqtt_device),
    ZOS_ADD_SETTER("mqtt.token_expiry", mqtt_token_expiry),
    ZOS_ADD_SETTER("mqtt.token_sig",    mqtt_token_sig),
    ZOS_ADD_SETTER("mqtt.qos",          mqtt_qos),
    ZOS_ADD_SETTER("mqtt.security",     mqtt_security),
    ZOS_ADD_SETTER("mqtt.keepalive",    mqtt_keepalive),
ZOS_SETTERS_END

/*************************************************************************************************
 * Commands List
 *************************************************************************************************/
ZOS_COMMANDS_START(mqtt)
    ZOS_ADD_COMMAND("mqtt_connect", 0, 0, ZOS_FALSE, mqtt_connect),
    ZOS_ADD_COMMAND("mqtt_disconnect", 0, 0, ZOS_FALSE, mqtt_disconnect),
    ZOS_ADD_COMMAND("mqtt_publish", 2, 2, ZOS_FALSE, mqtt_publish),
    ZOS_ADD_COMMAND("mqtt_subscribe", 1, 1, ZOS_FALSE, mqtt_subscribe),
    ZOS_ADD_COMMAND("mqtt_unsubscribe", 1, 1, ZOS_FALSE, mqtt_unsubscribe)
    ZOS_ADD_COMMAND("cmd", 2, 2, ZOS_FALSE, send_a_command),

ZOS_COMMANDS_END

ZOS_COMMAND_LISTS(mqtt);

/*************************************************************************************************
 * API's
 *************************************************************************************************/
void commands_init(void)
{
    mqtt_settings_t *settings;

    ZOS_CMD_REGISTER_COMMANDS(mqtt);

    ZOS_NVM_GET_REF(settings);

    // check that app_settings_t hasn't overflowed the NVM
    // if the build fails here then app_settings_t is too large to fit into the NVM
    // you need to make this struct smaller
    BUILD_CHECK_NVM_SIZE(mqtt_settings_t);

    // if the nvm settings haven't been initialized, do it now
    if(settings->magic != SETTINGS_MAGIC_NUMBER)
    {
        if(zn_load_ro_memory(settings, sizeof(mqtt_settings_t), &default_settings, 0) != ZOS_SUCCESS)
        {
            ZOS_LOG("Failed to loaded default settings");
        }
    }
}

/*************************************************************************************************/
void commands_deinit(void)
{
    ZOS_CMD_UNREGISTER_COMMANDS(mqtt);
}

/*************************************************************************************************
 * Commands
 *************************************************************************************************/
ZOS_DEFINE_COMMAND(mqtt_connect)
{
    if((mqtt_connection == NULL) || (mqtt_connection->session_init != ZOS_TRUE))
    {
        ZOS_LOG("Not initialized..., mqtt_connection=0x%X session_init=0x%X", mqtt_connection, mqtt_connection->session_init);
        mqtt_init(mqtt_connection);
        if ((mqtt_connection == NULL) || (mqtt_connection->session_init != ZOS_TRUE))
        {
            ZOS_LOG("even a re-init wouldn't fix the struct - dying now");
        }
        else
        {
            zn_event_issue(mqtt_app_connect, NULL, 0);
        }
    }
    else if(mqtt_connection->net_init_ok == ZOS_TRUE)
    {
        ZOS_LOG("Already initialized. Do nothing...");
    }
    else
    {
        zn_event_issue(mqtt_app_connect, NULL, 0);
    }
    return CMD_EXECUTE_AOK;
}

/*************************************************************************************************/
ZOS_DEFINE_COMMAND(mqtt_disconnect)
{
    if((mqtt_connection == NULL) || (mqtt_connection->net_init_ok != ZOS_TRUE))
    {
        ZOS_LOG("Connection not opened. Cannot disconnect! but trying anyway");
        zn_event_issue(mqtt_app_disconnect, NULL, 0);
    }
    else
    {
        zn_event_issue(mqtt_app_disconnect, NULL, 0);
    }
    return CMD_EXECUTE_AOK;
}

/*************************************************************************************************/
ZOS_DEFINE_COMMAND(mqtt_publish)
{
    if((mqtt_connection == NULL) || (mqtt_connection->net_init_ok != ZOS_TRUE))
    {
        ZOS_LOG("Not connected. Connect first using command 'mqtt_connect'");
    }
    else if(strlen(argv[0]) > MAX_TOPIC_STRING_SIZE)
    {
        ZOS_LOG("Failed (maximum topic length is %u)", MAX_TOPIC_STRING_SIZE);
        return CMD_BAD_ARGS;
    }
    else if(strlen(argv[1]) > MAX_MESSAGE_STRING_SIZE)
    {
        ZOS_LOG("Failed (maximum message length is %u)", MAX_MESSAGE_STRING_SIZE);
        return CMD_BAD_ARGS;
    }
    else
    {
        strncpy(topic, argv[0], MAX_TOPIC_STRING_SIZE);
        strncpy(message, argv[1], MAX_MESSAGE_STRING_SIZE);
        zn_event_issue(mqtt_app_publish, NULL, 0);
    }
    return CMD_EXECUTE_AOK;
}

/*************************************************************************************************/
ZOS_DEFINE_COMMAND(mqtt_subscribe)
{
    if((mqtt_connection == NULL) || (mqtt_connection->net_init_ok != ZOS_TRUE))
    {
        ZOS_LOG("Not connected. Connect first using command 'mqtt_connect'");
    }
    else if(strlen(argv[0]) > MAX_TOPIC_STRING_SIZE)
    {
        ZOS_LOG("Failed (maximum topic length is %u)", MAX_TOPIC_STRING_SIZE);
        return CMD_BAD_ARGS;
    }
    else
    {
        strncpy(topic, argv[0], MAX_TOPIC_STRING_SIZE);
        zn_event_issue(mqtt_app_subscribe, NULL, 0);
    }
    return CMD_EXECUTE_AOK;
}

/*************************************************************************************************/
ZOS_DEFINE_COMMAND(mqtt_unsubscribe)
{
    if((mqtt_connection == NULL) || (mqtt_connection->net_init_ok != ZOS_TRUE))
    {
        ZOS_LOG("Not connected. Connect first using command 'mqtt_connect' - mqtt_connection = %d, net_init_ok = %d", mqtt_connection, mqtt_connection->net_init_ok);
    }
    else if(strlen(argv[0]) > MAX_TOPIC_STRING_SIZE)
    {
        ZOS_LOG("Failed (maximum topic length is %u)", MAX_TOPIC_STRING_SIZE);
        return CMD_BAD_ARGS;
    }
    else
    {
        strncpy(topic, argv[0], MAX_TOPIC_STRING_SIZE);
        zn_event_issue(mqtt_app_unsubscribe, NULL, 0);
    }
    return CMD_EXECUTE_AOK;
}

/*************************************************************************************************/
ZOS_DEFINE_COMMAND(send_a_command)
{
    char send_string[10] = "";
    char cmd;

    ZOS_LOG("Sandra, argc=%d, argv[0]=%s", argc, argv[0]);

    if (argv[0][0] == '-')
    {
        ZOS_LOG("usage: cmd <cmd> <board#> - 0=off, 1=on, 2=toggle, 3=sparkle, 4=dazzle");
        ZOS_LOG("   or: cmd 6 <ABCD> to set order of boards to ABCD (replace with board numbers");
        return CMD_EXECUTE_AOK;
    }

    cmd = argv[0][0];  // grab the 1st character of the cmd
    if (cmd == '6')
    {
        // This command is the "order" command - we send an "O" not a "B"
        sprintf(send_string, "C%sO%s", argv[0], argv[1]);
    }
    else
    {
        sprintf(send_string, "C%sB%s", argv[0], argv[1]);
    }
    ZOS_LOG("Sending %s to BLE client", send_string);
    parse_received_request(send_string, strlen(send_string));
    return CMD_EXECUTE_AOK;
}


/*************************************************************************************************
 * Getters
 *************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_host)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%s", settings->host);
    return CMD_SUCCESS;
}

/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_port)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%u", settings->port);
    return CMD_SUCCESS;
}

/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_keepalive)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%u", settings->keepalive);
    return CMD_SUCCESS;
}

/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_qos)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%u", settings->qos);
    return CMD_SUCCESS;
}

/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_security)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%u", settings->security);
    return CMD_SUCCESS;
}

/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_token_sig)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%s", settings->token_sig);
    return CMD_SUCCESS;
}


/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_token_expiry)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%s", settings->token_expiry);
    return CMD_SUCCESS;
}

/*************************************************************************************************/
ZOS_DEFINE_GETTER(mqtt_device)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    zn_cmd_format_response(CMD_SUCCESS, "%s", settings->device);
    return CMD_SUCCESS;
}


/*************************************************************************************************
 * Setters
 *************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_host)
{
    mqtt_settings_t *settings;

    if(strlen(argv[1]) > sizeof(settings->host))
    {
        ZOS_LOG("Failed (maximum length is %u)", sizeof(settings->host));
        return CMD_BAD_ARGS;
    }
    else
    {
        ZOS_NVM_GET_REF(settings);
        strcpy((char*)settings->host, argv[1]);
        return CMD_SET_OK;
    }
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_port)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    ZOS_CMD_PARSE_INT_ARG_WITH_VAR(uint16_t, settings->port, argv[1], 0, 65535);
    return CMD_SET_OK;
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_keepalive)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    ZOS_CMD_PARSE_INT_ARG_WITH_VAR(uint16_t, settings->keepalive, argv[1], 0, 65535);
    return CMD_SET_OK;
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_qos)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    ZOS_CMD_PARSE_INT_ARG_WITH_VAR(uint16_t, settings->qos, argv[1], 0, 2);
    return CMD_SET_OK;
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_security)
{
    mqtt_settings_t *settings;
    ZOS_NVM_GET_REF(settings);
    ZOS_CMD_PARSE_INT_ARG_WITH_VAR(uint16_t, settings->security, argv[1], 0, 1);
    return CMD_SET_OK;
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_token_sig)
{
    mqtt_settings_t *settings;

    if(strlen(argv[1]) > sizeof(settings->token_sig))
    {
        ZOS_LOG("Failed (maximum length is %u)", sizeof(settings->token_sig));
        return CMD_BAD_ARGS;
    }
    else
    {
        ZOS_NVM_GET_REF(settings);
        strcpy((char*)settings->token_sig, argv[1]);
        return CMD_SET_OK;
    }
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_token_expiry)
{
    mqtt_settings_t *settings;

    if(strlen(argv[1]) > sizeof(settings->token_expiry))
    {
        ZOS_LOG("Failed (maximum length is %u)", sizeof(settings->token_expiry));
        return CMD_BAD_ARGS;
    }
    else
    {
        ZOS_NVM_GET_REF(settings);
        strcpy((char*)settings->token_expiry, argv[1]);
        return CMD_SET_OK;
    }
}

/*************************************************************************************************/
ZOS_DEFINE_SETTER(mqtt_device)
{
    mqtt_settings_t *settings;

    if(strlen(argv[1]) > sizeof(settings->device))
    {
        ZOS_LOG("Failed (maximum length is %u)", sizeof(settings->device));
        return CMD_BAD_ARGS;
    }
    else
    {
        ZOS_NVM_GET_REF(settings);
        strcpy((char*)settings->device, argv[1]);
        return CMD_SET_OK;
    }
}
