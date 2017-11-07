/** @file This file contains the api for sending/receiving data to the mesh
 *
 * Copyright Ambient Sensors 2017
 */
#ifndef _MESH_CONTROL_H_
#define _MESH_CONTROL_H_

/** @brief simple setup of serial port for communicating to Nordic Mesh
 */
int setup_serial_port(void);

/** @brief Parse data that has come in from wifi, send to the mesh
 *
 *  When data is received from the wifi, figure out which commands
 *  in the mesh it pertains to, and format it for the mesh.  Then
 *  send it to the mesh.
 */
int parse_received_request(char *buffer, size_t size);


#endif
