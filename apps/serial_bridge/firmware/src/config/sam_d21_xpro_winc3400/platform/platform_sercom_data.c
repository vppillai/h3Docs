#include "definitions.h"
#include "platform_sercom_data.h"

const PLATFORM_USART_PLIB_INTERFACE platformUsartPlibAPI = {
    .initialize             = SERCOM3_USART_Initialize,
    .readCallbackRegister   = SERCOM3_USART_ReadCallbackRegister,
    .read                   = SERCOM3_USART_Read,
    .readIsBusy             = SERCOM3_USART_ReadIsBusy,
    .readCountGet           = SERCOM3_USART_ReadCountGet,
    .writeCallbackRegister  = SERCOM3_USART_WriteCallbackRegister,
    .write                  = SERCOM3_USART_Write,
    .writeIsBusy            = SERCOM3_USART_WriteIsBusy,
    .writeCountGet          = SERCOM3_USART_WriteCountGet,
    .errorGet               = SERCOM3_USART_ErrorGet,
    .serialSetup            = SERCOM3_USART_SerialSetup
};

	
const DRV_USART_INTERRUPT_SOURCES platformInterruptSources =
{	
    /* Peripheral has single interrupt vector */
    .isSingleIntSrc                        = true,
 
    /* Peripheral interrupt line */
    .intSources.usartInterrupt             = SERCOM3_IRQn,
};