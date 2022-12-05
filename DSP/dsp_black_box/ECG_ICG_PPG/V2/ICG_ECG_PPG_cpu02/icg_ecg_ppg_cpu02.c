// Included Files
#include "driverlib.h"
#include "device.h"
//#include "F2837xD_Ipc_drivers.h"
#include "max30101.h"

//Globals
uint16_t cpu2RWArray[256];      // Mapped to GS14 of shared RAM owned by CPU02
#pragma DATA_SECTION(cpu2RWArray,"SHARERAMGS14");

// Function Prototypes
void initI2C(void);
void initMAX30101(void);


void main(void){
    Device_init();
    DEVICE_DELAY_US(1000000);

    int i;
    for (i = 0; i < 256; i++) // clean the shared RAM
        cpu2RWArray[i] = 0;

    initI2C();
    initMAX30101();

    uint8_t ppg_sample [3] = {0};
    uint16_t ppg_result = 0;
    i = 0;
    while(1){

        if (newDataReady()){ // 800Hz
            readRegister(MAX30105_FIFODATA, ppg_sample, 3);
            ppg_result = (ppg_sample[0] << 6) | (ppg_sample[1] >> 2); //16 bit ADC in 3 bytes, 17th bit is MSB
            ppg_result = (ppg_result << 8) | ((ppg_sample[1] << 6) | (ppg_sample[2] >> 2));
            cpu2RWArray[0] = ppg_result;
        }

    }
}
