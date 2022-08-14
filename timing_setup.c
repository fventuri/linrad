// Copyright (c) <2012> <Leif Asbrink>
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


rxin_local_workload_reset=workload_reset_flag;
timing_loop_counter_max=(int)(0.2*snd[RXAD].interrupt_rate);
if(timing_loop_counter_max == 0)timing_loop_counter_max=1;
timing_loop_counter=timing_loop_counter_max;
screen_loop_counter_max=(int)(0.1*snd[RXAD].interrupt_rate);
if(screen_loop_counter_max==0)screen_loop_counter_max=1;
screen_loop_counter=screen_loop_counter_max;
read_start_time=current_time();
total_reads=0;
initial_skip_flag=INITIAL_SKIP_FLAG_MAX;
