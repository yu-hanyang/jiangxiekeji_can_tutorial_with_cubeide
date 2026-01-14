
江协科技以keil+经典库作为开发环境进行教学，思路清晰、讲解细致是非常好的入门教程。实际开发中使用hal库比较常见，而且cubeide免费多平台支持，因此使用cubeide复现江西科技CAN的代码效果，所有硬件连接均与江协科技几乎保持一致（唯一差别是按键的gpio，原在pb1的按键被配置到pb0）。
*江协科技can教程链接 [https://jiangxiekeji.com/tutorial/can.html](https://jiangxiekeji.com/tutorial/can.html) 配合使用效果更佳*

cubeide工程代码地址 [https://github.com/yu-hanyang/jiangxiekeji_can_tutorial_with_cubeide](https://github.com/yu-hanyang/jiangxiekeji_can_tutorial_with_cubeide)
# can总线单个回环配置
## cubemx配置
使用cubeide创建新项目，首先配置Debug(pinout&configuration->system core -> sys->debug 下拉框选择serial wire)，然后配置时钟（在pinout&configuration->system core -> rcc中配置两个时钟源为crystal）（在clock configuration中配置APB1的时钟为36Mhz）。配置pb0和pb11为上拉输入，配置pb8和pb9为开漏输出（oled的引脚）。最后配置can（在(pinout&configuration->connectivity->CAN 点activated ，然后Prescaler配置为48，Time Quanta in BIt Segment 1配置3Times,Time Quanta in BIt Segment 2配置2Times,ReSynchronization jump width配置为2 Times. Basic Parameters中的配置全为disable.Advanced Parameters 中 Test mode 配置为loopback。



## 代码部分

cubemx会把大部分的配置都完成，因此代码中的配置需要修改，所修改添加的代码均符合cubemx代码框架
### oled部分
oled的引脚初始化函数就不需要了，在oled.c中把OLED_I2C_Init()函数删了.h文件中也一并删除，然后把
```c
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))
```
改成
```c
#define OLED_W_SCL(x)		 	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (GPIO_PinState)(x))
#define OLED_W_SDA(x)			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (GPIO_PinState)(x))
```

### Key部分
引脚初始化函数不需要了，在Key.c中把Key_Init()函数删了.h文件中也一并删除
函数Key_GetNum()中的一些经典库中的函数替换为HAL库的具体参考如下
```c
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == 0)
	{
		HAL_Delay(20);
		while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == 0);
		HAL_Delay(20);
		KeyNum = 1;
	}
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == 0)
	{
		HAL_Delay(20);
		while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11) == 0);
		HAL_Delay(20);
		KeyNum = 2;
	}
	
	return KeyNum;
}
```
### can部分
江协科技的MyCAN模块就不需要了，直接在cubemx生成的can代码的框架下添加用户代码部分
```c
/* USER CODE BEGIN 0 */
CAN_TxHeaderTypeDef TxHeader;      //发送
CAN_RxHeaderTypeDef RxHeader;      //接收

static uint8_t RxData[8];  //数据接收数组，can的数据帧只有8帧

/* USER CODE END 0 */
...
	/* USER CODE BEGIN CAN_Init 2 */

	CAN_FilterTypeDef sFilterConfig;
	sFilterConfig.FilterBank = 0; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; /* 32位。*/

	sFilterConfig.FilterIdHigh = 0x0000; /* 要过滤的ID高位 */
	sFilterConfig.FilterIdLow = 0x0000; /* 要过滤的ID低位 */
	sFilterConfig.FilterMaskIdHigh = 0x0000; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = 0x0000; /* 过滤器低16位每位必须匹配 */
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}

	HAL_CAN_Start(&hcan);
	/* USER CODE END CAN_Init 2 */
	...
	/* USER CODE BEGIN 1 */
void MyCAN_Transmit(uint32_t ID, uint8_t Length, uint8_t *Data) {
	uint8_t i = 0;
	uint32_t TxMailbox;
	uint8_t message[8];

	TxHeader.StdId = ID;
	TxHeader.ExtId = ID;        //扩展标识符(29位)
	TxHeader.IDE = CAN_ID_STD;    //使用扩展帧
	TxHeader.RTR = CAN_RTR_DATA;  //数据帧
	TxHeader.DLC = Length;

	for (i = 0; i < Length; i++) {
		message[i] = Data[i];
	}

	if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, message, &TxMailbox) != HAL_OK) //发送
			{
		return;
	}
	while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) != 3) {
	}

}

uint8_t MyCAN_ReceiveFlag(void) {
	if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) {
		return 1;
	}
	return 0;
}

void MyCAN_Receive(uint32_t * ID, uint8_t * Length, uint8_t *Data)
{
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &RxHeader, RxData);

	if (RxHeader.IDE == CAN_ID_STD)
	{
		*ID = RxHeader.StdId;

	}
	else
	{
		*ID = RxHeader.ExtId;
	}

	if (RxHeader.RTR == CAN_RTR_DATA)
	{
		*Length = RxHeader.DLC;
		for (uint8_t i = 0; i < *Length; i++)
		{
			Data[i] = RxData[i];
		}

	}
	else
	{
		//...
	}
}

/* USER CODE END 1 */
```
然后在.h文件中添加新增函数的声明

## 主函数部分
主函数方面和江协科技的教程基础一致，少了部分的初始化（因为cubemx已经生成好了）具体添加的部分如下
```c
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "Key.h"
/* USER CODE END Includes */
...
/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
uint32_t TxID = 0x555;
uint8_t TxLength = 4;
uint8_t TxData[8] = { 0x11, 0x22, 0x33, 0x44 };

uint32_t RxID;
uint8_t RxLength;
uint8_t RxData[8] = {0};

/* USER CODE END PV */
...
/* USER CODE BEGIN 2 */
	OLED_Init();

	OLED_ShowString(1, 1, "TxID:");
	OLED_ShowHexNum(1, 6, TxID, 3);
	OLED_ShowString(2, 1, "RxID:");
	OLED_ShowString(3, 1, "Leng:");
	OLED_ShowString(4, 1, "Data:");
	/* USER CODE END 2 */
	...
	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		KeyNum = Key_GetNum();

		if (KeyNum == 1) {
			TxData[0]++;
			TxData[1]++;
			TxData[2]++;
			TxData[3]++;

			MyCAN_Transmit(TxID, TxLength, TxData);
		}

		if (MyCAN_ReceiveFlag()) {
			MyCAN_Receive(&RxID, &RxLength, RxData);

			OLED_ShowHexNum(2, 6, RxID, 3);
			OLED_ShowHexNum(3, 6, RxLength, 1);
			OLED_ShowHexNum(4, 6, RxData[0], 2);
			OLED_ShowHexNum(4, 9, RxData[1], 2);
			OLED_ShowHexNum(4, 12, RxData[2], 2);
			OLED_ShowHexNum(4, 15, RxData[3], 2);
		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */

```



至此can总线单个回环配置完成
2026-01-13

---
# can总线多设备通信
参照江协科技的教程进行硬件连接，要是测试没成功请检查can收发器的电压是否符合数据手册的范围
## cubemx 配置
只需要把can的模式从loopback改成normal进行

2026-01-13

---

# 标准格式-扩展格式-数据帧-遥控帧
## cubemx配置
cubemx把can模式改成loopback（ps：我提供的工程为normal模式，适合多主机测试）

## 代码部分 
### can 部分
需要对can.c文件部分进行修改,文件的开头需要把用不掉的给注释掉
```c
/* USER CODE BEGIN 0 */
//CAN_TxHeaderTypeDef TxHeader;      //发送
//CAN_RxHeaderTypeDef RxHeader;      //接收

//static uint8_t RxData[8];  //数据接收数组，can的数据帧只有8帧

/* USER CODE END 0 */
...
/* USER CODE BEGIN 1 */
void MyCAN_Transmit(CAN_TxHeaderTypeDef _TxHeader, uint8_t *Data) {
	uint8_t i = 0;
	uint32_t TxMailbox;


	if (HAL_CAN_AddTxMessage(&hcan, &_TxHeader, Data, &TxMailbox) != HAL_OK) //发送
			{
		return;
	}
	while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) != 3) {
	}

}

uint8_t MyCAN_ReceiveFlag(void) {
	if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) {
		return 1;
	}
	return 0;
}

void MyCAN_Receive(CAN_RxHeaderTypeDef *_RxHeader, uint8_t *Data)
{
	HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, _RxHeader, Data);


}

/* USER CODE END 1 */
```
### 主函数部分
```c
/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
//uint32_t TxID = 0x222;
//uint8_t TxLength = 4;
uint8_t TxDataArray[4][8] = {
		{ 0x11, 0x22, 0x33, 0x44 },
		{ 0xaa, 0xbb, 0xcc, 0xdd },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 }

};

CAN_TxHeaderTypeDef TxMsgArray[5] = {
		{ .StdId = 0x555, .ExtId = 0x00000000, .IDE =CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
		{ .StdId = 0x000, .ExtId = 0x12345678, .IDE =CAN_ID_EXT, .RTR = CAN_RTR_DATA, .DLC = 4 },
		{ .StdId = 0x666, .ExtId = 0x00000000, .IDE =CAN_ID_STD, .RTR = CAN_RTR_REMOTE, .DLC = 0 },
		{ .StdId = 0x000, .ExtId = 0x12345678, .IDE =CAN_ID_EXT, .RTR = CAN_RTR_REMOTE, .DLC = 0 },
		{0}
};

uint8_t pTxMsgArray = 0;

//uint32_t RxID;
//uint8_t RxLength;
uint8_t RxData[8] = { 0 };
CAN_RxHeaderTypeDef RxMsg = { 0 };

/* USER CODE END PV */
...
 /* USER CODE BEGIN 2 */
	OLED_Init();

	OLED_ShowString(1, 1, "Rx :");
	OLED_ShowString(2, 1, "RxID:");
	OLED_ShowString(3, 1, "Leng:");
	OLED_ShowString(4, 1, "Data:");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		KeyNum = Key_GetNum();

		if (KeyNum == 1) {
			MyCAN_Transmit(TxMsgArray[pTxMsgArray], TxDataArray[pTxMsgArray]);
			pTxMsgArray ++;
			pTxMsgArray %= 4;
		}

		if (MyCAN_ReceiveFlag()) {
			MyCAN_Receive(&RxMsg, RxData);

			if (RxMsg.IDE == CAN_ID_STD) {
				OLED_ShowString(1, 6, "Std");
				OLED_ShowHexNum(2, 6, RxMsg.StdId, 8);

			} else if (RxMsg.IDE == CAN_ID_EXT) {
				OLED_ShowString(1, 6, "Ext");
				OLED_ShowHexNum(2, 6, RxMsg.ExtId, 8);
			}

			if (RxMsg.RTR == CAN_RTR_DATA) {
				OLED_ShowString(1, 10, "Data  ");
				OLED_ShowHexNum(3, 6, RxMsg.DLC, 1);
				OLED_ShowHexNum(4, 6, RxData[0], 2);
				OLED_ShowHexNum(4, 9, RxData[1], 2);
				OLED_ShowHexNum(4, 12, RxData[2], 2);
				OLED_ShowHexNum(4, 15, RxData[3], 2);
			} else if (RxMsg.RTR == CAN_RTR_REMOTE) {
				OLED_ShowString(1, 10, "Remote");
				OLED_ShowHexNum(3, 6, RxMsg.DLC, 1);
				OLED_ShowHexNum(4, 6, 0x00, 2);
				OLED_ShowHexNum(4, 9, 0x00, 2);
				OLED_ShowHexNum(4, 12, 0x00, 2);
				OLED_ShowHexNum(4, 15, 0x00, 2);
			}

		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
```
标准格式-扩展格式-数据帧-遥控帧 部分完成
2026-01-14

---

# 标识符过滤器-16位列表
## can部分
只需要更改can.c中过滤器的配制
```c
 /* USER CODE BEGIN CAN_Init 2 */

	CAN_FilterTypeDef sFilterConfig;
	sFilterConfig.FilterBank = 0; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT; /* 32位。*/

	sFilterConfig.FilterIdHigh = 0x234 << 5; /* 要过滤的ID高位 */
	sFilterConfig.FilterIdLow = 0x345 << 5; /* 要过滤的ID低位 */
	sFilterConfig.FilterMaskIdHigh = 0x456 << 5; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = 0x0000; /* 过滤器低16位每位必须匹配 */
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}

	sFilterConfig.FilterBank = 1; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT; /* 32位。*/

	sFilterConfig.FilterIdHigh = 0x123 << 5; /* 要过滤的ID高位 */
	sFilterConfig.FilterIdLow = 0x345 << 5; /* 要过滤的ID低位 */
	sFilterConfig.FilterMaskIdHigh = 0x456 << 5; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = 0x0000; /* 过滤器低16位每位必须匹配 */
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}

	HAL_CAN_Start(&hcan);
	/* USER CODE END CAN_Init 2 */
```
---
# 标识符过滤器-16位屏蔽
## can部分
```c
/* USER CODE BEGIN CAN_Init 2 */

	CAN_FilterTypeDef sFilterConfig;
	sFilterConfig.FilterBank = 0; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT; /* 32位。*/

	sFilterConfig.FilterIdHigh = 0x200 << 5; /* 要过滤的ID高位 */
	sFilterConfig.FilterMaskIdHigh = (0x700 << 5) | 0x10 | 0x8; /* 要过滤的ID低位 */
	sFilterConfig.FilterIdLow = 0x320 << 5; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = (0x7F0 << 5) | 0x10 | 0x8; /* 过滤器低16位每位必须匹配 */
	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}



	HAL_CAN_Start(&hcan);
	/* USER CODE END CAN_Init 2 */
```

## 主函数部分
```c
/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
//uint32_t TxID = 0x222;
//uint8_t TxLength = 4;
uint8_t TxDataArray[16][8] =
  {
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd },
    { 0x11, 0x22, 0x33, 0x44 },
    { 0xaa, 0xbb, 0xcc, 0xdd }

  };

CAN_TxHeaderTypeDef TxMsgArray[16] =
  {
    { .StdId = 0x100, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
	CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x101, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
	CAN_RTR_REMOTE, .DLC = 4 },
    { .StdId = 0x1FE, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x1FF, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x200, .ExtId = 0x00000000, .IDE =
    CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x201, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_REMOTE, .DLC = 4 },
    { .StdId = 0x2FE, .ExtId = 0x00000000, .IDE =
    CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x2FF, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x310, .ExtId = 0x00000000, .IDE =
    CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x311, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_REMOTE, .DLC = 4 },
    { .StdId = 0x31E, .ExtId = 0x00000000, .IDE =
    CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x31F, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x320, .ExtId = 0x00000000, .IDE =
    CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x321, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_REMOTE, .DLC = 4 },
    { .StdId = 0x32E, .ExtId = 0x00000000, .IDE =
    CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
    { .StdId = 0x32F, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
    CAN_RTR_DATA, .DLC = 4 } };

uint8_t pTxMsgArray = 0;

//uint32_t RxID;
//uint8_t RxLength;
uint8_t RxData[8] =
  { 0 };
CAN_RxHeaderTypeDef RxMsg =
  { 0 };

/* USER CODE END PV */
...
/* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {
      KeyNum = Key_GetNum ();

      if (KeyNum == 1)
	{
	  MyCAN_Transmit (TxMsgArray[pTxMsgArray], TxDataArray[pTxMsgArray]);
	  pTxMsgArray++;
	  pTxMsgArray %= 16;
	}

      if (MyCAN_ReceiveFlag ())
	{
	  MyCAN_Receive (&RxMsg, RxData);

	  if (RxMsg.IDE == CAN_ID_STD)
	    {
	      OLED_ShowString (1, 6, "Std");
	      OLED_ShowHexNum (2, 6, RxMsg.StdId, 8);

	    }
	  else if (RxMsg.IDE == CAN_ID_EXT)
	    {
	      OLED_ShowString (1, 6, "Ext");
	      OLED_ShowHexNum (2, 6, RxMsg.ExtId, 8);
	    }

	  if (RxMsg.RTR == CAN_RTR_DATA)
	    {
	      OLED_ShowString (1, 10, "Data  ");
	      OLED_ShowHexNum (3, 6, RxMsg.DLC, 1);
	      OLED_ShowHexNum (4, 6, RxData[0], 2);
	      OLED_ShowHexNum (4, 9, RxData[1], 2);
	      OLED_ShowHexNum (4, 12, RxData[2], 2);
	      OLED_ShowHexNum (4, 15, RxData[3], 2);
	    }
	  else if (RxMsg.RTR == CAN_RTR_REMOTE)
	    {
	      OLED_ShowString (1, 10, "Remote");
	      OLED_ShowHexNum (3, 6, RxMsg.DLC, 1);
	      OLED_ShowHexNum (4, 6, 0x00, 2);
	      OLED_ShowHexNum (4, 9, 0x00, 2);
	      OLED_ShowHexNum (4, 12, 0x00, 2);
	      OLED_ShowHexNum (4, 15, 0x00, 2);
	    }

	}
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
    }
  /* USER CODE END 3 */
```

---

# 标识符过滤器-32位列表
## 主函数部分
```c
/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
//uint32_t TxID = 0x222;
//uint8_t TxLength = 4;
uint8_t TxDataArray[7][8] = { { 0x11, 0x22, 0x33, 0x44 }, { 0xaa, 0xbb, 0xcc,
		0xdd }, { 0x11, 0x22, 0x33, 0x44 }, { 0xaa, 0xbb, 0xcc, 0xdd }, { 0xaa,
		0xbb, 0xcc, 0xdd }, { 0x11, 0x22, 0x33, 0x44 },
		{ 0xaa, 0xbb, 0xcc, 0xdd },

};

CAN_TxHeaderTypeDef TxMsgArray[7] = { { .StdId = 0x123, .ExtId = 0x00000000,
		.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 }, { .StdId = 0x234,
		.ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 },
		{ .StdId = 0x345, .ExtId = 0x00000000, .IDE = CAN_ID_STD, .RTR =
				CAN_RTR_DATA, .DLC = 4 }, { .StdId = 0x456, .ExtId = 0x00000000,
				.IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 4 }, { .StdId =
				0x000, .ExtId = 0x12345678, .IDE = CAN_ID_EXT, .RTR =
				CAN_RTR_DATA, .DLC = 4 }, { .StdId = 0x000, .ExtId = 0x0789ABCD,
				.IDE = CAN_ID_EXT, .RTR = CAN_RTR_DATA, .DLC = 4 }, { 0 } };

uint8_t pTxMsgArray = 0;

//uint32_t RxID;
//uint8_t RxLength;
uint8_t RxData[8] = { 0 };
CAN_RxHeaderTypeDef RxMsg = { 0 };

/* USER CODE END PV */
...
/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		KeyNum = Key_GetNum();

		if (KeyNum == 1) {
			MyCAN_Transmit(TxMsgArray[pTxMsgArray], TxDataArray[pTxMsgArray]);
			pTxMsgArray++;
			pTxMsgArray %= 6;
		}

		if (MyCAN_ReceiveFlag()) {
			MyCAN_Receive(&RxMsg, RxData);

			if (RxMsg.IDE == CAN_ID_STD) {
				OLED_ShowString(1, 6, "Std");
				OLED_ShowHexNum(2, 6, RxMsg.StdId, 8);

			} else if (RxMsg.IDE == CAN_ID_EXT) {
				OLED_ShowString(1, 6, "Ext");
				OLED_ShowHexNum(2, 6, RxMsg.ExtId, 8);
			}

			if (RxMsg.RTR == CAN_RTR_DATA) {
				OLED_ShowString(1, 10, "Data  ");
				OLED_ShowHexNum(3, 6, RxMsg.DLC, 1);
				OLED_ShowHexNum(4, 6, RxData[0], 2);
				OLED_ShowHexNum(4, 9, RxData[1], 2);
				OLED_ShowHexNum(4, 12, RxData[2], 2);
				OLED_ShowHexNum(4, 15, RxData[3], 2);
			} else if (RxMsg.RTR == CAN_RTR_REMOTE) {
				OLED_ShowString(1, 10, "Remote");
				OLED_ShowHexNum(3, 6, RxMsg.DLC, 1);
				OLED_ShowHexNum(4, 6, 0x00, 2);
				OLED_ShowHexNum(4, 9, 0x00, 2);
				OLED_ShowHexNum(4, 12, 0x00, 2);
				OLED_ShowHexNum(4, 15, 0x00, 2);
			}

		}
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
```
## can 部分
```c
/* USER CODE BEGIN CAN_Init 2 */

	CAN_FilterTypeDef sFilterConfig;
	sFilterConfig.FilterBank = 0; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; /* 32位。*/

	uint32_t ID1 = (0x123 << 21);
	sFilterConfig.FilterIdHigh = (ID1 >> 16) & 0xFFFF; /* 要过滤的ID高位 */
	sFilterConfig.FilterIdLow = ID1 & 0xFFFF; /* 要过滤的ID低位 */

	uint32_t ID2 = (0x12345678u << 3) | 0x4;
	sFilterConfig.FilterMaskIdHigh = (ID2 >> 16) & 0xFFFF; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = ID2 & 0xFFFF; /* 过滤器低16位每位必须匹配 */

	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}



	HAL_CAN_Start(&hcan);
	/* USER CODE END CAN_Init 2 */
```

#  标识符过滤器-32位屏蔽
## 主函数
```c
/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
//uint32_t TxID = 0x222;
//uint8_t TxLength = 4;
uint8_t TxDataArray[8][8] =
    {
	{
	0x11, 0x22, 0x33, 0x44
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},
	{
	0x11, 0x22, 0x33, 0x44
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},
	{
	0x11, 0x22, 0x33, 0x44
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},

    };

CAN_TxHeaderTypeDef TxMsgArray[8] =
    {
	{
	.StdId = 0x000, .ExtId = 0x12345600, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x12345601, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x123456FE, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x123456FF, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789AB00, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789AB01, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789ABFE, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789ABFF, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	}

    };

uint8_t pTxMsgArray = 0;

//uint32_t RxID;
//uint8_t RxLength;
uint8_t RxData[8] =
    {
    0
    };
CAN_RxHeaderTypeDef RxMsg =
    {
    0
    };

/* USER CODE END PV */
```
## can 部分
```c
/* USER CODE BEGIN CAN_Init 2 */

	CAN_FilterTypeDef sFilterConfig;
	sFilterConfig.FilterBank = 0; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; /* 32位。*/

	uint32_t ID1 = (0x12345600u << 3) | 0x4;
	sFilterConfig.FilterIdHigh = (ID1 >> 16) & 0xFFFF; /* 要过滤的ID高位 */
	sFilterConfig.FilterIdLow = ID1 & 0xFFFF; /* 要过滤的ID低位 */

	uint32_t ID2 = (0x1FFFFF00u << 3) | 0x4 | 0x2;
	sFilterConfig.FilterMaskIdHigh = (ID2 >> 16) & 0xFFFF; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = ID2 & 0xFFFF; /* 过滤器低16位每位必须匹配 */

	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}



	HAL_CAN_Start(&hcan);
	/* USER CODE END CAN_Init 2 */
```

#  标识符过滤器-只要遥控帧
## 主函数
```c
/* USER CODE BEGIN PV */
uint8_t KeyNum = 0;
//uint32_t TxID = 0x222;
//uint8_t TxLength = 4;
uint8_t TxDataArray[8][8] =
    {
	{
	0x11, 0x22, 0x33, 0x44
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},
	{
	0x11, 0x22, 0x33, 0x44
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},
	{
	0x11, 0x22, 0x33, 0x44
	},
	{
	0xaa, 0xbb, 0xcc, 0xdd
	},

    };

CAN_TxHeaderTypeDef TxMsgArray[8] =
    {
	{
	.StdId = 0x000, .ExtId = 0x12345600, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x12345601, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x123456FE, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x123456FF, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789AB00, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789AB01, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_DATA, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789ABFE, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_REMOTE, .DLC = 4
	},
	{
	.StdId = 0x000, .ExtId = 0x0789ABFF, .IDE = CAN_ID_EXT, .RTR =
	CAN_RTR_REMOTE, .DLC = 4
	}

    };

uint8_t pTxMsgArray = 0;

//uint32_t RxID;
//uint8_t RxLength;
uint8_t RxData[8] =
    {
    0
    };
CAN_RxHeaderTypeDef RxMsg =
    {
    0
    };

/* USER CODE END PV */
```
## can部分
```c
/* USER CODE BEGIN CAN_Init 2 */

	CAN_FilterTypeDef sFilterConfig;
	sFilterConfig.FilterBank = 0; /* 过滤器组0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK; /* 屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT; /* 32位。*/

	uint32_t ID1 =  0x2;
	sFilterConfig.FilterIdHigh = (ID1 >> 16) & 0xFFFF; /* 要过滤的ID高位 */
	sFilterConfig.FilterIdLow = ID1 & 0xFFFF; /* 要过滤的ID低位 */

	uint32_t ID2 =  0x2;
	sFilterConfig.FilterMaskIdHigh = (ID2 >> 16) & 0xFFFF; /* 过滤器高16位每位必须匹配 */
	sFilterConfig.FilterMaskIdLow = ID2 & 0xFFFF; /* 过滤器低16位每位必须匹配 */

	sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0; /* 过滤器被关联到FIFO 0 */
	sFilterConfig.FilterActivation = ENABLE; /* 使能过滤器 */
	sFilterConfig.SlaveStartFilterBank = 14;

	if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) {
		/* Filter configuration Error */
		Error_Handler();

	}



	HAL_CAN_Start(&hcan);
	/* USER CODE END CAN_Init 2 */
```