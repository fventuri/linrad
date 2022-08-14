//
// hmain.c --- AM radio receiver for PCIe-9842 with a HTTPD-based GUI
//
// Copyright (c) 2014 Juergen Kahrs
//
// Permission is hereby granted, free of charge, to any person 
// obtaining a copy of this software and associated documentation 
// files (the "Software"), to deal in the Software without restriction, 
// including without limitation the rights to use, copy, modify, 
// merge, publish, distribute, sublicense, and/or sell copies of 
// the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be 
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <time.h>
#include <math.h>
#include <complex.h>
#include <signal.h>

#include "httpd.h"

#define FS_WAV               11025
#define FS_ADC_IN            (20*1000*1000)
#define TWO_PI               (2.0 * M_PI)
#define DATA_SIZE            FS_ADC_IN
#define BL                   (FS_ADC_IN / FS_WAV)

// Define frequency raster of the public AM radio stations in Europe.
int am_freq_min         =   531000;
int am_freq_max         =  1602000;
int am_freq_raster      =     9000;
int center_frequency    =   972000;
int bandwidth           =     9000;

// Define global configuration parameters.
char * localhost        = "localhost";
int port                = 8888;
// The audio connection recognizes when to stop by reading a flag.
int request_fd;
int netaudio_fd;
int spectrum_fd;

time_t current_time;
time_t last_block_sent_time;
int chunk_no;

struct GlobalDigitizerContext {
  short * data ;
  short * data_p[2];
  unsigned short bufferId[2];
  short filledBuffer;
  short osciHandle;
  unsigned m_data_size;
  unsigned m_sample_duration_ns;
};
short digitizer_static_buffer [2 * sizeof (short int) * FS_ADC_IN] __attribute__ ((__aligned__(16)));

// These are the few functions supplied 
// by the library that we use.
// #include <wd-dask.h>
#define WD_AI_TRGMOD_POST      0x00   //Post Trigger Mode
#define WD_AI_TRGSRC_SOFT      0x00
#define WD_AI_TrgNegative      0x0
#define WD_IntTimeBase         0x3
#define WD_AI_ADCONVSRC_TimePacer 0
#define PCIe_9842       0x30
#define NoError 0
#define SOFTTRIG_AI      0x1
#define SYNCH_OP        1
#define ASYNCH_OP       2
#define ADL_BOOLEAN unsigned int
#define ManualSoftTrg 0x40

// Notify when the next buffer is ready for transfer.
//DAQ Event type for the event message  
#define DAQEnd   0
#define DBEvent  1
#define TrigEvent  2

short WD_AI_AsyncReTrigNextReady (u_int16_t wCardNumber, ADL_BOOLEAN *bReady, ADL_BOOLEAN *StopFlag, u_int32_t *RdyTrigCnt);
short WD_SoftTriggerGen(u_int16_t wCardNumber, u_int8_t op);
short WD_Register_Card (u_int16_t CardType, u_int16_t card_num);
// short WD_AI_Config (u_int16_t wCardNumber, u_int16_t TimeBase, ADL_BOOLEAN adDutyRestore, u_int16_t ConvSrc, ADL_BOOLEAN doubleEdged, ADL_BOOLEAN AutoResetBuf);
short WD_AI_Trig_Config (u_int16_t wCardNumber, u_int16_t trigMode, u_int16_t trigSrc, u_int16_t trigPol,
                       u_int16_t anaTrigchan, double anaTriglevel, u_int32_t postTrigScans, u_int32_t preTrigScans, u_int32_t trigDelayTicks, u_int32_t reTrgCnt);
short WD_Release_Card  (u_int16_t CardNumber);
short WD_AI_ContBufferSetup (u_int16_t wCardNumber, void *pwBuffer, u_int32_t dwReadCount, u_int16_t *BufferId);
short WD_AI_EventCallBack (u_int16_t wCardNumber, short mode, short EventType, void (*callbackAddr)(int));
short WD_AI_ContReadChannel (u_int16_t CardNumber, u_int16_t Channel,
               u_int16_t BufId, u_int32_t ReadScans, u_int32_t ScanIntrv, u_int32_t SampIntrv, u_int16_t SyncMode);
