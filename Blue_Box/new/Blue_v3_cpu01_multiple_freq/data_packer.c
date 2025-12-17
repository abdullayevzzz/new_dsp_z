/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"
#include "definitions.h"

void pack (uint8_t* data, uint8_t packetNumber, SignalsStruct *signalsArray, volatile  uint32_t  *accumD, char *mode, uint32_t *mux_mode, uint16_t* ppg,
           uint16_t activeFreqs[]) {

    data[0] = 0xC7;                                     // synchronization
    data[1] = 0xC7;                                     // synchronization

    data[2] = 25 & 0xFF;                                // packet length
    data[3] = packetNumber & 0xFF;                      // packet number
    data[4] = 0x00;                                     // packet mode - Default (Dual channel, 1kHz sampling, single frequency)

    data[5] = *mux_mode & 0xFF;                         // mux mode
    data[6] = (uint32_t)(*mux_mode >> 8) & 0xFF;        // mux mode
    data[7] = (uint32_t)(*mux_mode >> 16) & 0xFF;       // mux mode
    data[8] = (uint32_t)(*mux_mode >> 24) & 0xFF;       // mux mode

    data[9] = (uint32_t)(signalsArray[0].Response2I >> 16) & 0xFF;  // RAW ADC samples
    data[10] = (uint32_t)(signalsArray[0].Response2I >> 24) & 0xFF;
    data[11] = (uint32_t)(signalsArray[0].Response2Q >> 16) & 0xFF;
    data[12] = (uint32_t)(signalsArray[0].Response2Q >> 24) & 0xFF;

    int i=0, numOfFreqs=0, indexAdd;
    for(i; i<10; i++){
        if (activeFreqs[i]){
            indexAdd = numOfFreqs*8;
            numOfFreqs++;
            data[13 + indexAdd] = (uint32_t)(signalsArray[i].ExcitationI >> 16) & 0xFF;       //send only 16 MSB
            data[14 + indexAdd] = (uint32_t)(signalsArray[i].ExcitationI >> 24) & 0xFF;
            data[15 + indexAdd] = (uint32_t)(signalsArray[i].ExcitationQ >> 16) & 0xFF;
            data[16 + indexAdd] = (uint32_t)(signalsArray[i].ExcitationQ >> 24) & 0xFF;
            data[17 + indexAdd] = (uint32_t)(signalsArray[i].Response1I >> 16) & 0xFF;
            data[18 + indexAdd] = (uint32_t)(signalsArray[i].Response1I >> 24) & 0xFF;
            data[19 + indexAdd] = (uint32_t)(signalsArray[i].Response1Q >> 16) & 0xFF;
            data[20 + indexAdd] = (uint32_t)(signalsArray[i].Response1Q >> 24) & 0xFF;
        }
    }

    /*
    data[21] = (*accumD) & 0xFF; //ECG data
    data[22] = (*accumD >> 8) & 0xFF;
    data[23] = (ppg[0]) & 0xFF; //PPG data
    data[24] = (ppg[0] >> 8) & 0xFF;
    data[25] = activeFreq & 0xFF;
    */
}


