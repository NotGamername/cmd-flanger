#include <stdio.h>
#include <math.h>
#include "flanger.h"

void flanger(double *x, double *y, int N, struct Flanger *pf){
    int j, k, n, m, num_buf, fs;
    double drysum, wetsum, fullsum, wetmix;
    float local_buff[BUF_LEN], f0, phase, phase_inc, out_atten, osc_amp, mix;

    //pass data from flanger structure to local variables
    fs = pf->fs;
    f0 = pf->lfo_f0; //between 0 and 1
    phase = pf->lfo_phase;
    phase_inc = 2*PI*f0/fs;
    osc_amp = pf->range; //between 0.5 = high, 1.0 = mid, and 2.0 = low
    mix = pf->mix; //from 0 to 100

    //user chosen mix percentage used for linear gain calculations
    wetmix = mix/100;
    out_atten = (mix/(-200))+1; //mix 0 is 1, mix 100 is 0.5

    //number of callback buffers in BUF_LEN
    num_buf = BUF_LEN/N;
    //shift buffer
    j = BUF_LEN-(num_buf*N);
    k = BUF_LEN-((num_buf-1)*N);
    n = (num_buf-1)*N;
    for (int i=0; i<n; i++) {
        local_buff[j] = local_buff[k];
        j++;
        k++;
    }

    //copy new samples to buffer 
    //continue at index j
    for (int i=0; i<N; i++) {
        local_buff[j] = x[i];
        j++;
    }

    //start from this point in buffer
    m = BUF_MID;

	for(int n = 0; n < N; n++){

        //make copy of dry input
        drysum = local_buff[m+n];

        //calculate lfo value
        float sin_v = osc_amp * sin(phase);

        //increment phase
        phase += phase_inc;
        pf->lfo_phase = phase;

        //convert 1ms to samples and modulate with sin_v
        //NOTE: add osc_amp to sin_v so that oscillation occurs between 0 and some positive number so signal is always delayed
        float flange_delay = (((1/1000.0)*fs)) * ((sin_v + osc_amp));

        //interpolate for smooth lfo on non-whole number samples
        wetsum = get_sample((m+n+flange_delay), local_buff);

        //mix together dry signal and wet signal according to calculated mix values
        fullsum = out_atten * (drysum + (wetsum * wetmix));

        y[n] = fullsum;
	}
}

//interpolation function for smoother lfo
float get_sample(float samp_delay, float *buffer){
    int whole;
    float frac, a, b;
    double v;

    whole = floor(samp_delay);
    frac = samp_delay-whole;
    a = buffer[whole]; //sample before target time
    b = buffer[whole+1]; //sample after target time
    //linear interpolation
    v = a*(1.0-frac) + b*frac;

    return v;
}