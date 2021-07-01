/*
 * whackamole.h
 *
 *  Created on: 19 jun. 2021
 *      Author: nahuu
 */

#ifndef LIBS_SAPI_SAPI_V0_5_2_SOC_PERIPHERALS_INC_WHACKAMOLE_H_
#define LIBS_SAPI_SAPI_V0_5_2_SOC_PERIPHERALS_INC_WHACKAMOLE_H_

#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "queue.h"

#define KEY_COUNT  4

void whackamole_init();

typedef struct
{
	TickType_t timeRising;
	TickType_t timeFalling;
	gpioMap_t led;
	QueueHandle_t QueueButton;
	QueueHandle_t QueueButtonMole;
	QueueHandle_t QueueTimeHammer;
} mole_t;


#endif /* LIBS_SAPI_SAPI_V0_5_2_SOC_PERIPHERALS_INC_WHACKAMOLE_H_ */
