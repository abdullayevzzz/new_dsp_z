//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "constants.h"
#include "exc_signal_dma.h"
#include "sci_b_esp32.h"

// Initialize arrays to define the routing configuration for the multiplexer.
// 'inputs' array holds the input channel you want to connect.
// 'outputs' array holds the corresponding output channel numbers that each input will be connected to.
// The n-th element in 'inputs' will be connected to the n-th element in 'outputs'.
int mux_inputs_1[] = {EXC_R, SNS_AP, SNS_AN, SNS_BP, SNS_BN, RET_GND}; //EXC_I, EXC_R, RET_TIA, RET_GND, SNS_AP, SNS_AN, SNS_BP, SNS_BN
int mux_outputs_1[] = {OUT_1, OUT_2, OUT_3, OUT_5, OUT_6, OUT_7}; // Outputs are in range 1-32
int num_pairs_1 = 6; // Enter the number of pairs

int mux_inputs_2[] = {EXC_R, SNS_AP, SNS_AN, SNS_BP, SNS_BN, RET_GND};
int mux_outputs_2[] = {OUT_7, OUT_8, OUT_9, OUT_11, OUT_12, OUT_13}; // Outputs are in range 1-32
int num_pairs_2 = 6; // Enter the number of pairs

uint16_t mux_period = 100; // mux period in ms

//#include <sine_wave_gen.c>
void sigGen (int16_t signal[], int f, int len, char s); // f in kHz, s = 's' for sine, 'c' for cosine
void pack (uint8_t* data, uint16_t packet_number, int32_t  *accumA1I, int32_t  *accumA1Q, int32_t  *accumB1I, int32_t  *accumB1Q,
           int32_t  *accumC1I, int32_t *accumC1Q, volatile uint32_t *accumD,  char *mode, uint8_t mux_mode, uint16_t* ppg);
extern int32_t _dmac (int16_t *x1, int16_t *x2, int16_t count1, int16_t rsltBitShift);
void delay(int num);
void initShiftRegister(uint32_t sr_val);
void updateShiftRegister(uint32_t mask, uint32_t values);
void router_config(int numPairs, int *inputs, int *outputs);
void configure_gpios(void);
void init_CPU2(void);

//
// Defines
#define Fs           250000 //sampling frequency
#define NUM_OF_BYTES    24
//
#define EX_ADC_RESOLUTION       16
// 12 for 12-bit conversion resolution, which supports (ADC_MODE_SINGLE_ENDED)
// Sample on single pin (VREFLO is the low reference)
// Or 16 for 16-bit conversion resolution, which supports (ADC_MODE_DIFFERENTIAL)
// Sample on pair of pins (difference between pins is converted, subject to
// common mode voltage requirements; see the device data manual)

// Globals
const uint32_t sr_initial_val = 0b01010000000001111111111111111100;
//................................ISsIIIA_AABBBEEEMMMMMMMMMMMMMMMM
uint32_t sr_current_val = sr_initial_val;
volatile uint32_t adcDResult = 0;
uint32_t  accumD = 0;  //accumulate ECG data

int16_t adcAResults[BUFLEN] = {0};
int16_t adcBResults[BUFLEN] = {0};
int16_t adcCResults[BUFLEN] = {0};
//int16_t adcDResults[BUFLEN] = {0};

// Global buffer status flags using 3 bits
// bit0: half_filled flag - set by ISR
// bit1: full_filled flag - set by ISR
// bit2: next_expected flag (0 for half_filled, 1 for full_filled) - set in main loop
volatile uint8_t buffer_status = 0x4; // Set next_expected to full_filled.

volatile uint8_t dacOut = 0;

int16_t signal1sin[BUFLEN] = {0};
#pragma DATA_SECTION(signal1sin, "ramgs15");
int16_t signal1cos[BUFLEN] = {0};
#pragma DATA_SECTION(signal1cos, "ramgs15");
uint8_t data[NUM_OF_BYTES] = {0};
#pragma DATA_SECTION(data, "ramgs15");
uint16_t ppg_read[3] = {0};
#pragma DATA_SECTION(ppg_read, "SHARERAMGS1");
uint16_t mux_busy_flag;
#pragma DATA_SECTION(mux_busy_flag, "SHARERAMGS0");
// mux_busy_flag = 0;

char received_command = 0;
volatile uint16_t counter = 0; // loop counter
uint16_t mux_counter = 0;
volatile uint8_t packet_number = 0;
char mode = 'd';

uint8_t mux_mode = 1; // default mux mode

//==  Function Prototypes
void configureADC(uint32_t adcBase);
void initEPWM_PWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);

//
// Main
//