short WD_AI_AsyncClear (u_int16_t CardNumber, u_int32_t *StartPos, u_int32_t *AccessCnt);
short WD_AI_ContBufferReset (u_int16_t wCardNumber);
//short WD_AI_Set_Mode (u_int16_t  wCardNumber, u_int16_t  modeCtrl, u_int16_t  wIter);
//short WD_AI_AsyncDblBufferMode (u_int16_t CardNumber, ADL_BOOLEAN Enable);
//short WD_AI_AsyncDblBufferHandled (u_int16_t wCardNumber);
//short WD_AI_InitialMemoryAllocated (u_int16_t CardNumber, u_int32_t  *MemSize);
//short WD_AI_AsyncDblBufferHalfReady (u_int16_t CardNumber, ADL_BOOLEAN *HalfReady, ADL_BOOLEAN *StopFlag);
//short WD_AI_AsyncCheck (u_int16_t , ADL_BOOLEAN *Stopped, u_int32_t  *AccessCnt);
//short WD_AI_CurrentViewIndex (u_int16_t wCardNumber, u_int16_t* wDblBufferIndex, u_int32_t *AccessCnt);
//short WD_AI_AsyncDblBufferOverrun (u_int16_t wCardNumber, u_int16_t op, u_int16_t *overrunFlag);

struct GlobalDigitizerContext digitizer_globals;

void digitizerCallback (int sig_no)
{
  ADL_BOOLEAN bReady;
  ADL_BOOLEAN fStop;
  unsigned RdyTrigCnt = 0;

  WD_AI_AsyncReTrigNextReady(digitizer_globals.osciHandle, &bReady, &fStop, &RdyTrigCnt);
  if (digitizer_globals.filledBuffer >= 0)
    printf("digitizerCallback: buffer overflow (%d unhandled, %d filled)\n",
           digitizer_globals.filledBuffer, RdyTrigCnt);
  digitizer_globals.filledBuffer=RdyTrigCnt;
  WD_SoftTriggerGen(digitizer_globals.osciHandle, SOFTTRIG_AI);
  //printf("digitizerCallback: sig_no = %d RdyTrigCnt0 = %d RdyTrigCnt1 = %d filledBuffer = %d\n",
  //       sig_no, RdyTrigCnt[0], RdyTrigCnt[1], digitizer_globals.filledBuffer);
}

void initDigitizer() {
  digitizer_globals.osciHandle = -1;
  digitizer_globals.m_data_size = FS_ADC_IN;
  digitizer_globals.m_sample_duration_ns = 1000 * 1000 * 1000 / FS_ADC_IN;
  digitizer_globals.data = digitizer_static_buffer;
  digitizer_globals.data_p[0] = &(digitizer_globals.data[0 * digitizer_globals.m_data_size]);
  digitizer_globals.data_p[1] = &(digitizer_globals.data[1 * digitizer_globals.m_data_size]);
  digitizer_globals.bufferId[0] = 0;
  digitizer_globals.bufferId[1] = 0;
  digitizer_globals.filledBuffer = -1;
  if ((digitizer_globals.osciHandle=WD_Register_Card (PCIe_9842, 0)) <0 ) {
    perror("initDigitizer: Unable to open digitizer device\n");
    return;
  }

  short err = WD_AI_Trig_Config (digitizer_globals.osciHandle, WD_AI_TRGMOD_POST, WD_AI_TRGSRC_SOFT, WD_AI_TrgNegative, 0, 0.0, 0, 0, 0, 0);
  if (err!=0) {
     printf("initDigitizer: WD_AI_Trig_Config error=%d", err);
     WD_Release_Card(digitizer_globals.osciHandle);
     return;
  }
  short setup_ok0=WD_AI_ContBufferSetup (digitizer_globals.osciHandle,
                                    digitizer_globals.data_p[0],
                                    digitizer_globals.m_data_size,
                                    &digitizer_globals.bufferId[0]);
  short setup_ok1=WD_AI_ContBufferSetup (digitizer_globals.osciHandle,
                                    digitizer_globals.data_p[1],
                                    digitizer_globals.m_data_size,
                                    &digitizer_globals.bufferId[1]);
  if (setup_ok0 != NoError || setup_ok1 != NoError) {
    perror("initDigitizer: WD_AI_ContBufferSetup failed\n");
    WD_Release_Card(digitizer_globals.osciHandle);
    return;
  }
  err=WD_AI_EventCallBack (digitizer_globals.osciHandle, 1, TrigEvent, digitizerCallback);
  if (err != NoError) {
    printf("CDigitizer::acquire_data: WD_AI_EventCallBack failed\n");
  }
  err = WD_AI_ContReadChannel(digitizer_globals.osciHandle,
                                    0,
                                    digitizer_globals.bufferId[0],
                                    digitizer_globals.m_data_size,
                                    digitizer_globals.m_sample_duration_ns/5,
                                    digitizer_globals.m_sample_duration_ns/5,
                                    ASYNCH_OP);
  if (err != NoError) {
     perror("WD_AI_ContReadChannel failed\n");
     return;
   }
}

