/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"

void pack (uint8_t* data, uint8_t packetNumber, int32_t  *accumA1I, int32_t  *accumA1Q, int32_t  *accumB1I, int32_t  *accumB1Q,
           int32_t  *accumC1I, int32_t  *accumC1Q, volatile  uint32_t  *accumD, char *mode, uint32_t *mux_mode, uint16_t* ppg) {

    data[0] = 0xC7;                                     // synchronization
    data[1] = 0xC7;                                     // synchronization

    data[2] = 25 & 0xFF;                                // packet length
    data[3] = packetNumber & 0xFF;                      // packet number
    data[4] = 0x00;                                     // packet mode - Default (Dual channel, 1kHz sampling, single frequency)

    data[5] = *mux_mode & 0xFF;                         // mux mode
    data[6] = (uint32_t)(*mux_mode >> 8) & 0xFF;        // mux mode
    data[7] = (uint32_t)(*mux_mode >> 16) & 0xFF;       // mux mode
    data[8] = (uint32_t)(*mux_mode >> 24) & 0xFF;       // mux mode

    data[9] = (uint32_t)(*accumA1I >> 16) & 0xFF;       //send only 16 MSB
    data[10] = (uint32_t)(*accumA1I >> 24) & 0xFF;
    data[11] = (uint32_t)(*accumA1Q >> 16) & 0xFF;
    data[12] = (uint32_t)(*accumA1Q >> 24) & 0xFF;
    data[13] = (uint32_t)(*accumB1I >> 16) & 0xFF;
    data[14] = (uint32_t)(*accumB1I >> 24) & 0xFF;
    data[15] = (uint32_t)(*accumB1Q >> 16) & 0xFF;
    data[16] = (uint32_t)(*accumB1Q >> 24) & 0xFF;
    data[17] = (uint32_t)(*accumC1I >> 16) & 0xFF;
    data[18] = (uint32_t)(*accumC1I >> 24) & 0xFF;
    data[19] = (uint32_t)(*accumC1Q >> 16) & 0xFF;
    data[20] = (uint32_t)(*accumC1Q >> 24) & 0xFF;

    data[21] = (*accumD) & 0xFF; //ECG data
    data[22] = (*accumD >> 8) & 0xFF;

    data[23] = (ppg[0]) & 0xFF; //PPG data
    data[24] = (ppg[0] >> 8) & 0xFF;


    /*
    else if (*mode == 'e'){

        // signal 1
        float fAccumA1I = (float)*accumA1I;
        float fAccumA1Q = (float)*accumA1Q;
        float fAccumB1I = (float)*accumB1I;
        float fAccumB1Q = (float)*accumB1Q;
        float fAccumC1I = (float)*accumC1I;
        float fAccumC1Q = (float)*accumC1Q;

        // signal 2
        float fAccumA2I = (float)*accumA2I;
        float fAccumA2Q = (float)*accumA2Q;
        float fAccumB2I = (float)*accumB2I;
        float fAccumB2Q = (float)*accumB2Q;
        float fAccumC2I = (float)*accumC2I;
        float fAccumC2Q = (float)*accumC2Q;

        float ratioRealS1C1 = (fAccumA1I*fAccumB1I+fAccumA1Q*fAccumB1Q)/(fAccumB1I*fAccumB1I+fAccumB1Q*fAccumB1Q);  // signal 1, channel 1
        float ratioImagS1C1 = (-fAccumA1I*fAccumB1Q+fAccumA1Q*fAccumB1I)/(fAccumB1I*fAccumB1I+fAccumB1Q*fAccumB1Q);
        float ratioRealS1C2 = (fAccumA1I*fAccumC1I+fAccumA1Q*fAccumC1Q)/(fAccumC1I*fAccumC1I+fAccumC1Q*fAccumC1Q);
        float ratioImagS1C2 = (-fAccumA1I*fAccumC1Q+fAccumA1Q*fAccumC1I)/(fAccumC1I*fAccumC1I+fAccumC1Q*fAccumC1Q);

        float ratioRealS2C1 = (fAccumA2I*fAccumB2I+fAccumA2Q*fAccumB2Q)/(fAccumB2I*fAccumB2I+fAccumB2Q*fAccumB2Q);  // signal 2, channel 1
        float ratioImagS2C1 = (-fAccumA2I*fAccumB2Q+fAccumA2Q*fAccumB2I)/(fAccumB2I*fAccumB2I+fAccumB2Q*fAccumB2Q);
        float ratioRealS2C2 = (fAccumA2I*fAccumC2I+fAccumA2Q*fAccumC2Q)/(fAccumC2I*fAccumC2I+fAccumC2Q*fAccumC2Q);
        float ratioImagS2C2 = (-fAccumA2I*fAccumC2Q+fAccumA2Q*fAccumC2I)/(fAccumC2I*fAccumC2I+fAccumC2Q*fAccumC2Q);

        float fRatioRealS1C1 = ratioRealS1C1;
        uint32_t *dRatioRealS1C1;  //cast to int
        dRatioRealS1C1 = &fRatioRealS1C1;
        float fRatioImagS1C1 = ratioImagS1C1;
        uint32_t *dRatioImagS1C1;  //cast to int
        dRatioImagS1C1 = &fRatioImagS1C1;
        float fRatioRealS1C2 = ratioRealS1C2;
        uint32_t *dRatioRealS1C2;  //cast to int
        dRatioRealS1C2 = &fRatioRealS1C2;
        float fRatioImagS1C2 = ratioImagS1C2;
        uint32_t *dRatioImagS1C2;  //cast to int
        dRatioImagS1C2 = &fRatioImagS1C2;

        float fRatioRealS2C1 = ratioRealS2C1;
        uint32_t *dRatioRealS2C1;  //cast to int
        dRatioRealS2C1 = &fRatioRealS2C1;
        float fRatioImagS2C1 = ratioImagS2C1;
        uint32_t *dRatioImagS2C1;  //cast to int
        dRatioImagS2C1 = &fRatioImagS2C1;
        float fRatioRealS2C2 = ratioRealS2C2;
        uint32_t *dRatioRealS2C2;  //cast to int
        dRatioRealS2C2 = &fRatioRealS2C2;
        float fRatioImagS2C2 = ratioImagS2C2;
        uint32_t *dRatioImagS2C2;  //cast to int
        dRatioImagS2C2 = &fRatioImagS2C2;

        data[8] = (*dRatioRealS1C1) & 0xFF;
        data[9] = (*dRatioRealS1C1 >> 8) & 0xFF;
        data[10] = (*dRatioRealS1C1 >> 16) & 0xFF;
        data[11] = (*dRatioRealS1C1 >> 24) & 0xFF;
        data[12] = (*dRatioImagS1C1) & 0xFF;
        data[13] = (*dRatioImagS1C1 >> 8) & 0xFF;
        data[14] = (*dRatioImagS1C1 >> 16) & 0xFF;
        data[15] = (*dRatioImagS1C1 >> 24) & 0xFF;
        data[16] = (*dRatioRealS1C2) & 0xFF;
        data[17] = (*dRatioRealS1C2 >> 8) & 0xFF;
        data[18] = (*dRatioRealS1C2 >> 16) & 0xFF;
        data[19] = (*dRatioRealS1C2 >> 24) & 0xFF;
        data[20] = (*dRatioImagS1C2) & 0xFF;
        data[21] = (*dRatioImagS1C2 >> 8) & 0xFF;
        data[22] = (*dRatioImagS1C2 >> 16) & 0xFF;
        data[23] = (*dRatioImagS1C2 >> 24) & 0xFF;

        data[24] = (*dRatioRealS2C1) & 0xFF;
        data[25] = (*dRatioRealS2C1 >> 8) & 0xFF;
        data[26] = (*dRatioRealS2C1 >> 16) & 0xFF;
        data[27] = (*dRatioRealS2C1 >> 24) & 0xFF;
        data[28] = (*dRatioImagS2C1) & 0xFF;
        data[29] = (*dRatioImagS2C1 >> 8) & 0xFF;
        data[30] = (*dRatioImagS2C1 >> 16) & 0xFF;
        data[31] = (*dRatioImagS2C1 >> 24) & 0xFF;
        data[32] = (*dRatioRealS2C2) & 0xFF;
        data[33] = (*dRatioRealS2C2 >> 8) & 0xFF;
        data[34] = (*dRatioRealS2C2 >> 16) & 0xFF;
        data[35] = (*dRatioRealS2C2 >> 24) & 0xFF;
        data[36] = (*dRatioImagS2C2) & 0xFF;
        data[37] = (*dRatioImagS2C2 >> 8) & 0xFF;
        data[38] = (*dRatioImagS2C2 >> 16) & 0xFF;
        data[39] = (*dRatioImagS2C2 >> 24) & 0xFF;

    }
    */


    // eliminate sync. problem
    /*int i;
    for (i = 2; i < 11; i++){
        if (data[i] == 0x7F && data[i+1] == 0xFF)
            data[i] == 0x7E;
    }
    */
}


