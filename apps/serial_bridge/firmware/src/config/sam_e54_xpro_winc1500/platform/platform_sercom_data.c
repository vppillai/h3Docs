#include "definitions.h"
#include "platform_sercom_data.h"

const PLATFORM_USART_PLIB_INTERFACE platformUsartPlibAPI = {
    .initialize             = SERCOM2_USART_Initialize,
    .readCallbackRegister   = SERCOM2_USART_ReadCallbackRegister,
    .read                   = SERCOM2_USART_Read,
    .readIsBusy             = SERCOM2_USART_ReadIsBusy,
    .readCountGet           = SERCOM2_USART_ReadCountGet,
    .writeCallbackRegister  = SERCOM2_USART_WriteCallbackRegister,
    .write                  = SERCOM2_USART_Write,
    .writeIsBusy            = SERCOM2_USART_WriteIsBusy,
    .writeCountGet          = SERCOM2_USART_WriteCountGet,
    .errorGet               = SERCOM2_USART_ErrorGet,
    .serialSetup            = SERCOM2_USART_SerialSetup
};

	
const DRV_USART_INTERRUPT_SOURCES platformInterruptSources =
{	
    /* Peripheral has more than one interrupt vector */
    .isSingleIntSrc                        = false,
 
    /* Peripheral interrupt lines */
    .intSources.multi.usartTxCompleteInt   = SERCOM2_1_IRQn,
    .intSources.multi.usartTxReadyInt      = SERCOM2_0_IRQn,
    .intSources.multi.usartRxCompleteInt   = SERCOM2_2_IRQn,
    .intSources.multi.usartErrorInt        = SERCOM2_OTHER_IRQn,
};