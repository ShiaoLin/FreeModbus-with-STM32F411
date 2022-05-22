/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mbport.h"
 
/* -----------------------    variables     ---------------------------------*/
extern UART_HandleTypeDef PORT_MODBUS;
 
/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
  /* If xRXEnable enable serial receive interrupts. If xTxENable enable
  * transmitter empty interrupts.
  */
  
  if (xRxEnable) {        
    __HAL_UART_ENABLE_IT(&PORT_MODBUS, UART_IT_RXNE);
  } else {    
    __HAL_UART_DISABLE_IT(&PORT_MODBUS, UART_IT_RXNE);
  }
  
  if (xTxEnable) {    
    __HAL_UART_ENABLE_IT(&PORT_MODBUS, UART_IT_TXE);
  } else {
    __HAL_UART_DISABLE_IT(&PORT_MODBUS, UART_IT_TXE);
  }  
}
 
BOOL  xMBPortSerialInit( UCHAR ucPort,
                         ULONG ulBaudRate,
                         UCHAR ucDataBits,
                         eMBParity eParity )
{
  /* 
  Do nothing, Initialization is handled by MX_USART1_UART_Init() 
  Fixed port, baudrate, databit and parity  
  */
  return TRUE;
}
 
BOOL
xMBPortSerialPutByte( UCHAR ucByte )
{
  /* Put a byte in the UARTs transmit buffer. This function is called
  * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
  * called. */
  return (HAL_UART_Transmit(&PORT_MODBUS, (uint8_t*)&ucByte, 1, 1) == HAL_OK);
}
 
BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
  /* Return the byte in the UARTs receive buffer. This function is called
  * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
  */  
  return (HAL_UART_Receive(&PORT_MODBUS, (uint8_t*)pucByte, 1, 1) == HAL_OK);
}
