#include "device.h"
#include "max30101.h"

void initMAX30101(void)
{
    static uint16_t test = 0;
    writeRegister8(0x0C,0xAF);
    test = readRegister8(0x0C);
    setup();
}

void bitMask(uint8_t reg, uint8_t mask, uint8_t thing)
{
  // Grab current register context
  uint8_t originalContents = readRegister8(reg);

  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;

  // Change contents
  writeRegister8(reg, originalContents | thing);
}

void setup(){
    //set sample average to 4
    bitMask(MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_MASK, MAX30105_SAMPLEAVG_4);
    //enable fifo rollover
    bitMask(MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK, MAX30105_ROLLOVER_ENABLE);
    //activate LEDs, other options: MAX30105_MODE_REDONLY, MAX30105_MODE_REDIRONLY, MAX30105_MODE_MULTILED
    bitMask(MAX30105_MODECONFIG, MAX30105_MODE_MASK, MAX30105_MODE_REDIRONLY);
    //set adc range, one of MAX30105_ADCRANGE_2048, _4096, _8192, _16384
    bitMask(MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_MASK, MAX30105_ADCRANGE_16384);
    //set sample rate: one of MAX30105_SAMPLERATE_50, _100, _200, _400, _800, _1000, _1600, _3200
    bitMask(MAX30105_PARTICLECONFIG, MAX30105_SAMPLERATE_MASK, MAX30105_SAMPLERATE_800);
    //set pulse width (us): one of MAX30105_PULSEWIDTH_69, _118, _215, _411
    bitMask(MAX30105_PARTICLECONFIG, MAX30105_PULSEWIDTH_MASK, MAX30105_PULSEWIDTH_118); //16 bit
    //set amplitude values for all leds and proximity: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
    writeRegister8(MAX30105_LED1_PULSEAMP, 0x0F);
    writeRegister8(MAX30105_LED2_PULSEAMP, 0x0F);
    //writeRegister8(MAX30105_LED3_PULSEAMP, 0x0F);
    //writeRegister8(MAX30105_LED4_PULSEAMP, 0x0F); //connected to LED3, green led
    //Multi-LED Mode Configuration, Enable the reading of the three LEDs
    bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, SLOT_RED_LED);
    bitMask(MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, SLOT_IR_LED << 4);
    //bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, SLOT_GREEN_LED);
    //bitMask(MAX30105_MULTILEDCONFIG2, MAX30105_SLOT4_MASK, SLOT_GREEN_LED << 4);
    //enable PPG_RDY_EN
    bitMask(MAX30105_INTENABLE1, MAX30105_INT_DATA_RDY_MASK, MAX30105_INT_DATA_RDY_ENABLE);

}

uint32_t read_leds(records *led3){
    static uint8_t temp[4]; //Array of 4 bytes that we will convert into long
    uint32_t tempLong;

    uint8_t readPointer = readRegister8(MAX30105_FIFOREADPTR);
    uint8_t writePointer = readRegister8(MAX30105_FIFOWRITEPTR);

    if (readPointer != writePointer){ // new data
        //Burst read three bytes - RED
        readRegister9(MAX30105_FIFODATA, *temp); //read 3 bytes from FIFODATA register
        //Convert array to long
        memcpy(&tempLong, temp, sizeof(tempLong));
        tempLong &= 0x3FFFF; //Zero out all but 18 bits
        led3->red = tempLong;

        //Burst read three bytes - IR
        readRegister9(MAX30105_FIFODATA, *temp); //read 3 bytes from FIFODATA register
        //Convert array to long
        memcpy(&tempLong, temp, sizeof(tempLong));
        tempLong &= 0x3FFFF; //Zero out all but 18 bits
        led3->ir = tempLong;

        //Burst read three bytes - GREEN
        readRegister9(MAX30105_FIFODATA, *temp); //read 3 bytes from FIFODATA register
        //Convert array to long
        memcpy(&tempLong, temp, sizeof(tempLong));
        tempLong &= 0x3FFFF; //Zero out all but 18 bits
        led3->green = tempLong;

        return 1;
    }
    return 0;
}

float temp(){
    writeRegister8(MAX30105_DIETEMPCONFIG, 0x01); //enable temp measurement
    writeRegister8(MAX30105_INTENABLE2, 0x02); //enable temp interrupt
    while (1){
        uint8_t isTempMeasurementReady = readRegister8(MAX30105_INTSTAT2);
        if (isTempMeasurementReady & MAX30105_INT_DIE_TEMP_RDY_ENABLE)
            break;
    }
    int8_t tempInt = readRegister8(MAX30105_DIETEMPINT);
    uint8_t tempFrac = readRegister8(MAX30105_DIETEMPFRAC); //Causes the clearing of the DIE_TEMP_RDY interrupt
    return (float)tempInt + ((float)tempFrac * 0.0625);
}

uint8_t newDataReady(void){
    return (readRegister8(MAX30105_INTSTAT1) & MAX30105_INT_DATA_RDY_ENABLE);
}

uint8_t readRegister8(uint8_t reg){
    DEVICE_DELAY_US(1000);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_SEND_MODE));
    I2C_setDataCount(I2CA_BASE, 1);
    I2C_putData(I2CA_BASE, reg); //
    I2C_sendStartCondition(I2CA_BASE);
    DEVICE_DELAY_US(1000);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_RECEIVE_MODE));
    I2C_sendStartCondition(I2CA_BASE);
    DEVICE_DELAY_US(1000);
    return I2C_getData(I2CA_BASE);
}

void readRegister9(uint8_t reg, uint8_t redIrGreen[]){
    DEVICE_DELAY_US(100);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_SEND_MODE));
    I2C_setDataCount(I2CA_BASE, 1);
    I2C_putData(I2CA_BASE, reg); //
    I2C_sendStartCondition(I2CA_BASE);
    DEVICE_DELAY_US(100);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_RECEIVE_MODE));
    I2C_setDataCount(I2CA_BASE, 9);
    I2C_sendStartCondition(I2CA_BASE);
    DEVICE_DELAY_US(100);
    int i;
    for (i=0; i<9; i++)
        redIrGreen[i] = I2C_getData(I2CA_BASE);
}

void readRegister6(uint8_t reg, uint8_t redIr[]){
    DEVICE_DELAY_US(100);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_SEND_MODE));
    I2C_setDataCount(I2CA_BASE, 1);
    I2C_putData(I2CA_BASE, reg); //
    I2C_sendStartCondition(I2CA_BASE);
    DEVICE_DELAY_US(100);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_RECEIVE_MODE));
    I2C_setDataCount(I2CA_BASE, 6);
    I2C_sendStartCondition(I2CA_BASE);
    DEVICE_DELAY_US(100);
    int i;
    for (i=0; i<6; i++)
        redIr[i] = I2C_getData(I2CA_BASE);
}

void writeRegister8(uint8_t reg, uint8_t value){
    DEVICE_DELAY_US(100);
    I2C_setConfig(I2CA_BASE, (I2C_MASTER_SEND_MODE));
    I2C_setDataCount(I2CA_BASE, 2);
    I2C_putData(I2CA_BASE, reg);
    I2C_putData(I2CA_BASE, value);
    I2C_sendStartCondition(I2CA_BASE);
}
