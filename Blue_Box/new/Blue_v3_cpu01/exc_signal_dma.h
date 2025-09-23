/*
 * exc_signal_dma.h
 *
 *  Created on: Jan 8, 2024
 *      Author: admin
 */

#ifndef EXC_SIGNAL_DMA_H_
#define EXC_SIGNAL_DMA_H_

void start_exc_dma(void);
void set_excitation_frequency(const uint16_t* activeFreqs, const SignalsStruct* signalsArray);
void use_pwm_excitation(void);
void use_dac_excitation(void);


#endif /* EXC_SIGNAL_DMA_H_ */
