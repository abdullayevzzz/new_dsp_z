//
// Included Files
//
#include "definitions.h"
#include "driverlib.h"
#include "device.h"
#include "exc_signal_dma.h"
#include "sci_b_esp32.h"
#include "lookup_tables.h"


uint16_t mux_period = 5; // mux period in ms
uint16_t sweep_period = 100; // frequency sweep period (for one freq) in ms
#define NUM_OUTPUTS 16  // Number of output electrodes


// Initialize arrays to define the routing configuration for the multiplexer.
// 'inputs' array holds the input channel you want to connect.
// 'outputs' array holds the corresponding output channel numbers that each input will be connected to.
// The n-th element in 'inputs' will be connected to the n-th element in 'outputs'.
uint16_t mux_inputs_1[] = {EXC_R, SNS_AP, SNS_AN, SNS_BP, SNS_BN, RET_GND}; //EXC_I, EXC_R, RET_TIA, RET_GND, SNS_AP, SNS_AN, SNS_BP, SNS_BN
uint16_t mux_outputs_1[] = {OUT_1, OUT_2, OUT_3, OUT_5, OUT_6, OUT_7}; // Outputs are in range 1-32
uint16_t num_pairs_1 = 6; // Enter the number of pairs

uint16_t mux_inputs_2[] = {EXC_R, SNS_AP, SNS_AN, SNS_BP, SNS_BN, RET_GND};
uint16_t mux_outputs_2[] = {OUT_7, OUT_8, OUT_9, OUT_11, OUT_12, OUT_13}; // Outputs are in range 1-32
uint16_t num_pairs_2 = 6; // Enter the number of pairs

uint16_t mux_inputs_3[] = {EXC_R, SNS_AP, SNS_AN, SNS_BP, SNS_BN, RET_GND};
uint16_t mux_outputs_3[] = {OUT_13, OUT_14, OUT_15, OUT_17, OUT_18, OUT_19}; // Outputs are in range 1-32
uint16_t num_pairs_3 = 6; // Enter the number of pairs

uint16_t mux_inputs_4[] = {EXC_R, SNS_AP, SNS_AN, SNS_BP, SNS_BN, RET_GND};
uint16_t mux_outputs_4[] = {OUT_19, OUT_20, OUT_21, OUT_23, OUT_24, OUT_25}; // Outputs are in range 1-32
uint16_t num_pairs_4 = 6; // Enter the number of pairs



uint16_t mux_inputs[] = {EXC_R, SNS_AP, SNS_AN, RET_GND}; // Inputs array
uint16_t mux_outputs[] = {OUT_1, OUT_2, OUT_3, OUT_4, OUT_5, OUT_6, OUT_7, OUT_8, OUT_9, OUT_10, OUT_11, OUT_12, OUT_13, OUT_14, OUT_15, OUT_16,
                      OUT_17, OUT_18, OUT_19, OUT_20, OUT_21, OUT_22, OUT_23, OUT_24, OUT_25, OUT_26, OUT_27, OUT_28, OUT_29, OUT_30, OUT_31};
uint8_t tomo_mode = 0; // By default tomography mode is off, device operates in normal mode
uint8_t sweep_mode = 0; // By default frequency sweep mode is off



//#include <sine_wave_gen.c>
void sigGen (int16_t signal[], int f, int len, char s); // f in kHz, s = 's' for sine, 'c' for cosine
void pack (uint8_t* data, uint16_t packet_number, SignalsStruct *signalsArray, volatile uint32_t *accumD,  char *mode, uint32_t *mux_mode,
           uint16_t activeFreqs[], uint16_t* ppg);
extern int32_t _dmac (int16_t *x1, int16_t *x2, int16_t count1, int16_t rsltBitShift);
void delay(int num);
uint32_t mirror(uint32_t value);
void initShiftRegister(uint32_t sr_val);
void updateShiftRegister(uint32_t mask, uint32_t values);
void router_config(int numPairs, uint16_t *inputs, uint16_t *outputs);
void configure_gpios(void);
void initSPIBMaster(void);
void init_CPU2(void);
void configure_SPI_DMA(void);

