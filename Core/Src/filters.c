/*
 * filters.c
 *
 *  Created on: Nov 8, 2023
 *      Author: juant
 */

#include "filters.h"
#include "filter_coefficients.h"

arm_fir_instance_f32 filter_lowpass;

//fir state size is always number_of_samples + number_of_fir_tabs - 1
float32_t fir_l_state[BLOCK_SIZE_FLOAT + FILTER_TAP_NUM - 1];

void filters_init()
{
    arm_fir_init_f32(&filter_lowpass, FILTER_TAP_NUM, &filter_taps[0],
            &fir_l_state[0], BLOCK_SIZE_FLOAT);
}

/**
 * Callback que ocurre cuando se tiene un bloque de audio listo para procesar
 */
void rx_tx_data_process_float_cb(float32_t *signal_in, float32_t *signal_out,
        int16_t block_size)
{
    arm_fir_f32(&filter_lowpass, signal_in, signal_out, block_size);
}