void main(void)
{   sigGen(signal1sin,10,BUFLEN, 's');
    sigGen(signal1cos,10,BUFLEN, 'c');

    unsigned char *msg;

    // Initialize device clock and peripherals
    Device_init();
    // Initialize CPU2 and give control of I2CB and RAMGS0
    init_CPU2();
    // Disable pin locks and enable internal pullups.
    Device_initGPIO();
    // configure all utilized gpios
    configure_gpios();
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    Interrupt_initModule();
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    Interrupt_initVectorTable();

    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    // Configure SCIA
    SCI_performSoftwareReset(SCIA_BASE);
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 460800, (SCI_CONFIG_WLEN_8 |
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
    msg = "\r\n\n\nSTARTING!\n\0";
    SCI_writeCharArray(SCIA_BASE, (uint16_t*)msg, 15);

    // Init SCIB for communication with ESP32
    init_sci_b();

    //
    // Interrupts that are used in this example are re-mapped to ISR functions
    Interrupt_register(INT_ADCA1, &adcA1ISR);

    // Set up the ADC and the ePWM and initialize the SOC
    //
    configureADC(ADCA_BASE);
    configureADC(ADCB_BASE);
    configureADC(ADCC_BASE);
    configureADC(ADCD_BASE);

    // Configure Shift Register
    initShiftRegister(sr_initial_val);
    initEPWM_ADC();
    initADCSOC();
    // Enable ADC interrupt
    Interrupt_enable(INT_ADCA1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    //
    // Start ePWM1, enabling SOCA and putting the counter in up-count mode
    //
    EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
    EPWM_setTimeBaseCounterMode(EPWM_FOR_ADC_BASE , EPWM_COUNTER_MODE_UP);


    // The function will loop through each pair and program the multiplexer to connect each input to its corresponding output.
    // Can also route dynamically in the while loop
    // router(num_pairs_1, mux_inputs_1, mux_outputs_1);

    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);



    start_exc_dma();
    set_excitation_frequency(10);  // default frequency in kHz

    while(1){
        static int32_t  accumA1I = 0; //accumulate excitation signal 1 in phase component
        static int32_t  accumA1Q = 0; //accumulate excitation signal 1 quadrature component
        static int32_t  accumB1I = 0; //accumulate response 1 signal 1 in phase component
        static int32_t  accumB1Q = 0; //accumulate response 1 signal 1 quadrature component
        static int32_t  accumC1I = 0; //accumulate response 2 signal 1 in phase component
        static int32_t  accumC1Q = 0; //accumulate response 2 signal 1 quadrature component

        if (buffer_status == 0b001  ||  buffer_status == 0b011) //wait until half buffer to fill and next expected flag is 0
        {
            accumA1I = _dmac(signal1sin,adcAResults,BUFLEN/20-1,0);  //half of BUFLEN/5 (5 rpt calculates up to BUFLEN/2)
            accumA1Q = _dmac(signal1cos,adcAResults,BUFLEN/20-1,0);
            accumB1I = _dmac(signal1sin,adcBResults,BUFLEN/20-1,0);
            accumB1Q = _dmac(signal1cos,adcBResults,BUFLEN/20-1,0);
            accumC1I = _dmac(signal1sin,adcCResults,BUFLEN/20-1,0);
            accumC1Q = _dmac(signal1cos,adcCResults,BUFLEN/20-1,0);
            int j;
            for (j=12; j<24; j++){
                 HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];  // UART A
                 HWREGH(SCIB_BASE + SCI_O_TXBUF) = data[j];  // UART B
            }
            buffer_status = 0b100; // set next expected flag for full_filled
        }

        else if (buffer_status == 0b110  ||  buffer_status == 0b111) //wait until full buffer to fill and next expected flag is 1
        {
            accumD = adcDResult>>9UL; //sum of 500 samples - divided by 2^x. !! may be more than 500, check ISR
            adcDResult = 0;

            //do the impedance calculation
            accumA1I += _dmac(signal1sin+BUFLEN/2,adcAResults+BUFLEN/2,BUFLEN/20-1,0);  //half of BUFLEN/5 (5 rpt calculates up to BUFLEN/2)
            accumA1Q += _dmac(signal1cos+BUFLEN/2,adcAResults+BUFLEN/2,BUFLEN/20-1,0);
            accumB1I += _dmac(signal1sin+BUFLEN/2,adcBResults+BUFLEN/2,BUFLEN/20-1,0);
            accumB1Q += _dmac(signal1cos+BUFLEN/2,adcBResults+BUFLEN/2,BUFLEN/20-1,0);
            accumC1I += _dmac(signal1sin+BUFLEN/2,adcCResults+BUFLEN/2,BUFLEN/20-1,0);
            accumC1Q += _dmac(signal1cos+BUFLEN/2,adcCResults+BUFLEN/2,BUFLEN/20-1,0);

            //pack result
            pack (data, packet_number, &accumA1I, &accumA1Q, &accumB1I, &accumB1Q, &accumC1I, &accumC1Q, &accumD, &mode, mux_mode, ppg_read);

            //  SCI_writeCharArray(SCIA_BASE, data, 12); //replace with non-blocking code
            int j;
            for (j=0; j<12; j++){
                HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];  // UART A
                HWREGH(SCIB_BASE + SCI_O_TXBUF) = data[j];  // UART B
            }
            buffer_status = 0b000; // set next expected flag for half_filled
            mux_counter += 1; // 1kHz - 1 ms per sample
        }


        // Receive command from SCIA or SCIB
        if(SCI_getRxFIFOStatus(SCIA_BASE) != SCI_FIFO_RX0)
            received_command = (char)(HWREGH(SCIA_BASE + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);
        else if (SCI_getRxFIFOStatus(SCIB_BASE) != SCI_FIFO_RX0)
            received_command = (char)(HWREGH(SCIB_BASE + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);

        if(received_command){
            uint32_t temp_sr;
            switch (received_command){
                case 'd':
                    GPIO_writePin(34, 1);
                    mode = received_command;
                    break;
                case 'e':
                    GPIO_writePin(34, 0);
                    mode = received_command;
                    break;
                case '1': //1kHz
                    sigGen(signal1sin,1,BUFLEN, 's');
                    sigGen(signal1cos,1,BUFLEN, 'c');
                    //  EPWM_setTimeBasePeriod(myEPWMk_BASE, EPWM_TIMER_TBPRD2*10);
                    //  EPWM_setCounterCompareValue(myEPWMk_BASE, EPWM_COUNTER_COMPARE_A, EPWM_TIMER_TBPRD2*10/2);
                    set_excitation_frequency(1);  // frequency in kHz
                    break;
                case '2': //10kHz
                    sigGen(signal1sin,10,BUFLEN, 's');
                    sigGen(signal1cos,10,BUFLEN, 'c');
                    //  EPWM_setTimeBasePeriod(myEPWMk_BASE, EPWM_TIMER_TBPRD2);
                    //  EPWM_setCounterCompareValue(myEPWMk_BASE, EPWM_COUNTER_COMPARE_A, EPWM_TIMER_TBPRD2/2);
                    set_excitation_frequency(10);  // frequency in kHz
                    break;
                case '3': //100kHz
                    sigGen(signal1sin,100,BUFLEN, 's');
                    sigGen(signal1cos,100,BUFLEN, 'c');
                    //  EPWM_setTimeBasePeriod(myEPWMk_BASE, EPWM_TIMER_TBPRD2/10);
                    //  EPWM_setCounterCompareValue(myEPWMk_BASE, EPWM_COUNTER_COMPARE_A, EPWM_TIMER_TBPRD2/20);
                    set_excitation_frequency(100);  // frequency in kHz
                    break;
                case 'x': // Excitation gain -
                    updateShiftRegister(0x1C000000, sr_current_val + 0x04000000);
                    break;
                case 'X': // Excitation gain +
                    updateShiftRegister(0x1C000000, sr_current_val - 0x04000000);
                    break;
                case 'a': // Sense A gain -
                    temp_sr = (((sr_current_val & 0x02000000) | (sr_current_val & 0x00C00000) << 1)) + 0x00800000;
                    temp_sr = (temp_sr & 0x02000000) | ((temp_sr & 0x01800000) >> 1);
                    updateShiftRegister(0x03C00000, temp_sr);
                    break;
                case 'A': // Sense A gain +
                    temp_sr = (((sr_current_val & 0x00C00000) << 1) | (sr_current_val & 0x02000000)) - 0x00800000;
                    temp_sr = (temp_sr & 0x02000000) | ((temp_sr & 0x01800000) >> 1);
                    updateShiftRegister(0x03C00000, temp_sr);
                    break;
                case 'b': // Sense B gain -
                    updateShiftRegister(0x00380000, sr_current_val + 0x00080000);
                    break;
                case 'B': // Sense B gain -
                    updateShiftRegister(0x00380000, sr_current_val - 0x00080000);
                    break;
                case 'i': // Initialize again
                    sr_current_val = sr_initial_val;
                    initShiftRegister(sr_initial_val | 0x00000003); // test
                    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
                    break;
            }
            received_command = 0;
        }


        if (mux_counter == mux_period){
            // EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            mux_busy_flag = 1;
            router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
            mux_busy_flag = 0;
            // EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            mux_mode = 1;
        }
        else if (mux_counter == mux_period * 2){
            // EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            mux_busy_flag = 1;
            router_config(num_pairs_2, mux_inputs_2, mux_outputs_2);
            mux_busy_flag = 0;
            // EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            mux_mode = 2;
            mux_counter = 0;
       }

    }
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

    // Set resolution and signal mode (see #defines above) and load
    // corresponding trims.

#if(EX_ADC_RESOLUTION == 12)
    ADC_setMode(adcBase, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
#elif(EX_ADC_RESOLUTION == 16)
    ADC_setMode(adcBase, ADC_RESOLUTION_16BIT, ADC_MODE_DIFFERENTIAL);
#endif


    // exception for ECG
    if(adcBase == ADCD_BASE){
        ADC_setMode(adcBase, ADC_RESOLUTION_12BIT, ADC_MODE_SINGLE_ENDED);
    }


    // Set pulse positions to late
    ADC_setInterruptPulseMode(adcBase, ADC_PULSE_END_OF_CONV);

    // Power up the ADCs and then delay for 1 ms
    ADC_enableConverter(adcBase);

    // Delay for 1ms to allow ADC time to power up
    DEVICE_DELAY_US(1000);
}


//
void initEPWM_ADC(void)
{
    // Disable SOCA
    EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);

    // Configure the SOC to occur on the first up-count event
    EPWM_setADCTriggerSource(EPWM_FOR_ADC_BASE , EPWM_SOC_B, EPWM_SOC_TBCTR_U_CMPB);
    EPWM_setADCTriggerEventPrescale(EPWM_FOR_ADC_BASE , EPWM_SOC_B, 1);

    // 500kHz sampling rate is obtained by these values
    EPWM_setCounterCompareValue(EPWM_FOR_ADC_BASE , EPWM_COUNTER_COMPARE_B, 100);
    EPWM_setTimeBasePeriod(EPWM_FOR_ADC_BASE , 199);

    // Set the local ePWM module clock divider to /1
    EPWM_setClockPrescaler(EPWM_FOR_ADC_BASE , EPWM_CLOCK_DIVIDER_1,  EPWM_HSCLOCK_DIVIDER_1);

    // Freeze the counter
    EPWM_setTimeBaseCounterMode(EPWM_FOR_ADC_BASE , EPWM_COUNTER_MODE_UP);
}

//
// Function to configure SOCs on ADCA and ADCD to be triggered by ePWM1.
//
void initADCSOC(void)
{
        uint16_t acqps;
    // Determine minimum acquisition window (in SYSCLKS) based on resolution
    //
    if(EX_ADC_RESOLUTION == 12)
    {
        acqps = 14; // 75ns
    }
    else //resolution is 16-bit
    {
        acqps = 63; // 320ns
    }
    //
    // - NOTE: A longer sampling window will be required if the ADC driving
    //   source is less than ideal (an ideal source would be a high bandwidth

    // Select the channels to convert and the configure the ePWM trigger 
    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCB_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCC_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCD_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                    ADC_CH_ADCIN14, acqps);


    // Select SOC2 on ADCA as the interrupt source.  SOC2 on ADCD will end at
    // the same time, so either SOC2 would be an acceptable interrupt triggger.
    ADC_setInterruptSource(ADCA_BASE, ADC_INT_NUMBER1, ADC_SOC_NUMBER2);
    ADC_enableInterrupt(ADCA_BASE, ADC_INT_NUMBER1);
    ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
}

