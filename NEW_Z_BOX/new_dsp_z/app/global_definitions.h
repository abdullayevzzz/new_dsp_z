/*
 * global_definitions.h
 *
 *  Created on: Dec 11, 2025
 *      Author: admin
 */

#ifndef APP_GLOBAL_DEFINITIONS_H_
#define APP_GLOBAL_DEFINITIONS_H_


//-----------------------------------------------------------------------------
//
//          General Definitions
//
//-----------------------------------------------------------------------------

#define MAX_NUMBER_OF_FREQS                               10



//-----------------------------------------------------------------------------
//
//          Declarations @ ....c
//
//-----------------------------------------------------------------------------

struct SER_DATA_T
{
    uint32_t  sync_bytes;
    uint16_t  exc_raw;
};

extern struct IMP_DATA_T ser_data;






#endif /* APP_GLOBAL_DEFINITIONS_H_ */
