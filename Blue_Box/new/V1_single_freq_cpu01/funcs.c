#include "driverlib.h"
#include "constants.h"
#include "device.h"
extern uint32_t sr_current_val;  // Declare the external variable

void delay(int num){
    int i;
    for (i = 0; i < num; i++)
        asm(" NOP");
}

/*
uint32_t mirror(uint32_t value) {
    int i;
    uint32_t mirrored = 0;
    for (i = 0; i < 32; i++) {
        mirrored |= ((value >> i) & 0x00000001) << (31 - i);
    }
    return mirrored;
}
*/

uint32_t mirror(uint32_t x)
{
    x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1); // Swap _<>_
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2); // Swap __<>__
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4); // Swap ____<>____
    x = ((x & 0x00FF00FF) << 8) | ((x & 0xFF00FF00) >> 8); // Swap ...
    x = ((x & 0x0000FFFF) << 16) | ((x & 0xFFFF0000) >> 16); // Swap ...
    return x;
}

void initShiftRegister(uint32_t sr_val){
    static uint8_t firstRun = 1;
    // uint16_t i;

    // GPIO_writePin(63, 0);      // Set data low
    GPIO_writePin(26, 0);      // Set latch (rclk) LOW

    SPI_writeDataNonBlocking(SPIB_BASE, (uint32_t) (sr_val >> 16));
    SPI_writeDataNonBlocking(SPIB_BASE, (uint32_t) (sr_val & 0x0000FFFF));
    DEVICE_DELAY_US(4);
    GPIO_writePin(26, 1);      // Set latch (rclk) HIGH to latch the data

    // If this is the first run, enable output
    if (firstRun) {
        GPIO_writePin(27, 0); // Enable output
        firstRun = 0; // Set firstRun to false to prevent future execution
    }
}


void updateShiftRegister(uint32_t mask, uint32_t values) {
    // EPWM_disableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);

    // Clear the bits that we want to change
    sr_current_val &= ~mask;
    // Set the new bit values
    sr_current_val |= (mask & values);
    // Update the shift register
    initShiftRegister(sr_current_val);

    // EPWM_enableADCTrigger(EPWM_FOR_ADC_BASE , EPWM_SOC_B);
}


void router_config(uint16_t numPairs, uint16_t *inputs, uint16_t *outputs) {
    int i;
    uint32_t mask = 0;
    uint32_t values = 0;

    // Reset MUX, first select both chips and reset, open all switches by setting Reset high and then low
    mask = 0x0C000000;  // CS1 and CS2 positions
    values = 0x0C000000;  // Set both CS1 and CS2 high
    updateShiftRegister(mask, values);
    mask = 0x02800000;    // Mask Reset bit and Strobe bit
    values = 0x00800000;  // Set Reset bit high and Strobe bit low
    updateShiftRegister(mask, values);
    mask = 0x00800000;  // Mask Reset bit
    values = 0; // Set Reset bit low
    updateShiftRegister(mask, values);


    for (i = 0; i < numPairs; i++) {
        // Fetch switch address from input-output pair
        uint16_t input = inputs[i];
        uint16_t output = outputs[i];

        // Prepare MUX control bits, ensuring Strobe is low before setting addresses
        mask = 0x03000000; // Mask for control bits: Data, Strobe
        values = 0x01000000; // Set Data high, Strobe low
        updateShiftRegister(mask, values);
        //delay(1000);

        // Determine chip select (CS)
        if (output < 16) {
        // First mux chip is used
            mask = 0x0C000000;   // mask for CS pins
            values = 0x04000000;  // CS1 = 1, CS2 = 0
        }
        else {
        //  Second mux chip is used
        //  Adjust output to 0-16 range for the second chip
            output -= 16;
            mask = 0x0C000000;   // mask for CS pins
            values = 0x08000000;  // CS1 = 0, CS2 = 1
        }
        //updateShiftRegister(mask, values); // Activate selected chip

        // Prepare the mask for MUX addressing: input address (bits 9-11) and output address (bits 12-15)
        mask |= 0x007F0000; // Update mask for input and output addressing bits
        // Set the input and output addresses in the values variable by shifting the input and output values
        // to their respective bit positions in the shift register
        values |= ((uint32_t)(input & 0x00000007) << 20) | ((uint32_t)(output & 0x0000000F) << 16); // Apply input and output addresses to values
        // Update the shift register with the new configuration for input/output addresses
        updateShiftRegister(mask, values);

        // Activate the switch by pulsing, Strobe high
        mask = 0x03000000;
        values = 0x03000000;
        updateShiftRegister(mask, values);
    }
    // Finalize MUX operation, Strobe low.
    mask = 0x02000000;
    values = 0;
    updateShiftRegister(mask, values);
}