// ADC A Interrupt 1 ISR

// OM now globals

__interrupt void adcA1ISR(void)
{
    adcAResults[counter] = HWREGH(ADCADDR) - 32767; // make signed
    adcBResults[counter] = HWREGH(ADCBDDR) - 32767;
    adcCResults[counter] = HWREGH(ADCCDDR) - 32767;
    adcDResult += HWREGH(ADCDDDR);

    // Clear the interrupt flag
    HWREGH(ADCA_BASE + ADC_O_INTFLGCLR) = 0x0001;
    //ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);

    counter++;
    if (counter == BUFLEN/2 - 1)
        buffer_status |= 0b001;
    else if (counter >= BUFLEN){
        buffer_status |= 0b010;
        counter = 0;
        packet_number++;
    }

    // Acknowledge the interrupt
    HWREGH(PIECTRL_BASE + PIE_O_ACK) = INTERRUPT_ACK_GROUP1;
    //  Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}


void init_CPU2(void)
{
    // Hand-over the SCIB module access to CPU2
    SysCtl_selectCPUForPeripheralInstance(SYSCTL_CPUSEL_I2CB, SYSCTL_CPUSEL_CPU2);
    // Hand-over the shared RAMGS1 access to CPU2
    MemCfg_setGSRAMMasterSel((MEMCFG_SECT_GS1), MEMCFG_GSRAMMASTER_CPU2);

    // Send boot command to allow the CPU2 to begin execution
    #ifdef _STANDALONE
    #ifdef _FLASH    //
    Device_bootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_FLASH);
    #else
    Device_bootCPU2(C1C2_BROM_BOOTMODE_BOOT_FROM_RAM);
    #endif // _FLASH
    #endif // _STANDALONE
}

