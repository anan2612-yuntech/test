/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body for NUCLEO-G474RE Electronic Sparkler (Multi-Trigger + 5s Reset)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t rx_byte;                      // 宣告 UART 接收緩衝變數，用以儲存從序列埠讀取到的單一字元指令，原因在於接收中斷每次僅處理一個位元組
uint32_t led_pwm_val[7];              // 宣告 7 顆 LED 的當前 PWM 實質輸出亮度陣列（數值 0~1000），原因是用於在主迴圈中執行平滑漸變計算
uint32_t led_target_val[7];           // 宣告 7 顆 LED 的目標亮度陣列，原因是用以記錄點燃與燃燒消退時所要追隨的目標強度
uint8_t led_burn_state[7];            // 宣告 LED 燃燒狀態機陣列（0:未點燃/已碳化, 1:主燃燒/最亮, 2:次級燃燒/漸弱），原因是用於控制仙女棒擴散與消退邏輯
uint8_t led_carbonized[7];            // 宣告 LED 碳化旗標陣列（1表示已完全燃燒碳化），原因是用來模擬真實仙女棒碳化後無法再次點亮的物理特性
uint32_t carbonize_timer[7];          // 宣告碳化計時器陣列，原因是用以記錄各 LED 進入碳化狀態的時間點，以便計算 5 秒自動解除碳化
uint32_t tick_timer;                  // 宣告動畫時間戳變數，原因是用於記錄上一次更新燈光變化的時間點，實現不卡住系統的非阻塞式定時更新
uint32_t spread_timer;                // 宣告火花往兩側外擴的計時器變數，原因是用於控制仙女棒燃燒時向兩端延伸的擴散速度
uint8_t active_centers[7];            // 宣告多點燃燒中心陣列（記錄哪幾個位置被同時觸發點燃），原因是用於支援同時輸入多個位置同時外擴
uint8_t spread_radius[7];             // 宣告各個燃燒中心當前向兩側外擴的半徑距離陣列，原因是用於個別計算多點擴散進度
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  MX_TIM1_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();

  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // 啟動計時器 1 通道 1 的 PWM 輸出，原因是要驅動實體電路中的第 1 顆 LED (PA8)
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); // 啟動計時器 1 通道 2 的 PWM 輸出，原因是要驅動實體電路中的第 2 顆 LED (PA9)
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3); // 啟動計時器 1 通道 3 的 PWM 輸出，原因是要驅動實體電路中的第 3 顆 LED (PA10)
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // 啟動計時器 1 通道 4 的 PWM 輸出，原因是要驅動實體電路中的第 4 顆 LED (PA11)
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // 啟動計時器 4 通道 1 的 PWM 輸出，原因是要驅動實體電路中的第 5 顆 LED (PB6)
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); // 啟動計時器 4 通道 2 的 PWM 輸出，原因是要驅動實體電路中的第 6 顆 LED (PB7)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // 啟動計時器 3 通道 3 的 PWM 輸出，原因是要驅動實體電路中的第 7 顆 LED (PB0)
  HAL_UART_Receive_IT(&huart2, &rx_byte, 1); // 啟動 USART2 接收中斷模式，原因是要準備非同步接收來自使用者的單字元控制指令
  tick_timer = HAL_GetTick();           // 獲取當前系統啟動毫秒數並賦值給計時變數，原因是用於初始化動畫更新的時間基準點
  spread_timer = HAL_GetTick();         // 初始化外擴計時基準點，原因是用於控制火花向兩側蔓延的計時
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t current_tick = HAL_GetTick(); // 獲取當前系統時間毫秒數，原因是用於統一計算各個時間間隔以避免重複呼叫

    // 1. 改為 5 秒後自動解除碳化機制處理區段
    for (int i = 0; i < 7; i++)          // 迴圈遍歷所有 7 顆 LED，原因是要逐一檢查碳化狀態與計時
    {
      if (led_carbonized[i] == 1)        // 檢查該 LED 目前是否處於碳化狀態，原因是要判斷是否需要進行計時解除
      {
        if (current_tick - carbonize_timer[i] >= 5000) // 檢查自碳化起是否已過 5 秒（5000 毫秒），原因是要符合 5 秒後自動復亮的需求
        {
          led_carbonized[i] = 0;         // 將碳化旗標歸零解除碳化，原因是要讓該 LED 恢復可以再次被點亮的特性
        }
      }
    }

    // 2. 仙女棒多點同時點燃與向兩側外擴延伸機制
    if (current_tick - spread_timer >= 100) // 檢查是否已過 100 毫秒，原因是要控制火花向兩側蔓延的速度
    {
      spread_timer = current_tick;       // 更新外擴計時器時間點，原因是用於記錄本次向外擴散的時間

      for (int c = 0; c < 7; c++)        // 掃描所有可能的燃燒中心點，原因是要支援多個位置同時輸入與外擴
      {
        if (active_centers[c])           // 檢查該中心點是否處於活躍外擴狀態
        {
          int left_idx = c - spread_radius[c];  // 計算左側對應要點燃的 LED 索引位置，原因是要實現向左外擴
          int right_idx = c + spread_radius[c]; // 計算右側對應要點燃的 LED 索引位置，原因是要實現向右外擴

          uint8_t expanded = 0;          // 宣告本次半徑是否有成功點燃燈號的旗標

          if (left_idx >= 0 && !led_carbonized[left_idx]) // 檢查左側索引是否在範圍內且尚未碳化
          {
            led_burn_state[left_idx] = 1;    // 將左側對應 LED 設為主燃燒點（最亮），原因是要呈現火花向左外擴
            expanded = 1;
          }
          if (right_idx < 7 && !led_carbonized[right_idx]) // 檢查右側索引是否在範圍內且尚未碳化
          {
            led_burn_state[right_idx] = 1;   // 將右側對應 LED 設為主燃燒點（最亮），原因是要呈現火花向右外擴
            expanded = 1;
          }

          spread_radius[c]++;            // 將該中心的擴散半徑向外增加一格

          // 如果該中心的左右外擴已經超出燈條邊界，則關閉該中心的活動狀態
          if ((c - spread_radius[c] < 0) && (c + spread_radius[c] >= 7))
          {
            active_centers[c] = 0;       // 結束該點的外擴行程
          }
        }
      }
    }

    // 3. LED 亮度平滑漸變與碳化消退邏輯處理區段
    if (current_tick - tick_timer >= 25) // 檢查自上次更新以來是否已經過 25 毫秒，原因在於確保動畫更新頻率固定且視覺變化流暢
    {
      tick_timer = current_tick;        // 更新時間戳變數，原因是用記錄本次執行的時間點以供下次計算差值

      for (int i = 0; i < 7; i++)       // 使用迴圈逐一掃描並處理 7 顆 LED 的燃燒擴散與碳化消退狀態，原因是要統一管理全燈條狀態
      {
        if (led_burn_state[i] == 1)   // 檢查該顆 LED 是否處於剛點燃的主燃燒狀態，原因是要設定其達到最高亮度
        {
          led_target_val[i] = 1000;   // 將目標亮度設為最大值 1000（代表 100% 佔空比），原因是要呈現最亮狀態
          led_burn_state[i] = 2;      // 將狀態切換為次級燃燒階段，原因是要在下個週期開始執行漸弱遞減
        }
        else if (led_burn_state[i] == 2) // 檢查該顆 LED 是否處於燃燒消退階段，原因是要模擬仙女棒火花漸弱
        {
          if (led_target_val[i] > 100) // 檢查目標亮度是否仍高於碳化臨界點，原因是要確保亮度不會立刻歸零
          {
            led_target_val[i] -= 75;  // 穩步調降目標亮度數值，原因是要模擬仙女棒燃燒時能量逐漸消耗的過程
          }
          else                      // 若亮度已降至極低邊界，原因代表能量耗盡
          {
            led_target_val[i] = 0;    // 將目標亮度強制歸零，原因是要使 LED 完全熄滅
            led_burn_state[i] = 0;    // 關閉燃燒狀態標記，原因是要結束此顆 LED 的活躍行程
            led_carbonized[i] = 1;    // 將碳化旗標設為 1，原因是用來模擬真實仙女棒燃燒完碳化後無法再次點亮的物理特性
            carbonize_timer[i] = current_tick; // 記錄進入碳化的時間戳，原因是要啟動 5 秒自動解除碳化的計時
          }
        }

        if (led_pwm_val[i] < led_target_val[i]) // 檢查當前 PWM 亮度是否小於目標亮度，原因是要進行向上平滑遞增
        {
          led_pwm_val[i] += (led_target_val[i] - led_pwm_val[i]) / 3 + 1; // 運用比例追隨公式向上平滑調整，原因是要創造無段落感的呼吸燈效果
        }
        else if (led_pwm_val[i] > led_target_val[i]) // 檢查當前 PWM 亮度是否大於目標亮度，原因是要進行向下平滑遞減
        {
          led_pwm_val[i] -= (led_pwm_val[i] - led_target_val[i]) / 3 + 1; // 運用比例追隨公式向下平滑調整，原因是要讓熄滅或漸弱過程極為平順
        }
      }

      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, led_pwm_val[0]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 1 顆 LED 亮度
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, led_pwm_val[1]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 2 顆 LED 亮度
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, led_pwm_val[2]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 3 顆 LED 亮度
      __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, led_pwm_val[3]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 4 顆 LED 亮度
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, led_pwm_val[4]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 5 顆 LED 亮度
      __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_2, led_pwm_val[5]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 6 顆 LED 亮度
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, led_pwm_val[6]); // 將平滑計算後的 PWM 值寫入硬體暫存器，原因是用以控制第 7 顆 LED 亮度
    }
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
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 169;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) { Error_Handler(); }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim1);
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 169;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
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
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 169;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 999;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) { Error_Handler(); }
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) { Error_Handler(); }

  HAL_TIM_MspPostInit(&htim4);
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) // 定義 UART 接收完成中斷回調函數，原因在於當透過序列埠收到字元時會自動觸發此函式
{
  if (huart->Instance == USART2)                      // 檢查觸發中斷的是否為 USART2 序列埠，原因是要確保只處理正確的通訊介面
  {
    if (rx_byte >= '1' && rx_byte <= '7')         // 驗證接收到的字元是否在有效範圍 '1' 至 '7' 內，原因是要防止非法輸入造成陣列溢位
    {
      int index = rx_byte - '1';                // 將 ASCII 字元轉換為對應的 0 到 6 陣列索引值，原因是要對應 7 顆 LED 的陣列位置

      if (!led_carbonized[index])               // 檢查該 LED 是否尚未碳化，原因是要模擬真實仙女棒碳化後無法再次點亮的特性
      {
        active_centers[index] = 1;              // 標記該索引為一個活躍的燃燒中心點，原因是要支援同時輸入多個位置進行多點外擴
        spread_radius[index] = 0;               // 將該點的外擴半徑歸零從中心點開始，原因是要重新計算向兩側外擴的距離
        spread_timer = HAL_GetTick();           // 重設外擴計時基準點，原因是要確保觸發當下立即開始向兩側蔓延
      }
    }

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);    // 重新啟動 USART2 接收中斷，原因是要持續監聽並等待使用者輸入下一個序列埠指令
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
