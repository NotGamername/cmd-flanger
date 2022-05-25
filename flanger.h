#define MAX_CHAN	1
#define MAX_COEF	256
#define FRAMES_PER_BUFFER   1024
#define PI       3.14159265358979323846 //pi
#define SoS        343.0 //speed of sound, meters per second
#define BUF_LEN  8*1024
#define BUF_MID  (BUF_LEN/2)

struct Flanger {
    float output[FRAMES_PER_BUFFER];
    float fs; //sampling freq, Hz
    float buffer[BUF_LEN]; //delay buffer
    float lfo_f0; //user-defined lfo oscillation freq, Hz
    float lfo_phase;
    float lfo_phase_inc;
    int mix; //user-defined flanging mix percentage
    float range; //user-defined frequency range of flanging
};

//function prototypes
void flanger(double *x, double *y, int N, struct Flanger *pf);
float get_sample(float samp_delay, float *buffer);
int user_io_mix();
float user_io_rate();
float user_io_range();