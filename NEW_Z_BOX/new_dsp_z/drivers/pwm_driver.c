/*
 * pwm_driver.c
 *
 *  Created on: Dec 18, 2025
 *      Author: admin
 */
#include "project.h"
#include "pwm_driver.h"

void init_PWM(void)
{
    initEPWM_ADC(); // Set up EPWM to trigger ADC
}


//
void initEPWM_ADC(void)
{
    // Disable SOCB
    EPWM_disableADCTrigger(EPWM2_BASE , EPWM_SOC_B);

    // Configure the SOC to occur on the first up-count event
    EPWM_setADCTriggerSource(EPWM2_BASE , EPWM_SOC_B, EPWM_SOC_TBCTR_U_CMPB);
    EPWM_setADCTriggerEventPrescale(EPWM2_BASE , EPWM_SOC_B, 1);

    // 1Mhz sampling rate is obtained by these values
    EPWM_setCounterCompareValue(EPWM2_BASE , EPWM_COUNTER_COMPARE_B, 50);
    EPWM_setTimeBasePeriod(EPWM2_BASE , 99);

    // Set the local ePWM module clock divider to /1
    EPWM_setClockPrescaler(EPWM2_BASE , EPWM_CLOCK_DIVIDER_1,  EPWM_HSCLOCK_DIVIDER_1);

    // Freeze the counter
    EPWM_setTimeBaseCounterMode(EPWM2_BASE , EPWM_COUNTER_MODE_UP);
}
