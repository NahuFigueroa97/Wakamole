/*
 * random.h
 *
 *  Created on: 19 jun. 2021
 *      Author: nahuu
 */

#ifndef LIBS_SAPI_SAPI_V0_5_2_SOC_PERIPHERALS_INC_RANDOM_H_
#define LIBS_SAPI_SAPI_V0_5_2_SOC_PERIPHERALS_INC_RANDOM_H_

#include "stdint.h"

void random_set_seed( uint32_t semilla );
uint32_t random( int32_t min, int32_t max );
void random_seed_freertos();




#endif /* LIBS_SAPI_SAPI_V0_5_2_SOC_PERIPHERALS_INC_RANDOM_H_ */
