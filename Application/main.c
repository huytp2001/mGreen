/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Task includes */
#include "HAL_BSP.h"
#include "config.h"
#include "board_config.h"
#include "rtc.h"
#include "act_log.h"
#include "wdt.h"
#include "error_handler.h"
#include "sd_app.h"
#include "can_task.h"
#include "sensor.h"
#include "relay_control.h"
#include "params.h"
#include "gsm_resp_task.h"
#include "mqtt_task.h"
#include "setting.h"
#include "digital_input.h"
#include "rf.h"
#include "ports_control.h"
#include "schedule_task.h"
#include "irrigation.h"
//*****************************************************************************
//
// Global variable to keep track of the system clock.
//
//*****************************************************************************
uint32_t g_ui32_sys_clock = 0;

/*
 * Set up the hardware ready to run this application.
 */
static void prvSetupHardware( void );
static void vMainTaskInit(void);
/*-----------------------------------------------------------*/

int main( void )
{
	prvSetupHardware();
	
	/* Inital all threads */
	vMainTaskInit();
	
	/*Load necessary params from EEPROM */
	v_params_load();
	/* Init Sdcard */
	v_sdcard_init();
	
//	IntMasterEnable();
  /* Start the tasks and timer running. */
  vTaskStartScheduler();

	return 0;
}

static void prvSetupHardware( void )
{
	//Enable FPU
	FPULazyStackingEnable();
	FPUEnable();
	//
	// Run from the PLL at 120 MHz.
	//
	g_ui32_sys_clock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_16MHZ |
																					 SYSCTL_OSC_MAIN |
																					 SYSCTL_USE_PLL |
																					 SYSCTL_CFG_VCO_480), 120000000);
	g_ui32_sys_clock = SYSTEM_CLOCK_GET();
	
	IntMasterDisable();
	SysTickEnable();
	SysTickPeriodSet(g_ui32_sys_clock / 1000);	//1ms System Tick
	// System Tick Interrupt Handler is register in startup file	
	SysTickIntEnable();
	
	v_HAL_MSP_Init();
	
	ROM_SysCtlDelay((SYSTEM_CLOCK_GET() / 3) * 2);   //2000ms
	v_rtc_init();
}
/*-----------------------------------------------------------*/

uint32_t usrSysCtlClockGet( void )
{
	return g_ui32_sys_clock;
	
}

static void vMainTaskInit(void)
{
	#if WDT_ENABLE
	v_wdt_task_init();
	#endif
	
	#if PARAMS_ENABLE
	s32_params_init();
	#endif
	
	#if CAN_ENABLE
	v_can_task_init();
	#endif
	
	#if ERROR_HANDLER_ENABLE
	v_error_handler_task_init();
	#endif
	
	#if SENSOR_ENABLE
	v_sensor_task_init();
	#endif
	
	#if RELAY_ENABLE
	v_relay_task_init();
	#endif

	#if MQTT_ENABLE
	v_gsm_resp_task_init();
	#endif

	#if MQTT_ENABLE
	v_mqtt_task_init();
	#endif
	
	#if DIGITAL_INPUT_EN
	v_digital_input_task_init();
	#endif
	
	#if RF_ENABLE
	v_rf_task_init();
	#endif
	
	#if PORT_CONTROL_ENABLE
	v_ports_control_task_init();
	#endif
	
	#if SCHEDULE_TASK_ENALBE
	v_schedule_task_init();
	#endif
	
	#if IRRIGATION_TASK_ENABLE
	v_irrigation_manager_taskinit();
	#endif
	
	#if MANUAL_TASK_ENABLE
	
	#endif
}

/*-----------------------------------------------------------*/
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected.  This function is
	provided as an example only as stack overflow checking does not function
	when running the FreeRTOS Windows port. */
	vAssertCalled( __LINE__, __FILE__ );
}
/*-----------------------------------------------------------*/

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
	/* Called if an assertion passed to configASSERT() fails.  See
	http://www.freertos.org/a00110.html#configASSERT for more information. */

	/* Parameters are not used. */
	( void ) ulLine;
	( void ) pcFileName;

		/* Write detail info to action log */
	#if WRITE_ACTION_LOG
		b_write_action_log("Aseerted %s line: %d\r\n", pcFileName, ulLine);
	#endif
	taskDISABLE_INTERRUPTS(); 
	for( ;; ); 
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
