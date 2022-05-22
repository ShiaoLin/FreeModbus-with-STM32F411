/* ----------------------- Modbus includes ----------------------------------*/
#include <stdio.h>
#include <string.h>
#include "mb.h"
#include "mbport.h"
#include "network.h"
#include "debug.h"

W5500Debugger_TypeDef stModbusW5500Debugger;

W5500TcpSocket_TypeDef hW5500MBTCP;

/*FreeMODBUS transmit and receive*/
#define MB_TCP_BUF_SIZE 256
uint8_t au8TCPRequestFrame[MB_TCP_BUF_SIZE]; //receive buffer
uint16_t u16TCPRequestLen;
uint8_t au8TCPResponseFrame[MB_TCP_BUF_SIZE]; //transmit buffer
uint16_t u16TCPResponseLen;

static const uint16_t u16MBTCPPortDefined = 502;
static const wiz_NetInfo stMBDefaultNetInfo =
    {
        .mac = {0x00, 0x08, 0xdc, 0x00, 0x00, 0xff},
        .ip = {192, 168, 1, 254},
        .sn = {255, 255, 255, 0},
        .gw = {192, 168, 1, 1},
        .dns = {0, 0, 0, 0},
        .dhcp = NETINFO_STATIC};

W5500TcpSocketErr_TypeDef eW5500SPIMBTCP_TCPSocket_Init(void)
{
  W5500TcpSocketErr_TypeDef stError = NOERR;

  vSetCurSpiPort(W5500SPIMBTCP);

  hW5500MBTCP.eSpiPort = W5500SPIMBTCP;
  hW5500MBTCP.stWizNetinfo = stMBDefaultNetInfo;
  hW5500MBTCP.u16Port = u16MBTCPPortDefined;
  hW5500MBTCP.bIsPhyConnected = false;
  hW5500MBTCP.eSockState = CLOSED;
  hW5500MBTCP.bIsSocketConnected = false;
  hW5500MBTCP.bIsSocketTxEnable = false;
  hW5500MBTCP.bIsSocketTxSent = false;
  hW5500MBTCP.bIsSocketRxEnable = false;
  hW5500MBTCP.bIsSocketRxReceived = false;
  hW5500MBTCP.pu8TxData = au8TCPResponseFrame;
  hW5500MBTCP.u16TxSize = MB_TCP_BUF_SIZE;
  hW5500MBTCP.pu8RxData = au8TCPRequestFrame;
  hW5500MBTCP.u16RxSize = MB_TCP_BUF_SIZE;

  wiz_NetTimeout timeout;
  //retry times
  timeout.retry_cnt = 5;
  //timeout value 2000 * 100us = 200ms, totally 5*200ms = 1s
  timeout.time_100us = 2000;
  wizchip_settimeout(&timeout);

  // WIZCHIP SOCKET Buffer initialize
  uint8_t u8SocketBufSize[2][8] = {{16, 0, 0, 0, 0, 0, 0, 0},
                                   {16, 0, 0, 0, 0, 0, 0, 0}};
  int8_t tmp = ctlwizchip(CW_INIT_WIZCHIP, (void *)u8SocketBufSize);

  ctlnetwork(CN_SET_NETINFO, (void *)&hW5500MBTCP.stWizNetinfo);
  wiz_NetInfo ref;
  ctlnetwork(CN_GET_NETINFO, (void *)&ref);
  if (memcmp(&ref, &hW5500MBTCP.stWizNetinfo, sizeof(wiz_NetInfo)) != 0)
  {
    //TODO: use another indication to check if initialize error occur.
    hW5500MBTCP.eSpiPort = W5500SPI_NONE;
    stError = INITIAL_ERR;
  }

  //TODO: test, sendto DISCARD_PORT 9, for arp table
  uint8_t broadcastIp[] = {255, 255, 255, 255};
  uint8_t discardport = 9;
  sendto(SOCKN, (uint8_t *)"0", sizeof("0"), broadcastIp, discardport);
  close(SOCKN);

  return stError;
}

