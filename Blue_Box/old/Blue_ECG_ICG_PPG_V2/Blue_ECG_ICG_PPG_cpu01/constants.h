#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define BUFLEN       500

#define EPWM_FOR_ADC_BASE EPWM2_BASE

#define  myEPWMk_BASE  EPWM1_BASE

#define EPWM_TIMER_TBPRD2    1250UL

#define  PCB  1 // 0 for Anar's board, 1 for Marek's board

// MUX input Constants - 3 bits are mirrored
#define EXC_I 0     // Excitation with current source
#define EXC_R 4     // Excitation through shunt resistor
#define RET_TIA 2   // Transimpedance Amplifier, Virtual Ground - Return path for EXC_I
#define RET_GND 6   // Ground - Return path for EXC_R
#define SNS_AP 1    // Sense A+
#define SNS_AN 5    // Sense A-
#define SNS_BP 3    // Sense B+
#define SNS_BN 7    // Sense B-

// MUX output Constants - 4 bits are mirrored, refer data-sheet for truth-table and pin location
#define OUT_1   11
#define OUT_2   3
#define OUT_3   13
#define OUT_4   5
#define OUT_5   9
#define OUT_6   1
#define OUT_7   15
#define OUT_8   7
#define OUT_9   0
#define OUT_10  8
#define OUT_11  4
#define OUT_12  12
#define OUT_13  2
#define OUT_14  10
#define OUT_15  6
#define OUT_16  14
// second MUX chip, adds 16 to the number
#define OUT_17  27
#define OUT_18  19
#define OUT_19  29
#define OUT_20  21
#define OUT_21  25
#define OUT_22  17
#define OUT_23  31
#define OUT_24  23
#define OUT_25  16
#define OUT_26  24
#define OUT_27  20
#define OUT_28  28
#define OUT_29  18
#define OUT_30  26
#define OUT_31  22
#define OUT_32  30


#define ADCADDR (ADCARESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)
#define ADCBDDR (ADCBRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)
#define ADCCDDR (ADCCRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)
#define ADCDDDR (ADCDRESULT_BASE + ADC_RESULTx_OFFSET_BASE + ADC_SOC_NUMBER2)


#endif /* CONSTANTS_H_ */
