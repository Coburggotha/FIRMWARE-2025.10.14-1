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
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define K24C256_ADDR        0xA0    // K24C256 I2C 주소 (A0,A1,A2 = 000)
#define EEPROM_PAGE_SIZE    64      // K24C256 페이지 크기 (64 bytes)
#define EEPROM_SIZE         32768   // K24C256 총 크기 (32KB)
#define TEST_ADDRESS        0x0000  // 테스트용 주소
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t i2c_scan_found = 0;
uint8_t eeprom_address = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void I2C_Scan(void);
HAL_StatusTypeDef EEPROM_Write(uint16_t mem_addr, uint8_t *data, uint16_t size);
HAL_StatusTypeDef EEPROM_Read(uint16_t mem_addr, uint8_t *data, uint16_t size);
void EEPROM_Test(void);
void Test_0xB0_Device(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
PUTCHAR_PROTOTYPE
{

  if (ch == '\n')
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r", 1, 0xFFFF);
  HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 0xFFFF);

  return ch;
}

/**
  * @brief  I2C 주소 스캔 함수
  * @param  None
  * @retval None
  */
void I2C_Scan(void)
{
  printf("\n=== I2C Address Scan ===\n");
  printf("Scanning I2C bus...\n");

  i2c_scan_found = 0;

  for(uint8_t i = 0; i < 128; i++)
  {
    if(HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5) == HAL_OK)
    {
      printf("Found I2C device at address: 0x%02X (7-bit: 0x%02X)\n", (i<<1), i);
      i2c_scan_found++;

      // K24C256 주소 범위 확인 (0xA0~0xAE)
      if((i<<1) >= 0xA0 && (i<<1) <= 0xAE)
      {
        eeprom_address = (i<<1);
        printf("** K24C256 EEPROM detected at 0x%02X **\n", eeprom_address);
      }
    }
  }

  if(i2c_scan_found == 0)
  {
    printf("No I2C devices found!\n");
  }
  else
  {
    printf("Total %d I2C device(s) found.\n", i2c_scan_found);
  }
  printf("========================\n\n");
}

/**
  * @brief  EEPROM 쓰기 함수
  * @param  mem_addr: 메모리 주소
  * @param  data: 쓸 데이터 포인터
  * @param  size: 데이터 크기
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef EEPROM_Write(uint16_t mem_addr, uint8_t *data, uint16_t size)
{
  HAL_StatusTypeDef status = HAL_OK;
  uint16_t bytes_to_write;
  uint16_t current_addr = mem_addr;
  uint16_t data_index = 0;

  while(size > 0)
  {
    // 페이지 경계를 고려한 쓰기 크기 계산
    bytes_to_write = EEPROM_PAGE_SIZE - (current_addr % EEPROM_PAGE_SIZE);
    if(bytes_to_write > size)
      bytes_to_write = size;

    // EEPROM에 쓰기
    status = HAL_I2C_Mem_Write(&hi2c1, eeprom_address, current_addr,
                               I2C_MEMADD_SIZE_16BIT, &data[data_index],
                               bytes_to_write, HAL_MAX_DELAY);

    if(status != HAL_OK)
    {
      printf("EEPROM Write Error at address 0x%04X\n", current_addr);
      return status;
    }

    // EEPROM 쓰기 완료 대기 (Write Cycle Time)
    HAL_Delay(5);

    // 다음 쓰기를 위한 변수 업데이트
    current_addr += bytes_to_write;
    data_index += bytes_to_write;
    size -= bytes_to_write;
  }

  return status;
}

/**
  * @brief  EEPROM 읽기 함수
  * @param  mem_addr: 메모리 주소
  * @param  data: 읽을 데이터 포인터
  * @param  size: 데이터 크기
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef EEPROM_Read(uint16_t mem_addr, uint8_t *data, uint16_t size)
{
  return HAL_I2C_Mem_Read(&hi2c1, eeprom_address, mem_addr,
                          I2C_MEMADD_SIZE_16BIT, data, size, HAL_MAX_DELAY);
}

/**
  * @brief  EEPROM 테스트 함수
  * @param  None
  * @retval None
  */
