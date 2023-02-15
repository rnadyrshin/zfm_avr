//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// ��� ����������� ����������� ���������������� ��������. �� ������ ���������
// ��� �� ����� �����, �� �� �������� ������� ������ �� ��� YouTube-����� 
// "����������� � ���������" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// �����: �������� ������ / Nadyrshin Ruslan
//------------------------------------------------------------------------------
#include <ioavr.h>
#include <inavr.h>
#include <delay.h>
#include <uart.h>
#include <timers.h>


#if (UART_TX_MODE == UART_MODE_IRQ)
static uint8_t TxBuff[UartTxBuffSize];
static uint16_t TxRdIdx = 0, TxWrIdx = 0, TxCntr = 0;
#endif

#if (UART_TX_MODE == UART_MODE_POLLING)
static uint32_t TOcntr;
#endif

static uint8_t RxBuff[UartRxBuffSize];
static uint16_t RxRdIdx = 0;
static uint16_t RxWrIdx = 0;
static uart_RxFunc RxDataProcessFunc = 0;
static uint8_t IdleTimer;

typedef struct
{
  uint8_t TxRun:1;
  uint8_t TxOvr:1;
  uint8_t RxOvr:1;
}
tUARTstate;
tUARTstate UARTstate;


//==============================================================================
// ������� ���������� ��������� �������� ��� ��������� ��������� BaudRate ��� F_OSC = 8 MHz
//==============================================================================
uint16_t UART0_GetPrescaler(uint32_t BaudRate)
{
#if (F_CPU != 8)
#error �������� ��������� �� ������� ������������ AVR = 8 MHz!
#endif
  
  switch (BaudRate)
  {
  case 1200: return 832;        // 833,33333    0,1% ������
  case 2400: return 416;        // 416,66666    0,1% ������
  case 4800: return 207;        // 208,33333    0,2% ������
  case 9600: return 103;        // 104,16666    0,2% ������
  case 14400: return 68;        //  69,44444    0,6% ������
  case 19200: return 51;        //  52,08333    0,2% ������
  case 28800: return 34;        //  34,72222    0,8% ������
  case 38400: return 25;        //  26,04166    0,2% ������
  case 57600: return 16;        //  17,36111    2.1% ������
  case 76800: return 12;        //  13,02083    0,2% ������
  case 115200: return 8;        //   8,68055    3,5% ������
  default: return 0;
  }
}
//==============================================================================


//==============================================================================
// ������� ��������� ��������� ��������� ������ (�� ��������� ��������)
//==============================================================================
void RxIdle(void)
{
  if (IdleTimer == 0xFF)
    return;
  
  // ������� �������� ����� �������� �� ��������
  if (++IdleTimer == UART_RT_IDLE_TIME)
  {
    // ������������ �������� �����
    if (RxDataProcessFunc)
      RxDataProcessFunc();
    // ������������� ���� ������� ��������
    tmr2_stop();
  }  
}
//==============================================================================


//==============================================================================
// ������������� UART. RxFunc - �������, ���������� ��� ��������� ������ � ��������� Nextion
//==============================================================================
void UART_Init(uint32_t BaudRate, uart_RxFunc RxFunc)
{
  __disable_interrupt();

  RxDataProcessFunc = RxFunc;
  
  // ����������� ���� Tx,Rx
  DDRD |= (1 << 2);     // Tx
  DDRD &= ~(1 << 3);    // Rx
  
  // ����������� USART
  UCSR0A = (1 << U2X0);                                 // ��������� ������� ������������ UART
  UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0); // �������� Rx � Tx, �������� ���������� �� Rx
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               // 8 bit

  uint16_t Prescaler = UART0_GetPrescaler(BaudRate);
  UBRR0H = (Prescaler >> 8) & 0xF;
  UBRR0L = Prescaler & 0xFF;
  
  // �������������� ���� ������� �������� �� �����
  tmr2_init(BaudRate / 10, RxIdle);
  IdleTimer = 0xFF;
  
  __enable_interrupt();
}
//==============================================================================


#if (UART_TX_MODE == UART_MODE_IRQ)
//==============================================================================
// ��������� �������� ������ ������ �� ������������ ������ ���� �������� � ������ ������ �� ���
//==============================================================================
void UART_TxStart(void)
{
  // ���� �������� � ������ ������ �� ���
  if (!UARTstate.TxRun)
  {
    // ����� ������ ���� �� ��������
    UDR0 = TxBuff[TxRdIdx++];
    if (TxRdIdx == UartTxBuffSize)
      TxRdIdx = 0;
    // ��������� ���-�� ���� �� �������� � USART
    TxCntr--;
    // ������ ���� �������� �� USART � ��������
    UARTstate.TxRun = 1;
    // ��������� ���������� �� ������������ �������� �������� �� USART
    UCSR0B |= (1 << TXCIE0); // �������� ���������� �� Rx
  }
}
//==============================================================================


