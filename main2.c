#include <stdio.h>
#include <stdlib.h>		//malloc()
#include <unistd.h>		//sleep()
#include <stdbool.h>	//bool
#include <stdatomic.h>	//atomic read/write
#include <sndfile.h>	//sndfile
#include <portaudio.h>	//portaudio
#include "paUtils.h"	//portaudio utility functions
#include "flanger.h"	//declares struct Flanger and function

#define BLK_LEN	1024	//block length for block processing

//PortAudio callback structure
struct PABuf {
	float *ifbuf;
	float *ofbuf;
	int num_chan;
	int next_frame;
	int total_frame;
	atomic_bool done;
	struct Flanger *pf;
};

//PortAudio callback function protoype
static int paCallback( const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );

int main(int argc, char *argv[])
{
	char *ifile, *ofile;
	int N, C, icount, ocount;
	float *ifbuf, *ofbuf; //frame buffers
    SNDFILE *isndfile, *osndfile;
    SF_INFO isfinfo, osfinfo;
	struct PABuf paBuf; //PortAudio data struct
	struct Flanger flan;
    PaStream *stream;

	//usage and parse command line
	if (argc != 3){
		printf("Usage: %s input.wav output.wav\n",argv[0]);
		return -1;
	}

	//initialize pointers to files
	ifile = argv[1];
	ofile = argv[2];

	//open input file
	if ((isndfile = sf_open(ifile, SFM_READ, &isfinfo)) == NULL){
		printf("ERROR: could not open file %s\n",argv[1]);
		return -1;
	}

	//set N to number of frames and C to number of channels
	N = isfinfo.frames;
	C = isfinfo.channels;
	if (C > 1){ //only mono files allowed
		printf("ERROR: Please use a mono audio file\n");
		return -1;
	}

	//make sure output file has same stats as input file
	osfinfo.format = isfinfo.format;
	osfinfo.channels = C;
	osfinfo.samplerate = isfinfo.samplerate;

	//open output file
	if ((osndfile = sf_open(ofile,SFM_WRITE,&osfinfo)) == NULL){
		printf("ERROR: could not open file %s\n",argv[2]);
		return -1;
	}

	osfinfo.frames = N;

	//mallocate buffers
	ifbuf = (float *)malloc(N * C * sizeof(float));
	if (ifbuf == NULL){
		printf("ERROR: Returned pointer to ifbuf was null\n");
		return -1;
	}

	ofbuf = (float *)malloc(N * C * sizeof(float));
	if (ofbuf == NULL){
		printf("ERROR: Returned pointer to ofbuf was null\n");
		return -1;
	}

	//read input WAV file into ifbuf[]
	icount = sf_readf_float(isndfile, ifbuf, isfinfo.frames);
	if (icount != isfinfo.frames){
		printf("ERROR: input count does not equal number of frames\n");
		return -1;
	}

	//close input file
	sf_close(isndfile);

	//initialize Port Audio data struct
	paBuf.ifbuf = ifbuf;
	paBuf.ofbuf = ofbuf;
	paBuf.num_chan = isfinfo.channels;
	paBuf.next_frame = 0;
	paBuf.total_frame = isfinfo.frames;
	paBuf.done = false;

	flan.fs = isfinfo.samplerate;
	flan.lfo_phase = 0;

	//user sets rate
	flan.lfo_f0 = user_io_rate();
	if ((flan.lfo_f0 > 1) || (flan.lfo_f0 <= 0)){
		printf("Please input a number between 0 and 1\n");
		return -1;
	}

	//user sets range
	flan.range = user_io_range();
	if ((flan.range > 2.0) || (flan.range < 0.5)){
		printf("Please choose low, mid, or high\n");
		return -1;
	}

	//user sets mix
	flan.mix = user_io_mix();
	if ((flan.mix > 100) || (flan.mix < 0)){
		printf("Please input a whole number from 0 to 100\n");
		return -1;
	}

	//pass our flanger structure to the Port Audio structure
	paBuf.pf = &flan;

    //start up Port Audio
    printf("Starting PortAudio\n");
    stream = startupPa(1, isfinfo.channels, 
      isfinfo.samplerate, BLK_LEN, paCallback, &paBuf);

	//sleep and let callback process audio until done 
    while (!paBuf.done) {
    	printf("%d\n", paBuf.next_frame);
    	sleep(1);
    }

    //shut down Port Audio
    shutdownPa(stream);

    //write output buffer to WAV file
	ocount = sf_writef_float(osndfile, ofbuf, N);
	if (ocount != osfinfo.frames){
		printf("ERROR: output count does not equal number of frames\n");
		return -1;
	}

	//close WAV files 
	//free allocated storage
	sf_close(osndfile);

	free(ifbuf);
	free(ofbuf);

	//permission to feel good
	printf("Successfully closed files and freed memory!\n");

	return 0;
}

static int paCallback(
	const void *inputBuffer, 
	void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    //cast data passed via paCallback to our struct
    struct PABuf *p = (struct PABuf *)userData; 
    //cast input and output buffers */
    float *output = (float *)outputBuffer;
	//since blocks are short, just declare single-channel arrays
	double icbuf[BLK_LEN], ocbuf[BLK_LEN];
	int N = framesPerBuffer;
	int C = p->num_chan;
	//local pointers to ifbuf[] and ofbuf[]
    float *ifbuf = p->ifbuf + p->next_frame*C;
    float *ofbuf = p->ofbuf + p->next_frame*C;

	//zero PortAudio output buffer for:
	//partial output buffer
	//or call to PortAudio after done == true (after all input data has been processed)
	for (int i=0; i<N; i++) {
		output[i] = 0;
	}

	//return if done
	if (p->done == true) {
		 return 0;
	}

	//adjust N if last frame is partial frame
	if (p->next_frame + N > p->total_frame) {
		N = p->total_frame - p->next_frame;
	}

	//pass input buffer into the double version
	for (int i = 0; i < N; i++){
		icbuf[i] = ifbuf[i];
	}

	//flange it
	flanger(&icbuf[0], &ocbuf[0], N, p->pf);

	//pass the output into the float version
	for (int i = 0; i < N; i++){
		ofbuf[i] = ocbuf[i];
	}

	//copy ofbuf[] to portaudio output buffer 
	for (int i = 0; i < N; i++){
		output[i] = ofbuf[i];
	}

	//increment next_frame counter and check is done
	p->next_frame += N;
	if (p->next_frame >= p->total_frame) {
		p->done = true;
	}

	return 0;
}