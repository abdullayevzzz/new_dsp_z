//#############################################################################
//
// FILE:   i2c_ex3_external_loopback.c
//
// TITLE:  I2C Digital External Loopback with FIFO Interrupts
//
//! \addtogroup driver_example_list
//! <h1>I2C Digital External Loopback with FIFO Interrupts</h1>
//!
//! This program uses the I2CA and I2CB modules for achieving external
//! loopback. The I2CA TX FIFO and the I2CB RX FIFO are used along with
//! their interrupts.
//!
//! A stream of data is sent on I2CA and then compared to the received stream
//! on I2CB.
//! The sent data looks like this: \n
//!  0000 0001 \n
//!  0001 0002 \n
//!  0002 0003 \n
//!  .... \n
//!  00FE 00FF \n
//!  00FF 0000 \n
//!  etc.. \n
//! This pattern is repeated forever.
//!
//! \b External \b Connections \n
//!   - Connect SCLA(GPIO33) to SCLB (GPIO35) and SDAA(GPIO32) to SDAB (GPIO34)
//!   - Connect GPIO31 to an LED used to depict data transfers.
//!
//! \b Watch \b Variables \n
//!  - \b sData - Data to send
//!  - \b rData - Received data
//!  - \b rDataPoint - Used to keep track of the last position in the receive
//!    stream for error checking
//!
//
//#############################################################################
// $TI Release: F2837xD Support Library v3.12.00.00 $
// $Release Date: Fri Feb 12 19:03:23 IST 2021 $
// $Copyright:
// Copyright (C) 2013-2021 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions 
// are met:
// 
//   Redistributions of source code must retain the above copyright 
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the 
//   documentation and/or other materials provided with the   
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "max30101.h"

//
// Globals
uint32_t tes_res = 0;
//
// Function Prototypes
//
void initI2C(void);
void initMAX30101(void);
void pack (uint8_t* data, uint8_t* red);

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pullups.
    //
    Device_initGPIO();

    //
    // Initialize GPIOs 32 and 33 for use as SDA A and SCL A respectively
    GPIO_setPinConfig(GPIO_104_SDAA);
    GPIO_setPadConfig(104, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(104, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_105_SCLA);
    GPIO_setPadConfig(105, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(105, GPIO_QUAL_ASYNC);


    // Configuration for the SCI Rx pin.
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    // Configuration for the SCI Tx pin.
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);


    //--- Configure GPIO34 as output (connected to LED)
    GPIO_setPadConfig(34, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_34_GPIO34);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(34, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(34, 1);                            // Load output latch
    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Configure SCIA.
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 230400, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);
    // Send starting message.
    unsigned char *msg;
    msg = "\r\n\n\nSTARTING!\n\0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 15);

    // Interrupts that are used in this example are re-mapped to ISR functions
    // found within this file.
    //
    //Interrupt_register(INT_I2CA_FIFO, &i2cFIFOISR);
    //Interrupt_register(INT_I2CB_FIFO, &i2cFIFOISR);
    //
    // Set I2C use, initializing it for FIFO mode
    //
    initI2C();

    //

    //
    // Enable interrupts required for this example
    //
    //Interrupt_enable(INT_I2CA_FIFO);
    //Interrupt_enable(INT_I2CB_FIFO);

    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // Loop forever. Suspend or place breakpoints to observe the buffers.
    //
    initMAX30101();

    //records led3;

    uint8_t data [3] = {0};
    uint8_t red [3] = {0};
   // uint32_t redLed = 0;
   // uint32_t tempValues[10];
    int k = 0;

    while (1)
    {
       if (newDataReady()){
            readRegister9(MAX30105_FIFODATA, red);
            pack(data, red);
            int j;
            for (j=0; j<5; j++)
                HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j]; //write data
       }
        DEVICE_DELAY_US(1000);
    }
}
