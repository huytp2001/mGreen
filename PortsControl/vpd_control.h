#ifndef _VPD_CONTROL_H_
#define _VPD_CONTROL_H_

#include <stdint.h>

#define VPD_CONTROL_FAN_PORT		3
#define VPD_CONTROL_PUMP_PORT		4

#define VPD_NODE_ID							254
typedef enum
{
	VPD_IDLE = 0,
	VPD_RUN,
	VPD_REQUEST_STOP,
	VPD_STOP,
}E_VPD_STATE;

void v_vpd_process_init(void);
void v_update_vpd_value(uint32_t u32_node_ID, double d_T, double d_RH);
void v_vpd_process(uint64_t *u64_port_control_value);
void v_vpd_set_node_id(uint32_t u32_node_id);
void v_vpd_set_start_time(uint32_t u32_start_time);
void v_vpd_set_stop_time(uint32_t u32_stop_time);
void v_vpd_set_min_temperature(uint32_t u32_min_t);
void v_vpd_set_max_temperature(uint32_t u32_max_t);
void v_vpd_publish_params(void);

#endif /* _VPD_CONTROL_H_ */
