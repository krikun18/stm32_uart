/* This program echoes ASCII characters received by USART1 on STM32F103. 
    If the message received is "hello", 
    the message "Hello! I am working properly!" will be transmited.  */

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <string.h>

 /* Size of the input buffer.  */
#define RX_BUF_SIZE 80

 /* Size of the output buffer.  */
#define TX_BUF_SIZE 80

 /* ID of the current cell to record
    to the input buffer.  */
volatile int RX_buf_id_in = 0;

 /* ID of the current cell in the input buffer 
    whose value will be written to the output buffer.  */
volatile int RX_buf_id_out = 0;

 /* ID of the current cell in the output buffer.  */
volatile int TX_buf_id = 0;

 /* Storing the ASCII sign received by USART1.  */
volatile char RX_val;

 /* Storing the ASCII sign for USART1 transmission.  */
volatile char TX_val;

 /* Circular input buffer.  */
volatile char RX_buf[RX_BUF_SIZE] = {'\0'};

 /* Output buffer.  */
volatile char TX_buf[TX_BUF_SIZE] = {'\0'};

 /* Nonzero means an attempt was made 
    to overwrite an input buffer value that was not read.  */
volatile int RX_buf_full_error_flag = 0;

 /* Nonzero means the input buffer is full.  */
volatile int RX_buf_full_flag = 0;

 /* Nonzero means the input buffer is empty (no new values).  */
volatile int RX_buf_empty_flag = 1;

 /* Nonzero means TX_buf contains a message to transmit;
    zero means the message in TX_buf has not yet been formed.  */
volatile int transmit_message_ready_flag = 0;

 /* Initialization of USART1.  */
void usart_init(void)
{
    /* Enable USART1 and GPIOA clock.  */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
 
    /* NVIC Configuration.  */
    NVIC_InitTypeDef NVIC_InitStructure;
    /* Enable the USARTx Interrupt.  */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
 
    GPIO_InitTypeDef GPIO_InitStructure;
 
    /* Configure USART1 Tx (PA.09) as alternate function push-pull.  */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
 
    /* Configure USART1 Rx (PA.10) as input floating.  */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
 
  
    /* USART1 configuration.  */
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);		
}
 

 /* USART1 interrupt handler.  */
void USART1_IRQHandler(void)
{
    /* RXNE interraupt handler.
    Writing to the input buffer on receiving value.  */
    if ( (USART1->SR & USART_FLAG_RXNE) != (u16)RESET ){
        if (RX_buf_full_flag) RX_buf_full_error_flag = 1;
        else{
            RX_val = USART_ReceiveData(USART1);
            RX_buf[RX_buf_id_in] = RX_val;
            RX_buf_id_in = RX_buf_id_in + 1;
            RX_buf_empty_flag = 0;

            if (RX_buf_id_in >= RX_BUF_SIZE) RX_buf_id_in = 0;
	
            if (RX_buf_id_in == RX_buf_id_out) RX_buf_full_flag = 1;										
        }
    }

    /* TXE interrupt handler.
    Sending a message from output buffer.  */	
    if ((USART1->SR & USART_FLAG_TXE) != (u16)RESET){
        USART_ClearITPendingBit(USART3, USART_IT_TXE);
        if (transmit_message_ready_flag == 1){
            TX_val = TX_buf[TX_buf_id];
            TX_buf_id = TX_buf_id + 1;
            USART_SendData(USART1, TX_val);
            while(!USART_GetFlagStatus(USART1, USART_FLAG_TC)) {}
            if (TX_val == 13){
                TX_buf_id = 0;
                transmit_message_ready_flag = 0;
                USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
            }
        } else USART_ITConfig(USART1, USART_IT_TXE, DISABLE);				
	}
}

 /* SYSCLK, HCLK, PCLK2 and PCLK1 configuration.
    Setting PCLK2 to 72MHz.  */
void set_sys_clock_to_72(void)
{
    ErrorStatus HSEStartUpStatus;
    
    RCC_DeInit();
    RCC_HSEConfig( RCC_HSE_ON);
    HSEStartUpStatus = RCC_WaitForHSEStartUp();
 
    if (HSEStartUpStatus == SUCCESS){
        RCC_HCLKConfig( RCC_SYSCLK_Div1);
        RCC_PCLK2Config( RCC_HCLK_Div1);
        RCC_PCLK1Config( RCC_HCLK_Div2);
        RCC_PLLConfig(0x00010000, RCC_PLLMul_9);
        RCC_PLLCmd( ENABLE);
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}
        RCC_SYSCLKConfig( RCC_SYSCLKSource_PLLCLK);
        while(RCC_GetSYSCLKSource() != 0x08) {}
    } else while(1) {}
    
}
 

int main(void)
{
    set_sys_clock_to_72();
    usart_init();

    while(1) {  
        if (transmit_message_ready_flag == 0){
            /* TX_buf filling  */
            if (RX_buf_full_error_flag){
                strncpy(TX_buf, "Input Buffer is full\r\n", TX_BUF_SIZE);
                transmit_message_ready_flag = 1;
                TX_buf_id = 0;
                USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
                while(1) {}
            }
					
            if (!RX_buf_empty_flag){
                if (TX_buf_id >= TX_BUF_SIZE){ /* Output buffer is full.  */
                    strncpy(TX_buf, "Output buffer is full\r\n", TX_BUF_SIZE);
                    transmit_message_ready_flag = 1;
                    TX_buf_id = 0;
                    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
                    while(1) {}
                }
                TX_val = RX_buf[RX_buf_id_out];
                RX_buf_id_out = RX_buf_id_out + 1;
                TX_buf[TX_buf_id] = TX_val;
                TX_buf_id = TX_buf_id + 1;
                RX_buf_full_flag = 0;
								
                if (RX_buf_id_out >= RX_BUF_SIZE) RX_buf_id_out = 0;
									
                if (RX_buf_id_in == RX_buf_id_out)  RX_buf_empty_flag = 1;
									
                if (TX_val == 13) {
                    if (strncmp(TX_buf, "hello\r",6) == 0) {
                        strncpy(TX_buf, "Hello! I am working properly!\r\n", TX_BUF_SIZE);	
                    }                        
                    transmit_message_ready_flag = 1;
                    TX_buf_id = 0;
                    USART_ITConfig(USART1, USART_IT_TXE, ENABLE);									
                }
            }
        }
    }
}
