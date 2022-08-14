
// Copyright (c) <2014> Leif Asbrink and Juergen Kahrs
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

#if SERVER == 1
#if OSNUM == OSNUM_LINUX
#include "osnum.h"
#include "globdef.h"
#include <string.h>
#include <ctype.h>
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <time.h>
#include "thrdef.h"
#include "uidef.h"
#include "screendef.h"
#include "vernr.h"
#include "options.h"
#include "keyboard_def.h"
#include "xdef.h"
#include "httpd.h"

#define FS_WAV               11025

int center_frequency    =   972000;
int bandwidth           =     9000;

// Define global configuration parameters.
char * localhost        = "localhost";
int port                = 8888;
// The audio thread recognizes when to stop by reading a flag.
int html_request_fd;
int html_audio_fd;

time_t local_current_time;
time_t last_block_sent_time;
int chunk_no;




int html_server(void)
 {
  html_audio_fd = -1;
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
  local_current_time=time(NULL);
  last_block_sent_time = local_current_time;
  chunk_no=0;
  while (!kill_all_flag) {
    char request[10000];
    char amradio_filename[10000];

    local_current_time = time(NULL);
    // Use changing URL to enforce a reload by the browser.
    strftime(amradio_filename, sizeof(amradio_filename), "amradio_%Y-%m-%d_%H_%M_%S.wav", localtime(&local_current_time));
    // Wait for the next request, create a new file descriptor for the request.
    html_request_fd = accept(web_gui_fd, (struct sockaddr *) &cli_addr, &sin_len);
    if (html_request_fd > 0) {
      memset(request, 0, sizeof(request));
      if (recvfrom(html_request_fd, request, sizeof(request), 0, (struct sockaddr *) &cli_addr,&sin_len) > 0 ){
        printf("HTTP request from port %d:\n%s", ntohs(cli_addr.sin_port), request);
        if (strncmp(request, "GET /amradio_", 13) == 0) {
          if (html_audio_fd > 0)
            close(html_audio_fd);
          // Now start a new audio thread and remember.
          html_audio_fd = html_request_fd;
          sendAudioResponse(html_audio_fd, sizeof(short)*FS_WAV, FS_WAV);
          last_block_sent_time = local_current_time;
        } else if (strncmp(request, "GET /favicon.ico HTTP", 20) == 0) {
          // There is no favicon icon yet for Linrad.
          sendHttpResponse(html_request_fd, 0, 404);
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
          sendHttpResponse(html_request_fd, 0, 200);
        } else if (strncmp(request, "GET / ", 6) == 0) {
          // Handle all requests for GUI content.
          if (html_audio_fd > 0) {
            close(html_audio_fd);
            html_audio_fd = -1;
          }
          // Find the "Host:" header in the request and use it as local host name.
          localhost = strstr(request, "\nHost: ") + 7;
          // Cut off everything behind the host name.
          *strstr(localhost, "\r") = '\0';
          chunk_no = 0 ;
          // This is not a request that sets a variable but a request for the GUI's root element.
          sendHttpResponse(html_request_fd, 1, 200);
        } else {
          printf("Unrecognized HTTP request\n");
          sendHttpResponse(html_request_fd, 0, 404);
        }
      } else {
        perror("Accepted HTTP client is disconnected (recvfrom() failed).");
      }
      if (html_request_fd != html_audio_fd)
        close(html_request_fd);
    } else {
      // perror("Cannot accept or timed out");
      // We arrive here several times a second after a timeout.
      // The intention is to detect the presence of new audio data to be sent.
      // Is the audio stream open ?
      if (html_audio_fd > 0 ) {
        short audio_buffer[FS_WAV];
        // The audio stream stays connected to the browser over a file descriptor.
        // It supplies audio data to the browser's audio player in chunks of constant length.
        // The audio data is assembled on-the-fly after evaluating global settings.
        last_block_sent_time ++ ;
        // Send chunked .wav data of length 1 second.
        writeChunkHeader(html_audio_fd, sizeof(audio_buffer));
        if (sizeof(audio_buffer) == write(html_audio_fd, audio_buffer, sizeof(audio_buffer))) {
          writeChunkHeader(html_audio_fd, -1);
          chunk_no ++ ;
        } else {
          // close stream
          writeChunkHeader(html_audio_fd, 0);
          writeChunkHeader(html_audio_fd, -1);
          close(html_audio_fd);
          html_audio_fd = -1;
          chunk_no = 0 ;
        }
      } 
      continue;
    }
  }
  return EXIT_SUCCESS;
}

#else
int html_server(void)
{
return 0;
}
#endif
#endif
