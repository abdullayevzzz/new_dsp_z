#include "math.h"
#include "device.h"

void sigGen (int16_t signal[], int f, int len, char s){
//  float t = (float)f/(float)Fs; // f * delta t
    //0.002 for 1kHz ,0.02 for 10kHz, 0.2 for 100kHz

    // STARTS FROM 4 kHz
    float t;
    t = (double)f/1000;

    int i;
    for(i=0;i<len;i++){
        if (s == 's')
            signal[i] = round(2000.0*sin(2.0*M_PI*i*t));
        else
            signal[i] = round(2000.0*cos(2.0*M_PI*i*t));
    }
}

/*
void cosGen (int16_t signal[], int f, int len, int Fs){
//  float t = f/(float)Fs; // f * delta t
    float t = 0.02;
    int i;
    for(i=0;i<len;i++)
        signal[i] = round(1000.0*cos(2.0*M_PI*i*t));
}


void sinGenX (int16_t signal[], int len, int A, int P){
//  float t = (float)f/(float)Fs; // f * delta t
    float t = 0.0125; // 6250 / 500000 not working as in the line above??
    float phase = ((float)P*M_PI)/180.0;
    int i;
    for(i=0;i<len;i++)
        signal[i] = round(((float)A)*sin(2.0*M_PI*i*t + phase));
    //signal[0] = 1000.0*t; //debug
}
*/