void configure_gpios(void){
    // Configuration for the I2CB Pins
    GPIO_setMasterCore(40, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_40_SDAB);
    GPIO_setPadConfig(40, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(40, GPIO_QUAL_ASYNC);
    GPIO_setMasterCore(41, GPIO_CORE_CPU2);
    GPIO_setPinConfig(GPIO_41_SCLB);
    GPIO_setPadConfig(41, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(41, GPIO_QUAL_ASYNC);

    // Configuration for the SCI A Rx pin.
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    //
    // Configuration for the SCI A Tx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    //--- Configure GPIO5 as output (connected to ECG shutdown)
    GPIO_setPadConfig(5, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO5
    GPIO_setPinConfig(GPIO_5_GPIO5);               // GPIO34 = GPIO5
    GPIO_setDirectionMode(5, GPIO_DIR_MODE_OUT);    // GPIO5 = output
    GPIO_writePin(5, 1);                            // Load output latch

    //--- Configure GPIO24 as output (connected to ECG LOD+)
    GPIO_setPadConfig(24, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO5
    GPIO_setPinConfig(GPIO_24_GPIO24);               // GPIO34 = GPIO5
    GPIO_setDirectionMode(24, GPIO_DIR_MODE_OUT);    // GPIO5 = output
    GPIO_writePin(24, 0);

    //--- Configure GPIO34 as output (connected to LED)
    GPIO_setPadConfig(34, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_34_GPIO34);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(34, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_setMasterCore(34, GPIO_CORE_CPU2);
    //GPIO_writePin(34, 1);                            // Load output latch

    //== Olev Relay 67
    GPIO_setPadConfig(67, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_67_GPIO67);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(67, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(67, 1);

    // 5V enable
    GPIO_setPadConfig(1, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO34
    GPIO_setPinConfig(GPIO_1_GPIO1);               // GPIO34 = GPIO34
    GPIO_setDirectionMode(1, GPIO_DIR_MODE_OUT);    // GPIO34 = output
    GPIO_writePin(1, 1);

    // Make sure GPIO 0 - PWM pin is not used in DAC mode
    GPIO_setPadConfig(0, GPIO_PIN_TYPE_PULLUP);     // Enable pull-up on GPIO0
    GPIO_setPinConfig(GPIO_0_GPIO0);               // GPIO0 = GPIO0
    GPIO_setDirectionMode(0, GPIO_DIR_MODE_IN);    // GPIO0 = output

    //
    // Shift register GPIOs
    //
    // GPIO63 is the SDAT.
    /*
    GPIO_setMasterCore(63, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_63_GPIO63);
    // GPIO_setPadConfig(63, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(63, GPIO_DIR_MODE_OUT);
    GPIO_writePin(63, 1);
    */
    // GPIO63 is the SPISIMOB - SDAT.
    GPIO_setMasterCore(63, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_63_SPISIMOB);
    GPIO_setPadConfig(63, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(63, GPIO_QUAL_ASYNC);

    // GPIO65 is the SRCLK.
    /*
    GPIO_setMasterCore(65, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_65_GPIO65);
    // GPIO_setPadConfig(65, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(65, GPIO_DIR_MODE_OUT);
    GPIO_writePin(65, 1);
    */
    // GPIO65 is the SPICLKB - SRCLK
    GPIO_setMasterCore(65, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_65_SPICLKB);
    GPIO_setPadConfig(65, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(65, GPIO_QUAL_ASYNC);

    // GPIO26 is the RCLK.
    GPIO_setMasterCore(26, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_26_GPIO26);
    // GPIO_setPadConfig(26, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(26, GPIO_DIR_MODE_OUT);
    GPIO_writePin(26, 1);

    /*
    // GPIO26 is the RCLK.
    GPIO_setMasterCore(26, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_26_GPIO26);
    // GPIO_setPadConfig(26, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(26, GPIO_DIR_MODE_IN);
    // GPIO_writePin(26, 1);

    GPIO_setMasterCore(66, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_66_SPISTEB);
    GPIO_setPadConfig(66, GPIO_PIN_TYPE_PULLUP);
    GPIO_setQualificationMode(66, GPIO_QUAL_ASYNC);
    */

    // GPIO27 is the !OE.
    GPIO_setMasterCore(27, GPIO_CORE_CPU1);
    GPIO_setPinConfig(GPIO_27_GPIO27);
    // GPIO_setPadConfig(27, GPIO_PIN_TYPE_PULLUP);
    GPIO_setDirectionMode(27, GPIO_DIR_MODE_OUT);
    GPIO_writePin(27, 1); //disable output
}
