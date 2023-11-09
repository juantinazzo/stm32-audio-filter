/*
 * rx_tx_manager.c
 *
 *  Created on: Nov 8, 2023
 *      Author: juant
 */

#include "rx_tx_manager.h"
#include "stdbool.h"
#include "cs43l22.h"
#include "pdm2pcm.h"

#define AUDIO_I2C_ADDRESS 0x94

static I2S_HandleTypeDef *hi2s2;
static I2S_HandleTypeDef *hi2s3;

uint16_t txBuf[AUDIO_BUFFER_SIZE];
uint16_t pdmRxBuf[PDM_BUF_SIZE];
int16_t signal_out[AUDIO_BUFFER_SIZE / 8];
int16_t signal_in[AUDIO_BUFFER_SIZE / 8];
float32_t signal_in_float[BLOCK_SIZE_FLOAT];
float32_t signal_out_float[BLOCK_SIZE_FLOAT];
uint16_t MidBuffer[PDM_BUF_SIZE / 8];
uint8_t txstate = 0;
uint8_t rxstate = 0;

#define FIFO_BUFFFER_SIZE AUDIO_BUFFER_SIZE * 2
uint16_t fifobuf[FIFO_BUFFFER_SIZE];
uint16_t fifo_w_ptr = 0;
uint16_t fifo_r_ptr = 0;
uint8_t fifo_read_enabled = 0;
uint8_t new_data_available = 0;

void rx_tx_init(I2S_HandleTypeDef *hi2s2_handle,
        I2S_HandleTypeDef *hi2s3_handle)
{
    hi2s2 = hi2s2_handle;
    hi2s3 = hi2s3_handle;
    /* Initialize the audio driver structure */
    cs43l22_Init(AUDIO_I2C_ADDRESS, OUTPUT_DEVICE_HEADPHONE, 100,
    I2S_AUDIOFREQ_48K);
    cs43l22_Play(AUDIO_I2C_ADDRESS, NULL, 0);
    HAL_I2S_Transmit_DMA(hi2s3, &txBuf[0], AUDIO_BUFFER_SIZE / 2);
    HAL_I2S_Receive_DMA(hi2s2, &pdmRxBuf[0], PDM_BUF_SIZE / 2);

}

static void FifoWrite(uint16_t data)
{
    fifobuf[fifo_w_ptr] = data;
    fifo_w_ptr++;
    if (fifo_w_ptr >= FIFO_BUFFFER_SIZE) fifo_w_ptr = 0;
}

static uint16_t FifoRead()
{
    uint16_t val = fifobuf[fifo_r_ptr];
    fifo_r_ptr++;
    if (fifo_r_ptr >= FIFO_BUFFFER_SIZE) fifo_r_ptr = 0;
    return val;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == I2S3)
    {
        txstate = 1;
    }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == I2S3)
    {
        txstate = 2;
    }
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    rxstate = 1;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    rxstate = 2;
}

static void resolve_rx_events()
{
    if (rxstate == 1)
    {
        PDM_Filter(&pdmRxBuf[0], &MidBuffer[0], &PDM1_filter_handler);
        for (int i = 0; i < PDM_BUF_SIZE / 8; i++)
        {
            FifoWrite(MidBuffer[i]);
        }
        if (fifo_w_ptr - fifo_r_ptr > AUDIO_BUFFER_SIZE) fifo_read_enabled = 1;
        rxstate = 0;

    }

    if (rxstate == 2)
    {
        PDM_Filter(&pdmRxBuf[PDM_BUF_SIZE / 2], &MidBuffer[0],
                &PDM1_filter_handler);
        for (int i = 0; i < PDM_BUF_SIZE / 8; i++)
        {
            FifoWrite(MidBuffer[i]);
        }
        rxstate = 0;

    }

}

static void resolve_tx_events()
{
    if (txstate == 1)
    {

        for (int i = 0, j = 0; i < AUDIO_BUFFER_SIZE / 2; i += 4, j++)
        {

#ifdef USE_FLOAT_BUFFERS
            signal_out[j] = (((int) signal_out_float[j])) & 0xFFFF;
#endif
            uint16_t data = signal_out[j];
            txBuf[i] = data;
            txBuf[i + 2] = data;
        }

        txstate = 0;
    }

    if (txstate == 2)
    {

        for (int i = AUDIO_BUFFER_SIZE / 2, j = 0; i < AUDIO_BUFFER_SIZE; i +=
                4, j++)
        {
#ifdef USE_FLOAT_BUFFERS
            signal_out[j] = (((int) signal_out_float[j])) & 0xFFFF;
#endif
            uint16_t data = signal_out[j];
            txBuf[i] = data;
            txBuf[i + 2] = data;
        }

        txstate = 0;
    }

}

static bool tx_events_ready()
{
    return (txstate == 1 || txstate == 2);
}

static bool get_signal_in_from_fifo()
{
    if (fifo_read_enabled)
    {
        for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; i++)
        {
            signal_in[i] = FifoRead();
#ifdef USE_FLOAT_BUFFERS
            signal_in_float[i] = (float) ((int) (signal_in[i]));
#endif
        }
        return true;
    }
    else return false;

}

static void clear_tx_event_flag()
{
    txstate = 0;
}

void rx_tx_process()
{
    resolve_rx_events();

    if (tx_events_ready())
    {
        if (get_signal_in_from_fifo())
        {
            //Processing here
#if BYPASS_FILTERS
#if USE_FLOAT_BUFFERS
            for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; i++)
            {
                signal_out_float[i] = signal_in_float[i];
            }
#else
            for (int i = 0; i < AUDIO_BUFFER_SIZE / 8; i++)
            {
                signal_out[i] = signal_in[i];
            }
#endif
#else
            //process FIR
            //
#if USE_FLOAT_BUFFERS
            rx_tx_data_process_float_cb(&signal_in_float[0],
                    &signal_out_float[0], BLOCK_SIZE_FLOAT);
#else
            rx_tx_data_process_int16_cb(&signal_in[0], &signal_out[0],BLOCK_SIZE_FLOAT);
#endif

#endif

            resolve_tx_events();
        }
        else
        {
            clear_tx_event_flag();
        }
    }
}

__weak void rx_tx_data_process_int16_cb(int16_t *signal_in, int16_t *signal_out,
        int16_t block_size)
{
    for (int i = 0; i < block_size; i++)
    {
        signal_out[i] = signal_in[i];
    }
}

__weak void rx_tx_data_process_float_cb(float32_t *signal_in,
        float32_t *signal_out, int16_t block_size)
{
    for (int i = 0; i < block_size; i++)
    {
        signal_out[i] = signal_in[i];
    }
}