void EEPROM_Test(void)
{
  if(eeprom_address == 0)
  {
    printf("EEPROM not detected! Cannot perform test.\n\n");
    return;
  }

  printf("=== EEPROM Test ===\n");

  // 테스트 데이터 준비
  char write_data[] = "Hello, STM32F103 with K24C256 EEPROM!";
  uint8_t read_data[100] = {0};
  uint16_t data_len = strlen(write_data);

  printf("Test Address: 0x%04X\n", TEST_ADDRESS);
  printf("Write Data: \"%s\" (%d bytes)\n", write_data, data_len);

  // EEPROM에 데이터 쓰기
  printf("Writing to EEPROM...\n");
  if(EEPROM_Write(TEST_ADDRESS, (uint8_t*)write_data, data_len) == HAL_OK)
  {
    printf("Write successful!\n");
  }
  else
  {
    printf("Write failed!\n");
    return;
  }

  // 잠시 대기
  HAL_Delay(10);

  // EEPROM에서 데이터 읽기
  printf("Reading from EEPROM...\n");
  if(EEPROM_Read(TEST_ADDRESS, read_data, data_len) == HAL_OK)
  {
    printf("Read successful!\n");
    printf("Read Data: \"%s\" (%d bytes)\n", (char*)read_data, data_len);

    // 데이터 비교
    if(memcmp(write_data, read_data, data_len) == 0)
    {
      printf("** Data verification PASSED! **\n");
    }
    else
    {
      printf("** Data verification FAILED! **\n");
    }
  }
  else
  {
    printf("Read failed!\n");
  }

  printf("===================\n\n");

  // 추가 테스트: 숫자 데이터
  printf("=== Number Data Test ===\n");
  uint8_t num_write[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  uint8_t num_read[10] = {0};
  uint16_t num_addr = TEST_ADDRESS + 100;

  printf("Writing numbers 0-9 to address 0x%04X...\n", num_addr);
  if(EEPROM_Write(num_addr, num_write, 10) == HAL_OK)
  {
    HAL_Delay(10);

    if(EEPROM_Read(num_addr, num_read, 10) == HAL_OK)
    {
      printf("Write Data: ");
      for(int i = 0; i < 10; i++) printf("%d ", num_write[i]);
      printf("\n");

      printf("Read Data:  ");
      for(int i = 0; i < 10; i++) printf("%d ", num_read[i]);
      printf("\n");

      if(memcmp(num_write, num_read, 10) == 0)
      {
        printf("** Number test PASSED! **\n");
      }
      else
      {
        printf("** Number test FAILED! **\n");
      }
    }
  }
  printf("========================\n\n");
}
void Test_0xB0_Device(void)
{
  printf("=== Testing 0xB0 Device Identity ===\n");
  printf("Checking if 0xB0 is an extended memory block...\n\n");

  uint8_t write_data[] = "Block1Test";
  uint8_t read_data[20] = {0};
  uint16_t test_addr = 0x0100;
  HAL_StatusTypeDef status;

  // 테스트 1: EEPROM처럼 동작하는지 확인
  printf("Test 1: Writing to 0xB0 at address 0x%04X...\n", test_addr);
  status = HAL_I2C_Mem_Write(&hi2c1, 0xB0, test_addr,
                             I2C_MEMADD_SIZE_16BIT, write_data,
                             strlen((char*)write_data), HAL_MAX_DELAY);

  if(status == HAL_OK)
  {
    printf("  Write to 0xB0: SUCCESS\n");
    HAL_Delay(10);

    // 읽기 시도
    printf("Test 2: Reading from 0xB0 at address 0x%04X...\n", test_addr);
    status = HAL_I2C_Mem_Read(&hi2c1, 0xB0, test_addr,
                              I2C_MEMADD_SIZE_16BIT, read_data,
                              strlen((char*)write_data), HAL_MAX_DELAY);

    if(status == HAL_OK)
    {
      printf("  Read from 0xB0: SUCCESS\n");
      printf("  Write Data: \"%s\"\n", write_data);
      printf("  Read Data:  \"%s\"\n", read_data);

      if(memcmp(write_data, read_data, strlen((char*)write_data)) == 0)
      {
        printf("\n** 0xB0 IS A VALID EEPROM BLOCK! **\n");
        printf("** Your chip is likely 64KB (512Kbit), not 32KB! **\n");
        printf("** Block 0 (0xA0): 32KB **\n");
        printf("** Block 1 (0xB0): 32KB **\n");
        printf("** Total: 64KB available! **\n");
      }
      else
      {
        printf("\n** Data mismatch - 0xB0 behavior unclear **\n");
      }
    }
    else
    {
      printf("  Read from 0xB0: FAILED\n");
      printf("** 0xB0 accepts write but not read - unusual device **\n");
    }
  }
  else
  {
    printf("  Write to 0xB0: FAILED\n");

    // 테스트 3: 다른 프로토콜 시도
    printf("\nTest 3: Trying different access methods...\n");

    // 8비트 주소로 시도
    status = HAL_I2C_Mem_Read(&hi2c1, 0xB0, 0x00,
                              I2C_MEMADD_SIZE_8BIT, read_data, 8, 1000);
    if(status == HAL_OK)
    {
      printf("  8-bit address read: SUCCESS\n");
      printf("  Data: ");
      for(int i=0; i<8; i++) printf("%02X ", read_data[i]);
      printf("\n");
      printf("** 0xB0 is likely an RTC, sensor, or other I2C device **\n");
    }
    else
    {
      // 단순 수신 시도
      status = HAL_I2C_Master_Receive(&hi2c1, 0xB0, read_data, 8, 1000);
      if(status == HAL_OK)
      {
        printf("  Simple receive: SUCCESS\n");
        printf("  Data: ");
        for(int i=0; i<8; i++) printf("%02X ", read_data[i]);
        printf("\n");
        printf("** 0xB0 responds but protocol unclear **\n");
      }
      else
      {
        printf("  All access methods: FAILED\n");
        printf("** 0xB0 detected but not accessible **\n");
        printf("** Possible ghost address or bus issue **\n");
      }
    }
  }

  // 크로스 체크: 0xA0와 0xB0가 같은 메모리를 공유하는지 확인
  printf("\nTest 4: Cross-check with 0xA0...\n");
  memset(read_data, 0, sizeof(read_data));

  // 0xA0에 특별한 데이터 쓰기
  uint8_t marker[] = "CrossCheck";
  if(EEPROM_Write(0x0200, marker, strlen((char*)marker)) == HAL_OK)
  {
    HAL_Delay(10);

    // 0xB0의 같은 주소에서 읽기 시도
    status = HAL_I2C_Mem_Read(&hi2c1, 0xB0, 0x0200,
                              I2C_MEMADD_SIZE_16BIT, read_data,
                              strlen((char*)marker), HAL_MAX_DELAY);

    if(status == HAL_OK)
    {
      printf("  0xB0 read at 0x0200: SUCCESS\n");
      printf("  0xA0 wrote: \"%s\"\n", marker);
      printf("  0xB0 read:  \"%s\"\n", read_data);

      if(memcmp(marker, read_data, strlen((char*)marker)) == 0)
      {
        printf("\n** 0xA0 and 0xB0 share SAME memory! **\n");
        printf("** 0xB0 is an ALIAS of 0xA0 - same 32KB chip **\n");
        printf("** Your chip reports multiple addresses **\n");
      }
      else
      {
        printf("\n** 0xA0 and 0xB0 have DIFFERENT data! **\n");
        printf("** They are INDEPENDENT memory blocks **\n");
        printf("** Total 64KB confirmed! **\n");
      }
    }
    else
    {
      printf("  0xB0 read at 0x0200: FAILED\n");
      printf("** Cannot determine relationship **\n");
    }
  }

  printf("====================================\n\n");
}
/* USER CODE END 0 */

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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  printf("\n\n");
    printf("========================================\n");
    printf("  STM32F103 I2C EEPROM K24C256 Test    \n");
    printf("  System Clock: 64MHz                  \n");
    printf("  I2C Speed: 100kHz                    \n");
    printf("========================================\n");

    // I2C 주소 스캔
    I2C_Scan();

    // EEPROM 테스트
    EEPROM_Test();
    Test_0xB0_Device();

    printf("Test completed. Entering main loop...\n\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    uint32_t loop_count = 0;
  while (1)
  {
	  // 10초마다 상태 출력
	  	    if(loop_count % 1000 == 0)
	  	    {
	  	      printf("System running... Loop count: %lu\n", loop_count/1000);

	  	      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	  	    }

	  	    loop_count++;
	  	    HAL_Delay(10);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

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
  hi2c1.Init.ClockSpeed = 100000;
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
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