// Use a sliding Fourier transform band-pass to extract one channel.
// After processing high frequency samples, run an AM demodulator.
// Finally, run an anti-alias filter to remove noise.
// http://en.wikipedia.org/wiki/Goertzel_algorithm
// http://en.wikipedia.org/wiki/High-pass_filter
// http://en.wikipedia.org/wiki/Low-pass_filter
//
void oneGoertzel(short * data, int f_station, int bandwidth, short wav_buf[]) {
  // Variables needed for Goertzel algorithm.
  double s       = 0.0;
  double s_prev  = 0.0;
  double s_prev2 = 0.0;
  double coeff = 2*cos(TWO_PI * f_station/FS_ADC_IN);
  // Variables needed for AM demodulation with a 1st order high-pass filter.
  double RC_d = 0.005;
  double dt = 1.0 / FS_WAV;
  double alpha_d = RC_d / (RC_d + dt);
  double yi_d=0.0;
  double yi1_d=0.0;
  double xi_d=0.0;
  double xi1_d=0.0;
  // Variables needed for anti-alias filter with a 1st order low-pass filter.
  double RC_a = 0.000003;
  double alpha_a = dt / (RC_a + dt);
  double yi_a=0.0;
  double yi1_a=0.0;
  double xi_a=0.0;
  // double xi1_a=0.0;
  double mean=0.0;
  unsigned aidx, widx;

  // The window function used to protect against spectral leakage in digital filtering.
  int window_len = 1.33 * 2.72 * (double) FS_ADC_IN / (double)bandwidth;
  double * window = (double *) malloc(window_len * sizeof(double));
  // check(!window, "oneGoertzel: malloc failed\n");
  // Use a Blackman-Harris 4 term (92dB) window in filtering.
  // http://web.engr.oregonstate.edu/~moon/ece323/hspice98/files/chapter_25.pdf
  unsigned i;
  for (i=0; i<window_len; i++)
    window[i] = 0.35875
              - 0.48829*cos(TWO_PI *     (double)i/(double)window_len)
              + 0.14128*cos(TWO_PI * 2.0*(double)i/(double)window_len)
              - 0.01168*cos(TWO_PI * 3.0*(double)i/(double)window_len);
  memset((void *) wav_buf, 0, DATA_SIZE/BL);
  for (aidx=0; aidx < (DATA_SIZE-window_len)/BL; aidx ++) {
    for (widx=0; widx<window_len; widx++) {
      s = data[aidx*BL + widx] * window[widx] + coeff*s_prev - s_prev2;
      s_prev2 = s_prev;
      s_prev = s;
    }
    xi_d = sqrt(s_prev2*s_prev2 + s_prev*s_prev - coeff*s_prev*s_prev2) / window_len;
    yi_d = alpha_d * (yi1_d + xi_d - xi1_d);

    xi_a = yi_d;
    yi_a = yi1_a + alpha_a * (xi_a - yi1_a);

    wav_buf[aidx] =  (short)(300.0 * yi_a);

    yi1_d=yi_d;
    xi1_d=xi_d;
    yi1_a=yi_a;
    // xi1_a=xi_a;
    // We measure the strength of the demodulated signal.
    mean += yi_a * yi_a;
    s       = 0.0;
    s_prev  = 0.0;
    s_prev2 = 0.0;
  }
  mean /= DATA_SIZE/BL;
  free(window);
  // return sqrt(mean);
}

