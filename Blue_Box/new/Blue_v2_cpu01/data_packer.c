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

    int i, activeFreq;
    for(i = 0; i < 10; i++)
        if (activeFreqs[i]){
            activeFreq = i;
            data[9] = (uint32_t)(signalsArray[i].ExcitationI >> 16) & 0xFF;       //send only 16 MSB
            data[10] = (uint32_t)(signalsArray[i].ExcitationI >> 24) & 0xFF;
            data[11] = (uint32_t)(signalsArray[i].ExcitationQ >> 16) & 0xFF;
            data[12] = (uint32_t)(signalsArray[i].ExcitationQ >> 24) & 0xFF;
            data[13] = (uint32_t)(signalsArray[i].Response1I >> 16) & 0xFF;
            data[14] = (uint32_t)(signalsArray[i].Response1I >> 24) & 0xFF;
            data[15] = (uint32_t)(signalsArray[i].Response1Q >> 16) & 0xFF;
            data[16] = (uint32_t)(signalsArray[i].Response1Q >> 24) & 0xFF;
            data[17] = (uint32_t)(signalsArray[i].Response2I >> 16) & 0xFF;
            data[18] = (uint32_t)(signalsArray[i].Response2I >> 24) & 0xFF;
            data[19] = (uint32_t)(signalsArray[i].Response2Q >> 16) & 0xFF;
            data[20] = (uint32_t)(signalsArray[i].Response2Q >> 24) & 0xFF;
            break;
        }

    data[21] = (*accumD) & 0xFF; //ECG data
    data[22] = (*accumD >> 8) & 0xFF;
    data[23] = (ppg[0]) & 0xFF; //PPG data
    data[24] = (ppg[0] >> 8) & 0xFF;
    data[25] = activeFreq & 0xFF;
}


