/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sr04.h"
#include <stdio.h>
#include "fonts.h"
#include "ssd1306.h"
#include "servomotor.h"
#include "stdlib.h"
#define DIST_THRESHOLD 200   // mm
uint8_t servo_state = 0;
// 0 = Başlangıç (0°)
// 1 = 90°'de
#define BUZZER_PORT GPIOD
#define BUZZER_PIN  GPIO_PIN_14

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
/* USER CODE BEGIN PV */
sr04_t sr04_1;
sr04_t sr04_2;
servo_t servo1;

uint8_t car_seen_inside = 0;
uint8_t door_open_flag = 0;
uint8_t car_seen_outside = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
void door_open(void)
{
    if (door_open_flag == 0)
    {
        servo_set_position(&servo1, 90);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET); // LED
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET); // Buzzer
        door_open_flag = 1;
    }
}

void door_close(void)
{
    if (door_open_flag == 1)
    {
        servo_set_position(&servo1, 0);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
        door_open_flag = 0;
    }
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  servo_init(&servo1,&htim3,TIM_CHANNEL_1);
  /* USER CODE BEGIN 2 */
  /* USER CODE BEGIN 2 */

  // -------- SENSOR 1 --------
  // -------- SENSOR 1 --------
  SSD1306_Init();
  char string[25];
  SSD1306_GotoXY (0,0);
  SSD1306_Puts ("Arac Park ", &Font_11x18, SSD1306_COLOR_WHITE);
  SSD1306_GotoXY (0, 30);
  SSD1306_Puts ("Sistemi", &Font_11x18, SSD1306_COLOR_WHITE);
  SSD1306_UpdateScreen();
  HAL_Delay (1000);


  SSD1306_GotoXY (30,0);

  sr04_1.trig_port    = GPIOB;
  sr04_1.trig_pin     = GPIO_PIN_1;
  sr04_1.echo_htim    = &htim1;
  sr04_1.echo_channel = TIM_CHANNEL_1;
  sr04_init(&sr04_1);

  // -------- SENSOR 2 --------
  sr04_2.trig_port    = GPIOB;
  sr04_2.trig_pin     = GPIO_PIN_2;
  sr04_2.echo_htim    = &htim1;
  sr04_2.echo_channel = TIM_CHANNEL_2;
  sr04_init(&sr04_2);
  servo1.htim      = &htim3;
  servo1.channel   = TIM_CHANNEL_1;
  servo1.min = 1000;   // 1 ms
  servo1.max = 2000;   // 2 ms



  // Input Capture + Base timer interrupt
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2);
  HAL_TIM_Base_Start_IT(&htim1);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  servo_init(&servo1, &htim3, TIM_CHANNEL_1);
  uint8_t servo_state  = 0;   // 0: BOS, 1: DOLU
  uint8_t screen_state = 255;
  typedef enum
  {
      STATE_IDLE = 0,     // Kapı kapalı, araç yok
      STATE_ENTERING,     // Araç giriyor
      STATE_INSIDE,       // Araç içeride
      STATE_EXITING       // Araç çıkıyor
  } garage_state_t;

  garage_state_t garage_state = STATE_IDLE;
  while (1)
  {
      /* -------- SENSÖRLERİ OKU -------- */
      sr04_trigger(&sr04_1);          // DIS sensör
      HAL_Delay(60);

      sr04_trigger(&sr04_2);          // IC sensör
      HAL_Delay(60);

      uint32_t sA = sr04_1.distance;  // DIS
      uint32_t sB = sr04_2.distance;  // IC

      printf("A:%lu B:%lu STATE:%d\r\n", sA, sB, garage_state);

      /* -------- STATE MACHINE -------- */
      switch (garage_state)
      {
          /* ===== KAPI KAPALI / BOS ===== */
          case STATE_IDLE:
              car_seen_inside  = 0;
              car_seen_outside = 0;

              if (sA > 0 && sA < DIST_THRESHOLD)
              {
                  door_open();
                  garage_state = STATE_ENTERING;
              }
              break;

          /* ===== ARAÇ GIRISI ===== */
          case STATE_ENTERING:
              /* Dıştan koptu + iç sensör gördü */
              if (sB > 0 && sB < DIST_THRESHOLD &&
                  (sA == 0 || sA > DIST_THRESHOLD))
              {
                  door_close();
                  car_seen_inside = 1;
                  garage_state = STATE_INSIDE;
              }
              break;

          /* ===== ARAÇ ICERIDE ===== */
          case STATE_INSIDE:
              /* İçerideyken flag garanti */
              if (sB > 0 && sB < DIST_THRESHOLD)
              {
                  car_seen_inside = 1;
              }

              /* İç sensör önce gördü → sonra BOS */
              if (car_seen_inside == 1 &&
                 (sB == 0 || sB > DIST_THRESHOLD))
              {
                  door_open();
                  car_seen_outside = 0;
                  garage_state = STATE_EXITING;
              }
              break;

          /* ===== ARAÇ CIKIYOR ===== */
          case STATE_EXITING:
              /* Dış sensör aracı gördü mü? */
              if (sA > 0 && sA < DIST_THRESHOLD)
              {
                  car_seen_outside = 1;
              }

              /* ÇIKIŞ TAMAMLANDI */
              if (car_seen_outside == 1 &&
                 (sA == 0 || sA > DIST_THRESHOLD) &&
                 (sB == 0 || sB > DIST_THRESHOLD))
              {
                  door_close();
                  car_seen_inside  = 0;
                  car_seen_outside = 0;
                  garage_state = STATE_IDLE;
              }
              break;
      }

      /* -------- OLED -------- */
      static garage_state_t last_state = 255;

      if (garage_state != last_state)
      {
          SSD1306_Fill(SSD1306_COLOR_BLACK);
          SSD1306_GotoXY(0, 0);

          if (garage_state == STATE_IDLE)
              SSD1306_Puts("ARABA YOK", &Font_11x18, SSD1306_COLOR_WHITE);
          else if (garage_state == STATE_ENTERING)
              SSD1306_Puts("ARABA GIRIS", &Font_11x18, SSD1306_COLOR_WHITE);
          else if (garage_state == STATE_INSIDE)
              SSD1306_Puts("DOLU", &Font_11x18, SSD1306_COLOR_WHITE);
          else if (garage_state == STATE_EXITING)
              SSD1306_Puts("ARABA CIKIS", &Font_11x18, SSD1306_COLOR_WHITE);

          SSD1306_UpdateScreen();
          last_state = garage_state;
      }

      HAL_Delay(50);
  }



  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 84-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0xffff-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 20000-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pins : PB1 PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
