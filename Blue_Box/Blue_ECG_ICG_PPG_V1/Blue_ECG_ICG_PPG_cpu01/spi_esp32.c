/*
 * spi_esp32.c
 *
 *  Created on: Feb 20, 2024
 *      Author: admin
 */
//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "spi_esp32.h"

//
// Globals
//
volatile uint16_t sData[12];         // Send data buffer
volatile uint16_t rDataPoint = 0;   // To keep track of where we are in the
                                    // data stream to check received data
//
// Function Prototypes
//
void initSPIAMaster(void);
void configGPIOs(void);

//
// Main
//
void init_spi(void)
{
    //
    // Configure GPIOs for external loopback.
    //
    configGPIOs();
    //
    // Set up SPI A as master, initializing it for FIFO mode
    //
    initSPIAMaster();

    /*
    //
    // Initialize the data buffers
    //
    for(i = 0; i < 12; i++)
    {
        sData[i] = i;
    }

    //
    // Loop forever. Suspend or place breakpoints to observe the buffers.
    //

    // Generate data packet
    sData[0] = 0xAC; //start
    sData[1] = 0xAC; //start
    sData[2] = 0x11; //dummy settings
    for(i = 0; i < 8; i++)
    {
        sData[i + 3] = i; //dummy data
    }
    sData[11] = 0x7B; //end


    while(1)
    {

    uint32_t k;

    // Send data continuosly
    for(i = 0; i < 12; i++)
    {
        SPI_writeDataNonBlocking(SPIA_BASE, sData[i]);
    }

    // Dummy delay for approximately 50ms
    for (k = 0; k < 5000000; k++) {
        asm(" NOP");
    }

    }

    */
}


//
// Function to configure SPI A as master.
//
void initSPIAMaster(void)
{
    //
    // Must put SPI into reset before configuring it
    //
    SPI_disableModule(SPIA_BASE);

    //
    // SPI configuration. Use a 500kHz SPICLK and 8-bit word size.
    //
    SPI_setConfig(SPIA_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA0,
                  SPI_MODE_MASTER, 1000000, 16);
    SPI_disableLoopback(SPIA_BASE);
    SPI_setEmulationMode(SPIA_BASE, SPI_EMULATION_FREE_RUN);

    //
    // FIFO and configuration
    //
    SPI_enableFIFO(SPIA_BASE);

    //
    // Configuration complete. Enable the module.
    //
    SPI_enableModule(SPIA_BASE);
}

//
// Configure GPIOs for external communication.
//
void configGPIOs(void)
{
    //
    // This test is designed for an external loopback between SPIA and ESP32
    // External Connections:
    // -GPIO17 - SPISOMI
    // -GPIO16 - SPISIMO
    // -GPIO19 - SPISTE
    // -GPIO18 - SPICLK
    //

    //
    // GPIO58 is the SPISIMOA clock pin.
    //
    GPIO_setMasterCore(58, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_58_SPISIMOA);
    GPIO_setPadConfig(58, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(58, GPIO_QUAL_ASYNC);

    //
    // GPIO59 is the SPISOMIA.
    //
    GPIO_setMasterCore(59, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_59_SPISOMIA);
    GPIO_setPadConfig(59, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(59, GPIO_QUAL_ASYNC);

    //
    // GPIO60 is the SPICLKA.
    //
    GPIO_setMasterCore(60, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_60_SPICLKA);
    GPIO_setPadConfig(60, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(60, GPIO_QUAL_ASYNC);


    //
    // GPIO122 is the SPICS.
    //
    GPIO_setMasterCore(122, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_122_GPIO122);
    GPIO_setPadConfig(122, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(122, GPIO_DIR_MODE_OUT);    // GPIO122 = output
    GPIO_writePin(122, 0);
}

//
// End of File
//



