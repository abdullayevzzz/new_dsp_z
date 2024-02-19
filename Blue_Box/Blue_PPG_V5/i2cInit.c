#include "device.h"

#define SLAVE_ADDRESS   0x57

void initI2C()
{
    //
    // Must put I2C into reset before configuring it
    //
    I2C_disableModule(I2CA_BASE);
    I2C_disableModule(I2CB_BASE);

    //
    // I2C configuration. Use a 400kHz I2CCLK with a 50% duty cycle.
    //
    I2C_initMaster(I2CB_BASE, DEVICE_SYSCLK_FREQ, 400000, I2C_DUTYCYCLE_50);
    I2C_setConfig(I2CB_BASE, I2C_MASTER_SEND_MODE | I2C_REPEAT_MODE);

    I2C_setDataCount(I2CB_BASE, 1);
    I2C_setBitCount(I2CB_BASE, I2C_BITCOUNT_8);

    //
    // I2C slave configuration
    //
    //I2C_setConfig(I2CB_BASE, I2C_SLAVE_RECEIVE_MODE);

    //I2C_setDataCount(I2CB_BASE, 2);
    //I2C_setBitCount(I2CB_BASE, I2C_BITCOUNT_8);

    //
    // Configure for external loopback
    //
    // I2C_setOwnSlaveAddress(I2CB_BASE, 0x11);

    I2C_setSlaveAddress(I2CB_BASE, SLAVE_ADDRESS);

    //
    // FIFO and interrupt configuration
    //
    I2C_enableFIFO(I2CB_BASE);
    //I2C_clearInterruptStatus(I2CA_BASE, I2C_INT_TXFF);
    // Configuration complete. Enable the module.

    I2C_setEmulationMode(I2CB_BASE, I2C_EMULATION_STOP_SCL_LOW);
    //I2C_setEmulationMode(I2CB_BASE, I2C_EMULATION_FREE_RUN);

    //
    I2C_enableModule(I2CB_BASE);
    //I2C_enableModule(I2CB_BASE);
    DEVICE_DELAY_US(100);
}
