/**
  ***************************************************************************************
  * @file     user_mb_app.c
  * @brief    FreeMODBUS objects interface implementation
  * @change   DATE        VERSION        DETAIL
  *           20211019    1.0            JackZhang: basic architecture implementation
  *           20211020    1.1            JackZhang: implement getters
  *                                                 correct some variable naming & type
  ***************************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "mb.h"
#include "mbutils.h"
#include "user_mb_app.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RTU_UART_PORT 1U

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
//DiscreteInputs variables
static USHORT usRegDiscreteStart;
#ifdef DISCRETE_BYTE_ALIGNED
static UCHAR aucRegDiscrete[DISCRETE_INPUT_QTY >> 3];
#else
static UCHAR aucRegDiscrete[(DISCRETE_INPUT_QTY >> 3) + 1];
#endif
//Coils variables
static USHORT usRegCoilsStart;
#ifdef COIL_BYTE_ALIGNED
static UCHAR aucRegCoils[COIL_QTY >> 3];
#else
static UCHAR aucRegCoils[(COIL_QTY >> 3) + 1];
#endif
//InputRegister variables
static USHORT usRegInputStart;
static SHORT asRegInput[INPUT_REG_QTY];
//HoldingRegister variables
static USHORT usRegHoldingStart;
static SHORT asRegHolding[HOLDING_REG_QTY]; 
//use in stack and for lacking auguments passing
static uint8_t ucCurSlaveAddress;
static uint32_t ulCurBaudrate;
static ModbusUartParity_Typedef eCurMBParity;

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/**
  * @brief  Modbus slave discrete callback function.
  * @param  pucRegBuffer discrete buffer
  *         usAddress discrete address
  *         usNDiscrete discrete number
  * @return result
  */
eMBErrorCode eMBRegDiscreteCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
  eMBErrorCode eStatus = MB_ENOERR;
  USHORT       iRegIndex, iRegBitIndex, iNReg;

  iNReg = usNDiscrete / 8 + 1;

  /*it plus one in modbus function method. */
  usAddress--;

  if ((usAddress >= DISCRETE_INPUT_START) && (usAddress + usNDiscrete <= DISCRETE_INPUT_START + DISCRETE_INPUT_QTY))
  {
    iRegIndex = (USHORT)(usAddress - usRegDiscreteStart) / 8;
    iRegBitIndex = (USHORT)(usAddress - usRegDiscreteStart) % 8;

    while (iNReg > 0)
    {
      *pucRegBuffer++ = xMBUtilGetBits(&aucRegDiscrete[iRegIndex++], iRegBitIndex, 8);
      iNReg--;
    }
    pucRegBuffer--;
    /* last discrete */
    usNDiscrete = usNDiscrete % 8;
    /* filling zero to high bit */
    *pucRegBuffer = *pucRegBuffer << (8 - usNDiscrete);
    *pucRegBuffer = *pucRegBuffer >> (8 - usNDiscrete);
  }
  else
  {
    eStatus = MB_ENOREG;
  }

  return eStatus;
}

/**
  * @brief  Modbus slave coils callback function.
  * @param  pucRegBuffer coils buffer
  *         usAddress coils address
  *         usNCoils coils number
  *         eMode read or write
  * @return result
  */
