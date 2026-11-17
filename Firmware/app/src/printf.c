
#include "app/printf.h"
//
#include "main.h"
#include "usart.h"
//
#include "tx_api.h"
//
#include "app/robo-con.h"

#define CDC_MAX_TRANSPORT_SIZE 512

#define USB_TX_BUFFER_SIZE CDC_MAX_TRANSPORT_SIZE * 16U
#define USB_RX_BUFFER_SIZE CDC_MAX_TRANSPORT_SIZE * 4U
__attribute__RAM_D1__BSS__ uint8_t TX_Buffer_0[USB_TX_BUFFER_SIZE];
__attribute__RAM_D1__BSS__ uint8_t TX_Buffer_1[USB_TX_BUFFER_SIZE];
__attribute__RAM_D1__BSS__ uint8_t RX_Buffer[USB_RX_BUFFER_SIZE];

__attribute__RAM_D2__ uint8_t ex_tx[] = "SETUP CDC Thread\r\n";

uint16_t Buffer_len;
bool Tramsmit_request;
bool tx_cplt;
bool select_buffer;

TX_THREAD CDC_Thread;

__attribute__ITCM__ void CDC_thread(ULONG thred_input);

#define CDC_APP_STACK_SIZE                  512
#define CDC_APP_THREAD_PRIO                 10
#define CDC_APP_THREAD_PREEMPTION_THRESHOLD CDC_APP_THREAD_PRIO
#define CDC_APP_THREAD_TIME_SLICE           TX_NO_TIME_SLICE
#define CDC_APP_THREAD_AUTO_START           TX_AUTO_START
#define CDC_APP_THREAD_DONT_START           TX_DONT_START

UINT cdc_init(VOID *memory_ptr) {
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;

  CHAR *pointer;

  if (tx_byte_allocate(byte_pool, (VOID **)&pointer, CDC_APP_STACK_SIZE, TX_NO_WAIT) !=
      TX_SUCCESS) {
    printf("cdc pool allocate error\r\n");
    return CDC_POOLERROR;
  }

  if (tx_thread_create(
          &CDC_Thread,
          "CDC Thread",
          CDC_thread,
          0,
          pointer,
          CDC_APP_STACK_SIZE,
          CDC_APP_THREAD_PRIO,
          CDC_APP_THREAD_PREEMPTION_THRESHOLD,
          CDC_APP_THREAD_TIME_SLICE,
          CDC_APP_THREAD_AUTO_START
      )) {
    printf("cdc thread initializing error\r\n");
    return CDC_THREAD_ERROR;
  }

  HAL_UART_Transmit_DMA(&huart1, ex_tx, sizeof(ex_tx));

  return CDC_SUCCSES;
}

void CDC_thread(ULONG thred_input) {
  while (1) {
    __enable_irq();

    HAL_GPIO_TogglePin(LEDUSB_GPIO_Port, LEDUSB_Pin);

    if (Tramsmit_request && tx_cplt) {
      if (select_buffer) {
        HAL_UART_Transmit_DMA(&huart1, TX_Buffer_1, Buffer_len);
      } else {
        HAL_UART_Transmit_DMA(&huart1, TX_Buffer_0, Buffer_len);
      }

      select_buffer    = !select_buffer;
      Buffer_len       = 0;
      tx_cplt          = false;
      Tramsmit_request = false;
    }

    tx_thread_sleep(50);
  };
}

int _write(int file, char *ptr, int len) {
  if (select_buffer) {
    for (uint16_t i = 0; i < len; i++) {
      TX_Buffer_1[Buffer_len + i] = ptr[i];
    }
  } else {
    for (uint16_t i = 0; i < len; i++) {
      TX_Buffer_0[Buffer_len + i] = ptr[i];
    }
  }

  Buffer_len += len;
  Tramsmit_request = true;

  return len;
}

void print_tx_cplt(UART_HandleTypeDef *huart) {
  if (huart == &huart1) {
    tx_cplt = true;
  }
}