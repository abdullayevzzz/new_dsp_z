/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"

void pack (uint8_t* data, uint8_t packetNumber, int32_t  *accumA1I, int32_t  *accumA1Q, int32_t  *accumB1I, int32_t  *accumB1Q,
           int32_t  *accumC1I, int32_t  *accumC1Q, volatile  uint32_t  *accumD, char *mode, uint32_t mux_mode, uint16_t* ppg) {

    data[0] = 0x7F;  //synchronization
    data[1] = 0xFF;

    static uint8_t config = 0x00;
    static uint16_t test_counter = 0x00;

    // data[2] = (*packetNumber) & 0xFF;  //prepared packet count
    // data[3] = (*packetNumber >> 8) & 0xFF;

    data[2] = (28 << 2) & 0xFC;
    data[2] |= 0x00; // packet type. 0x00 -> data, ...
    data[3] = packetNumber;
    data[4] = mux_mode & 0xFF; //
    data[5] = (uint32_t)(mux_mode >> 8) & 0xFF; //
    data[6] = (uint32_t)(mux_mode >> 16) & 0xFF; //
    data[7] = (uint32_t)(mux_mode >> 24) & 0xFF; //


    if (*mode == 'd') {
        data[8] = (*accumA1I >> 16) & 0xFF; //send only 16 MSB
        data[9] = (*accumA1I >> 24) & 0xFF;

        data[10] = (*accumA1Q >> 16) & 0xFF;
        data[11] = (*accumA1Q >> 24) & 0xFF;

        data[12] = (*accumB1I >> 16) & 0xFF;
        data[13] = (*accumB1I >> 24) & 0xFF;

        data[14] = (*accumB1Q >> 16) & 0xFF;
        data[15] = (*accumB1Q >> 24) & 0xFF;

        data[16] = (*accumC1I >> 16) & 0xFF;
        data[17] = (*accumC1I >> 24) & 0xFF;

        data[18] = (*accumC1Q >> 16) & 0xFF;
        data[19] = (*accumC1Q >> 24) & 0xFF;
    }

    else if (*mode == 'e'){
/*      float nomin = 2.5;
        float denom = 5;
        float fResult = nomin/denom;
        uint32_t *dResult;  //cast to int
        dResult = &fResult;
*/
        float fAccumA1I = (float)*accumA1I;
        float fAccumA1Q = (float)*accumA1Q;
        float fAccumB1I = (float)*accumB1I;
        float fAccumB1Q = (float)*accumB1Q;
        float fAccumC1I = (float)*accumC1I;
        float fAccumC1Q = (float)*accumC1Q;

        float ratioReal = (fAccumA1I*fAccumB1I+fAccumA1Q*fAccumB1Q)/(fAccumB1I*fAccumB1I+fAccumB1Q*fAccumB1Q);
        float ratioImag = (-fAccumA1I*fAccumB1Q+fAccumA1Q*fAccumB1I)/(fAccumB1I*fAccumB1I+fAccumB1Q*fAccumB1Q);
        float ratioReal2 = (fAccumA1I*fAccumC1I+fAccumA1Q*fAccumC1Q)/(fAccumC1I*fAccumC1I+fAccumC1Q*fAccumC1Q);
        float ratioImag2 = (-fAccumA1I*fAccumC1Q+fAccumA1Q*fAccumC1I)/(fAccumC1I*fAccumC1I+fAccumC1Q*fAccumC1Q);

        float fRatioReal = ratioReal;
        uint32_t *dRatioReal;  //cast to int
        dRatioReal = &fRatioReal;

        float fRatioImag = ratioImag;
        uint32_t *dRatioImag;  //cast to int
        dRatioImag = &fRatioImag;

        float fRatioReal2 = ratioReal2;
        uint32_t *dRatioReal2;  //cast to int
        dRatioReal2 = &fRatioReal2;

        float fRatioImag2 = ratioImag2;
        uint32_t *dRatioImag2;  //cast to int
        dRatioImag2 = &fRatioImag2;

        data[8] = (*dRatioReal) & 0xFF;
        data[9] = (*dRatioReal >> 8) & 0xFF;
        data[10] = (*dRatioReal >> 16) & 0xFF;
        data[11] = (*dRatioReal >> 24) & 0xFF;

        data[12] = (*dRatioImag) & 0xFF;
        data[13] = (*dRatioImag >> 8) & 0xFF;
        data[14] = (*dRatioImag >> 16) & 0xFF;
        data[15] = (*dRatioImag >> 24) & 0xFF;

        data[16] = (*dRatioReal2) & 0xFF;
        data[17] = (*dRatioReal2 >> 8) & 0xFF;
        data[18] = (*dRatioReal2 >> 16) & 0xFF;
        data[19] = (*dRatioReal2 >> 24) & 0xFF;

        data[20] = (*dRatioImag2) & 0xFF;
        data[21] = (*dRatioImag2 >> 8) & 0xFF;
        data[22] = (*dRatioImag2 >> 16) & 0xFF;
        data[23] = (*dRatioImag2 >> 24) & 0xFF;
    }

    data[24] = (*accumD) & 0xFF; //ECG data
    data[25] = (*accumD >> 8) & 0xFF;

    data[27] = (ppg[0]) & 0xFF; //PPG data
    data[26] = (ppg[0] >> 8) & 0xFF;


    // eliminate sync. problem
    /*int i;
    for (i = 2; i < 11; i++){
        if (data[i] == 0x7F && data[i+1] == 0xFF)
            data[i] == 0x7E;
    }
    */
}


