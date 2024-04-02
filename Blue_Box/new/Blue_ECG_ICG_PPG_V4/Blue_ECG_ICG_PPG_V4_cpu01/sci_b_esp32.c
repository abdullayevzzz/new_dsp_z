/*
 * spi_esp32.c
 *
 *  Created on: Feb 20, 2024
 *      Author: admin
 */
//
// Included Files
//
#include <sci_b_esp32.h>
#include "driverlib.h"
#include "device.h"

//
// Main
//
void init_sci_b(void)
{
    // Configuration for the SCIB Rx pin.
    GPIO_setMasterCore(19, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_19_SCIRXDB);
    GPIO_setDirectionMode(19, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(19, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(19, GPIO_QUAL_ASYNC);

    // Configuration for the SCIB Tx pin.
    GPIO_setMasterCore(18, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_18_SCITXDB);
    GPIO_setDirectionMode(18, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(18, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(18, GPIO_QUAL_ASYNC);

    // Configure SCIB
    SCI_performSoftwareReset(SCIB_BASE);
    SCI_setConfig(SCIB_BASE, DEVICE_LSPCLK_FREQ, 460800, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIB_BASE);
    SCI_resetRxFIFO(SCIB_BASE);
    SCI_resetTxFIFO(SCIB_BASE);
    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIB_BASE);
    SCI_enableModule(SCIB_BASE);
    SCI_performSoftwareReset(SCIB_BASE);
}


//
// End of File
//



