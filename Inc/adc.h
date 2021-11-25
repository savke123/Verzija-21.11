/**
  ******************************************************************************
  * File Name          : ADC.h
  * Description        : This file provides code for the configuration
  *                      of the ADC instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __adc_H
#define __adc_H
#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;

/* USER CODE BEGIN Private defines */
struct Prekidac
{
	uint16_t I1;
	uint16_t I2;
};

/* USER CODE END Private defines */

void MX_ADC1_Init(void);
void MX_ADC3_Init(void);

/* USER CODE BEGIN Prototypes */
void init_adc1_ch9(void);
void init_adc1_ch10(void);
void init_adc1_ch12(void);
void init_adc1_ch13(void);
void init_adc3_ch14(void);
void init_adc3_ch9(void);

uint16_t DoADCon1(void);
uint16_t DoADCon3(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ adc_H */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
