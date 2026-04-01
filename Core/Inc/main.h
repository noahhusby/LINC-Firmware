/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DIO1_Pin GPIO_PIN_0
#define DIO1_GPIO_Port GPIOC
#define DIO2_Pin GPIO_PIN_1
#define DIO2_GPIO_Port GPIOC
#define DIO3_Pin GPIO_PIN_2
#define DIO3_GPIO_Port GPIOC
#define DIO4_Pin GPIO_PIN_3
#define DIO4_GPIO_Port GPIOC
#define GPIB_EN_Pin GPIO_PIN_1
#define GPIB_EN_GPIO_Port GPIOA
#define GPIB_DIR_Pin GPIO_PIN_2
#define GPIB_DIR_GPIO_Port GPIOA
#define EOI_SENSE_Pin GPIO_PIN_7
#define EOI_SENSE_GPIO_Port GPIOA
#define DIO5_Pin GPIO_PIN_4
#define DIO5_GPIO_Port GPIOC
#define DIO6_Pin GPIO_PIN_5
#define DIO6_GPIO_Port GPIOC
#define IREN_Pin GPIO_PIN_0
#define IREN_GPIO_Port GPIOB
#define iEOI_Pin GPIO_PIN_1
#define iEOI_GPIO_Port GPIOB
#define iDAV_Pin GPIO_PIN_2
#define iDAV_GPIO_Port GPIOB
#define iNRFD_Pin GPIO_PIN_10
#define iNRFD_GPIO_Port GPIOB
#define iNDAC_Pin GPIO_PIN_12
#define iNDAC_GPIO_Port GPIOB
#define iIFC_Pin GPIO_PIN_13
#define iIFC_GPIO_Port GPIOB
#define DIO7_Pin GPIO_PIN_6
#define DIO7_GPIO_Port GPIOC
#define DIO8_Pin GPIO_PIN_7
#define DIO8_GPIO_Port GPIOC
#define iATN_Pin GPIO_PIN_8
#define iATN_GPIO_Port GPIOC
#define NRFD_SENSE_Pin GPIO_PIN_9
#define NRFD_SENSE_GPIO_Port GPIOC
#define DAV_SENSE_Pin GPIO_PIN_8
#define DAV_SENSE_GPIO_Port GPIOA
#define SRQ_SENSE_Pin GPIO_PIN_9
#define SRQ_SENSE_GPIO_Port GPIOA
#define NDAC_SENSE_Pin GPIO_PIN_10
#define NDAC_SENSE_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
