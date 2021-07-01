/*=============================================================================
 * Copyright (c) 2021, Franco Bucafusco <franco_bucafusco@yahoo.com.ar>
 * 					   Martin N. Menendez <mmenendez@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Version: v1.0
 *===========================================================================*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "queue.h"
#include "keys.h"
#include "whackamole.h"

#define USED_UART UART_USB
#define UART_RATE 9600

/*==================[definiciones de datos externos]=========================*/
DEBUG_PRINT_ENABLE;

int main(void) {
	/* Inicializar la placa */
	boardConfig();
	debugPrintConfigUart( USED_UART , UART_RATE );	// UART for debug messages
	printf("Initiating Game\n");

	/* inicializo driver de teclas */
	keys_Init();

	/* inicializo drive de moles */
	whackamole_init();

	/* arranco el scheduler */
	vTaskStartScheduler();

	// ---------- REPETIR POR SIEMPRE --------------------------
	configASSERT(0);

	// NO DEBE LLEGAR NUNCA AQUI, debido a que a este programa se ejecuta
	// directamenteno sobre un microcontroladore y no es llamado por ningun
	// Sistema Operativo, como en el caso de un programa para PC.

	return TRUE;

//    return 0;
}

