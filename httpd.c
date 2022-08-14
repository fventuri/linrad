//
// httpd.c --- networking functions for a HTTPD-based GUI
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
#if SERVER == 1
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "httpd.h"

void writeWavHeader(int netaudio_fd, int wav_number_of_samples, int fs_wav) {
  (void) wav_number_of_samples;
  write(netaudio_fd, "RIFF", 4);
  // Use -1 as file length since the length is unknown in a stream.
  int some_int = -1;
  write(netaudio_fd, & some_int, 4);
  write(netaudio_fd, "WAVE", 4);
  write(netaudio_fd, "fmt ", 4);
  some_int = 16;
  write(netaudio_fd, & some_int, 4);           /* SubChunk1Size is 16 */
  short some_short = 1;
  write(netaudio_fd, & some_short, 2);         /* PCM is format 1 */
  write(netaudio_fd, & some_short, 2);         /* mono */
  some_int = fs_wav;
  write(netaudio_fd, & some_int, 4);           /* sample rate */
  write(netaudio_fd, & some_int, 4);           /* byte rate */
  some_short = sizeof(some_short);
  write(netaudio_fd, & some_short, 2);         /* block align */
  some_short = 16;
  write(netaudio_fd, & some_short, 2);         /* bits per sample */
  write(netaudio_fd, "data", 4);
  some_int = -1;
  write(netaudio_fd, & some_int, 4);           /* length in bytes */
}

void writeChunkHeader(int netaudio_fd, int s) {
  char response   [10000];
  response[0] = '\0';
  if (s>0) {
    sprintf(&response[strlen(response)], "%x\r\n", s);
  } else {
    sprintf(&response[strlen(response)], "\r\n");
  }
  write(netaudio_fd, response, strlen(response));
}

void sendAudioResponse(int netaudio_fd, int size_audio_buffer, int fs_wav) {
  char response   [10000] = "";
  // Ignore broken network connections so that they don't cause a SIGPIPE.
  // signal(SIGPIPE, SIG_IGN);

  // Upon first request for .wav data, send the header of the data.
  // Upon each subsequent request, send the next block of audio data.
  // The browser uses the header to indicate the amount of requested samples.
  sprintf(&response[strlen(response)], "HTTP/1.1 200 OK\r\n");
  time_t t = time(NULL);
  strftime(&response[strlen(response)], 100, "Date: %a, %e %B %Y %k:%M:%S %Z\r\n", localtime(&t));
  sprintf(&response[strlen(response)], "%s\r\n", "Pragma: no-cache");
  sprintf(&response[strlen(response)], "%s\r\n", "Content-type: audio/x-wav");
  sprintf(&response[strlen(response)], "%s\r\n", "Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0");
  sprintf(&response[strlen(response)], "%s\r\n", "Expires: 0");
  sprintf(&response[strlen(response)], "%s\r\n", "Transfer-Encoding: chunked");
  sprintf(&response[strlen(response)], "\r\n");
  printf("\nResponse: %s", response);
  write(netaudio_fd, response, strlen(response));

  // Send chunked .wav header.
  writeChunkHeader(netaudio_fd, 44);
  writeWavHeader(netaudio_fd, size_audio_buffer/sizeof(short), fs_wav);
  writeChunkHeader(netaudio_fd, -1);
}

void sendFile(int request_fd, char * file_name) {
  FILE *gui_html = fopen(file_name, "rb");
  if (gui_html != NULL) {
    fseek(gui_html, 0, SEEK_END);
    int64_t fsize = ftell(gui_html);
    fseek(gui_html, 0, SEEK_SET);
    char *gui_content = malloc(fsize);
    if (gui_content != NULL) {
      fread(gui_content, fsize, 1, gui_html);
      write(request_fd, gui_content, fsize);
      free(gui_content);
    } else {
      char not_found[] = "GUI internal error with malloc()";
      write(request_fd, not_found, strlen(not_found));
    }
    fclose(gui_html);
  } else {
    char not_found[] = "could not find file";
    write(request_fd, not_found, strlen(not_found));
  }
}

void sendHttpResponse(int request_fd, int content_selector, int statusCode) 
{
char response   [10000] = "";
int k, m, len, cnt;
// Send the reply string to the browser.
// There must be a header in the reply string, but the body may be left empty.
sprintf(&response[strlen(response)], "%s %d %s", "HTTP/1.0 ", statusCode, " OK\r\n");
sprintf(&response[strlen(response)], "%s\r\n", "Content-Type: text/html");
time_t t = time(NULL);
strftime(&response[strlen(response)], 100, "Date: %a, %e %B %Y %k:%M:%S %Z\r\n", localtime(&t));
sprintf(&response[strlen(response)], "%s\r\n", "charset=UTF-8");
// sprintf(&response[strlen(response)], "%s\r\n", "Access-Control-Allow-Origin: http://localhost:8888");
sprintf(&response[strlen(response)], "%s\r\n", "Connection: close");
// End of HTTP header.
sprintf(&response[strlen(response)], "\r\n");
len=strlen(response);
cnt=0;
m=0;
while(len > 0 && cnt < 10)
  {
  k=write(request_fd, &response[m], len);
  cnt++;
  len-=k;
  m+=k;
  }
if (content_selector > 0) {
  // Insert HTML GUI into the HTTP body.
  // Workaround: Insert some HTML with an <audio> tag.
  // sprintf(&http_body[strlen(http_body)], "%s%s%s", "  <audio id='amradio_id' autoplay preload='none'> <source src=\"", amradio_filename, "\"> <p>Your browser does not support the audio element of HTML5</p> </audio>\n");
  sendFile(request_fd, "web_gui.html");
  }
}

#endif
