// Define some constants to use in the UART example program

#define PB_CLOCK 10000000
#define UART_BAUD_RATE 19200
#define MY_UART UART1


    char char_to_send, char_to_receive;


    // Configure the UART the way we want to use it...
    // See the Plib include file (uart.h) and notes from
    // class to understand the settings.

    UARTConfigure(MY_UART, UART_ENABLE_PINS_TX_RX_ONLY);

    UARTSetDataRate(MY_UART, PB_CLOCK, UART_BAUD_RATE);

    UARTSetLineControl(MY_UART, UART_DATA_SIZE_8_BITS|UART_PARITY_NONE|UART_STOP_BITS_1);

    UARTEnable(MY_UART, UART_ENABLE|UART_PERIPHERAL|UART_RX|UART_TX);


    // Blocking write of a character over the UART.

    while (!UARTTransmitterIsReady(MY_UART)) {}
 
    UARTSendDataByte(MY_UART, char_to_send);


    // Blocking read of a character from the UART.
    // Lab 2 requires a *non-blocking* read function.
    // In other words, the function returns the character if one was available,
    // and -1 if a character was not ready.

    while (!UARTReceivedDataIsAvailable(MY_UART)) {}
    
    char_received = UARTGetDataByte(MY_UART);

}
