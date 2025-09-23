#include "device.h"
#include "max30101.h"

uint8_t rdata[16];

void init_max30101(void)
{
    init_i2c();
    uint8_t test = 0;
    test = read_register(MAX30105_PARTID, rdata, 1);
    setup();
}


void init_i2c()
{
    I2C_disableModule(I2CB_BASE);
    I2C_initMaster(I2CB_BASE, DEVICE_SYSCLK_FREQ, 400000, I2C_DUTYCYCLE_50);
    I2C_setConfig(I2CB_BASE, I2C_MASTER_SEND_MODE | I2C_REPEAT_MODE);
    I2C_setDataCount(I2CB_BASE, 1);
    I2C_setBitCount(I2CB_BASE, I2C_BITCOUNT_8);
    I2C_setSlaveAddress(I2CB_BASE, 0x57);
    I2C_enableFIFO(I2CB_BASE);
    I2C_setEmulationMode(I2CB_BASE, I2C_EMULATION_STOP_SCL_LOW);
    I2C_enableModule(I2CB_BASE);
    DEVICE_DELAY_US(100);
}



void setup(){

    //Multi-LED Mode Configuration, Enable the reading of the three LEDs
    //bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, SLOT_RED_LED);
    //bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, SLOT_IR_LED << 4);
    //bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, SLOT_GREEN_LED);
    //bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT4_MASK, SLOT_GREEN_LED << 4);

    write_register(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_8 | MAX30105_ROLLOVER_ENABLE);
    //write_register(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_2 | MAX30105_ROLLOVER_DISABLE);
    //write_register(MAX30105_MODECONFIG, MAX30105_MODE_REDONLY);
    write_register(MAX30105_MODECONFIG, MAX30105_MODE_MULTILED);
    write_register(MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_2048 | MAX30105_SAMPLERATE_800 | MAX30105_PULSEWIDTH_215);
    write_register(MAX30105_LED1_PULSEAMP, 0x0F);
    // write_register(MAX30105_LED3_PULSEAMP, 0xFF);
    write_register(MAX30105_MULTILEDCONFIG1, SLOT_RED_LED);
    // write_register(MAX30105_MULTILEDCONFIG1, SLOT_GREEN_LED);
    write_register(MAX30105_INTENABLE1, MAX30105_INT_DATA_RDY_ENABLE);
    write_register(MAX30105_FIFOWRITEPTR, 0x00);
    write_register(MAX30105_FIFOREADPTR, 0x00);
    write_register(MAX30105_FIFOOVERFLOW, 0x00);
}


uint32_t read_leds(records *leds){
    uint16_t temp;
    //Burst read three bytes - RED
    read_register(MAX30105_FIFODATA, rdata, 3); //read 3 bytes from FIFODATA register
    write_register(MAX30105_FIFOWRITEPTR, 0x00);
    write_register(MAX30105_FIFOREADPTR, 0x00);
    //Convert array to 16bit value
    temp = (rdata[0] << 14) | (rdata[1] << 6) | (rdata[2] >> 2);
    leds->red = temp;
    return 0;
}

float read_temperature(){
    write_register(MAX30105_DIETEMPCONFIG, 0x01); //enable temp measurement
    write_register(MAX30105_INTENABLE2, 0x02); //enable temp interrupt
    while (1){
        uint8_t isTempMeasurementReady = read_register(MAX30105_INTSTAT2, rdata, 1);
        if (isTempMeasurementReady & MAX30105_INT_DIE_TEMP_RDY_ENABLE)
            break;
    }
    int8_t tempInt = read_register(MAX30105_DIETEMPINT, rdata, 1);
    uint8_t tempFrac = read_register(MAX30105_DIETEMPFRAC, rdata, 1); //Causes the clearing of the DIE_TEMP_RDY interrupt
    return (float)tempInt + ((float)tempFrac * 0.0625);
}

uint8_t new_data_ready(void){
    return (read_register(MAX30105_INTSTAT1, rdata, 1) & MAX30105_INT_DATA_RDY_ENABLE);
}