// Do a recursive fast fourier transform like it is defined in the textbooks.
// This is a very short, readable and un-tweaked FFT.
// It looks different from other "classic" implementations in C because it uses
// the data type "float complex". 
// http://rosettacode.org/wiki/Fast_Fourier_transform#C
void recursiveFFT(float complex inbuf[], float complex outbuf[], int n, int step) {
  if (step < n) {
    recursiveFFT(outbuf, inbuf, n, step * 2);
    recursiveFFT(outbuf + step, inbuf + step, n, step * 2);
    int i;
    for (i = 0; i < n; i += 2 * step) {
      float complex t = cexp(-I * M_PI * i / n) * outbuf[i + step];
      inbuf[ i      / 2] = outbuf[i] + t;
      inbuf[(i + n) / 2] = outbuf[i] - t;
    }
  }
}

// Do we have gathered enough input data ?
// This function clocks the audio output.
int digitizer_buffer_complete() {
  if (digitizer_globals.osciHandle < 0) {
    return current_time > last_block_sent_time;
  } else {
    return digitizer_globals.filledBuffer >= 0;
  }
}

// Read the samples from the filled buffer and demodulate them.
// Put the output into the audio buffer.
void readDigitizer(short audio_buffer[], int sizeof_audio_buffer, int chunk_no) {
  int sidx;
  // If no digitizer available, produce a sine wave.
  if (digitizer_globals.osciHandle < 0) {
    for (sidx=0; sidx<sizeof_audio_buffer/sizeof(short); sidx++)
      audio_buffer[sidx]=10000.0*sin((center_frequency+chunk_no*10*1000)/1000.0*2.0*M_PI*((double)sidx/((double)sizeof_audio_buffer/(double)sizeof(short))));
  } else {
    // Save buffer index of the filled buffer.
    short my_buffer = digitizer_globals.filledBuffer;
    // Tell the digitizer to re-use the filled buffer.
    digitizer_globals.filledBuffer = -1;
    if (my_buffer >= 0) {
      // Demodulate the filled buffer.
      memset((void *) audio_buffer, 0, sizeof_audio_buffer);
      double mean=0.0;
      for (sidx=0; sidx < sizeof_audio_buffer/sizeof(short); sidx ++) {
        audio_buffer[sidx] /= 4;
        mean += audio_buffer[sidx];
      }
      mean /= (double) sizeof_audio_buffer/sizeof(short);
      short * data = digitizer_globals.data_p[my_buffer];
      // Apply both notch filters sequentially to each sample.
      for (sidx=0; sidx < sizeof_audio_buffer/sizeof(short); sidx ++) {
        data[sidx] -= (short) mean;
      }
      oneGoertzel(data, center_frequency, bandwidth, audio_buffer);
    } else if (0 /* switchedOn */) {
      printf("readDigitizer: invalid buffer index %d\n", my_buffer);
      sleep(1);
    }
  }
}

/* The main program implements a network service.
 * It listens on a TCP port for a client (browser).
 * The client sends its request as HTTP requests.
 * The service replies and sends data that implements a GUI.
 */ 
