
// FILE:   i2c_ex3_external_loopback.c
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
//! on I2CB.r.
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
    // Initialize device clock and peripherals
    Device_init();
    // Disable pin locks and enable internal pullups.
    Device_initGPIO();
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
    //--- Configure GPIO1 as output (connected to VEN)
    GPIO_setPadConfig(1, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_1_GPIO1);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(1, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(1, 1);                            // Load output latch

    //--- Configure GPIO34 as output (connected to LED)
    GPIO_setPadConfig(34, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_34_GPIO34);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(34, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(34, 1);                            // Load output latch
    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();
    // Configure SCIA.
//    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 230400,
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 460800, (SCI_CONFIG_WLEN_8 |SCI_CONFIG_STOP_ONE |SCI_CONFIG_PAR_NONE));

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
    //Interrupt_register(INT_I2CA_FIFO, &i2cFIFOISR);
    //Interrupt_register(INT_I2CB_FIFO, &i2cFIFOISR);
    // Set I2C use, initializing it for FIFO mode

    initI2C();

    // Enable interrupts required for this example
    //Interrupt_enable(INT_I2CA_FIFO);(INT_I2CB_FIFO);

    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    // Loop forever. Suspend or place breakpoints to observe the buffers.
    initMAX30101();
    //records led3;
    // Olev
uint8_t  xrd1, xwr1,xrd2, xwr2;

    uint8_t txdata [8] = {0};
    uint8_t data9 [9] = {0};
   // uint32_t redLed = 0;
   // uint32_t tempValues[10];

    while (1)
    {
    DEVICE_DELAY_US(20); // Olev
    if (newDataReady())
       {

 // olev,just for interest:  RD and WR pointers!
     // xrd1= readRegister8(MAX30105_FIFOREADPTR);
     // xwr1= readRegister8(MAX30105_FIFOWRITEPTR);

// Olev
     readRegister9x(MAX30105_FIFODATA, data9); // read 3x3=9 bytes of data for 3 channels

     //--data[j]= readRegister8(MAX30105_FIFODATA);
     pack(txdata, data9);
     int j; for (j=0; j<8; j++) // OLev 3 -> 18
                HWREGH(SCIA_BASE + SCI_O_TXBUF) = txdata[j]; //send data

// olev, read again (just for interest):  RD and WR pointers!
      // xrd2= readRegister8(MAX30105_FIFOREADPTR);
      // xwr2= readRegister8(MAX30105_FIFOWRITEPTR);
       }
        //DEVICE_DELAY_US(1000);
    }
}