eMBErrorCode eMBRegCoilsCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode)
{
  eMBErrorCode eStatus = MB_ENOERR;
  USHORT       iRegIndex, iRegBitIndex, iNReg;

  iNReg = usNCoils / 8 + 1;

  /* it already plus one in modbus function method. */
  usAddress--;

  if ((usAddress >= COIL_START) && (usAddress + usNCoils <= COIL_START + COIL_QTY))
  {
    iRegIndex = (USHORT)(usAddress - usRegCoilsStart) / 8;
    iRegBitIndex = (USHORT)(usAddress - usRegCoilsStart) % 8;
    switch (eMode)
    {
      /* read current coil values from the protocol stack. */
    case MB_REG_READ:
      while (iNReg > 0)
      {
        *pucRegBuffer++ = xMBUtilGetBits(&aucRegCoils[iRegIndex++], iRegBitIndex, 8);
        iNReg--;
      }
      pucRegBuffer--;
      /* last coils */
      usNCoils = usNCoils % 8;
      /* filling zero to high bit */
      *pucRegBuffer = *pucRegBuffer << (8 - usNCoils);
      *pucRegBuffer = *pucRegBuffer >> (8 - usNCoils);
      break;

      /* write current coil values with new values from the protocol stack. */
    case MB_REG_WRITE:
      while (iNReg > 1)
      {
        xMBUtilSetBits(&aucRegCoils[iRegIndex++], iRegBitIndex, 8, *pucRegBuffer++);
        iNReg--;
      }
      /* last coils */
      usNCoils = usNCoils % 8;
      /* xMBUtilSetBits has bug when ucNBits is zero */
      if (usNCoils != 0)
      {
        xMBUtilSetBits(&aucRegCoils[iRegIndex++], iRegBitIndex, usNCoils, *pucRegBuffer++);
      }
      break;
    }
  }
  else
  {
    eStatus = MB_ENOREG;
  }
  return eStatus;
}

/**
  * @brief  Modbus slave input register callback function.
  * @param  pucRegBuffer input register buffer
  *         usAddress input register address
  *         usNRegs input register number
  * @return result
  */
eMBErrorCode eMBRegInputCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
  eMBErrorCode eStatus = MB_ENOERR;
  USHORT       iRegIndex;

  /* it already plus one in modbus function method. */
  usAddress--;

  if ((usAddress >= INPUT_REG_START) && (usAddress + usNRegs <= INPUT_REG_START + INPUT_REG_QTY))
  {
    iRegIndex = usAddress - usRegInputStart;
    while (usNRegs > 0)
    {
      *pucRegBuffer++ = (UCHAR)(asRegInput[iRegIndex] >> 8);
      *pucRegBuffer++ = (UCHAR)(asRegInput[iRegIndex] & 0xFF);
      iRegIndex++;
      usNRegs--;
    }
  }
  else
  {
    eStatus = MB_ENOREG;
  }

  return eStatus;
}

/**
  * @brief  Modbus slave holding register callback function.
  * @param  pucRegBuffer holding register buffer
  *         usAddress holding register address
  *         usNRegs holding register number
  *         eMode read or write
  * @return result
  */
eMBErrorCode eMBRegHoldingCB(UCHAR* pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode)
{
  eMBErrorCode    eStatus = MB_ENOERR;
  USHORT          iRegIndex;

  /* it already plus one in modbus function method. */
  usAddress--;

  if ((usAddress >= HOLDING_REG_START) && (usAddress + usNRegs <= HOLDING_REG_START + HOLDING_REG_QTY))
  {
    iRegIndex = usAddress - usRegHoldingStart;
    switch (eMode)
    {
      /* read current register values from the protocol stack. */
    case MB_REG_READ:
      while (usNRegs > 0)
      {
        *pucRegBuffer++ = (UCHAR)(asRegHolding[iRegIndex] >> 8);
        *pucRegBuffer++ = (UCHAR)(asRegHolding[iRegIndex] & 0xFF);
        iRegIndex++;
        usNRegs--;
      }
      break;

      /* write current register values with new values from the protocol stack. */
    case MB_REG_WRITE:
      while (usNRegs > 0)
      {
        asRegHolding[iRegIndex] = *pucRegBuffer++ << 8;
        asRegHolding[iRegIndex] |= *pucRegBuffer++;
        iRegIndex++;
        usNRegs--;
      }
      break;
    }
  }
  else
  {
    eStatus = MB_ENOREG;
  }
  return eStatus;
}

/**
  * @brief  modbus initial function
  * @param  uint8_t ucSlaveAddr
  *         uint32_t ulBaudrate
  *         ModbusUartParity_Typedef eParityMode
  * @return bool: is succeed
  */
