/**
  ***************************************************************************************
  * @file     user_mb_app.h
  * @brief    Header for user_mb_app.c file.
  *           This file contains the common defines of the application.
  ***************************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _USER_MB_APP_H_
#define _USER_MB_APP_H_
 
/* Includes ------------------------------------------------------------------*/
#include "port.h"

/* Exported macro ------------------------------------------------------------*/
/* DiscreteInputs all address */
#define DISCRETE_INPUT_START 1
#define DISCRETE_INPUT_QTY   8
#if !(DISCRETE_INPUT_QTY % 8)
  #define DISCRETE_BYTE_ALIGNED
#endif
/* Coils all address */
#define COIL_START           1
#define COIL_QTY             8
#if !(COIL_QTY % 8)
  #define COIL_BYTE_ALIGNED
#endif
/* InputRegister all address */
#define INPUT_REG_START      1
#define INPUT_REG_QTY     2048
/* HoldingRegister all address */
#define HOLDING_REG_START    1
#define HOLDING_REG_QTY   2048

/* Exported typedef ----------------------------------------------------------*/
typedef enum {
  MODE8N2 = 1,
  MODE9O1,
  MODE9E1
}ModbusUartParity_Typedef;

typedef enum {
  DISCRETE_INPUT,
  COIL,
  INPUT_REG,
  HOLDING_REG
}ModbusRegType_Typedef;

/* Exported functions prototypes ---------------------------------------------*/
bool bModbus_Init(uint8_t ucSlaveAddr, uint32_t ulBaudrate, ModbusUartParity_Typedef eParityMode);
bool bModbus_SetSlaveAddr(uint8_t ucSlaveAddr);
uint8_t ucModbus_GetSlaveAddr(void);
bool bModbus_SetRtuBaudrate(uint32_t ulBaudrate);
uint32_t ulModbus_GetRtuBaudrate(void);
bool bModbus_SetRtuParity(ModbusUartParity_Typedef eParityMode);
ModbusUartParity_Typedef eModbus_GetRtuParity(void);
void vModbus_SetTcpNetCfg(wiz_NetInfo* hNetinfoToSet, uint16_t usPortToSet);
bool bModbus_ReadRegs(ModbusRegType_Typedef eRegType, int16_t* psData, const uint16_t usAddress, const uint16_t usNumOfObj);
bool bModbus_WriteRegs(ModbusRegType_Typedef eRegType, const int16_t* psData, const uint16_t usAddress, const uint16_t usNumOfObj);

#endif /*_USER_MB_APP_H_*/
