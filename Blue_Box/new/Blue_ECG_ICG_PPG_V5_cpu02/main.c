//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "max30101.h"


uint16_t ppg_write[3] = {0};
#pragma DATA_SECTION(ppg_write, "SHARERAMGS1");
uint16_t mux_busy_flag;
#pragma DATA_SECTION(mux_busy_flag, "SHARERAMGS0");

records leds_data;

//
// Main
//
void main(void)
{
    // Initialize device clock and peripherals
    Device_init();

    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();

    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();


    EINT;
    ERTM;

    int i = 0;
    // TEST
    for (; i<10; i++){
        GPIO_togglePin(34);
        DEVICE_DELAY_US(100000);
    }

    init_max30101();

    while (1){
        //if (!mux_busy_flag  &&  new_data_ready()){
        if (!mux_busy_flag){
            read_leds(&leds_data);
            ppg_write[0] = leds_data.red;
            }
        DEVICE_DELAY_US(1000);
    }
}

