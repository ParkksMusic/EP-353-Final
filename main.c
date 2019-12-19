#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sndfile.h>

//Compile with:
//gcc main.c -o Main -lsndfile
//Run with:
//./Main

#define kInputFileName "Dry.wav"
#define kOutputFileName "Wet.wav"
//Distortion
#define kGain 100.0
#define kLevel 0.5
//Delay
#define kDelayTime 0.375 //seconds
#define kDecay 0.5
#define kNumEcho 10
#define kMix 0.75 //Dry/Wet
//flanger
#define kDepth 4.0
#define kOffset 4.0
#define kRate 0.5
//Vibrato
#define kVibDepth 4.0 //Hz
#define kVibOffset 4.0 //Hz
#define kVibRate 0.5 //Hz
//tremolo
#define kTremDepth 0.5
#define kTremRate 5.0

typedef struct SoundFile {
  SNDFILE *file;
  SF_INFO info;
} SoundFile;

//Function prototypes
int openInputSndFile(SoundFile*);
int createOutputSndFile(SoundFile *inFile, SoundFile *outFile);
void process(float *inBuffer, float *outBuffer, sf_count_t bufferSize);

//Global Variables
int input;
float gDelayTime = 1.0f; 
double gOffset, gDepth, gSampleRate;


int main(void)
{
    printf("What effect would you like to apply to the audio?\n1)Delay\n2)Flanger\n3)Distortion\n4)Vibrato\n5)Tremolo\n");
    scanf("%d", &input);
  
  SoundFile inFile, outFile;
  
  //Open input file and create output file
  int error = openInputSndFile(&inFile);

  if(error) return 1;

  // Get buffer size
  sf_count_t bufferSize = inFile.info.frames;

  //Allocate buffers for sound processing
  float *inBuffer = (float *) malloc(bufferSize*sizeof(float));
  float *outBuffer = (float *) calloc(bufferSize,sizeof(float));

  //Copy content the file content to the buffer
  sf_read_float(inFile.file, inBuffer, bufferSize);

  //Set delay time based on the sampling rate
  gDelayTime = inFile.info.samplerate * kDelayTime; //Delay Time
    
  //Get sample rate for adjusting the tremolo frequency correctly
  gSampleRate = inFile.info.samplerate;

  // Get vibrato offset and depth. Also sample rate.
  gOffset = inFile.info.samplerate * (kOffset/1000.0);
  gDepth = inFile.info.samplerate * (kDepth/1000.0);
  gSampleRate = inFile.info.samplerate;

  //process inBuffer and copy the result to outBuffer
  process(inBuffer, outBuffer, bufferSize);
  
  // //Create output file and write the result
  error = createOutputSndFile(&inFile, &outFile);
  if(error) return 1;
  sf_write_float(outFile.file, outBuffer, bufferSize);
  
  // //Clean up
  sf_close(inFile.file);
  sf_close(outFile.file);
  free(inBuffer);
  free(outBuffer);
  
  return 0;
}

void process(float *inBuffer, float *outBuffer, sf_count_t bufferSize){
  sf_count_t m;
    double rate, t, tau, delta;
    double a = 0;

    if(input == 1)
    {
          for(sf_count_t n = 0; n < bufferSize; n++){
            outBuffer[n] = inBuffer[n]; //Copy input to output
                for(int i = 1; i <= kNumEcho; i++){
                    m = (int)((double)n - (double)i * gDelayTime); //get previous samples based on delayTime and number of echo
                    if(m >= 0){
                         //Add past audio data to current audio data
                        outBuffer[n] += kMix * pow(kDecay, (double) i) * inBuffer[m];
        }
      }
    } 
  }
    if(input == 2)
    {   
        for(sf_count_t n = 0; n < bufferSize; n++){
            outBuffer[n] = inBuffer[n];
    
            tau = gOffset + gDepth * sin(2.0 * M_PI * kRate * n / gSampleRate);
            t = (double)n - tau;
            m = (int)t;
            delta = t - (double)m;
    
            if (m >= 0 && m + 1 < bufferSize){//Mix original signals
                outBuffer[n] += delta * inBuffer[m + 1] + (1.0 - delta) * inBuffer[m]; 
      }
    }
  }
    if(input == 3)
    {
        for(sf_count_t n = 0; n < bufferSize; n++){
            outBuffer[n] = inBuffer[n] * kGain; //Amplify orignal audio data
            if(outBuffer[n] > 1.0f){
                outBuffer[n] = 1.0f; //Clip signal to 1
        }       else if(outBuffer[n] < - 1.0f){
           outBuffer[n] = -1.0; //Clip signal to -1
        }
        outBuffer[n] *= kLevel; //Adjust volume
      }
    }
    if(input == 4)
    {
        for(sf_count_t n = 0; n < bufferSize; n++){
            tau = gOffset + gDepth * sin(2.0 * M_PI * kVibRate * n / gSampleRate);
            t = (double)n - tau;
            m = (int)t;
            delta = t - (double)m;
            if (m >= 0 && m + 1 < bufferSize){
                outBuffer[n] = delta * inBuffer[m + 1] + (1.0 - delta) * inBuffer[m]; 
    }
  }
}
    if(input == 5)
    {
        for(sf_count_t n = 0; n < bufferSize; n++){
            a = 1.0 + kDepth * sin(2.0 * M_PI * kRate * n / gSampleRate);
            outBuffer[n] = a * inBuffer[n]; //Copy input to output with tremolo effect    
    }
  }
    else
      printf("Error: No effect selected");
      return 1;
}

int openInputSndFile(SoundFile *sndFile){
  //Initialize SF_INFO with 0s (Required for reading)
  memset(&sndFile->info, 0, sizeof(SF_INFO));

  //Open the original sound file as read mode
  sndFile->file = sf_open(kInputFileName, SFM_READ, &sndFile->info);
  if(!sndFile->file){//Check if the file was succefully opened
    printf("Error : could not open file : %s\n", kInputFileName);
		puts(sf_strerror(NULL));
		return 1;
  }

  //Check if the file format is in good shape
  if(!sf_format_check(&sndFile->info)){	
    sf_close(sndFile->file);
		printf("Invalid encoding\n");
		return 1;
	}

  //Check if the file is mono
  if(sndFile->info.channels > 1){
    printf("Input file is not mono\n");
    return 1;
  }

  //print out information about opened sound file
  printf("Sample rate for this file is %d\n", sndFile->info.samplerate);
	printf("A number of channels in this file is %d\n", sndFile->info.channels);
  printf("A number of frames in this file is %lld\n", sndFile->info.frames);
  printf("time is %f\n", (double)sndFile->info.frames / sndFile->info.samplerate);

  return 0;
}

int createOutputSndFile(SoundFile *inFile, SoundFile *outFile){
  //Open another sound file in write mode
  outFile->file = sf_open(kOutputFileName, SFM_WRITE, &inFile->info);
  if(!outFile->file){//Check if the file was succefully opened
    printf("Error : could not open file : %s\n", kOutputFileName);
		puts(sf_strerror(NULL));
		return 1;
	}
  return 0;
}