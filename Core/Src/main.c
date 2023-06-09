/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MPU6050_ADDRESS   0x68
#define MPU6050_GYRO_XOUT_H  0x43
#define MPU6050_ACCEL_XOUT_H  0x3B
#define MPU6050_PWR_MGMT_1    0x6B
#define MPU6050_ACCEL_CONFIG  0x1C
#define MPU6050_GYRO_CONFIG   0x1B
#define GYRO_SENSITIVITY      131.0
#define alpha 0.95
#define SPACE_WIDTH 101
#define PROGRESS 50
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t MSG[50] = {'\0'};

char* winMessage ="\t\t.#.....................#.....###########.....#...........#\r\n"
				  "\t\t..#.........#.........#...........#..........#.#.........#\r\n"
				  "\t\t...#.......#.#.......#............#..........#...#.......#\r\n"
				  "\t\t....#.....#...#.....#.............#..........#.....#.....#\r\n"
				  "\t\t.....#...#.....#...#..............#..........#.......#...#\r\n"
				  "\t\t......#.#.......#.#...............#..........#.........#.#\r\n"
				  "\t\t.......#.........#...........###########.....#...........#\r\n";
int progress = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
void MPU6050_Init(void);
int ReadInt(void);
void UART_SEND_TXT(UART_HandleTypeDef *huart, char buffer[]);
void UART_SEND_INT(UART_HandleTypeDef *huart, int i);
void UART_SEND_CHR(UART_HandleTypeDef *huart, char c);
void UART_SEND_NL(UART_HandleTypeDef *huart);
int scale_roll(double value);
int scale_pitch(double value);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float gx_dps, gy_dps, gz_dps, gyro_x, gyro_y, gyro_z;
float angleX = 0, angleY = 0, angleZ = 0;
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */
	char space_line[SPACE_WIDTH];
	for (int i = 0; i < SPACE_WIDTH; i++)
	{
		space_line[i] = '.';
	}
	int x_pos = SPACE_WIDTH/2+1;
	int x_prev_pos = 0;

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
	MPU6050_Init();

	if(HAL_I2C_IsDeviceReady(&hi2c1, MPU6050_ADDRESS <<1, 1, HAL_MAX_DELAY))
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);
	}
	else
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 0);
	}

	float gyro_roll;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		uint8_t buf[6];
		int16_t ax, ay, az;
		float ax_g, ay_g, az_g;

		HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDRESS << 1, MPU6050_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, buf, 6, HAL_MAX_DELAY);

		ax = (buf[0] << 8) | buf[1];
		ay = (buf[2] << 8) | buf[3];
		az = (buf[4] << 8) | buf[5];

		ax_g = (float) ax/8192.0;
		ay_g = (float) ay/8192.0;
		az_g = (float) az/8192.0;

		float acc_roll = atan2(ay_g, az_g) * 180.0 /M_PI; // Roll angle from accelerometer
		float acc_pitch = atan2(ax_g, sqrt(ay_g*ay_g+az_g*az_g))*180.0/M_PI; // Pitch from acceleration

		/* If we are measuring gyroscope */
		uint8_t buf_GYRO[6];
		int16_t gx_raw, gy_raw, gz_raw;

		HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDRESS << 1, MPU6050_GYRO_XOUT_H, I2C_MEMADD_SIZE_8BIT, buf_GYRO, 6, HAL_MAX_DELAY);

		gx_raw = (buf_GYRO[0] << 8) | buf_GYRO[1];
		gy_raw = (buf_GYRO[2] << 8) | buf_GYRO[3];
		gz_raw = (buf_GYRO[4] << 8) | buf_GYRO[5];

		gx_dps = (float) gx_raw / GYRO_SENSITIVITY;
		gy_dps = (float) gy_raw / GYRO_SENSITIVITY;
		gz_dps = (float) gz_raw / GYRO_SENSITIVITY;

		float dt = 0.01;

		gyro_x = gx_dps*dt;
		gyro_y = gy_dps*dt;
		gyro_z = gy_dps*dt;


		float roll_angle = -1*(alpha*gyro_x + (1-alpha)*acc_roll);
		float pitch_angle = -1*(alpha*gyro_y + (1-alpha)*acc_pitch);



