/*
 * DIO.c
 *
 * Author: abdallah mohamed
 * Target: STM32F401RCT6 (STM32CubeMX HAL)
 */

#include "DIO.h"

uint8_t Dio_ReadChannel(uint16_t ID)
{
    /* Read from the HAL library */
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(GPIO_Port_Used, ID);

    /* Convert HAL enum to standard integer level */
    if (pin_state == GPIO_PIN_SET)
    {
        return DIO_HIGH;
    }
    else
    {
        return DIO_LOW;
    }
}

void Dio_WriteChannel(uint16_t ID, uint8_t Level)
{
    /* Convert standard integer level to HAL enum */
    GPIO_PinState pin_state = (Level == DIO_HIGH) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    /* Write to the HAL library */
    HAL_GPIO_WritePin(GPIO_Port_Used, ID, pin_state);
}
