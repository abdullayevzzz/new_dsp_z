#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define BUFLEN       1000

#define EPWM_FOR_ADC_BASE EPWM2_BASE

#define  myEPWMk_BASE  EPWM1_BASE

#define EPWM_TIMER_TBPRD2    1250UL

#define  PCB  1 // 0 for Anar's board, 1 for Marek's board

// MUX input Constants
#define EXC_I 0     // Excitation with current source
#define EXC_R 1     // Excitation through shunt resistor
#define RET_TIA 2   // Transimpedance Amplifier, Virtual Ground - Return path for EXC_I
#define RET_GND 3   // Ground - Return path for EXC_R
#define SNS_AP 4    // Sense A+
#define SNS_AN 5    // Sense A-
#define SNS_BP 6    // Sense B+
#define SNS_BN 7    // Sense B-

// MUX output Constants - 4 bits, refer data-sheet for truth-table and pin location
#define OUT_1   13
#define OUT_2   12
#define OUT_3   11
#define OUT_4   10
#define OUT_5   9
#define OUT_6   8
#define OUT_7   15
#define OUT_8   14
#define OUT_9   0
#define OUT_10  1
#define OUT_11  2
#define OUT_12  3
#define OUT_13  4
#define OUT_14  5
#define OUT_15  6
#define OUT_16  7
// second MUX chip, adds 16 to the number
#define OUT_17  29
#define OUT_18  28
#define OUT_19  27
#define OUT_20  26
#define OUT_21  25
#define OUT_22  24
#define OUT_23  31
#define OUT_24  30
#define OUT_25  16
#define OUT_26  17
#define OUT_27  18
#define OUT_28  19
#define OUT_29  20
#define OUT_30  21
#define OUT_31  22
#define OUT_32  23

#define ADCADDR (ADCARESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)
#define ADCBDDR (ADCBRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)
#define ADCCDDR (ADCCRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)
#define ADCDDDR (ADCDRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)


#endif /* CONSTANTS_H_ */