//
// Defines
#define Fs           500000 //sampling frequency
#define NUM_OF_BYTES    26
//
#define EX_ADC_RESOLUTION       16
// 12 for 12-bit conversion resolution, which supports (ADC_MODE_SINGLE_ENDED)
// Sample on single pin (VREFLO is the low reference)
// Or 16 for 16-bit conversion resolution, which supports (ADC_MODE_DIFFERENTIAL)
// Sample on pair of pins (difference between pins is converted, subject to
// common mode voltage requirements; see the device data manual)

// Globals
//  ................................ISsIIIA_AABBBEEEMMMMMMMMMMMMMMMM
//const uint32_t sr_initial_val = 0b01010000000001111111111111111110;

// ...............................MMMMMMMMMMMMMMMMEEDBBBAA_AIIIsSI
const uint32_t sr_initial_val = 0b01111111111111111010000000001000; // rotated for SPI
volatile uint32_t sr_current_val = sr_initial_val;

const uint8_t def_num_z_channels = 2;  // default num of z channels (2, 4 or 8)
uint8_t num_z_channels = def_num_z_channels;
volatile uint32_t mux_mode = 1; // default mux mode

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

// Global buffer status flags using 8 bits
// Bit 0: Indicates if Buffer Part 1 is filled.
// Bit 1: Indicates if Buffer Part 2 is filled.
// Bit 2: Indicates if Buffer Part 3 is filled.
// Bit 3: Indicates if Buffer Part 4 is filled.
// Bit 4: Indicates the next expected Buffer Part is 1.
// Bit 5: Indicates the next expected Buffer Part is 2.
// Bit 6: Indicates the next expected Buffer Part is 3.
// Bit 7: Indicates the next expected Buffer Part is 4.
volatile uint8_t buffer_status = 0b10000000; // Set next_expected to Part 4 (full_filled).


volatile uint8_t dacOut = 0;

int16_t signal1sin[BUFLEN] = {0};
#pragma DATA_SECTION(signal1sin, "ramgs15");
int16_t signal1cos[BUFLEN] = {0};
#pragma DATA_SECTION(signal1cos, "ramgs15");

/*
int16_t signal2sin[BUFLEN] = {0};
#pragma DATA_SECTION(signal2sin, "ramgs15");
int16_t signal2cos[BUFLEN] = {0};
#pragma DATA_SECTION(signal2cos, "ramgs15");
*/


uint8_t data[NUM_OF_BYTES] = {0};
#pragma DATA_SECTION(data, "ramgs15");
uint16_t ppg_read[3] = {0};
#pragma DATA_SECTION(ppg_read, "SHARERAMGS1");
uint16_t mux_busy_flag;
#pragma DATA_SECTION(mux_busy_flag, "SHARERAMGS0");
// mux_busy_flag = 0;

volatile uint16_t counter = 0; // loop counter
volatile uint16_t mux_counter = 0;
volatile uint16_t sweep_counter = 0;
volatile uint8_t packet_number = 0;
char mode = 'd';


volatile uint16_t led_counter = 0;


//==  Function Prototypes
void configureADC(uint32_t adcBase);
void initEPWM_PWM(void);
void initADCSOC(void);
void execute_normal_mux(void);
void execute_tomography_mux(void);
void execute_freq_sweep(void);
__interrupt void adcA1ISR(void);

// Create signals array (10 frequencies)
SignalsStruct signalsArray[10];
#pragma DATA_SECTION(signalsArray, "ram_extended");

const uint16_t freqValues[] = {1,2,4,7,10,20,40,70,100,200}; // frequency values in kHz
uint16_t activeFreqs[10] = {0,0,0,0,1,0,0,0,0,0}; // by default only 5th frequency (10 kHz) is active

