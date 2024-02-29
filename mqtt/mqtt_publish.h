/****************************************************************************
 * @Copyright Mimosa Technology 2020                                    		*
 *                                                                          *
 * This file is part of Rata machine.                                       *
 *                                                                          *
 ****************************************************************************/
 /**
 * @file mqtt_publish.h
 * @author Danh Pham
 * @date 25 Nov 2020
 * @version: 1.0.0
 * @brief Contains parameters used to send data to server
 */
#ifndef MQTT_PUBLISH_H_
#define MQTT_PUBLISH_H_
#include <stdint.h>
#include <stdbool.h>
#include "frame_parser.h"
#include "mqtt_task.h"
#include "threshold.h"
/*!
* @public functions prototype
*/
void v_mqtt_pub_hello(uint32_t u32_hw_version, uint32_t u32_fw_version,
											mqtt_connect_code_t e_mqtt_connect_code);
void v_mqtt_pub_keep_alive(uint32_t u32_value);
void v_mqtt_fertigation_start(uint16_t u16_location_id);
void v_mqtt_fertigation_info_pub(uint16_t u16_location_id, uint32_t *pu32_rate,
													double *pd_fertilizer_volume, double d_water_volume);
void v_mqtt_ph_statistic_pub(uint16_t u16_location_id, 
											uint16_t u16_min_ph, uint16_t u16_mean_ph, uint16_t u16_max_ph);
void v_mqtt_ec_statistic_pub(uint16_t u16_location_id,
										uint16_t u16_min_ec, uint16_t u16_mean_ec, uint16_t u16_max_ec);
void v_mqtt_ec_calculated_pub(uint16_t u16_location_id,
										uint16_t u16_calculated_ec);
void v_mqtt_fertigation_finish_pub(uint16_t u16_location_id, uint32_t u32_time_start);
void v_mqtt_phec_pub(uint16_t u16_location_id, uint16_t u16_ph, uint16_t u16_ec);
void v_mqtt_fw_version_pub(uint32_t u32_fw_version);
void v_mqtt_schedule_version_pub(char *c_csv_ver);
void v_mqtt_noti_pub(uint16_t u16_noti_id, E_NOTI_TYPE e_type_noti);
void v_mqtt_fertigation_state(uint8_t u8_state);
void v_mqtt_pub_feedback(mqtt_data_frame_t* pstru_mqtt_data_frame, uint32_t u32_value);
void v_mqtt_feeback_setting(mqtt_data_frame_t* pstru_mqtt_data_frame, bool b_successed);
void v_mqtt_feedback_ftp(E_DOWNLOAD_FILE_TYPE e_file_type,
							E_FTP_FEEDBACK_STATUS e_feedback_status, const char *c_filename);
void v_mqtt_data_pub(uint16_t u16_data_id, uint32_t u32_value);
void v_mqtt_fertilizer_rate_pub(uint16_t u16_location_id, uint32_t *pu32_rate);
void v_mqtt_fertilizer_volume_pub(uint16_t u16_location_id, double *pd_volume);
void v_mqtt_water_volume_pub(uint16_t u16_location_id, double d_volume);
void v_mqtt_threshold_event_pub(STRU_THD stru_thd_data);
void v_vpd_data_pub(uint32_t u32_node_id, uint16_t u16_data_id, uint32_t u32_value);
void v_mqtt_thd_info_pub(STRU_THD stru_thd_data);
void v_mqtt_ports_state(uint64_t u64_ports_state);
void v_mqtt_debug_rate_pub(uint8_t u8_index, uint32_t *u32_volume);

#endif /* MQTT_PUBLISH_H_ */
