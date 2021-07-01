/*=============================================================================
 * Copyright (c) 2021, Franco Bucafusco <franco_bucafusco@yahoo.com.ar>
 * 					   Martin N. Menendez <mmenendez@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2021/10/03
 * Version: v1.2
 *===========================================================================*/

/*==================[ Inclusions ]============================================*/
#include "keys.h"
#include "whackamole.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"
#include "queue.h"

/*=====[ Definitions of private data types ]===================================*/
/*=====[Definition macros of private constants]==============================*/
#define DEBOUNCE_TIME   40
#define DEBOUNCE_TIME_MS pdMS_TO_TICKS(DEBOUNCE_TIME)
#define START_TIME   500
#define START_TIME_MS pdMS_TO_TICKS(START_TIME)
#define MOLE 1
/*=====[Prototypes (declarations) of private functions]======================*/

static void keys_ButtonError(uint32_t index);
static void buttonPressed(uint32_t index);
static void buttonReleased(uint32_t index);

/*=====[Definitions of private global variables]=============================*/

/*=====[Definitions of public global variables]==============================*/

t_key_data keys_data[KEY_COUNT];
t_key_config keys_config[] = { TEC1, TEC2, TEC3, TEC4 };
TickType_t GameTime;
extern mole_t mole[KEY_COUNT];
extern TaskHandle_t taskMole[KEY_COUNT];
extern TaskHandle_t WhakamoleServiceLogic;
extern QueueHandle_t QueueIndex;

/*=====[prototype of private functions]=================================*/
void task_tecla(void* taskParmPtr);

/*=====[Implementations of public functions]=================================*/
TickType_t get_diff(uint32_t index) {
	TickType_t tiempo;

	taskENTER_CRITICAL();
	tiempo = keys_data[index].time_diff;
	taskEXIT_CRITICAL();

	return tiempo;
}

void clear_diff(uint32_t index) {
	taskENTER_CRITICAL();
	keys_data[index].time_diff = KEYS_INVALID_TIME;
	taskEXIT_CRITICAL();
}

void keys_Init(void) {
	BaseType_t res;
	uint32_t i;

	for (i = 0; i < KEY_COUNT; i++) {
		keys_data[i].state = BUTTON_UP;  // Set initial state
		keys_data[i].time_down = KEYS_INVALID_TIME;
		keys_data[i].time_up = KEYS_INVALID_TIME;
		keys_data[i].time_diff = KEYS_INVALID_TIME;
	}
	// Crear tareas en freeRTOS
	res = xTaskCreate(task_tecla,			// Funcion de la tarea a ejecutar
			(const char *) "task_tecla",// Nombre de la tarea como String amigable para el usuario
			configMINIMAL_STACK_SIZE * 2,	// Cantidad de stack de la tarea
			0,							// Parametros de tarea
			tskIDLE_PRIORITY + 1,			// Prioridad de la tarea
			0						// Puntero a la tarea creada en el sistema
			);

	// Gestión de errores
	configASSERT(res == pdPASS);
}

// keys_ Update State Function
void keys_Update(uint32_t index) {
	switch (keys_data[index].state) {
	case STATE_BUTTON_UP:
		/* CHECK TRANSITION CONDITIONS */
		if (!gpioRead(keys_config[index].tecla)) {
			keys_data[index].state = STATE_BUTTON_FALLING;
		}
		break;

	case STATE_BUTTON_FALLING:
		/* ENTRY */

		/* CHECK TRANSITION CONDITIONS */
		if (!gpioRead(keys_config[index].tecla)) {
			keys_data[index].state = STATE_BUTTON_DOWN;

			/* ACCION DEL EVENTO !*/
			buttonPressed(index);
		} else {
			keys_data[index].state = STATE_BUTTON_UP;
		}

		/* LEAVE */
		break;

	case STATE_BUTTON_DOWN:
		/* CHECK TRANSITION CONDITIONS */
		if (gpioRead(keys_config[index].tecla)) {
			keys_data[index].state = STATE_BUTTON_RISING;
		}
		break;

	case STATE_BUTTON_RISING:
		/* ENTRY */

		/* CHECK TRANSITION CONDITIONS */

		if (gpioRead(keys_config[index].tecla)) {
			keys_data[index].state = STATE_BUTTON_UP;

			/* ACCION DEL EVENTO ! */
			buttonReleased(index);
		} else {
			keys_data[index].state = STATE_BUTTON_DOWN;
		}

		/* LEAVE */
		break;

	default:
		keys_ButtonError(index);
		break;
	}
}

/*=====[Implementations of private functions]================================*/

/* accion de el evento de tecla pulsada */
static void buttonPressed(uint32_t index) {
	TickType_t current_tick_count = xTaskGetTickCount();
	TickType_t tiempo_pulsado;
	taskENTER_CRITICAL();
	keys_data[index].time_down = current_tick_count;
	taskEXIT_CRITICAL();
#if MOLE == 1
	xQueueSend(QueueIndex, &index, 0);
	xQueueSend(mole[index].QueueButton, &index, 0);
	xQueueSend(mole[index].QueueTimeHammer, &(keys_data[index].time_down), 0);
#endif

}

/* accion de el evento de tecla liberada */
static void buttonReleased(uint32_t index) {
	TickType_t current_tick_count = xTaskGetTickCount();
	taskENTER_CRITICAL();
	keys_data[index].time_up = current_tick_count;
	keys_data[index].time_diff = keys_data[index].time_up - keys_data[index].time_down;
	taskEXIT_CRITICAL();
#if MOLE == 1
		if (keys_data[index].time_diff > START_TIME_MS) {
			// El juego inicia. Las tareas de los topos, principal y print son lanzadas
			printf("Go !! \r\n");
			//guardo la cuenta de ticks en una variable global para utilizar en el conteo total del tiempo de juego
			taskENTER_CRITICAL();
			GameTime = xTaskGetTickCount();
			taskEXIT_CRITICAL();
			vTaskResume(WhakamoleServiceLogic);
			for (uint8_t i = 0; i < KEY_COUNT; i++) {
				vTaskResume(taskMole[i]);
			}
		}
#endif
}

static void keys_ButtonError(uint32_t index) {
	taskENTER_CRITICAL();
	keys_data[index].state = BUTTON_UP;
	taskEXIT_CRITICAL();
}

/*=====[Implementations of private functions]=================================*/
void task_tecla(void* taskParmPtr) {
	uint32_t i;
	while ( TRUE) {
		for (i = 0; i < KEY_COUNT; i++)
			keys_Update(i);
		vTaskDelay( DEBOUNCE_TIME_MS);
	}

}