//
// Main
//
void main(void)
{
    sigGen(signal1sin,10,BUFLEN, 's');
    sigGen(signal1cos,10,BUFLEN, 'c');

    /*
    sigGen(signal2sin,100,BUFLEN, 's');
    sigGen(signal2cos,100,BUFLEN, 'c');
    */


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


    // Initialize the signals
    uint16_t i;
    for (i = 0; i < 10; i++) {
        signalsArray[i].ExcitationI = 0;
        signalsArray[i].ExcitationQ = 0;
        signalsArray[i].Response1I = 0;
        signalsArray[i].Response1Q = 0;
        signalsArray[i].Response2I = 0;
        signalsArray[i].Response2Q = 0;
        sigGen(signalsArray[i].RefSigSin,freqValues[i],BUFLEN, 's');
        sigGen(signalsArray[i].RefSigCos,freqValues[i],BUFLEN, 'c');
    }


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

    //Configure DMA to transfer data pack to the SPI buffer
    configure_SPI_DMA();

    // Init SPIB for writing data to shit register
    initSPIBMaster();

    //
    // Interrupts that are used in this example are re-mapped to ISR functions
    Interrupt_register(INT_ADCA1, &adcA1ISR);

    // Set up the ADC and the ePWM and initialize the SOC
    //
    configureADC(ADCA_BASE);
    configureADC(ADCB_BASE);
    configureADC(ADCC_BASE);
    configureADC(ADCD_BASE);

    initEPWM_ADC();
    initADCSOC();
    // Enable ADC interrupt
    Interrupt_enable(INT_ADCA1);
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_TBCLKSYNC);
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    EINT;
    ERTM;

    // Start ePWM1, enabling SOCA and putting the counter in up-count mode
    EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
    EPWM_setTimeBaseCounterMode(EPWM_FOR_ADC_BASE , EPWM_COUNTER_MODE_UP);


    // Configure Shift Register
    initShiftRegister(sr_initial_val);

    // The function will loop through each pair and program the multiplexer to connect each input to its corresponding output.
    // Can also route dynamically in the while loop
    // router(num_pairs_1, mux_inputs_1, mux_outputs_1);
    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);



    start_exc_dma();
    set_excitation_frequency(activeFreqs, signalsArray);  // default frequency in kHz

    while(1){

        static int32_t  accumA1I = 0; //accumulate excitation signal 1 in phase component
        static int32_t  accumA1Q = 0; //accumulate excitation signal 1 quadrature component
        static int32_t  accumB1I = 0; //accumulate response 1 signal 1 in phase component
        static int32_t  accumB1Q = 0; //accumulate response 1 signal 1 quadrature component
        static int32_t  accumC1I = 0; //accumulate response 2 signal 1 in phase component
        static int32_t  accumC1Q = 0; //accumulate response 2 signal 1 quadrature component


        /*
        static int32_t  accumA2I = 0; //accumulate excitation signal 2 in phase component
        static int32_t  accumA2Q = 0; //accumulate excitation signal 2 quadrature component
        static int32_t  accumB2I = 0; //accumulate response 1 signal 2 in phase component
        static int32_t  accumB2Q = 0; //accumulate response 1 signal 2 quadrature component
        static int32_t  accumC2I = 0; //accumulate response 2 signal 2 in phase component
        static int32_t  accumC2Q = 0; //accumulate response 2 signal 2 quadrature component
        */

        int j, k;

        if ((buffer_status & 0b00010001) == 0b00010001){ //Check if Part 1 is filled and next expected is Part 1
            for (j = NUM_OF_BYTES / 4; j < NUM_OF_BYTES / 2; j++){
                 HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];  // UART A
                 HWREGH(SCIB_BASE + SCI_O_TXBUF) = data[j];  // UART B
            }
            buffer_status = 0b00100000; // set next expected flag for Part 2
        }

        else if ((buffer_status & 0b00100010) == 0b00100010) ////Check if Part 2 is filled and next expected is Part 2
        {

            for(k=0;k<10;k++){
                if(activeFreqs[k]){
                    //half of BUFLEN/10 (10 rpt calculates up to BUFLEN/2)
                    signalsArray[k].ExcitationI = _dmac(signalsArray[k].RefSigSin,adcAResults,BUFLEN/40-1,0);
                    signalsArray[k].ExcitationQ = _dmac(signalsArray[k].RefSigCos,adcAResults,BUFLEN/40-1,0);
                    signalsArray[k].Response1I = _dmac(signalsArray[k].RefSigSin,adcBResults,BUFLEN/40-1,0);
                    signalsArray[k].Response1Q = _dmac(signalsArray[k].RefSigCos,adcBResults,BUFLEN/40-1,0);
                    signalsArray[k].Response2I = _dmac(signalsArray[k].RefSigSin,adcCResults,BUFLEN/40-1,0);
                    signalsArray[k].Response2Q = _dmac(signalsArray[k].RefSigCos,adcCResults,BUFLEN/40-1,0);
                }
            }


            for (j = NUM_OF_BYTES / 2; j < 3 * NUM_OF_BYTES / 4; j++){
                 HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];  // UART A
                 HWREGH(SCIB_BASE + SCI_O_TXBUF) = data[j];  // UART B
            }
            buffer_status = 0b01000000; // set next expected flag for Part 3
        }

        else if ((buffer_status & 0b01000100) == 0b01000100){ //Check if Part 3 is filled and next expected is Part 3
            for (j = 3 * NUM_OF_BYTES / 4; j < NUM_OF_BYTES; j++){
                 HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];  // UART A
                 HWREGH(SCIB_BASE + SCI_O_TXBUF) = data[j];  // UART B
            }
            buffer_status = 0b10000000; // set next expected flag for Part 4
        }

        else if ((buffer_status & 0b10001000) == 0b10001000) //Check if Part 4 is filled and next expected is Part 4
        {
            accumD = adcDResult>>9UL; //sum of 500 samples - divided by 2^x. !! may be more than 500, check ISR
            adcDResult = 0;

            for(k=0;k<10;k++){
                if(activeFreqs[k]){
                    //half of BUFLEN/10 (10 rpt calculates up to BUFLEN/2)
                    signalsArray[k].ExcitationI += _dmac(signalsArray[k].RefSigSin+BUFLEN/2,adcAResults+BUFLEN/2,BUFLEN/40-1,0);
                    signalsArray[k].ExcitationQ += _dmac(signalsArray[k].RefSigCos+BUFLEN/2,adcAResults+BUFLEN/2,BUFLEN/40-1,0);
                    signalsArray[k].Response1I += _dmac(signalsArray[k].RefSigSin+BUFLEN/2,adcBResults+BUFLEN/2,BUFLEN/40-1,0);
                    signalsArray[k].Response1Q += _dmac(signalsArray[k].RefSigCos+BUFLEN/2,adcBResults+BUFLEN/2,BUFLEN/40-1,0);
                    signalsArray[k].Response2I += _dmac(signalsArray[k].RefSigSin+BUFLEN/2,adcCResults+BUFLEN/2,BUFLEN/40-1,0);
                    signalsArray[k].Response2Q += _dmac(signalsArray[k].RefSigCos+BUFLEN/2,adcCResults+BUFLEN/2,BUFLEN/40-1,0);
                }
            }


            //pack result
            pack (data, packet_number, signalsArray, &accumD, &mode, &mux_mode, ppg_read, activeFreqs);

            /*
            //pack result
            pack (data, packet_number, &accumA1I, &accumA1Q, &accumB1I, &accumB1Q, &accumC1I, &accumC1Q,
                  &accumD, &mode, &mux_mode, ppg_read);
            */

            //  SCI_writeCharArray(SCIA_BASE, data, 12); //replace with non-blocking code
            for (j = 0; j < NUM_OF_BYTES / 4; j++){
                HWREGH(SCIA_BASE + SCI_O_TXBUF) = data[j];  // UART A
                HWREGH(SCIB_BASE + SCI_O_TXBUF) = data[j];  // UART B
            }

            buffer_status = 0b00010000; // set next expected flag for Part 1

            // led_counter += 1; in interrupt
            /*
            if (led_counter >= 500){
                led_counter = 0;
                updateShiftRegister(0xC0000000, sr_current_val ^ 0xC0000000);
                //updateShiftRegister(0xFFFFFFFF, sr_current_val);
            }
            */
        }

        static char received_command[16] = {0};
        static uint32_t active_sci_base = SCIA_BASE;
        static uint16_t gain_table_1[] = {7, 3, 5, 1, 6, 2, 4, 0};
        static uint16_t gain_table_2[] = {13, 5, 9, 1, 12, 4, 8, 0};

        // Receive command from SCIA or SCIB
        if(SCI_getRxFIFOStatus(SCIA_BASE) != SCI_FIFO_RX0){
            received_command[0] = (char)(HWREGH(SCIA_BASE + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);
            active_sci_base = SCIA_BASE;
        }
        else if (SCI_getRxFIFOStatus(SCIB_BASE) != SCI_FIFO_RX0){
            received_command[0] = (char)(HWREGH(SCIB_BASE + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);
            active_sci_base = SCIB_BASE;
        }

        if(received_command[0]){
            DEVICE_DELAY_US(10000);  // wait 10ms for all message to arrive
            uint32_t temp_sr;
            switch (received_command[0]){
                /*
                case 'd':
                    GPIO_writePin(34, 1);
                    mode = received_command[0];
                    break;
                case 'e':
                    GPIO_writePin(34, 0);
                    mode = received_command[0];
                    break;
                */
                case 'f':  // frequency signal
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    // received_command[2] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // one byte for single frequency
                    // static uint16_t freq_table[] = {1, 2, 4, 7, 10, 20, 40, 70, 100, 200};
                    uint16_t num_of_freqs = received_command[1] - '0' + 1;
                    uint16_t freq_index;
                    int n;
                    for(n=0;n<10;n++)
                        activeFreqs[n] = 0;  // reset active frequencies
                    for(n=0;n<num_of_freqs;n++){
                        freq_index = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M) - '0';  // read, convert to the digit
                        activeFreqs[freq_index] = 1;
                    }
                    // uint16_t freq_kHz = freqValues[received_command[2] - '0'];  // convert to the digit and then fetch the frequency
                    // sigGen(signal1sin, freq_kHz, BUFLEN, 's');
                    // sigGen(signal1cos, freq_kHz, BUFLEN, 'c');
                    // Now, change this function below to generate all active frequencies
                    set_excitation_frequency(activeFreqs, signalsArray);  // frequency in kHz
                    mux_counter = 0;
                    break;

                case 's':  // Frequency sweep
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // zero, dummy read
                    for(n=0;n<10;n++)
                        activeFreqs[n] = 0;  // reset active frequencies
                    // activeFreqs[0] = 1;  // start from the first frequency
                    // set_excitation_frequency(activeFreqs, signalsArray);  // frequency in kHz
                    mux_counter = 0;
                    sweep_mode = 1;
                    break;


                case 'x': // Excitation gain -
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    updateShiftRegister(0x00000038, mirror(mirror(sr_current_val) + 0x04000000));
                    break;
                case 'X': // Excitation gain + or set
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    if (received_command[1] == '0')
                        updateShiftRegister(0x00000038, mirror(mirror(sr_current_val) - 0x04000000));
                    else{
                        received_command[2] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // gain
                        uint16_t bit_values = gain_table_1[received_command[2] - '0'];
                        updateShiftRegister(0x00000038, (uint32_t)(bit_values << 3));
                    }
                    break;
                case 'a': // Sense A gain -
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    temp_sr = (((mirror(sr_current_val) & 0x02000000) | (mirror(sr_current_val) & 0x00C00000) << 1)) + 0x00800000;
                    temp_sr = (temp_sr & 0x02000000) | ((temp_sr & 0x01800000) >> 1);
                    updateShiftRegister(0x000003C0, mirror(temp_sr));
                    break;
                case 'A': // Sense A gain + or set
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    if (received_command[1] == '0'){
                        temp_sr = (((mirror(sr_current_val) & 0x00C00000) << 1) | (mirror(sr_current_val) & 0x02000000)) - 0x00800000;
                        temp_sr = (temp_sr & 0x02000000) | ((temp_sr & 0x01800000) >> 1);
                        updateShiftRegister(0x000003C0, mirror(temp_sr));
                    }
                    else{
                        received_command[2] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // gain
                        uint16_t bit_values = gain_table_2[received_command[2] - '0'];
                        updateShiftRegister(0x000003C0, (uint32_t)(bit_values << 6));
                    }
                    break;
                case 'b': // Sense B gain -
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    updateShiftRegister(0x00001C00, mirror(mirror(sr_current_val) + 0x00080000));
                    break;
                case 'B': // Sense B gain + or set
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    if (received_command[1] == '0')
                        updateShiftRegister(0x00001C00, mirror(mirror(sr_current_val) - 0x00080000));
                    else{
                        received_command[2] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // gain
                        uint16_t bit_values = gain_table_1[received_command[2] - '0'];
                        updateShiftRegister(0x00001C00, (uint32_t)(bit_values << 10));
                    }
                    break;
                case 'w':   // wave mode
                    tomo_mode = 0;
                    received_command[1] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // length
                    received_command[2] = (char)(HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M);  // number of channels
                    if (received_command[2] == '4'){  // 4 channel mode
                        num_z_channels = 4;
                        mux_counter = 0;
                    }
                    else if (received_command[2] == '8'){   // 8 channel mode
                        num_z_channels = 8;
                        mux_counter = 0;
                    }
                    else {   // default 2 Channel mode. Set mux for first 2 channels and don't change dynamically
                        num_z_channels = 2;
                        mux_counter = 0;
                        mux_busy_flag = 1;
                        router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
                        mux_busy_flag = 0;
                        mux_mode = 1;
                    }
                    break;
                case 't':
                    tomo_mode = 1;
                    mux_counter = mux_period;
                    break;
                case 'n':
                    tomo_mode = 0;
                    break;
                case 'i': // Initialize again
                    sr_current_val = sr_initial_val;
                    initShiftRegister(sr_initial_val); // test
                    router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
                    num_z_channels = def_num_z_channels;
                    mux_mode = 1;
                    mux_counter = 0;
                    tomo_mode = 0;
                    sweep_mode = 0;
                    break;
            }
            int k = 0;
            for(k=0; k<16; k++){
                received_command[k] = 0;
                HWREGH(active_sci_base + SCI_O_RXBUF) & SCI_RXBUF_SAR_M;  // dummy read to flush FIFO
            }
        }


        if (tomo_mode)
            execute_tomography_mux();
        else
            execute_normal_mux();

        if (sweep_mode){
            execute_freq_sweep();
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
    EPWM_setCounterCompareValue(EPWM_FOR_ADC_BASE , EPWM_COUNTER_COMPARE_B, 50);
    EPWM_setTimeBasePeriod(EPWM_FOR_ADC_BASE , 99);

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


void execute_normal_mux(void){
    if (num_z_channels == 2)
        return;

    if (mux_mode!= 1  &&  mux_counter == mux_period) {
        mux_busy_flag = 1;
        router_config(num_pairs_1, mux_inputs_1, mux_outputs_1);
        mux_busy_flag = 0;
        mux_mode = 1;
    } else if (mux_mode!= 2  &&  mux_counter == mux_period * 2) {
        mux_busy_flag = 1;
        router_config(num_pairs_2, mux_inputs_2, mux_outputs_2);
        mux_busy_flag = 0;
        mux_mode = 2;
        if (num_z_channels == 4) {
            mux_counter = 0; // Only reset the counter for 4-channel mode here
        }
    }

    if (num_z_channels > 4) {
        if (mux_mode!= 3  &&  mux_counter == mux_period * 3) {
            mux_busy_flag = 1;
            router_config(num_pairs_3, mux_inputs_3, mux_outputs_3);
            mux_busy_flag = 0;
            mux_mode = 3;
        } else if (mux_mode!= 4  &&  mux_counter == mux_period * 4) {
            mux_busy_flag = 1;
            router_config(num_pairs_4, mux_inputs_4, mux_outputs_4);
            mux_busy_flag = 0;
            mux_mode = 4;
            if (num_z_channels == 8) {
                mux_counter = 0; // Only reset the counter for 8-channel mode here
            }
        }
    }
}


void execute_tomography_mux(void){
    static uint16_t mux_index = 0;
    static uint16_t* tomo_inputs = mux_inputs;  // Inputs are the same all the time
    static uint16_t exc_out = 0;
    static uint16_t ret_out = 0;
    static uint16_t sns_ap_out = 0;
    static uint16_t sns_an_out = 0;
    static uint16_t sns_bp_out = 0; // b channel not used yet
    static uint16_t sns_bn_out = 0; // b channel not used yet

    if (mux_counter < mux_period)
        return;

    // Get the current configuration from the lookup table
    MuxConfig current_config = tomo_mux_lookup_table[mux_index];

    exc_out = current_config.exc_out;
    sns_ap_out = current_config.sns_ap_out;
    sns_an_out = current_config.sns_an_out;
    ret_out = current_config.ret_out;

    // Setup tomo_outputs array using the current configuration
    uint16_t tomo_outputs[] = {mux_outputs[exc_out], mux_outputs[sns_ap_out], mux_outputs[sns_an_out], mux_outputs[ret_out]};
    mux_busy_flag = 1;
    router_config(4, tomo_inputs, tomo_outputs); // Assuming 'mux_inputs' is defined somewhere
    mux_busy_flag = 0;

    // Set mux_mode for the current configuration
    mux_mode =  ((uint32_t)exc_out << 25) | ((uint32_t)sns_ap_out << 20) | ((uint32_t)sns_an_out << 15) |
                ((uint32_t)sns_bp_out << 10) | ((uint32_t)sns_bn_out << 5) | ((uint32_t)ret_out);

    // Increment the index for the next configuration
    mux_index += 1;
    if (mux_index == NUM_CONFIGS)
        mux_index = 0;

    // Reset mux_counter for the next period
    mux_counter = 0;
}

void execute_freq_sweep(void){
    static int active_freq_index = 9;
    if (sweep_counter >= sweep_period){
        activeFreqs[active_freq_index] = 0;
        active_freq_index++;
        if(active_freq_index == 10)
            active_freq_index = 0;
        activeFreqs[active_freq_index] = 1;

        set_excitation_frequency(activeFreqs, signalsArray);  // frequency in kHz
        sweep_counter = 0;
        // mux_counter = 0;  // ? check with mux
    }
}


void configure_SPI_DMA(void){
}


void initSPIBMaster(void)
{
    // Must put SPI into reset before configuring it
    SPI_disableModule(SPIB_BASE);

    // SPI configuration. Use a 1MHz SPICLK and 16-bit word size.
    SPI_setConfig(SPIB_BASE, DEVICE_LSPCLK_FREQ, SPI_PROT_POL0PHA1,
                  SPI_MODE_MASTER, 10000000, 16);

    // Set high-speed mode
    HWREGH(SPIB_BASE + SPI_O_CCR) = HWREGH(SPIB_BASE + SPI_O_CCR) | SPI_CCR_HS_MODE;

    SPI_disableLoopback(SPIB_BASE);
    SPI_setEmulationMode(SPIB_BASE, SPI_EMULATION_FREE_RUN);

    // FIFO and configuration
    SPI_enableFIFO(SPIB_BASE);

    // Configuration complete. Enable the module.
    SPI_enableModule(SPIB_BASE);
}

__interrupt void adcA1ISR(void)
{   adcAResults[counter] = HWREGH(ADCADDR) - 32767; // make signed
    adcBResults[counter] = HWREGH(ADCBDDR) - 32767;
    adcCResults[counter] = HWREGH(ADCCDDR) - 32767;
    adcDResult += HWREGH(ADCDDDR);

    // Clear the interrupt flag
    // ADC_clearInterruptStatus(ADCA_BASE, ADC_INT_NUMBER1);
    HWREGH(ADCA_BASE + ADC_O_INTFLGCLR) = 0x0001;

    counter++;
    if (counter == BUFLEN/4){
        buffer_status |= 0b00000001;  // Part 1 is ready
    }
    else if (counter == BUFLEN/2){  // if (counter == BUFLEN/2 - 1){
        buffer_status |= 0b00000010;  // Part 2 is ready
    }
    else if (counter == 3*BUFLEN/4){
        buffer_status |= 0b00000100;  // Part 3 is ready
    }
    else if (counter >= BUFLEN){
        buffer_status |= 0b00001000;  // Part 4 is ready
        counter = 0;
        packet_number++;
        mux_counter += 1; // 1kHz - 1 ms per sample
        sweep_counter += 1;
        led_counter += 1;
    }

    // Acknowledge the interrupt
    //  Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
    HWREGH(PIECTRL_BASE + PIE_O_ACK) = INTERRUPT_ACK_GROUP1;

    /*
    HWREGH(ADCA_BASE + ADC_O_INTFLGCLR) = 0x0001;
    counter++;
    if (counter >= 500000){
        updateShiftRegister(0x00000003, sr_current_val ^ 0x00000003);
        counter = 0;
    }
    HWREGH(PIECTRL_BASE + PIE_O_ACK) = INTERRUPT_ACK_GROUP1;
    */
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
