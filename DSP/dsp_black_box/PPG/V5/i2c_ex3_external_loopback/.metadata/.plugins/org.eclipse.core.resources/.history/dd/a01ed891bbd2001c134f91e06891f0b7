/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"

void pack (uint8_t* data, uint8_t  *redIr) {

    data[0] = 0x7F;  //synchronization
    data[1] = 0xFF;

    // red
    data[2] = (redIr[0] << 6) | (redIr[1] >> 2); //16 bit ADC in 3 bytes, 17th bit is MSB
    data[3] = (redIr[1] << 6) | (redIr[2] >> 2);

    // ir
    data[4] = (redIr[3] << 6) | (redIr[4] >> 2); //16 bit ADC in 3 bytes, 17th bit is MSB
    data[5] = (redIr[4] << 6) | (redIr[5] >> 2);
}