//==============================================================================
// ������� �������� ������ (��������� �� � ����������� �����)
//==============================================================================
int8_t UART_Send(uint8_t *pBuff, uint16_t Len)
{
  UARTstate.TxOvr = 0;
  
  while (Len--)
  {
    TxBuff[TxWrIdx++] = *pBuff++;
    if (TxWrIdx == UartTxBuffSize)
      TxWrIdx = 0;
    
    if (++TxCntr == UartTxBuffSize)     // ����� �� �������� ��������
    {
      UARTstate.TxOvr = 1;              // ������� ���� ������������ ������ �� �������� �� ������ ������
      TxWrIdx = TxCntr = 0;
      return UART_ERR_BUFF_OVF;
    }
  }
  
  UART_TxStart();
  
  return UART_ERR_OK;
}
//==============================================================================
#endif


#if (UART_TX_MODE == UART_MODE_POLLING)
//==============================================================================
// ������� �������� ������ (�������� � UART ��������� ���������)
//==============================================================================
int8_t UART_Send(uint8_t *pBuff, uint16_t Len)
{
  UARTstate.TxOvr = 0;
  
  while (Len--)
  {
    UDR0 = *(pBuff++);

    TOcntr = UART_XT_TIMEOUT_BYTE;
    while ((!(UCSR0A & (1<<UDRE0))) && TOcntr)
    {
      TOcntr--;
      delay_us(10);
    }
    if (!TOcntr)
      return UART_ERR_HW_TIMEOUT;
  }

  return UART_ERR_OK;
}
//==============================================================================
#endif


//==============================================================================
// ������� ���������� ���-�� ���� � Rx ������
//==============================================================================
uint16_t USART_RxDataToProc(void)
{
  return RxWrIdx >= RxRdIdx ? RxWrIdx - RxRdIdx : RxWrIdx + sizeof(RxBuff) - RxRdIdx;
}
//==============================================================================


//==============================================================================
// ������� ��������� ���������� ����� �� Rx ������������ ������
//==============================================================================
uint8_t USART_GetByteToProc(void)
{
  uint8_t Byte = RxBuff[RxRdIdx];
  if (++RxRdIdx == sizeof(RxBuff))
    RxRdIdx = 0;
  
  return Byte;
}
//==============================================================================


//==============================================================================
// ���������� ���������� �� ����� ����� �� UART
//==============================================================================
#pragma vector=USART_RX_vect
__interrupt void UART_RX_ISR(void)
{
  uint8_t Byte = UDR0;
  RxBuff[RxWrIdx] = Byte;
  if (++RxWrIdx == sizeof(RxBuff))
    RxWrIdx = 0;

  // �������� ���� ������� �������� �� ��������
  IdleTimer = 0;
  tmr2_start();
    
  if (RxWrIdx == RxRdIdx)     // ����� ������������
  {
    RxWrIdx = RxRdIdx = 0;
    UARTstate.RxOvr = 1;
  }
    
  if (USART_RxDataToProc() == (UartRxBuffSize - 64))   // ����� ����� ��������
  {
    if (RxDataProcessFunc)
      RxDataProcessFunc();
  }
}
//==============================================================================


#if (UART_TX_MODE == UART_MODE_IRQ)
//==============================================================================
// ���������� ���������� �� �������� ����� �� UART
//==============================================================================
#pragma vector=USART_TX_vect
__interrupt void UART_TX_ISR(void)
{
  if (TxCntr)                 // ���� ��� ������ � ������ �� ������
  {
    // ����� ��������� ���� �� ��������
    UDR0 = TxBuff[TxRdIdx++];
    if (TxRdIdx == UartTxBuffSize)
      TxRdIdx = 0;
    // ��������� ���-�� ���� �� �������� � USART
    TxCntr--;
  }
  else                        // ��� ������ �� ������ ��������
  {
    // ��������� ���������� �� ������������ �������� �������� �� USART
    UCSR0B &= ~(1 << TXCIE0); 
    // ������� ���� �������� �� USART � ��������
    UARTstate.TxRun = 0;
  }
}
//==============================================================================
#endif
