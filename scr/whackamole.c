#include <stdarg.h>
#include <stdbool.h>
#include "whackamole.h"
#include "random.h"
#include "sapi.h"
#include "keys.h"
#include "queue.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"


#define WAM_GAMEPLAY_TIMEOUT        20000   //gameplay time
#define WAM_MOLE_SHOW_MAX_TIME      6000
#define WAM_MOLE_OUTSIDE_MAX_TIME   2000
#define WAM_MOLE_SHOW_MIN_TIME      1000
#define WAM_MOLE_OUTSIDE_MIN_TIME   500
#define nQueue   1
#define END_GAME "END GAME! !\r\n"
#define SCORE "SCORE: %d\r\n"
#define TOTAL_POINTS "TOTAL POINTS: %d\r\n"
#define LEDINIT 0
#define LEDEND  3

gpioMap_t leds[] = { LEDB, LED1, LED2, LED3 };
TaskHandle_t taskMole[KEY_COUNT];
TaskHandle_t WhakamoleServiceLogic;
mole_t mole[KEY_COUNT];
QueueHandle_t QueueIndex, QueuePoint;
extern TickType_t GameTime;

/* prototypes */
void mole_service_logic(void* pvParameters);
void whackamole_service_logic(void* pvParameters);

/**
 @brief init game

 */
void whackamole_init() {
	BaseType_t res;

	res = xTaskCreate(whackamole_service_logic,
			(const char*) "whackamole_service_logic",
			configMINIMAL_STACK_SIZE * 2, 0, tskIDLE_PRIORITY + 1,
			&WhakamoleServiceLogic);

	configASSERT(res == pdPASS);

	vTaskSuspend(WhakamoleServiceLogic);

	for (uint8_t i = 0; i < KEY_COUNT; i++) {

		res = xTaskCreate(mole_service_logic,
				(const char*) "mole_service_logic",
				configMINIMAL_STACK_SIZE * 2, 0, tskIDLE_PRIORITY + 1,
				&taskMole[i]);

		configASSERT(res == pdPASS);

		mole[i].led = leds[i];

		mole[i].QueueButton = xQueueCreate(nQueue, sizeof(uint8_t));

		configASSERT(mole[i].QueueButton != NULL);

		mole[i].QueueButtonMole = xQueueCreate(nQueue, sizeof(uint8_t));

		configASSERT(mole[i].QueueButtonMole != NULL);

		mole[i].QueueTimeHammer = xQueueCreate(nQueue, sizeof(TickType_t));

		configASSERT(mole[i].QueueTimeHammer != NULL);

		vTaskSuspend(taskMole[i]);
	}

	QueuePoint = xQueueCreate(nQueue, sizeof(int32_t));

	configASSERT(QueuePoint != NULL);

	QueueIndex = xQueueCreate(nQueue, sizeof(int32_t));

	configASSERT(QueueIndex != NULL);

}

/**
 @brief devuelve el puntaje de haber martillado al mole

 @param tiempo_afuera             tiempo q hubiera estado el mole esperando
 @param tiempo_reaccion_usuario   tiempo de reaccion del usuario en martillar
 @return uint32_t
 */
int32_t whackamole_points_success(TickType_t tiempo_afuera, TickType_t tiempo_reaccion_usuario) {
	return ((10 * (tiempo_afuera - tiempo_reaccion_usuario) / tiempo_afuera));
}

/**
 @brief devuelve el puntaje por haber perdido al mole

 @return uint32_t
 */
int32_t whackamole_points_miss() {
	return -10;
}

/**
 @brief devuelve el puntaje por haber martillado cuando no habia mole

 @return uint32_t
 */
int32_t whackamole_points_no_mole() {
	return -20;
}

/**
 @brief servicio principal del juego

 @param pvParameters
 */
void whackamole_service_logic(void* pvParameters) {
	gpioMap_t led;
	uint8_t hammer;
	int32_t point = 0, accumulator = 0, i;
	TickType_t presentTime = xTaskGetTickCount();

	while ((presentTime - GameTime) < pdMS_TO_TICKS(WAM_GAMEPLAY_TIMEOUT)) {

		if (xQueueReceive(QueueIndex, &i, 0) == pdTRUE) {

			if (xQueueReceive(mole[i].QueueButton, &hammer, 0) == pdTRUE) {

				xQueueSend(mole[i].QueueButtonMole, &hammer, 0);
			}
		}

		if (xQueueReceive(QueuePoint, &point, 0) == pdTRUE) {

			taskENTER_CRITICAL();

			printf(SCORE, point);

			accumulator = accumulator + point;

			taskEXIT_CRITICAL();

		}

		taskENTER_CRITICAL();
		presentTime = xTaskGetTickCount();
		taskEXIT_CRITICAL();

	}

	taskENTER_CRITICAL();
	printf(TOTAL_POINTS, accumulator);
	printf(END_GAME);
	taskEXIT_CRITICAL();

	for (uint8_t i = 0; i < KEY_COUNT; i++) {
		vTaskDelete(taskMole[i]);
		gpioWrite(LEDB + i, OFF);
	}

	vTaskDelete(WhakamoleServiceLogic);

}

/**
 @brief servicio instanciado de cada mole

 @param pvParameters
 */
void mole_service_logic(void* pvParameters) {
	uint8_t hammer, indexLed;
	int32_t point;
	TickType_t reactionTime, currentTime, diffTime;

	while (1) {
		random_seed_freertos();
		indexLed = random( LEDINIT, LEDEND);
		mole[indexLed].timeRising = random( WAM_MOLE_SHOW_MIN_TIME, WAM_MOLE_SHOW_MAX_TIME);
		mole[indexLed].timeFalling = random( WAM_MOLE_OUTSIDE_MIN_TIME, WAM_MOLE_OUTSIDE_MAX_TIME);

		gpioWrite(mole[indexLed].led, OFF);

		if ((xQueueReceive(mole[indexLed].QueueButtonMole, &hammer, mole[indexLed].timeFalling) == pdTRUE)) {
			taskENTER_CRITICAL();
			point = whackamole_points_no_mole();
			taskEXIT_CRITICAL();

		} else {

			gpioWrite(mole[indexLed].led, ON);

			if ((xQueueReceive(mole[indexLed].QueueButtonMole, &hammer, mole[indexLed].timeRising) == pdTRUE) ) {

				gpioWrite(mole[indexLed].led, OFF);
				taskENTER_CRITICAL();
				xQueueReceive(mole[indexLed].QueueTimeHammer, &reactionTime, 0);
				currentTime = xTaskGetTickCount();
				diffTime = reactionTime - currentTime;
				point = whackamole_points_success(mole[indexLed].timeRising, diffTime);
				taskEXIT_CRITICAL();
			}

			else {

				taskENTER_CRITICAL();
				point = whackamole_points_miss();
				taskEXIT_CRITICAL();
				gpioWrite(mole[indexLed].led, OFF);

			}

		}

		xQueueSend(QueuePoint, &point, 0);

	}

}

