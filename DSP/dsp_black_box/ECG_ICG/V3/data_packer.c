/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"

void pack (uint8_t* data, uint16_t *packetNumber, int32_t  *accumA1I, int32_t  *accumA1Q, int32_t  *accumB1I, int32_t  *accumB1Q,  uint32_t  *accumC, char *mode) {

    data[0] = 0x7F;  //synchronization
    data[1] = 0xFF;

    data[2] = (*packetNumber) & 0xFF;  //prepared packet count
    data[3] = (*packetNumber >> 8) & 0xFF;

    if (*mode == 'd') {
        data[4] = (*accumA1I >> 16) & 0xFF; //send only 16 MSB
        data[5] = (*accumA1I >> 24) & 0xFF;

        data[6] = (*accumA1Q >> 16) & 0xFF;
        data[7] = (*accumA1Q >> 24) & 0xFF;

        data[8] = (*accumB1I >> 16) & 0xFF;
        data[9] = (*accumB1I >> 24) & 0xFF;

        data[10] = (*accumB1Q >> 16) & 0xFF;
        data[11] = (*accumB1Q >> 24) & 0xFF;
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

        float ratioReal = (fAccumA1I*fAccumB1I+fAccumA1Q*fAccumB1Q)/(fAccumB1I*fAccumB1I+fAccumB1Q*fAccumB1Q);
        float ratioImag = (-fAccumA1I*fAccumB1Q+fAccumA1Q*fAccumB1I)/(fAccumB1I*fAccumB1I+fAccumB1Q*fAccumB1Q);

        float fRatioReal = ratioReal;
        uint32_t *dRatioReal;  //cast to int
        dRatioReal = &fRatioReal;

        float fRatioImag = ratioImag;
        uint32_t *dRatioImag;  //cast to int
        dRatioImag = &fRatioImag;

        data[4] = (*dRatioReal) & 0xFF;
        data[5] = (*dRatioReal >> 8) & 0xFF;

        data[6] = (*dRatioReal >> 16) & 0xFF;
        data[7] = (*dRatioReal >> 24) & 0xFF;

        data[8] = (*dRatioImag) & 0xFF;
        data[9] = (*dRatioImag >> 8) & 0xFF;

        data[10] = (*dRatioImag >> 16) & 0xFF;
        data[11] = (*dRatioImag >> 24) & 0xFF;
    }

    data[12] = (*accumC) & 0xFF; //ECG data
    data[13] = (*accumC >> 8) & 0xFF;

    // eliminate sync. problem
    /*int i;
    for (i = 2; i < 11; i++){
        if (data[i] == 0x7F && data[i+1] == 0xFF)
            data[i] == 0x7E;
    }
    */
}


