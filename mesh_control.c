/** @file This file contains the code for sending/receiving data to the mesh
 *
 * Additionally, when data comes in from wifi, process it here
 *
 * Copyright Ambient Sensors 2017
 */

#include "zos.h"

#define MAX_CMD_LENGTH 16
#define ORDER_CMD_LENGTH 5
#define STD_BOARD_CMD_LENGTH 4
#define SERIAL_MESH_CMD 0x20

#define POLL_UART_MS 200
// how big do we want our receive buffer??
static uint8_t ring_buffer_data[1024];


static void uart_rx_data_handler(void *arg)
{
    const uint8_t *dummy;
    uint16_t bytes_read;
    uint8_t rx_buffer[256];

    // see if the UART had any data available
    zn_uart_peek_bytes(ZOS_UART_1, &dummy, &bytes_read);

    if (bytes_read > 0)
    {
        zn_uart_receive_bytes(ZOS_UART_1, rx_buffer, bytes_read, ZOS_NO_WAIT);
        /// uncomment the loop below if you really want to see all the stuff coming back
#if 0
        for (i=0; i<bytes_read; i++)
        {
            ZOS_LOG("We just read 0x%X", rx_buffer[i]);
        }
#endif
    }
}

int setup_serial_port(void)
{
    /// setup the UART
    const zos_uart_config_t config =
    {
        .baud_rate = 115200,
        .data_width = UART_WIDTH_8BIT,
        .parity = UART_NO_PARITY,
        .stop_bits = UART_STOP_BITS_1,
        .flow_control = UART_FLOW_CTS_RTS,
    };
    const zos_uart_buffer_t uart_buffer =
    {
        .buffer = ring_buffer_data,
        .length = sizeof(ring_buffer_data)
    };

    // do we need to send a rigado reset pulse here?
    ZOS_LOG("uart config returned 0x%X", zn_uart_configure(ZOS_UART_1, &config, &uart_buffer));
    /// register handler to periodically poll UART
    zn_event_register_periodic(uart_rx_data_handler, NULL, POLL_UART_MS, EVENT_FLAGS1(RUN_NOW));
    return 0;
}

int parse_received_request(char *buffer, size_t size)
{
    int cmd, board_num, parsed, order;
    char parm_type;
    char temp_array[100];
    uint8_t byte_array_send[MAX_CMD_LENGTH];

    // check for the "Order command type"
    parsed = sscanf(buffer, "C%d%c%s", &cmd, &parm_type, temp_array);
    if (parsed != 3)
    {
    	ZOS_LOG("ERROR, %s didn't do a basic parse", __func__);
    	return parsed;
    }
    ZOS_LOG("Sandra, cmd=0x%X, parm_type=%c, temp_array=%s", cmd, parm_type, temp_array);
    if (parm_type == 'O')
    {
    	// this is an order command - parse the parm as hex (without 0x in front)
    	parsed = sscanf(buffer, "C%dO%x", &cmd, &order);
    	if (parsed != 2)
    	{
    		ZOS_LOG("ERROR, %s didn't parse the order properly", __func__);
    		return parsed;
    	}
    	ZOS_LOG("cmd=%d, order=0x%X", cmd, order);
    	byte_array_send[0] = ORDER_CMD_LENGTH - 1; /// don't include this byte in length
    	byte_array_send[1] = SERIAL_MESH_CMD;
    	byte_array_send[2] = cmd;
    	byte_array_send[3] = (order >> 8) & 0xFF;
        ZOS_LOG("Just set byte 3 to 0x%X", byte_array_send[3]);
    	byte_array_send[4] = order & 0xFF;
        ZOS_LOG("Just set byte 4 to 0x%X", byte_array_send[4]);
    }
    else
    {
    	parsed = sscanf(buffer, "C%dB%d", &cmd, &board_num);
    	if (parsed != 2)
    	{
    		ZOS_LOG("ERROR, %s didn't parse the 2 commands", __func__);
    		return parsed;
    	}
    	ZOS_LOG("cmd=%d, board_num=%d", cmd, board_num);
        /// don't include this byte in length
    	byte_array_send[0] = STD_BOARD_CMD_LENGTH - 1;
    	byte_array_send[1] = SERIAL_MESH_CMD;
    	byte_array_send[2] = cmd;
    	byte_array_send[3] = board_num;
    }

    zn_uart_transmit_bytes(ZOS_UART_1, byte_array_send, byte_array_send[0] + 1);
    ZOS_LOG("Sent length of 0x%X", byte_array_send[0] + 1);
    /// do we want to add a thread or isr that will read data back from serial, and send back to azure?
    return 0;
}
