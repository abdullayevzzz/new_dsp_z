#include "driverlib.h"
#include "constants.h"
extern uint32_t sr_current_val;  // Declare the external variable

void delay(int num){
    int i;
    for (i = 0; i < num; i++)
        asm(" NOP");
}


void initShiftRegister(uint32_t sr_val){
    static uint8_t firstRun = 1;
    uint16_t i;
    GPIO_writePin(26, 0);      // Set latch (rclk) LOW
    for (i = 0; i < 32; i++) {
        GPIO_writePin(65, 0);      // Set srclk (serial clock) LOW
        //delay(1);                 // Delay for stability (adjust as needed)
        // Set data pin to the current bit value (0 or 1)
        uint8_t bit_val = (sr_val >> i) & 1;
        GPIO_writePin(63, bit_val);
        GPIO_writePin(65, 1);      // Toggle srclk (serial clock) HIGH to shift data
        //delay(1);                 // Delay for stability (adjust as needed)
        GPIO_writePin(65, 0);      // Set srclk (serial clock) LOW
        //delay(1);                 // Delay for stability (adjust as needed)
    }
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


void router(int numPairs, int *inputs, int *outputs) {
    int i;
    uint32_t mask = 0;
    uint32_t values = 0;

    // Reset MUX, first select both chips and reset, open all switches by setting Reset high and then low
    mask = (1 << 4) | (1 << 5);    // Mask both CS signals
    values = mask;  // Set both CS signals high
    updateShiftRegister(mask, values);
    mask = (1 << 8) | (1 << 6);    // Mask Reset bit and Strobe bit
    values = (1 << 8) | (0 << 6);  // Set Reset bit high and Strobe bit low
    updateShiftRegister(mask, values);
    mask = (1 << 8);    // Mask Reset bit
    values = 0; // Set Reset bit low
    updateShiftRegister(mask, values);


    for (i = 0; i < numPairs; i++) {
        // Fetch switch address from input-output pair
        int input = inputs[i];
        int output = outputs[i];

        // Prepare MUX control bits, ensuring Strobe is low before setting addresses
        mask = 0x000000C0; // Mask for control bits: Data, Strobe
        values = 0x00000080; // Set Data high, Strobe low
        updateShiftRegister(mask, values);
        //delay(1000);


        // Determine chip select (CS)
        if (output < 16) {
        // First mux chip is used
            mask = 0x00000030;   // mask for CS pins
            values = 0x00000020;  // CS1 = 1, CS2 = 0
        }
        else {
        //  Second mux chip is used
        //  Adjust output to 0-16 range for the second chip
            output -= 16;
            mask = 0x00000030;   // mask for CS pins
            values = 0x00000010;  // CS1 = 0, CS2 = 1
        }
        //updateShiftRegister(mask, values); // Activate selected chip

        // Prepare the mask for MUX addressing: input address (bits 20-22) and output address (bits 16-19)
        mask |= 0x0000FE00; // Update mask for input and output addressing bits
        // Set the input and output addresses in the values variable by shifting the input and output values
        // to their respective bit positions in the shift register
        values |= ((input & 0x00000007) << 9) | ((output & 0x0000000F) << 12); // Apply input and output addresses to values
        // Update the shift register with the new configuration for input/output addresses
        updateShiftRegister(mask, values);

        // Activate the switch by pulsing Strobe
        mask = 0x000000C0;
        values = 0x000000C0;
        updateShiftRegister(mask, values);

    }


    // Finalize MUX operation, Strobe low.
    mask = 0x000000C0;
    values = 0;
    updateShiftRegister(mask, values);

}
