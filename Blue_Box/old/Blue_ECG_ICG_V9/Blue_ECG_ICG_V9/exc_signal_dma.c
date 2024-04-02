// #include "driverlib.h"
// #include "device.h"
#include "F28x_Project.h"
#include "math.h"
#include "exc_signal_dma.h"
#include "constants.h"

//
// Defines
//
#define REFERENCE_VDAC        0
#define REFERENCE_VREF        1
#define REFERENCE             REFERENCE_VREF
#define CPUFREQ_MHZ           200
#define PI                    3.14159265
#define SINE_TBL_SIZE         BUFLEN

//
// Globals
//
static int16_t SINE_TBL_P[SINE_TBL_SIZE];
static int16_t SINE_TBL_N[SINE_TBL_SIZE];
#pragma DATA_SECTION(SINE_TBL_P, "ramgs14");
#pragma DATA_SECTION(SINE_TBL_N, "ramgs14");
static volatile struct DAC_REGS* DAC_PTR[4] = {0x0,&DacaRegs,&DacbRegs,&DaccRegs};
static uint16_t tableStep = 1;
static float waveformGain = 0.5; // Range 0.0 -> 1.0


static volatile uint16_t *DMADest_P;
static volatile uint16_t *DMASource_P;
static volatile uint16_t *DMADest_N;
static volatile uint16_t *DMASource_N;

//
// Function Prototypes
//
static void configureDAC(void);
static void configureDMA(void);
static void configureWaveform(uint16_t new_freq);

void sigGen (int16_t signal[], int f, int len, char s); // f in kHz, s = 's' for sine, 'c' for cosine


void start_exc_dma(void){

    //
    // Configure DAC
    //
        configureDAC();

    //
    // Configure Waveform
    //
        configureWaveform(10);  // 10kHz default frequency

    //
    // Configure DMA
    //
        configureDMA();

}


//
// configureDAC - Enable and configure the requested DAC module
//
void configureDAC()
{
    EALLOW;

    DAC_PTR[1]->DACCTL.bit.DACREFSEL = REFERENCE;
    DAC_PTR[1]->DACOUTEN.bit.DACOUTEN = 1;
    DAC_PTR[1]->DACVALS.all = 0;

    DAC_PTR[2]->DACCTL.bit.DACREFSEL = REFERENCE;
    DAC_PTR[2]->DACOUTEN.bit.DACOUTEN = 1;
    DAC_PTR[2]->DACVALS.all = 0;

    DELAY_US(10); // Delay for buffered DAC to power up

    EDIS;
}

//
// configureDMA - Configures the DMA to read from the SINE table and write to the DAC
//
void configureDMA()
{
    //
    // Ensure DMA is connected to Peripheral Frame 1 bridge which contains the DAC
    //
    EALLOW;
    CpuSysRegs.SECMSEL.bit.PF1SEL = 1;
    EDIS;

    //
    // Initialize DMA
    //
    DMAInitialize();
    DMASource_P = (volatile uint16_t *)&SINE_TBL_P[0];
    DMADest_P = (volatile uint16_t *)&DAC_PTR[1]->DACVALS;

    DMASource_N = (volatile uint16_t *)&SINE_TBL_N[0];
    DMADest_N = (volatile uint16_t *)&DAC_PTR[2]->DACVALS;
    //
    // Configure DMA CH5
    //
    DMACH5AddrConfig(DMADest_P,DMASource_P);
    DMACH5BurstConfig(0,0,0);
    DMACH5TransferConfig(SINE_TBL_SIZE/tableStep-1,tableStep,0);
    DMACH5WrapConfig(SINE_TBL_SIZE/tableStep-1,0,0,0);
    DMACH5ModeConfig(0,PERINT_ENABLE,ONESHOT_DISABLE,CONT_ENABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_DISABLE);

    //
    // Configure DMA CH6
    //
    DMACH6AddrConfig(DMADest_N,DMASource_N);
    DMACH6BurstConfig(0,0,0);
    DMACH6TransferConfig(SINE_TBL_SIZE/tableStep-1,tableStep,0);
    DMACH6WrapConfig(SINE_TBL_SIZE/tableStep-1,0,0,0);
    DMACH6ModeConfig(0,PERINT_ENABLE,ONESHOT_DISABLE,CONT_ENABLE,
                     SYNC_DISABLE,SYNC_SRC,OVRFLOW_DISABLE,SIXTEEN_BIT,
                     CHINT_END,CHINT_DISABLE);

    EALLOW;
    DmaClaSrcSelRegs.DMACHSRCSEL2.bit.CH5 = DMA_EPWM2B;    //
    DmaClaSrcSelRegs.DMACHSRCSEL2.bit.CH6 = DMA_EPWM2B;    //
    EDIS;

    StartDMACH5(); // Start DMA channel
    StartDMACH6(); // Start DMA channel
}


//
// configureWaveform - Configure the SINE waveform
//
void configureWaveform(uint16_t new_freq)
{
    uint16_t j;
    float offset;
    float waveformValue;


    sigGen(SINE_TBL_P,new_freq,BUFLEN, 's');
    for(j=0;j<SINE_TBL_SIZE;j++)
        {
            SINE_TBL_P[j] = (int16_t)(SINE_TBL_P[j]*waveformGain + 1000)*2.0475;
        }

    for(j=0;j<SINE_TBL_SIZE;j++)
    {
        SINE_TBL_N[j] = 4095 - SINE_TBL_P[j];
    }
}

// Function to set sampling frequency
void set_excitation_frequency(uint16_t new_freq) {
    configureWaveform(new_freq);
}
//
// End of file
//
