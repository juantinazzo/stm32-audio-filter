/*
 * filters.c
 *
 *  Created on: Nov 8, 2023
 *      Author: juant
 */

#include "filters.h"
#include "filter_coefficients.h"


void PB_Init(Button_TypeDef Button)
{
  GPIO_InitTypeDef GPIO_InitStruct;


  __HAL_RCC_GPIOA_CLK_ENABLE();


    GPIO_InitStruct.Pin = KEY_BUTTON_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(KEY_BUTTON_GPIO_PORT, &GPIO_InitStruct);

}

/**
  * @brief  Returns the selected Button state.
  * @param  Button: Specifies the Button to be checked.
  *   This parameter should be: BUTTON_KEY
  * @retval The Button GPIO pin value.
  */
uint32_t PB_GetState(Button_TypeDef Button)
{
  return HAL_GPIO_ReadPin(KEY_BUTTON_GPIO_PORT, KEY_BUTTON_PIN);
}


arm_fir_instance_f32 filter_lowpass;

//fir state size is always number_of_samples + number_of_fir_tabs - 1
float32_t fir_l_state[BLOCK_SIZE_FLOAT + FILTER_TAP_NUM - 1];

void filters_init()
{
	PB_Init(0);

    arm_fir_init_f32(&filter_lowpass, FILTER_TAP_NUM, &low_pass_coef[0],
            &fir_l_state[0], BLOCK_SIZE_FLOAT);
}

/**
 * Callback que ocurre cuando se tiene un bloque de audio listo para procesar
 */
void rx_tx_data_process_float_cb(float32_t *signal_in, float32_t *signal_out,
        int16_t block_size)
{
    if (PB_GetState(0)) arm_fir_f32(&filter_lowpass, signal_in, signal_out, block_size);
    else {
        for (int i = 0; i < block_size; i++)
        {
        	signal_out[i] = signal_in[i];
        }
    }
}

