/*
 * main.c
 *
 * Author: Abdallah Mohamed
 * Target: STM32F401RCT6 (STM32CubeMX HAL)
 *
 * Project: Door Lock Indication – AUTOSAR-style SWC Integration Test
 *
 * HW Setup:
 *   PA1 (DIO_Door / GPIO_PIN_1) → Door sensor input  (INPUT, PULL-UP)
 *                                   Sensor OPEN  = PA1 HIGH → DoorState = 1 → LED ON
 *                                   Sensor CLOSED= PA1 LOW  → DoorState = 0 → LED OFF
 *   PA0 (LED_ID   / GPIO_PIN_0) → LED output         (OUTPUT, PUSH-PULL)
 *                                   LEDON  = 0  (active-low)
 *                                   LEDOFF = 1
 *
 * Runnable scheduling:
 *   Both ReadDoorSensor_runnable() and DoorLock_indication_runnable()
 *   have a 10 ms period (Period = 0.01 s as declared in the ARXML).
 *   They are dispatched from the SysTick / HAL_GetTick() time base.
 *
 * Call order per cycle (matches AUTOSAR data-dependency):
 *   1. ReadDoorSensor_runnable()      – reads PA1, writes RTE buffer
 *   2. DoorLock_indication_runnable() – reads RTE buffer, calls Led_Switch
 */

/* ------------------------------------------------------------------ */
/*  HAL / BSP includes                                                  */
/* ------------------------------------------------------------------ */
#include "stm32f4xx_hal.h"

/* ------------------------------------------------------------------ */
/* Application-layer SWC headers & Runnables                           */
/* ------------------------------------------------------------------ */
#include "MCAL_Layer/DIO.h"

/* Declare runnables externally to avoid Rte_Instance struct conflicts */
extern void ReadDoorSensor_runnable(void);
extern void DoorLock_indication_runnable(void);
/* ------------------------------------------------------------------ */
/*  Private defines                                                     */
/* ------------------------------------------------------------------ */
#define RUNNABLE_PERIOD_MS   10U    /* Both runnables: 10 ms           */

/* ------------------------------------------------------------------ */
/*  Private function prototypes                                         */
/* ------------------------------------------------------------------ */
static void SystemClock_Config(void);
static void GPIO_Init(void);
static void Error_Handler(void);

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    /* -------------------------------------------------------------- */
    /*  1. HAL initialisation – must be first                          */
    /* -------------------------------------------------------------- */
    HAL_Init();

    /* -------------------------------------------------------------- */
    /*  2. Configure system clock                                       */
    /*     STM32F401RCT6 @ 84 MHz (HSI → PLL, APB1 /2, APB2 /1)      */
    /* -------------------------------------------------------------- */
    SystemClock_Config();

    /* -------------------------------------------------------------- */
    /*  3. Peripheral initialisation                                    */
    /* -------------------------------------------------------------- */
    GPIO_Init();

    /* -------------------------------------------------------------- */
    /*  4. Scheduler loop – simple time-triggered dispatcher            */
    /* -------------------------------------------------------------- */
    uint32_t lastTick = HAL_GetTick();

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* Fire both runnables every RUNNABLE_PERIOD_MS milliseconds   */
        if ((now - lastTick) >= RUNNABLE_PERIOD_MS)
        {
            lastTick = now;

            /*
             * Step 1 – Sensor abstraction SWC
             *   Reads PA1 via Dio_ReadChannel(DIO_Door)
             *   Writes result to the RTE sender/receiver buffer via
             *   Rte_IWrite_DoorSensorAbstractionSWC_ReadDoorSensor_PP_SR_DoorState()
             */
            ReadDoorSensor_runnable();

            /*
             * Step 2 – Door lock indication SWC
             *   Reads DoorState from the RTE buffer via
             *   Rte_Read_DoorLock_indication_SWC_RP_SR_DoorState_DoorState()
             *   Calls Led_Switch_runnable() through the CS port to
             *   drive PA0 (LED) accordingly
             */
            DoorLock_indication_runnable();
        }

        /* ---------------------------------------------------------- */
        /*  Place additional runnables or tasks here if needed          */
        /* ---------------------------------------------------------- */
    }
}

/* ------------------------------------------------------------------ */
/*  SystemClock_Config                                                  */
/*                                                                      */
/*  HSI (16 MHz) → PLL → SYSCLK = 84 MHz                              */
/*  AHB  prescaler = 1  → HCLK  = 84 MHz                              */
/*  APB1 prescaler = 2  → PCLK1 = 42 MHz  (≤ 42 MHz limit)           */
/*  APB2 prescaler = 1  → PCLK2 = 84 MHz                              */
/* ------------------------------------------------------------------ */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Enable HSI, configure PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM       = 16;   /* VCO input  = 16/16 = 1 MHz  */
    RCC_OscInitStruct.PLL.PLLN       = 336;  /* VCO output = 336 MHz        */
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV4; /* SYSCLK = 336/4 = 84 MHz */
    RCC_OscInitStruct.PLL.PLLQ       = 7;   /* USB FS clock = 336/7 = 48 MHz */

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Select PLL as system clock source and configure bus prescalers */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  |
                                       RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1  |
                                       RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    /* Flash latency = 2 wait states for 84 MHz @ 3.3 V */
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ------------------------------------------------------------------ */
/*  GPIO_Init                                                           */
/*                                                                      */
/*  PA0  – LED output (push-pull, no pull, initially HIGH = LED OFF)   */
/*  PA1  – Door sensor input (pull-up so that an open circuit = HIGH)  */
/* ------------------------------------------------------------------ */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* -------------------------------------------------------------- */
    /*  PA0 – LED (active-low: LEDOFF = GPIO_PIN_SET = 1)             */
    /* -------------------------------------------------------------- */
    /* Drive LED off at startup before configuring the pin direction   */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); /* LEDOFF = 1 */

    GPIO_InitStruct.Pin   = GPIO_PIN_0;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* -------------------------------------------------------------- */
    /*  PA1 – Door sensor (pull-up: no sensor connected → HIGH = open) */
    /* -------------------------------------------------------------- */
    GPIO_InitStruct.Pin  = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* ------------------------------------------------------------------ */
/*  Error_Handler                                                       */
/*  Halt execution so a debugger can identify the failure point.       */
/* ------------------------------------------------------------------ */
static void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        /* Spin here – connect a debugger to inspect the call stack    */
    }
}