BOOL xMBTCPPortInit(USHORT usTCPPort)
{
  return true;
}

void vMBTCPPortClose(void)
{
  //handled in vW5500ServerPoll().
}

void vMBTCPPortDisable(void)
{
  //handled in vW5500ServerPoll().
}

BOOL xMBTCPPortGetRequest(UCHAR **ppucMBTCPFrame, USHORT *usTCPLength)
{
  *ppucMBTCPFrame = hW5500MBTCP.pu8RxData;
  *usTCPLength = hW5500MBTCP.u16RxSize;
  /* Reset the buffer. */
  //hW5500MBTCP.u16RxSize = 0;
  return TRUE;
}

BOOL xMBTCPPortSendResponse(const UCHAR *pucMBTCPFrame, USHORT usTCPLength)
{
  hW5500MBTCP.u16TxSize = usTCPLength;
  memcpy(hW5500MBTCP.pu8TxData, pucMBTCPFrame, hW5500MBTCP.u16TxSize);
  //tx control flag
  hW5500MBTCP.bIsSocketTxEnable = true;
  return TRUE;
}

void vModbusTCPServerPoll(W5500TcpSocket_TypeDef *stMBW5500TcpSocket)
{
  if (stMBW5500TcpSocket->eSpiPort == W5500SPI_NONE)
  {
    eW5500SPIMBTCP_TCPSocket_Init();
  }

  vSetCurSpiPort(stMBW5500TcpSocket->eSpiPort);

  //vW5500Debugger(&stModbusW5500Debugger, eGetCurSpiPort());

  stMBW5500TcpSocket->bIsPhyConnected = (bool)(getPHYCFGR() & PHYCFGR_LNK_ON);
  if (!(stMBW5500TcpSocket->bIsPhyConnected))
  {
    if (stMBW5500TcpSocket->bIsSocketConnected)
    {
      vReleaseSocket();
      stMBW5500TcpSocket->bIsSocketConnected = false;
    }
  }
  stMBW5500TcpSocket->eSockState = (SocketState_TypeDef)getSn_SR(SOCKN);
  //get status of socket N
  switch (stMBW5500TcpSocket->eSockState)
  {
  case SOCK_CLOSED:
    stMBW5500TcpSocket->bIsSocketConnected = false;
    socket(SOCKN, Sn_MR_TCP, stMBW5500TcpSocket->u16Port, SF_TCP_NODELAY | SF_IO_NONBLOCK);
    break;
  case SOCK_INIT:
    listen(SOCKN);
    break;
  case SOCK_LISTEN:
    break;
  case SOCK_ESTABLISHED:
    stMBW5500TcpSocket->bIsSocketConnected = true;

    stMBW5500TcpSocket->u16RxSize = getSn_RX_RSR(SOCKN);
    if (stMBW5500TcpSocket->u16RxSize > 0)
    {
      //Modbus TCP request received
      eMBEventType eQueuedEventToStore;
      xMBPortEventGet(&eQueuedEventToStore);
      recv(SOCKN, stMBW5500TcpSocket->pu8RxData, stMBW5500TcpSocket->u16RxSize);
      eMBSwitchMode(MB_TCP);
      xMBPortEventPost(EV_FRAME_RECEIVED);
      eMBPoll();
      //Modbus TCP response send
      if (stMBW5500TcpSocket->bIsSocketTxEnable)
      {
        send(SOCKN, stMBW5500TcpSocket->pu8TxData, stMBW5500TcpSocket->u16TxSize);
        stMBW5500TcpSocket->bIsSocketTxSent = true;
        stMBW5500TcpSocket->bIsSocketTxEnable = false;
      }
      eMBSwitchMode(MB_RTU);
      xMBPortEventPost(eQueuedEventToStore);
    }
    // set auto keepalive 5sec(1*5)
    setSn_KPALVTR(SOCKN, 1);
    break;
  case SOCK_CLOSE_WAIT:
    disconnect(SOCKN);
    break;
  default:
    break;
  } //switch end
}
