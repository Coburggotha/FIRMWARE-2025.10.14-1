# FIRMWARE-2025.10.14--
FIRMWARE-2025.10.14- 


- I2C EEPROM 활용 
- TOD : AT24C128

- I2C 핀 활성화

<p align="center">
<img width="1157" height="689" alt="image" src="https://github.com/user-attachments/assets/4db7cdb3-441a-4187-a407-80b3d87d9548" />
</p>
<br>

<p align="center">
<img width="638" height="694" alt="image" src="https://github.com/user-attachments/assets/6d5af50e-90ac-4a06-ab68-fdb3dc5a4553" />
</p>
<br>

- 문자 출력 헤더파일 추가
```c
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */
```
<br>

- I2C EEPROM 정보 정의
<br>

```c
/* USER CODE BEGIN PD */
#define K24C256_ADDR        0xA0    // K24C256 I2C 주소 (A0,A1,A2 = 000) 
#define EEPROM_PAGE_SIZE    64      // K24C256 페이지 크기 (64 bytes)
#define EEPROM_SIZE         32768   // K24C256 총 크기 (32KB)
#define TEST_ADDRESS        0x0000  // 테스트용 주소
/* USER CODE END PD */
```
<br>

- 하드웨어적으로 A1, A0(주소핀)이 묶여있지 않으므로 주소는 0, 0
<p align="center">
<img width="763" height="276" alt="image" src="https://github.com/user-attachments/assets/4f1e0a5b-1e2e-41f1-a6a1-087abba5fa06" />
</p>
<br>

- 1010000
<p align="center">
<img width="853" height="161" alt="image" src="https://github.com/user-attachments/assets/f1355547-29b2-4d8e-9390-0989b0e97acd" />
</p>
<br>

- 페이지 크기는 64 Bytes (256 제품은 512 페이지 따라서 총 크기는 32KB)


<p align="center">
<img width="778" height="217" alt="image" src="https://github.com/user-attachments/assets/f9a7dc5f-ba1a-480f-bd6f-d6737c0eabfe" />
</p>
<br>

- 
```c
/* USER CODE BEGIN PV */
uint8_t i2c_scan_found = 0;
uint8_t eeprom_address = 0;
/* USER CODE END PV */
```

- Data Write, Read 함수 설정
```c
/* USER CODE BEGIN PFP */
void I2C_Scan(void);
HAL_StatusTypeDef EEPROM_Write(uint16_t mem_addr, uint8_t *data, uint16_t size);
HAL_StatusTypeDef EEPROM_Read(uint16_t mem_addr, uint8_t *data, uint16_t size);
void EEPROM_Test(void);
/* USER CODE END PFP *
```

```c
/* USER CODE BEGIN 0 */
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART2 and Loop until the end of transmission */
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
/* USER CODE END 0 */
```
<br>

```c
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

  printf("Test completed. Entering main loop...\n\n");
  /* USER CODE END 2 */
```
<br>
```c
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t loop_count = 0;

  while (1)
  {
    /* USER CODE END WHILE */

	    /* USER CODE BEGIN 3 */

	    // 10초마다 상태 출력
	    if(loop_count % 1000 == 0)
	    {
	      printf("System running... Loop count: %lu\n", loop_count/1000);

	      HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	    }

	    loop_count++;
	    HAL_Delay(10);
	  }
	  /* USER CODE END 3 */
```





