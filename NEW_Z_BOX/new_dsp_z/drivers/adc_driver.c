/*
 * adc_driver.c
 *
 *  Created on: Dec 11, 2025
 *      Author: admin
 */
#include "project.h"
#include "adc_driver.h"


void init_ADC(void)
{
    configureADC(ADCA_BASE);
    configureADC(ADCB_BASE);
    initADCSOC();
}



//
// configureADC - Write ADC configurations and power up the ADC for the
// selected ADC
//
void configureADC(uint32_t adcBase)
{
    //
    // Set ADCDLK divider to /4
    //
    ADC_setPrescaler(adcBase, ADC_CLK_DIV_4_0);

    // Set pulse positions to late
    ADC_setInterruptPulseMode(adcBase, ADC_PULSE_END_OF_CONV);

    // Power up the ADCs and then delay for 1 ms
    ADC_enableConverter(adcBase);

    // Delay for 1ms to allow ADC time to power up
    DEVICE_DELAY_US(1000);
}

//
// Function to configure SOCs on ADCA and ADCB to be triggered by ePWM2.
//
void initADCSOC(void)
{
    uint16_t acqps = 63; // 320ns for 16 bit ADC

    // Select the channels to convert and the configure the ePWM trigger
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCB_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);

    // Select SOC2 on ADCA as the interrupt source.  SOC2 on ADCD will end at
    // the same time, so either SOC2 would be an acceptable interrupt triggger.
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER2);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
}

