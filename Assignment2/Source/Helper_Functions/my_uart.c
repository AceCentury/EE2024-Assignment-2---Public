//Helper file to add in the missing UART SendString function.

#include "my_uart.h"
#include "lpc17xx_uart.h"
#include <string.h>

/**
 * @brief UART call-back function type definitions
 */
typedef struct {
	fnTxCbs_Type *pfnTxCbs; 	// Transmit callback
	fnRxCbs_Type *pfnRxCbs;		// Receive callback
	fnABCbs_Type *pfnABCbs;		// Auto-Baudrate callback
	fnErrCbs_Type *pfnErrCbs;	// Error callback
} UART_CBS_Type;

/** Call-back function pointer data */
UART_CBS_Type uartCbsDat[4] = {
		{NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL},
		{NULL, NULL, NULL, NULL},
};

/** UART1 modem status interrupt callback pointer data */
fnModemCbs_Type *pfnModemCbs = NULL;

/**
 * @brief		Get UART number due to UART peripheral pointer
 * @param[in]	UARTx	UART pointer
 * @return		UART number
 */
uint8_t getUartNum(LPC_UART_TypeDef *UARTx) {
	if (UARTx == (LPC_UART_TypeDef*)LPC_UART0) return (0);
	else if (UARTx == (LPC_UART_TypeDef *)LPC_UART1) return (1);
	else if (UARTx == LPC_UART2) return (2);
	else return (3);
}

/*********************************************************************//**
 * @brief		General UART interrupt handler and router
 * @param[in]	UARTx	Selected UART peripheral, should be UART0..3
 * @return		None
 *
 * Note:
 * - Handles transmit, receive, and status interrupts for the UART.
 * Based on the interrupt status, routes the interrupt to the
 * respective call-back to be handled by the user application using
 * this driver.
 * - If callback is not installed, corresponding interrupt will be disabled
 * - All these interrupt source below will be checked:
 *   		- Transmit Holding Register Empty.
 * 			- Received Data Available and Character Time Out.
 * 			- Receive Line Status (not implemented)
 * 			- End of auto-baud interrupt (not implemented)
 * 			- Auto-Baudrate Time-Out interrupt (not implemented)
 * 			- Modem Status interrupt (UART0 Modem functionality)
 * 			- CTS signal transition interrupt (UART0 Modem functionality)
 **********************************************************************/
void UART_GenIntHandler(LPC_UART_TypeDef *UARTx)
{
	uint8_t pUart, modemsts;
	uint32_t intsrc, tmp, tmp1;

	pUart = getUartNum(UARTx);

	/* Determine the interrupt source */
	intsrc = UARTx->IIR;
	tmp = intsrc & UART_IIR_INTID_MASK;

	/*
	 * In case of using UART1 with full modem,
	 * interrupt ID = 0 that means modem status interrupt has been detected
	 */
	if (pUart == 1) {
		if (tmp == 0){
			// Check Modem status
			modemsts = LPC_UART1->MSR & UART1_MSR_BITMASK;
			// Call modem status call-back
			if (pfnModemCbs != NULL){
				pfnModemCbs(modemsts);
			}
			// disable modem status interrupt and CTS status change interrupt
			// if its callback is not installed
			else {
				LPC_UART1->IER &= ~(UART1_IER_MSINT_EN | UART1_IER_CTSINT_EN);
			}
		}
	}

	// Receive Line Status
	if (tmp == UART_IIR_INTID_RLS){
		// Check line status
		tmp1 = UARTx->LSR;
		// Mask out the Receive Ready and Transmit Holding empty status
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE \
				| UART_LSR_BI | UART_LSR_RXFE);
		// If any error exist
		if (tmp1) {
			// Call Call-back function with error input value
			if (uartCbsDat[pUart].pfnErrCbs != NULL) {
				uartCbsDat[pUart].pfnErrCbs(tmp1);
			}
			// Disable interrupt if its call-back is not install
			else {
				UARTx->IER &= ~(UART_IER_RLSINT_EN);
			}
		}
	}

	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI)){
		// Call Rx call back function
		if (uartCbsDat[pUart].pfnRxCbs != NULL) {
			uartCbsDat[pUart].pfnRxCbs();
		}
		// Disable interrupt if its call-back is not install
		else {
			UARTx->IER &= ~(UART_IER_RBRINT_EN);
		}
	}

	// Transmit Holding Empty
	if (tmp == UART_IIR_INTID_THRE){
		// Call Tx call back function
		if (uartCbsDat[pUart].pfnTxCbs != NULL) {
			uartCbsDat[pUart].pfnTxCbs();
		}
		// Disable interrupt if its call-back is not install
		else {
			UARTx->IER &= ~(UART_IER_THREINT_EN);
		}
	}

	intsrc &= (UART_IIR_ABEO_INT | UART_IIR_ABTO_INT);
	// Check if End of auto-baudrate interrupt or Auto baudrate time out
	if (intsrc){
		// Clear interrupt pending
		UARTx->ACR |= ((intsrc & UART_IIR_ABEO_INT) ? UART_ACR_ABEOINT_CLR : 0) \
						| ((intsrc & UART_IIR_ABTO_INT) ? UART_ACR_ABTOINT_CLR : 0);
		if (uartCbsDat[pUart].pfnABCbs != NULL) {
			uartCbsDat[pUart].pfnABCbs(intsrc);
		} else {
			// Disable End of AB interrupt
			UARTx->IER &= ~(UART_IER_ABEOINT_EN | UART_IER_ABTOINT_EN);
		}
	}
}

