/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"

void pack (uint8_t* data, uint8_t  *data9) {

    data[0] = 0x7F;  //synchronization
    data[1] = 0xFF;

    data[3] = (data9[0] << 6) | (data9[1] >> 2); //16 bit ADC in 3 bytes, 17th bit is MSB
    data[2] = (data9[1] << 6) | (data9[2] >> 2);

    data[5] = (data9[3] << 6) | (data9[4] >> 2); //16 bit ADC in 3 bytes, 17th bit is MSB
    data[4] = (data9[4] << 6) | (data9[5] >> 2);

    data[7] = (data9[6] << 6) | (data9[7] >> 2); //16 bit ADC in 3 bytes, 17th bit is MSB
    data[6] = (data9[7] << 6) | (data9[8] >> 2);

}