int8_t num_new_samples(void){
    int8_t r = read_register(MAX30105_FIFOREADPTR, rdata, 1);
    int8_t w = read_register(MAX30105_FIFOWRITEPTR, rdata, 1);
    int8_t dif = w-r+1; //results by debugging. read and write is equal for the first data
    if (dif < 0)
       dif = dif + 32;
    return dif;
}


uint8_t read_register(uint8_t reg, uint8_t received_data[], uint8_t num){
    // Wait until the STP bit is cleared from any previous master communication.
    int timeout = 0;
    while((HWREGH(I2CB_BASE + I2C_O_MDR) & I2C_MDR_STP))
    {
        timeout ++;
        if (timeout > 10000){
            //asm("   ESTOP0");
            break;
        }
    }

    // check if bus busy
    if ((HWREGH(I2CB_BASE + I2C_O_STR) & I2C_STR_BB) == I2C_STR_BB){
        //asm("   ESTOP0");
    }

    // set data count to be sent. no need in RM mode
    // HWREGH(I2CB_BASE + I2C_O_CNT) = 1;

    // put register byte to the FIFO
    HWREGH(I2CB_BASE + I2C_O_DXR) = reg;

    // Send start, configure as master transmitter
    HWREGH(I2CB_BASE + I2C_O_MDR) = I2C_MDR_MST | I2C_MDR_STT | I2C_MDR_TRX | I2C_MDR_RM | I2C_MDR_IRS;

    // Wait for the register access to be ready
    timeout = 0;
    while(!(HWREGH(I2CB_BASE + I2C_O_STR) & I2C_STR_ARDY)){
        timeout ++;
        if (timeout > 10000){
            //asm("   ESTOP0");
            break;
        }
    }

    //check if NACK is received, set STP and clear NACK if so.

    // set number of bytes to be received
    // HWREGH(I2CB_BASE + I2C_O_CNT) = 1;

    // Send restart, configure as master receiver
    HWREGH(I2CB_BASE + I2C_O_MDR) = I2C_MDR_MST | I2C_MDR_STT | I2C_MDR_RM | I2C_MDR_IRS;

    // Wait until the acquired number of bytes are read to the FIFO.
    timeout = 0;
    while(((HWREGH(I2CB_BASE + I2C_O_FFRX) & I2C_FFRX_RXFFST_M) >> I2C_FFRX_RXFFST_S) < num)
    {
        timeout ++;
        if (timeout > 30000){
            //asm("   ESTOP0");
            break;
        }
    }

    // Send stop condition
    HWREGH(I2CB_BASE + I2C_O_MDR) |= I2C_MDR_STP;  // stp = 1

    int i;
    for (i=0; i < num; i++)
        received_data[i] = HWREGH(I2CB_BASE + I2C_O_DRR);

    return received_data[0];
}

void write_register(uint8_t reg, uint8_t value){
    // Wait until the STP bit is cleared from any previous master communication.
    int timeout = 0;
    while((HWREGH(I2CB_BASE + I2C_O_MDR) & I2C_MDR_STP))
    {
        timeout ++;
        if (timeout > 10000){
            //asm("   ESTOP0");
            break;
        }
    }

    // check if bus busy
    if ((HWREGH(I2CB_BASE + I2C_O_STR) & I2C_STR_BB) == I2C_STR_BB){
        //asm("   ESTOP0");
    }

    // set data count to be sent
    // HWREGH(I2CB_BASE + I2C_O_CNT) = 2;

    // put register and data bytes to the FIFO
    HWREGH(I2CB_BASE + I2C_O_DXR) = reg;
    HWREGH(I2CB_BASE + I2C_O_DXR) = value;

    // Send start, configure as master transmitter
    HWREGH(I2CB_BASE + I2C_O_MDR) = I2C_MDR_MST | I2C_MDR_STT | I2C_MDR_TRX | I2C_MDR_RM | I2C_MDR_IRS;

    // Wait for the register access to be ready
    timeout = 0;
    while(!(HWREGH(I2CB_BASE + I2C_O_STR) & I2C_STR_ARDY)){
        timeout ++;
        if (timeout > 10000){
            //asm("   ESTOP0");
            break;
        }
    }
    // Send stop condition
    HWREGH(I2CB_BASE + I2C_O_MDR) |= I2C_MDR_STP;  // stp = 1
}
