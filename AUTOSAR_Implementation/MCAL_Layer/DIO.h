/*
 * DIO.h
 *
 * Author: abdallah mohamed
 * Target: STM32F401RCT6 (STM32CubeMX HAL)
 */

#ifndef MCAL_LAYER_DIO_H_
#define MCAL_LAYER_DIO_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Standardized Level Definitions */
#define DIO_LOW  0x00
#define DIO_HIGH 0x01

#define DIO_Door GPIO_PIN_1
#define LED_ID   GPIO_PIN_0

/* Default Port Used */
#define GPIO_Port_Used GPIOA

/* Function Prototypes */
uint8_t Dio_ReadChannel(uint16_t ID);
void    Dio_WriteChannel(uint16_t ID, uint8_t Level);

#endif /* MCAL_LAYER_DIO_H_ */
