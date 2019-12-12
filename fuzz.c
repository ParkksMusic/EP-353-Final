#include <stdio.h>
#include <math.h>
#include "portaudio.h"

#define SAMPLE_RATE (44100)
#define PA_SAMPLE_TYPE paFloat32
#define FRAMES_PER_BUFFER (64)

typedef float SAMPLE;

float CubicAmplifier( float input);
static int fuzzCallback( const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

/*Non-Linear amplifier with soft distortion curve */
float CubicAmplifier(float input)
{
    float output, temp;
    if( input < 0.0 )
    {
        temp = input + 1.0f;
        output = (temp * temp * temp) - 1.0f;
    }
    else
    {
        temp = input - 1.0f;
        output = (temp * temp * temp) + 1.0f;
    }

    return output;
}
#define FUZZ(x) CubicAmplifier(CubicAmplifier(CubicAmplifier(CubicAmplifier(x))))

static int gNumNoInputs = 0;
static int fuzzCallback( const void *inputBuffer, void *outputBuffer, unsigned long framesPerbuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
    SAMPLE *out = (SAMPLE*)outputBuffer;
    const SAMPLE *in = (const SAMPLE*)inputBuffer;
    unsigned int i;
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if(inputBuffer == NULL)
    {
        for(i=0; i<framesPerbuffer; i++)
        {
            *out++ = 0;
            *out++ = 0;
        }
        gNumNoInputs += 1;
    }
    else
    {
        for(i=0; i<framesPerbuffer; i++)
        {
            *out++ = FUZZ(*in++);
            *out++ = *in++;
        }
    }
    return paContinue;
}

int main(void);
int main(void)
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream;
    PaError err;

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    inputParameters.device = Pa_GetDefaultInputDevice(); 
    if (inputParameters.device == paNoDevice) {
        fprintf(stderr,"No default input device.\n");
        goto error;
    }
    inputParameters.channelCount = 2;
    inputParameters.sampleFormat = PA_SAMPLE_TYPE;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device);

    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device + Pa_GetDefaultOutputDevice;
    if (outputParameters.device == paNoDevice) {
        fprintf(stderr, "No default output  device.\n");
        goto error;
    }
    outputParameters.channelCount = 2;
    outputParameters.sampleFormat = PA_SAMPLE_TYPE;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device);

    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&stream, &inputParameters, &outputParameters, SAMPLE_RATE, FRAMES_PER_BUFFER, 0, fuzzCallback, NULL);
    if( err != paNoError) goto error;

    err = Pa_StartStream(stream);
    if(err != paNoError) goto error;

    printf("Hit ENTEr to stop program.\n");
    Pa_Terminate();
    return 0;

error:
    Pa_Terminate();
    fpringf( stderr, "An error occured while using the portaudio stream\n");
    return 0;
}

