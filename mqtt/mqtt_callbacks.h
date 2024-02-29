/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file mqtt_control_callbacks.h
 * @author Danh Pham
 * @date 01 Dec 2020
 * @version: 1.0.0
 * @brief This file contains prototype of control callback functions.
 */
 
 #ifndef MQTT_CONTROL_CALLBACKS_H_
 #define MQTT_CONTROL_CALLBACKS_H_
 
 #include "frame_parser.h"
 
 extern void mqtt_control_callback(mqtt_data_frame_t stru_mqtt_data_frame);
 #endif /* MQTT_CONTROL_CALLBACKS_H_ */

