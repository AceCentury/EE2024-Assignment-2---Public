//Helper include to add in the missing UART SendString function.

#ifndef __MY_UART_H
#define __MY_UART_H
#include "LPC17xx.h"
#include "lpc_types.h"

/* UART call-back function type definitions */
/** UART Receive Call-back function type */
typedef void (fnRxCbs_Type)(void);
/** UART Transmit Call-back function type */
typedef void (fnTxCbs_Type)(void);
/** UART Auto-Baudrate Call-back function type */
typedef void (fnABCbs_Type)(uint32_t bABIntType);
/** UART Error Call-back function type */
typedef void (fnErrCbs_Type)(uint8_t bError);
/** UART1 modem status interrupt callback type */
typedef void (fnModemCbs_Type)(uint8_t ModemStatus);

uint32_t UART_SendString(LPC_UART_TypeDef *UARTx, uint8_t *txbuf);
void UART_SetupCbs(LPC_UART_TypeDef *UARTx, uint8_t CbType, void *pfnCbs);
void UART3_StdIntHandler(void);
uint8_t UART_ReceiveData(LPC_UART_TypeDef* UARTx);

#endif /* end __MY_UART_H */