//		sprintf(MSG, "\r\x1b[1LRoll_angle: %d		Pitch: %d\r", scale_roll(roll_angle), scale_pitch(pitch_angle));
//		HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 100);
//		HAL_Delay(100);

		x_prev_pos = x_pos;
		int frequency = scale_pitch(pitch_angle);

		if(roll_angle < -0.1)
		{
			x_pos--;
		}

		else if(roll_angle > 0.1)
		{
			x_pos++;
		}

		else
			x_pos = x_prev_pos;


		if (x_pos == 0 || x_pos == SPACE_WIDTH - 1)
		{
			sprintf(MSG, "\t\t\t\tCRASH!!!\r\n");
			HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 100);
			break;
		}
		else
		{
			progress++;
			space_line[x_pos] = 'X';
			if (x_pos != x_prev_pos)
				space_line[x_prev_pos] = '.';
			HAL_UART_Transmit(&huart2, space_line, sizeof(space_line), 100);
			sprintf(MSG, "\t\t\t\tFrequency = %d", frequency);
			HAL_UART_Transmit(&huart2, MSG, sizeof(MSG), 100);
			UART_SEND_NL(&huart2);
			HAL_Delay(frequency);
			if(progress == PROGRESS)
			{
				HAL_UART_Transmit(&huart2, (uint8_t*)winMessage , strlen(winMessage), 100);
				break;
			}
		}
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

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 80;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
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

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LD2_Pin */
	GPIO_InitStruct.Pin = LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void MPU6050_Init(void)
{
	uint8_t buffer[2];

	buffer[0] = MPU6050_PWR_MGMT_1;
	buffer[1] = 0x00;
	HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDRESS << 1, buffer, 2, HAL_MAX_DELAY);

	buffer[0] = MPU6050_ACCEL_CONFIG;
	buffer[1] = 0x08;
	HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDRESS << 1, buffer, 2, HAL_MAX_DELAY);

	buffer[0] = MPU6050_GYRO_CONFIG;
	buffer[1] = 0x18;
	HAL_I2C_Master_Transmit(&hi2c1, MPU6050_ADDRESS << 1, buffer, 2, HAL_MAX_DELAY);
}


int scale_roll(double value)
{
	double range = 6.2; // the range of values we're scaling
	double offset = -3.1; // the minimum value of the range we're scaling
	double scale = 100.0 / range; // the scaling factor to convert the range to 0-100

	return (int)(value - offset) * scale; // apply the scaling factor and return the scaled value
}

int scale_pitch(double value)
{
	double range = 8.0; // the range of values we're scaling
	double offset = -4.0; // the minimum value of the range we're scaling
	double outputRange = 167.0; // the range of the output values (250-83)
	double outputOffset = 83.0; // the minimum value of the output range

	double scale = outputRange / range; // the scaling factor to convert the range to 250-83

	return (int)(value - offset) * scale + outputOffset; // apply the scaling factor and output offset, and return the scaled value
}

//00001000
// This function displays a new-line
//
void UART_SEND_NL(UART_HandleTypeDef *huart)
{
	HAL_UART_Transmit(huart, (uint8_t*)"\n\r", 2, HAL_MAX_DELAY);
}

//
// This function displays a character
//
void UART_SEND_CHR(UART_HandleTypeDef *huart, char c)
{
	HAL_UART_Transmit(huart, (uint8_t*)&c, 1, HAL_MAX_DELAY);
}



//
// This function displays an integer number
//
void UART_SEND_INT(UART_HandleTypeDef *huart, int i)
{
	char buffer[10];
	itoa(i, buffer, 10);
	HAL_UART_Transmit(huart, (uint8_t*) buffer, strlen(buffer), HAL_MAX_DELAY);
}

//
// This function displays text
//
void UART_SEND_TXT(UART_HandleTypeDef *huart, char buffer[])
{
	HAL_UART_Transmit(huart, (uint8_t*) buffer, strlen(buffer), HAL_MAX_DELAY);
}

//
// Read an integer number
//
int ReadInt(void)
{
	int n, N = 0;
	char readBuf[1];
	while(1)
	{
		HAL_UART_Receive(&huart2, (uint8_t*)readBuf, 1, HAL_MAX_DELAY);
		UART_SEND_CHR(&huart2, readBuf[0]);
		if(readBuf[0] == '\r')break;
		n = atoi(readBuf);
		N = 10*N + n;
	}
}
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