uint32_t UART_SendString(LPC_UART_TypeDef *UARTx, uint8_t *txbuf) {
	return UART_Send(UARTx, txbuf, strlen(txbuf), BLOCKING);
}

/*********************************************************************//**
 * @brief		Setup call-back function for UART interrupt handler for each
 * 				UART peripheral
 * @param[in]	UARTx	Selected UART peripheral, should be UART0..3
 * @param[in]	CbType	Call-back type, should be:
 * 						0 - Receive Call-back
 * 						1 - Transmit Call-back
 * 						2 - Auto Baudrate Callback
 * 						3 - Error Call-back
 * 						4 - Modem Status Call-back (UART1 only)
 * @param[in]	pfnCbs	Pointer to Call-back function
 * @return		None
 **********************************************************************/
void UART_SetupCbs(LPC_UART_TypeDef *UARTx, uint8_t CbType, void *pfnCbs)
{
	uint8_t pUartNum;

	pUartNum = getUartNum(UARTx);
	switch(CbType){
	case 0:
		uartCbsDat[pUartNum].pfnRxCbs = (fnTxCbs_Type *)pfnCbs;
		break;
	case 1:
		uartCbsDat[pUartNum].pfnTxCbs = (fnRxCbs_Type *)pfnCbs;
		break;
	case 2:
		uartCbsDat[pUartNum].pfnABCbs = (fnABCbs_Type *)pfnCbs;
		break;
	case 3:
		uartCbsDat[pUartNum].pfnErrCbs = (fnErrCbs_Type *)pfnCbs;
		break;
	case 4:
		pfnModemCbs = (fnModemCbs_Type *)pfnCbs;
		break;
	default:
		break;
	}
}

/*********************************************************************//**
 * @brief		Standard UART3 interrupt handler
 * @param[in]	None
 * @return
 **********************************************************************/
void UART3_StdIntHandler(void)
{
	UART_GenIntHandler(LPC_UART3);
}

/*********************************************************************//**
 * @brief		Receive a single data from UART peripheral
 * @param[in]	UARTx	UART peripheral selected, should be UART0, UART1,
 * 						UART2 or UART3.
 * @return 		Data received
 **********************************************************************/
uint8_t UART_ReceiveData(LPC_UART_TypeDef* UARTx)
{
	if (((LPC_UART1_TypeDef *)UARTx) == LPC_UART1)
	{
		return (((LPC_UART1_TypeDef *)UARTx)->/*RBTHDLR.*/RBR & UART_RBR_MASKBIT);
	}
	else
	{
		return (UARTx->/*RBTHDLR.*/RBR & UART_RBR_MASKBIT);
	}
}