int main() {
  netaudio_fd = -1;
  spectrum_fd = -1;
  signal(SIGPIPE, SIG_IGN);
  int sock_opt_val = 1;
  struct sockaddr_in svr_addr, cli_addr;
  socklen_t sin_len = sizeof(cli_addr);

  // Open a socket for accepting all requests.
  int web_gui_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (web_gui_fd < 0) {
    perror("can't open socket");
    return EXIT_FAILURE;
  }
  // While waiting for network request, pause the waiting and feed the audio stream.
  struct timeval web_gui_timeout;
  web_gui_timeout.tv_sec =  0;
  web_gui_timeout.tv_usec = 100*1000;
  setsockopt(web_gui_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&web_gui_timeout,sizeof(struct timeval));
  setsockopt(web_gui_fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt_val, sizeof(int));
 
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(port);
 
  if (bind(web_gui_fd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(web_gui_fd);
    perror("Can't bind");
    return EXIT_FAILURE;
  }
  listen(web_gui_fd, 20);

  /* Now that the socket is open, start reading HTTP requests.
   * Depending on the URL in the request, decide what to reply.
   * Repeat serving the requests until process termination.
   */
  current_time=time(NULL);
  last_block_sent_time = current_time;
  chunk_no=0;

  initDigitizer();

  while (1) {
    char request[10000];

    current_time = time(NULL);
    // Wait for the next request, create a new file descriptor for the request.
    request_fd = accept(web_gui_fd, (struct sockaddr *) &cli_addr, &sin_len);
    if (request_fd > 0) {
      memset(request, 0, sizeof(request));
      if (recvfrom(request_fd, request, sizeof(request), 0, (struct sockaddr *) &cli_addr,&sin_len) > 0 ){
        printf("HTTP request from port %d:\n%s", ntohs(cli_addr.sin_port), request);
        if (strncmp(request, "GET /amradio_", 13) == 0) {
          if (netaudio_fd > 0)
            close(netaudio_fd);
          // Now start a new audio connection and remember.
          netaudio_fd = request_fd;
          sendAudioResponse(netaudio_fd, sizeof(short)*FS_WAV, FS_WAV);
          last_block_sent_time = current_time;
        } else if (strncmp(request, "GET /spectrum", 12) == 0) {
          if (spectrum_fd > 0)
            close(spectrum_fd);
          // Now start a new audio connection and remember.
          spectrum_fd = request_fd;
          printf("Opening new spectrum connection\n");
          char * freq_min = strstr(request, "freq_min=");
          char * freq_max = strstr(request, "freq_max=");
          char * freq_bin = strstr(request, "freq_bin=");
          int  dummy_int = 0;
          if ((freq_min != NULL) && sscanf(freq_min, "freq_min=%d", &dummy_int) == 1)
            am_freq_min = 1000 * dummy_int;
          if ((freq_max != NULL) && sscanf(freq_max, "freq_max=%d", &dummy_int) == 1)
            am_freq_max = 1000 * dummy_int;
          if ((freq_bin != NULL) && sscanf(freq_bin, "freq_bin=%d", &dummy_int) == 1)
            am_freq_raster = 1000 * dummy_int;
        } else if (strncmp(request, "GET /favicon.ico HTTP", 20) == 0) {
          // There is no favicon icon yet for Linrad.
          sendHttpResponse(request_fd, 0, 404);
        } else if (strncmp(request, "GET /?", 6) == 0) {
          // Handle all requests for variable setting.
          int  dummy_int          = center_frequency;
          // Is this a request that sets the frequency variable ?
          if (sscanf(request, "GET /\?center_frequency_khz=%d", &dummy_int) == 1 &&
                     dummy_int * 1000 != center_frequency) {
            center_frequency = dummy_int * 1000;
          }
          if (sscanf(request, "GET /\?bandwidth_khz=%d", &dummy_int) == 1 &&
                     dummy_int * 1000 != bandwidth) {
            bandwidth = dummy_int * 1000;
          }
          sendHttpResponse(request_fd, 0, 200);
        } else if (strncmp(request, "GET / ", 6) == 0) {
          // Handle all requests for GUI content.
          if (netaudio_fd > 0) {
            close(netaudio_fd);
            netaudio_fd = -1;
          }
          if (spectrum_fd > 0) {
            printf("Closing new spectrum connection\n");
            close(spectrum_fd);
            spectrum_fd = -1;
          }
          // Find the "Host:" header in the request and use it as local host name.
          localhost = strstr(request, "\nHost: ") + 7;
          // Cut off everything behind the host name.
          *strstr(localhost, "\r") = '\0';
          chunk_no = 0 ;
          // This is not a request that sets a variable but a request for the GUI's root element.
          sendHttpResponse(request_fd, 1, 200);
        } else {
          printf("Unrecognized HTTP request\n");
          sendHttpResponse(request_fd, 0, 404);
        }
      } else {
        perror("Accepted HTTP client is disconnected (recvfrom() failed).");
        sleep(1);
      }
      if (request_fd != netaudio_fd && request_fd != spectrum_fd)
        close(request_fd);
    } else {
      // perror("Cannot accept or timed out");
      // We arrive here several times a second after a timeout.
      // The intention is to detect the presence of new audio data to be sent.
      // Is the audio stream open ?
      if (netaudio_fd > 0 && digitizer_buffer_complete()) {
        short audio_buffer[FS_WAV];
        // The audio stream stays connected to the browser over a file descriptor.
        // It supplies audio data to the browser's audio player in chunks of constant length.
        // The audio data is assembled on-the-fly after evaluating global settings.
        last_block_sent_time ++ ;
        // Send chunked .wav data of length 1 second.
        writeChunkHeader(netaudio_fd, sizeof(audio_buffer));
        readDigitizer(audio_buffer, sizeof(audio_buffer), chunk_no);
        if (sizeof(audio_buffer) == write(netaudio_fd, audio_buffer, sizeof(audio_buffer))) {
          writeChunkHeader(netaudio_fd, -1);
          chunk_no ++ ;
        } else {
          // close stream
          writeChunkHeader(netaudio_fd, 0);
          writeChunkHeader(netaudio_fd, -1);
          close(netaudio_fd);
          netaudio_fd = -1;
          chunk_no = 0 ;
        }
        // The new input data has been freed from a DC offset.
        // Convert the data into floating point numbers.
        short my_buffer = digitizer_globals.filledBuffer;
        short * data = digitizer_globals.data_p[my_buffer];
        // Calculate the spectrum of the new data with a fast fourier transform.
        // Choose the window length according to frequency bin width.
        int i, j, n = 1 << (2 + (int) floor(log((double) FS_ADC_IN / am_freq_raster) / log(2.0)));
        float complex inbuf[n];
        float complex outbuf[n];
        float complex spectrum[n];
        for (i = 0; i < n; i++)
          spectrum[i] = 0.0;
        int number_of_FFTs = (1 << 18) / n;
        for (j=0; j<number_of_FFTs; j++) {
          for (i = 0; i < n; i++)
            outbuf[i] = inbuf[i] = data[j*n+i];
          recursiveFFT(inbuf, outbuf, n, 1);
          for (i = 0; i < n; i++)
            spectrum[i] += inbuf[i];
        }

        printf("Sending new spectrum connection\n");
        char JSONdata[100000];
        float freq;
        float freq_min  = am_freq_min / 1000.0;
        float freq_max  = am_freq_max / 1000.0;
        float freq_step = am_freq_raster / 1000;
        sendHttpResponse(spectrum_fd, 0, 200);
        for (freq=freq_min; freq<=freq_max; freq += freq_step) {
          if (freq == freq_min)
            write(spectrum_fd, "{", 1);
          else
            write(spectrum_fd, ",", 1);
          int idx = n*((freq*1000.0)/FS_ADC_IN);
          int dB = 20.0 * (log(cabs(spectrum[idx]) / number_of_FFTs) / log(10.0));
          sprintf(JSONdata, "\"%.1f\":\"%d\"", freq, dB);
          write(spectrum_fd, JSONdata, strlen(JSONdata));
        }
        write(spectrum_fd, "}", 1);
        close(spectrum_fd);
        spectrum_fd = -1;
      } 
      continue;
    }
  }
  if (digitizer_globals.osciHandle >= 0) {
    // Tell digitizer to generate no more triggers.
    unsigned count=0, startPos=0;
    WD_AI_AsyncClear(digitizer_globals.osciHandle, &startPos, &count);
    WD_AI_ContBufferReset(digitizer_globals.osciHandle);
    WD_AI_EventCallBack (digitizer_globals.osciHandle, 0, TrigEvent, NULL);
  }
  return EXIT_SUCCESS;
}
