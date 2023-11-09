/*
 * rx_tx_manager.h
 *
 *  Created on: Nov 8, 2023
 *      Author: juant
 */

#ifndef INC_RX_TX_MANAGER_H_
#define INC_RX_TX_MANAGER_H_

#include "main.h"
#include "arm_math.h"

void rx_tx_process();

void rx_tx_init(I2S_HandleTypeDef *hi2s2_handle,
        I2S_HandleTypeDef *hi2s3_handle);

void rx_tx_data_process_int16_cb(int16_t *signal_in, int16_t *signal_out,
        int16_t block_size);

void rx_tx_data_process_float_cb(float32_t *signal_in, float32_t *signal_out,
        int16_t block_size);

#endif /* INC_RX_TX_MANAGER_H_ */
