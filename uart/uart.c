//------------------------------------------------------------------------------
// This is Open source software. You can place this code on your site, but don't
// forget a link to my YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Это программное обеспечение распространяется свободно. Вы можете размещать
// его на вашем сайте, но не забудьте указать ссылку на мой YouTube-канал 
// "Электроника в объектике" https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ
// Автор: Надыршин Руслан / Nadyrshin Ruslan
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
// Функция возвращает наилучший делитель для получения заданного BaudRate при F_OSC = 8 MHz
//==============================================================================
uint16_t UART0_GetPrescaler(uint32_t BaudRate)
{
#if (F_CPU != 8)
#error Делители расчитаны на частоту тактирования AVR = 8 MHz!
#endif
  
  switch (BaudRate)
  {
  case 1200: return 832;        // 833,33333    0,1% ошибка
  case 2400: return 416;        // 416,66666    0,1% ошибка
  case 4800: return 207;        // 208,33333    0,2% ошибка
  case 9600: return 103;        // 104,16666    0,2% ошибка
  case 14400: return 68;        //  69,44444    0,6% ошибка
  case 19200: return 51;        //  52,08333    0,2% ошибка
  case 28800: return 34;        //  34,72222    0,8% ошибка
  case 38400: return 25;        //  26,04166    0,2% ошибка
  case 57600: return 16;        //  17,36111    2.1% ошибка
  case 76800: return 12;        //  13,02083    0,2% ошибка
  case 115200: return 8;        //   8,68055    3,5% ошибка
  default: return 0;
  }
}
//==============================================================================


//==============================================================================
// Функция обработки окончания входящего пакета (по интервалу молчания)
//==============================================================================
void RxIdle(void)
{
  if (IdleTimer == 0xFF)
    return;
  
  // Истекло заданное время молчания на приёмнике
  if (++IdleTimer == UART_RT_IDLE_TIME)
  {
    // Обрабатываем принятый пакет
    if (RxDataProcessFunc)
      RxDataProcessFunc();
    // Останавливаем счёт времени молчания
    tmr2_stop();
  }  
}
//==============================================================================


//==============================================================================
// Инициализация UART. RxFunc - функция, вызываемая при получении пакета с суффиксом Nextion
//==============================================================================
void UART_Init(uint32_t BaudRate, uart_RxFunc RxFunc)
{
  __disable_interrupt();

  RxDataProcessFunc = RxFunc;
  
  // Настраиваем ноги Tx,Rx
  DDRD |= (1 << 2);     // Tx
  DDRD &= ~(1 << 3);    // Rx
  
  // Настраиваем USART
  UCSR0A = (1 << U2X0);                                 // Удвоенная частота тактирования UART
  UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0); // Включаем Rx и Tx, включаем прерывание по Rx
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);               // 8 bit

  uint16_t Prescaler = UART0_GetPrescaler(BaudRate);
  UBRR0H = (Prescaler >> 8) & 0xF;
  UBRR0L = Prescaler & 0xFF;
  
  // Инициализируем счёт времени молчания по приёму
  tmr2_init(BaudRate / 10, RxIdle);
  IdleTimer = 0xFF;
  
  __enable_interrupt();
}
//==============================================================================


#if (UART_TX_MODE == UART_MODE_IRQ)
//==============================================================================
// Процедура стартует выдачу данных из циклического буфера если передача в данный момент не идёт
//==============================================================================
void UART_TxStart(void)
{
  // Если передача в данный момент не идёт
  if (!UARTstate.TxRun)
  {
    // Пишем первый байт на передачу
    UDR0 = TxBuff[TxRdIdx++];
    if (TxRdIdx == UartTxBuffSize)
      TxRdIdx = 0;
    // Декремент кол-ва байт на передачу в USART
    TxCntr--;
    // Ставим флаг передачи по USART в процессе
    UARTstate.TxRun = 1;
    // Разрешаем прерывание по освобождению регистра передачи по USART
    UCSR0B |= (1 << TXCIE0); // Включаем прерывание по Rx
  }
}
//==============================================================================


//==============================================================================
// Функция отправки данных (добавляет их в циклический буфер)
//==============================================================================
int8_t UART_Send(uint8_t *pBuff, uint16_t Len)
{
  UARTstate.TxOvr = 0;
  
  while (Len--)
  {
    TxBuff[TxWrIdx++] = *pBuff++;
    if (TxWrIdx == UartTxBuffSize)
      TxWrIdx = 0;
    
    if (++TxCntr == UartTxBuffSize)     // Буфер на передачу заполнен
    {
      UARTstate.TxOvr = 1;              // Оставим флаг переполнения буфера на передачу на всякий случай
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
// Функция отправки данных (работает с UART поллингом регистров)
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
// Функция возвращает кол-во байт в Rx буфере
//==============================================================================
uint16_t USART_RxDataToProc(void)
{
  return RxWrIdx >= RxRdIdx ? RxWrIdx - RxRdIdx : RxWrIdx + sizeof(RxBuff) - RxRdIdx;
}
//==============================================================================


//==============================================================================
// Функция получения очередного байта из Rx циклического буфера
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
// Обработчик прерывания по приёму байта по UART
//==============================================================================
#pragma vector=USART_RX_vect
__interrupt void UART_RX_ISR(void)
{
  uint8_t Byte = UDR0;
  RxBuff[RxWrIdx] = Byte;
  if (++RxWrIdx == sizeof(RxBuff))
    RxWrIdx = 0;

  // Начинаем счёт времени молчания на приёмнике
  IdleTimer = 0;
  tmr2_start();
    
  if (RxWrIdx == RxRdIdx)     // Буфер переполнился
  {
    RxWrIdx = RxRdIdx = 0;
    UARTstate.RxOvr = 1;
  }
    
  if (USART_RxDataToProc() == (UartRxBuffSize - 64))   // Буфер почти заполнен
  {
    if (RxDataProcessFunc)
      RxDataProcessFunc();
  }
}
//==============================================================================


#if (UART_TX_MODE == UART_MODE_IRQ)
//==============================================================================
// Обработчик прерывания по отправке байта по UART
//==============================================================================
#pragma vector=USART_TX_vect
__interrupt void UART_TX_ISR(void)
{
  if (TxCntr)                 // Есть ещё данные в буфере на выдачу
  {
    // Пишем очередной байт на передачу
    UDR0 = TxBuff[TxRdIdx++];
    if (TxRdIdx == UartTxBuffSize)
      TxRdIdx = 0;
    // Декремент кол-ва байт на передачу в USART
    TxCntr--;
  }
  else                        // Все данные из буфера переданы
  {
    // Запрещаем прерывание по освобождению регистра передачи по USART
    UCSR0B &= ~(1 << TXCIE0); 
    // Снимаем флаг передачи по USART в процессе
    UARTstate.TxRun = 0;
  }
}
//==============================================================================
#endif
