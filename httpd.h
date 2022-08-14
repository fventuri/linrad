//
// httpd.h --- networking functions for a HTTPD-based GUI
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


#ifndef __HTTPD_H
#define __HTTPD_H

// Write the demodulated audio data into a .wav stream.
void writeWavHeader(int netaudio_fd, int wav_number_of_samples, int fs_wav);

// Write the number of transmitted bytes as a chunk header into the stream.
// If the number is less than 0, then write no number but a terminating chunk header.
void writeChunkHeader(int netaudio_fd, int s);

// Upon opening a new audio stream, the stream header is sent once.
// The audio thread stays connected to the browser over a file descriptor.
// It supplies audio data to the browser's audio player in chunks of constant length.
// The audio data is assembled on-the-fly after evaluating global settings.
void sendAudioResponse(int netaudio_fd, int size_audio_buffer, int fs_wav);

// Send a response to the browser with a standard header.
// The HTTP status code and a content selector are passed as parameters.
void sendHttpResponse(int request_fd, int content_selector, int statusCode);

#endif

