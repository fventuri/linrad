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


    if(rxin_local_workload_reset != workload_reset_flag)
      {
      if(timing_loop_counter <= timing_loop_counter_max)
        {
        rxin_local_workload_reset=workload_reset_flag;
        timing_loop_counter=timing_loop_counter_max;
        initial_skip_flag=INITIAL_SKIP_FLAG_MAX;
        }
      }
    timing_loop_counter--;
    if(timing_loop_counter == 0)
      {
      timing_loop_counter=timing_loop_counter_max;
      total_reads+=timing_loop_counter_max;
      if(initial_skip_flag != 0)
        {
        initial_skip_flag--;
        if(initial_skip_flag == INITIAL_SKIP_FLAG_MAX-2)
          {
          read_start_time=current_time();
          total_reads=0;
          }
        }
      else
        {
        measured_ad_speed=(float)(total_reads*snd[RXAD].block_frames/
                                       (current_time()-read_start_time));
        }
      }
