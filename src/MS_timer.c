/*
 * MS_timer.c
 *
 * Created: 5/24/2023 10:55:50 AM
 *  Author: Takuya
 */ 

#include "MS_definitions.h"

void timerInit(void)
{
	#if defined(PYTHON480_ENABLE)
	// Setup a timer to count in milliseconds
	TIMER_0_task1.interval	= 1; // Need to check this value
	TIMER_0_task1.cb		= millisecondTimer_cb;
	TIMER_0_task1.mode		= TIMER_TASK_REPEAT;
	timer_add_task(&TIMER_0, &TIMER_0_task1);
	#endif
	
	#if defined(BATTERY_ENABLE) || defined(WPT_ADC_ENABLE)
	TIMER_0_task2.interval = 1000; // Units are in ms so 1000 should check every 1 second
	TIMER_0_task2.cb       = checkBattVoltage_cb;
	TIMER_0_task2.mode     = TIMER_TASK_REPEAT;
	timer_add_task(&TIMER_0, &TIMER_0_task2);
	#endif
	
	#if defined(PYTHON480_ENABLE) || defined(BATTERY_ENABLE) || defined(WPT_ADC_ENABLE)
	timer_start(&TIMER_0);
	#endif
}

uint32_t getCurrentTimeMS(void)
{
	return timeMS;
}


