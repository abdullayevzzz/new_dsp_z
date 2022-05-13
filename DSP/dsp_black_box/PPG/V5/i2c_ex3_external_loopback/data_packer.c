/*
 * data_packer.c
 *
 *  Created on: Oct 12, 2021
 *      Author: anar
 */
#include "device.h"

void pack (uint8_t* data, uint8_t  *red) {

    data[0] = 0x7F;  //synchronization
    data[1] = 0xFF;

    data[2] = red[2];
    data[3] = red[1];
    data[4] = red[0];

}