bool bModbus_Init(uint8_t ucSlaveAddr, uint32_t ulBaudrate, ModbusUartParity_Typedef eParityMode)
{
  ucCurSlaveAddress = ucSlaveAddr;
  ulCurBaudrate = ulBaudrate;
  eCurMBParity = eParityMode;

  eMBDisable();
  //do eMBTCPInit before eMBInit
  eMBTCPInit(hW5500MBTCP.u16Port);
  eMBInit(MB_RTU, ucCurSlaveAddress, RTU_UART_PORT, ulCurBaudrate, (eMBParity)eCurMBParity);

  switch (eParityMode)
  {
  case MODE8N2:
    PORT_MODBUS.Init.WordLength = UART_WORDLENGTH_8B;
    PORT_MODBUS.Init.Parity = UART_PARITY_NONE;
    PORT_MODBUS.Init.StopBits = UART_STOPBITS_2;
    break;
  case MODE9O1:
    PORT_MODBUS.Init.WordLength = UART_WORDLENGTH_9B;
    PORT_MODBUS.Init.Parity = UART_PARITY_ODD;
    PORT_MODBUS.Init.StopBits = UART_STOPBITS_1;
    break;
  case MODE9E1:
    PORT_MODBUS.Init.WordLength = UART_WORDLENGTH_9B;
    PORT_MODBUS.Init.Parity = UART_PARITY_EVEN;
    PORT_MODBUS.Init.StopBits = UART_STOPBITS_1;
    break;
  }

  if ((MB_ENOERR != eMBEnable()) && (HAL_OK != HAL_UART_Init(&PORT_MODBUS)))
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
  * @brief  modbus set slave address function
  * @param  uint8_t: ucSlaveAddr
  * @return bool: is succeed
  */
bool bModbus_SetSlaveAddr(uint8_t ucSlaveAddr)
{
  ucCurSlaveAddress = ucSlaveAddr;

  eMBDisable();
  eMBInit(MB_RTU, ucCurSlaveAddress, RTU_UART_PORT, ulCurBaudrate, (eMBParity)eCurMBParity);

  if (MB_ENOERR != eMBEnable())
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
  * @brief  modbus get slave address function
  * @param  void
  * @return uint8_t: ucCurSlaveAddress
  */
uint8_t ucModbus_GetSlaveAddr(void)
{
  return ucCurSlaveAddress;
}

/**
  * @brief  modbus set uart baudrate function
  * @param  uint32_t: ulBaudrate
  * @return bool: is succeed
  */
bool bModbus_SetRtuBaudrate(uint32_t ulBaudrate)
{
  ulCurBaudrate = ulBaudrate;

  eMBDisable();
  eMBInit(MB_RTU, ucCurSlaveAddress, RTU_UART_PORT, ulCurBaudrate, (eMBParity)eCurMBParity);

  PORT_MODBUS.Init.BaudRate = ulBaudrate;

  if ((MB_ENOERR != eMBEnable()) && (HAL_OK != HAL_UART_Init(&PORT_MODBUS)))
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
  * @brief  modbus get uart baudrate function
  * @param  void
  * @return uint8_t: ulCurBaudrate
  */
uint32_t ulModbus_GetRtuBaudrate(void)
{
  return ulCurBaudrate;
}
/**
  * @brief  modbus set uart parity function
  * @param  ModbusUartParity_Typedef: eParityMode
  * @return bool: is succeed
  */
bool bModbus_SetRtuParity(ModbusUartParity_Typedef eParityMode)
{
  eCurMBParity = eParityMode;

  eMBDisable();
  eMBInit(MB_RTU, ucCurSlaveAddress, RTU_UART_PORT, ulCurBaudrate, (eMBParity)eCurMBParity);

  switch (eParityMode)
  {
  case MODE8N2:
    PORT_MODBUS.Init.WordLength = UART_WORDLENGTH_8B;
    PORT_MODBUS.Init.Parity = UART_PARITY_NONE;
    PORT_MODBUS.Init.StopBits = UART_STOPBITS_2;
    break;
  case MODE9O1:
    PORT_MODBUS.Init.WordLength = UART_WORDLENGTH_9B;
    PORT_MODBUS.Init.Parity = UART_PARITY_ODD;
    PORT_MODBUS.Init.StopBits = UART_STOPBITS_1;
    break;
  case MODE9E1:
    PORT_MODBUS.Init.WordLength = UART_WORDLENGTH_9B;
    PORT_MODBUS.Init.Parity = UART_PARITY_EVEN;
    PORT_MODBUS.Init.StopBits = UART_STOPBITS_1;
    break;
  }

  if ((MB_ENOERR != eMBEnable()) && (HAL_OK != HAL_UART_Init(&PORT_MODBUS)))
  {
    return false;
  }
  else
  {
    return true;
  }
}

/**
  * @brief  modbus get uart parity function
  * @param  void
  * @return ModbusUartParity_Typedef: eCurMBParity
  */
ModbusUartParity_Typedef eModbus_GetRtuParity(void)
{
  return eCurMBParity;
}
/**
  * @brief  modbus set tcp network config function
  * @param  wiz_NetInfo*: hNetinfoToSet
  *         uint16_t: usPortToSet
  * @return void
  */
void vModbus_SetTcpNetCfg(wiz_NetInfo* hNetinfoToSet, uint16_t usPortToSet)
{
  vW5500SetNetInfo(&hW5500MBTCP, hNetinfoToSet, usPortToSet);
}

/**
  * @brief  modbus read regs function
  * @param  ModbusRegType_Typedef: type of regs <DISCRETE_INPUT, COIL, INPUT_REG, HOLDING_REG>
  *         int16_t*: psData
  *         const uint16_t: usAddress
  *         const uint16_t: usNumOfObj
  * @return bool: is succeed
  */
bool bModbus_ReadRegs(ModbusRegType_Typedef eRegType, int16_t* psData, const uint16_t usAddress, const uint16_t usNumOfObj)
{
  switch (eRegType)
  {
  case DISCRETE_INPUT:
    if ((usAddress >= DISCRETE_INPUT_START) && (usAddress + usNumOfObj <= DISCRETE_INPUT_START + DISCRETE_INPUT_QTY))
    {
      uint16_t usIterator = usNumOfObj / 8 + 1;
      uint16_t usRegIndex = (uint16_t)(usAddress - usRegDiscreteStart) / 8;
      uint16_t usRegBitOffset = (uint16_t)(usAddress - usRegDiscreteStart) % 8;

      while (usIterator-- > 0)
      {
        *psData++ = (SHORT)xMBUtilGetBits(&aucRegDiscrete[usRegIndex++], usRegBitOffset, 8);
      }
      psData--;
      /* last discrete */
      /* filling zero to high bit */
      *psData = (SHORT)usNumOfObj % 8;
      return true;
    }
    else
    {
      return false;
    }
  case COIL:
    if ((usAddress >= COIL_START) && (usAddress + usNumOfObj <= COIL_START + COIL_QTY))
    {
      uint16_t usIterator = usNumOfObj / 8 + 1;
      uint16_t usRegIndex = (uint16_t)(usAddress - usRegCoilsStart) / 8;
      uint16_t usRegBitOffset = (uint16_t)(usAddress - usRegCoilsStart) % 8;

      while (usIterator-- > 0)
      {
        *psData++ = (SHORT)xMBUtilGetBits(&aucRegCoils[usRegIndex++], usRegBitOffset, 8);
      }
      psData--;
      /* last discrete */
      /* filling zero to high bit */
      *psData = (SHORT)usNumOfObj % 8;
      return true;
    }
    else
    {
      return false;
    }
  case INPUT_REG:
    if ((usAddress >= INPUT_REG_START) && (usAddress + usNumOfObj <= INPUT_REG_START + INPUT_REG_QTY))
    {
      uint16_t usIterator = usNumOfObj;
      uint16_t usRegIndex = usAddress - usRegInputStart;
      /* write current register values with new values from the protocol stack. */
      while (usIterator-- > 0)
      {
        *psData++ = asRegInput[usRegIndex++];
      }
      return true;
    }
    else
    {
      return false;
    }
  case HOLDING_REG:
    if ((usAddress >= INPUT_REG_START) && (usAddress + usNumOfObj <= INPUT_REG_START + INPUT_REG_QTY))
    {
      uint16_t usIterator = usNumOfObj;
      uint16_t usRegIndex = usAddress - usRegHoldingStart;
      /* write current register values with new values from the protocol stack. */
      while (usIterator-- > 0)
      {
        *psData++ = asRegHolding[usRegIndex++];
      }
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}

/**
  * @brief  modbus write regs function
  * @param  ModbusRegType_Typedef: type of regs <DISCRETE_INPUT, COIL, INPUT_REG, HOLDING_REG>
  *         const int16_t*: psData
  *         const uint16_t: usAddress
  *         const uint16_t: usNumOfObj
  * @return bool: is succeed
  */
bool bModbus_WriteRegs(ModbusRegType_Typedef eRegType, const int16_t* psData, const uint16_t usAddress, const uint16_t usNumOfObj)
{
  switch (eRegType)
  {
  case DISCRETE_INPUT:
    if ((usAddress >= DISCRETE_INPUT_START) && (usAddress + usNumOfObj <= DISCRETE_INPUT_START + DISCRETE_INPUT_QTY))
    {
      uint16_t usIterator = usNumOfObj / 8 + 1;
      uint16_t usRegIndex = (uint16_t)(usAddress - usRegDiscreteStart) / 8;
      uint16_t usRegBitOffset = (uint16_t)(usAddress - usRegDiscreteStart) % 8;

      while (usIterator-- > 1)
      {
        xMBUtilSetBits(&aucRegDiscrete[usRegIndex++], usRegBitOffset, 8, *psData++);
      }
      /* last coils */
      /* xMBUtilSetBits has bug when ucNBits is zero */
      if (usNumOfObj % 8 != 0)
      {
        xMBUtilSetBits(&aucRegDiscrete[usRegIndex++], usRegBitOffset, usNumOfObj % 8, *psData++);
      }
      return true;
    }
    else
    {
      return false;
    }
  case COIL:
    if ((usAddress >= COIL_START) && (usAddress + usNumOfObj <= COIL_START + COIL_QTY))
    {
      uint16_t usIterator = usNumOfObj / 8 + 1;
      uint16_t usRegIndex = (USHORT)(usAddress - usRegCoilsStart) / 8;
      uint16_t usRegBitOffset = (USHORT)(usAddress - usRegCoilsStart) % 8;

      while (usIterator-- > 1)
      {
        xMBUtilSetBits(&aucRegCoils[usRegIndex++], usRegBitOffset, 8, *psData++);
      }

      /* last coils */
      /* xMBUtilSetBits has bug when ucNBits is zero */
      if (usNumOfObj % 8 != 0)
      {
        xMBUtilSetBits(&aucRegCoils[usRegIndex++], usRegBitOffset, usNumOfObj % 8, *psData++);
      }
      return true;
    }
    else
    {
      return false;
    }
  case INPUT_REG:
    if ((usAddress >= INPUT_REG_START) && (usAddress + usNumOfObj <= INPUT_REG_START + INPUT_REG_QTY))
    {
      uint16_t usIterator = usNumOfObj;
      uint16_t usRegIndex = usAddress - usRegInputStart;
      /* write current register values with new values from the protocol stack. */
      while (usIterator-- > 0)
      {
        asRegInput[usRegIndex++] = *psData++;
      }
      return true;
    }
    else
    {
      return false;
    }
  case HOLDING_REG:
    if ((usAddress >= INPUT_REG_START) && (usAddress + usNumOfObj <= INPUT_REG_START + INPUT_REG_QTY))
    {
      uint16_t usIterator = usNumOfObj;
      uint16_t usRegIndex = usAddress - usRegHoldingStart;
      /* write current register values with new values from the protocol stack. */
      while (usIterator-- > 0)
      {
        asRegHolding[usRegIndex++] = *psData++;
      }
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}
