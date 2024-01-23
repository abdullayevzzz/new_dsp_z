//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "constants.h"
#include "exc_signal_dma.h"

// Initialize arrays to define the routing configuration for the multiplexer.
// 'inputs' array holds the input channel we want to connect.
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
void pack (uint8_t* data, uint16_t packet_number, int32_t  *accumA1I, int32_t  *accumA1Q, int32_t  *accumB1I, int32_t  *accumB1Q, int32_t  *accumC1I, int32_t *accumC1Q, volatile uint32_t *accumD,  char *mode, uint8_t mux_mode);
extern int32_t _dmac (int16_t *x1, int16_t *x2, int16_t count1, int16_t rsltBitShift);
void delay(int num);
void initShiftRegister(uint32_t sr_val);
void updateShiftRegister(uint32_t mask, uint32_t values);
void router_config(int numPairs, int *inputs, int *outputs);
//
// Defines
#define Fs           250000 //sampling frequency
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


volatile uint8_t halfFilled = 0;  //flags to check if read buffers are full
volatile uint8_t fullFilled = 0;

volatile uint8_t dacOut = 0;

int16_t signal1sin[BUFLEN] = {0};
#pragma DATA_SECTION(signal1sin, "ramgs15");
int16_t signal1cos[BUFLEN] = {0};
#pragma DATA_SECTION(signal1cos, "ramgs15");

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
    uint8_t* data;

    // Initialize device clock and peripherals
    Device_init();
    // Disable pin locks and enable internal pullups.
    Device_initGPIO();

    // from 'Board_init' (.c/.h)
    //--   EALLOW;
    // EPWM1 -> myEPWM1 Pinmux
    // GPIO_setPinConfig(GPIO_0_EPWM1A);
    // GPIO_setPinConfig(GPIO_1_EPWM1B);
    //--   EDIS;

    // Configuration for the SCI Rx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    //
    // Configuration for the SCI Tx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    //--- Configure GPIO5 as output (connected to ECG shutdown)
    GPIO_setPadConfig(5, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO5
    GPIO_setPinConfig(GPIO_5_GPIO5);               // GPIO34 = GPIO5
    GPIO_setDirectionMode(5, GPIO_DIR_MODE_OUT);    // GPIO5 = output
    GPIO_writePin(5, 1);                            // Load output latch

    //--- Configure GPIO24 as output (connected to ECG LOD+)
    GPIO_setPadConfig(24, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO5
    GPIO_setPinConfig(GPIO_24_GPIO24);               // GPIO34 = GPIO5
    GPIO_setDirectionMode(24, GPIO_DIR_MODE_OUT);    // GPIO5 = output
    GPIO_writePin(24, 0);

    //--- Configure GPIO34 as output (connected to LED)
    GPIO_setPadConfig(34, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_34_GPIO34);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(34, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(34, 1);                            // Load output latch


    /*
    //--- Configure GPIO4, GPIO3, GPIO2 as vGain (connected to voltage amplifier gain control)
    GPIO_setPadConfig(4, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_4_GPIO4);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(4, GPIO_DIR_MODE_OUT);    // GPIO34 = output

    GPIO_setPadConfig(3, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_3_GPIO3);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(3, GPIO_DIR_MODE_OUT);    // GPIO34 = output

    GPIO_setPadConfig(2, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_2_GPIO2);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(2, GPIO_DIR_MODE_OUT);    // GPIO34 = output

    GPIO_setPinConfig(GPIO_123_GPIO123);               // DAC(0) or PWM(1)
    GPIO_setDirectionMode(123, GPIO_DIR_MODE_OUT);    //  Marek
    GPIO_writePin(123, 1);
    */


    /*
    //--- Configure GPIO22, GPIO60, GPIO111 as vGain (connected to voltage amplifier gain control)
    GPIO_setPadConfig(22, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_22_GPIO22);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(22, GPIO_DIR_MODE_OUT);    // GPIO34 = output

    GPIO_setPadConfig(60, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_60_GPIO60);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(60, GPIO_DIR_MODE_OUT);    // GPIO34 = output

    GPIO_setPadConfig(111, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_111_GPIO111);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(111, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    */


    //== Olev Relay 67
    GPIO_setPadConfig(67, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_67_GPIO67);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(67, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(67, 1);


    // 5V enable
    GPIO_setPadConfig(1, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_1_GPIO1);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(1, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(1, 1);



    // Make sure GPIO 0 - PWM pin is not used in DAC mode
    GPIO_setPadConfig(0, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO0
    GPIO_setPinConfig(GPIO_0_GPIO0);               // GPIO0 = GPIO0
    GPIO_setDirectionMode(0, GPIO_DIR_MODE_IN);    // GPIO0 = output

    // Shift register GPIOs
    //
    // GPIO63 is the SDAT.
    //
    GPIO_setMasterCore(63, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_63_GPIO63);
    GPIO_setPadConfig(63, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(63, GPIO_DIR_MODE_OUT);
    GPIO_writePin(63, 1);

    //
    // GPIO65 is the SRCLK.
    //
    GPIO_setMasterCore(65, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_65_GPIO65);
    GPIO_setPadConfig(65, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(65, GPIO_DIR_MODE_OUT);
    GPIO_writePin(65, 1);

    //
    // GPIO26 is the RCLK.
    //
    GPIO_setMasterCore(26, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_26_GPIO26);
    GPIO_setPadConfig(26, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(26, GPIO_DIR_MODE_OUT);
    GPIO_writePin(26, 1);

    //
    // GPIO27 is the !OE.
    //
    GPIO_setMasterCore(27, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_27_GPIO27);
    GPIO_setPadConfig(27, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(27, GPIO_DIR_MODE_OUT);
    GPIO_writePin(27, 1); //disable output


    //
    // Initialize PIE and clear PIE registers. Disables CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();
//--    Board_init();
    SysCtl_disablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);

    SCI_performSoftwareReset(SCIA_BASE);

    //
    // Configure SCIA for echoback.
    //
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

    //   initEPWM_PWM(); //OM
    //  initPWMchopper ();
    // initEPWM_chopper (myEPWMk_BASE); // OM


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
    //  EPWM_setTimeBaseCounterMode(EPWM_FOR_PWM_BASE , EPWM_COUNTER_MODE_UP); //OM

    // The function will loop through each pair and program the multiplexer to connect each input to its corresponding output.
    // Can also route dynamically in the while loop
    // router(num_pairs_1, mux_inputs_1, mux_outputs_1);
    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);

    start_exc_dma();
    set_excitation_frequency(10);  // default frequency in kHz

    while(1){
        // dummy test for SR
        /*
        static long int delay_test = 0;
        delay_test++;
        uint32_t mask = 0x00000003;  // mask for bits 30 and 31
        //uint32_t values = (1 << 1) | (1 << 0);  // values for bits 30 and 31
        // updateShiftRegister(mask, values);  // Set bit 30 and bit 31 to 1
        if (delay_test == 100000)
            updateShiftRegister(mask, 0xFFFFFFF2);
        else if (delay_test == 200000)
            updateShiftRegister(mask, 0xFFFFFFF0);
        else if (delay_test == 300000)
            updateShiftRegister(mask, 0xFFFFFFF1);
        else if (delay_test == 400000){
            updateShiftRegister(mask, 0xFFFFFFF3);
            delay_test = 0;
            }
        */
        // dummy test


        /*
        // dummy test for MUX
        static long int delay_test = 0;
        delay_test++;
        uint32_t mask = 0x00000003;  // mask for bits 30 and 31 LED for debug
        if (delay_test == 20000){
            router(num_pairs_1, mux_inputs_1, mux_outputs_1);
            updateShiftRegister(mask, 0xFFFFFFF3);
        }
        else if (delay_test == 40000){
            router(num_pairs_2, mux_inputs_2, mux_outputs_2);
            updateShiftRegister(mask, 0xFFFFFFF0);
            delay_test = 0;
        }
        // dummy test
        */

        static int32_t  accumA1I = 0; //accumulate excitation signal 1 in phase component
        static int32_t  accumA1Q = 0; //accumulate excitation signal 1 quadrature component
        static int32_t  accumB1I = 0; //accumulate response 1 signal 1 in phase component
        static int32_t  accumB1Q = 0; //accumulate response 1 signal 1 quadrature component
        static int32_t  accumC1I = 0; //accumulate response 2 signal 1 in phase component
        static int32_t  accumC1Q = 0; //accumulate response 2 signal 1 quadrature component

        if (halfFilled) //wait until half buffer to fill
        {
            static uint16_t first_time = 1;
            accumA1I = _dmac(signal1sin,adcAResults,BUFLEN/20-1,0);  //half of BUFLEN/5 (5 rpt calculates up to BUFLEN/2)
            accumA1Q = _dmac(signal1cos,adcAResults,BUFLEN/20-1,0);
            accumB1I = _dmac(signal1sin,adcBResults,BUFLEN/20-1,0);
            accumB1Q = _dmac(signal1cos,adcBResults,BUFLEN/20-1,0);
            accumC1I = _dmac(signal1sin,adcCResults,BUFLEN/20-1,0);
            accumC1Q = _dmac(signal1cos,adcCResults,BUFLEN/20-1,0);
            if (!first_time){
                int j;
                for (j=11; j<22; j++){
                    HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];
                 }
            }
            first_time = 0;
            halfFilled = 0;
        }

        if (fullFilled) //wait until full buffer to fill
        {
            accumD = adcDResult>>0UL; //sum of 500 samples - divided by x. !! may be more than 500, check ISR
            adcDResult = 0;

            //do the impedance calculation
            accumA1I += _dmac(signal1sin+BUFLEN/2,adcAResults+BUFLEN/2,BUFLEN/20-1,0);  //half of BUFLEN/5 (5 rpt calculates up to BUFLEN/2)
            accumA1Q += _dmac(signal1cos+BUFLEN/2,adcAResults+BUFLEN/2,BUFLEN/20-1,0);
            accumB1I += _dmac(signal1sin+BUFLEN/2,adcBResults+BUFLEN/2,BUFLEN/20-1,0);
            accumB1Q += _dmac(signal1cos+BUFLEN/2,adcBResults+BUFLEN/2,BUFLEN/20-1,0);
            accumC1I += _dmac(signal1sin+BUFLEN/2,adcCResults+BUFLEN/2,BUFLEN/20-1,0);
            accumC1Q += _dmac(signal1cos+BUFLEN/2,adcCResults+BUFLEN/2,BUFLEN/20-1,0);

            //send result
            pack (data, packet_number, &accumA1I, &accumA1Q, &accumB1I, &accumB1Q, &accumC1I, &accumC1Q, &accumD, &mode, mux_mode);

            //  SCI_writeCharArray(SCIA_BASE, data, 12); //replace with non-blocking code
            int j;
            for (j=0; j<11; j++){
                HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];
            }
            fullFilled = 0;
            mux_counter += 1; // 1kHz - 1 ms per sample
        }

        if(SCI_getRxFIFOStatus(SCIA_BASE) != SCI_FIFO_RX0){
            char r = (char)(HWREGH(SCIA_BASE + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);
            uint32_t temp_sr;
            switch (r){
                case 'd':
                    GPIO_writePin(34, 1);
                    mode = r;
                    break;
                case 'e':
                    GPIO_writePin(34, 0);
                    mode = r;
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
                /*
                case 'n': // Mux mode 1
                    EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
                    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
                    mux_mode = 1;
                    EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
                    break;
                case 'm': // Mux mode 2
                    EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
                    router_config(num_pairs_2, mux_inputs_2, mux_outputs_2);
                    mux_mode = 2;
                    EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
                    break;
                */
                case 'i': // Initialize again
                    sr_current_val = sr_initial_val;
                    initShiftRegister(sr_initial_val | 0x00000003); // test
                    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
                    break;
            }
        }

        if (mux_counter == mux_period){
            // EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
            // EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            mux_mode = 1;
        }
        else if (mux_counter == mux_period * 2){
            // EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
            router_config(num_pairs_2, mux_inputs_2, mux_outputs_2);
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

    //
    // Select the channels to convert and the configure the ePWM trigger 
    //

    ADC_setupSOC(ADCA_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCB_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCC_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                 ADC_CH_ADCIN2_ADCIN3, acqps);
    ADC_setupSOC(ADCD_BASE, ADC_SOC_NUMBER2, ADC_TRIGGER_EPWM2_SOCB,
                    ADC_CH_ADCIN14, acqps);

    //
    // Select SOC2 on ADCA as the interrupt source.  SOC2 on ADCD will end at
    // the same time, so either SOC2 would be an acceptable interrupt triggger.
    //
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
        halfFilled = 1;
    else if (counter >= BUFLEN){
        fullFilled = 1;
        counter = 0;
        packet_number++;
    }

    // Acknowledge the interrupt
    HWREGH(PIECTRL_BASE + PIE_O_ACK) = INTERRUPT_ACK_GROUP1;
    //  Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}
