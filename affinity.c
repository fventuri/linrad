
#include "osnum.h"

#if OSNUM == OSNUM_LINUX

// Do not define USE_AFFINITY unless you are a real experimenter!
// The advantages are small but there are many possible problems.
// Maybe things can be done to make control of processor affinity
// better. If you finde ways of making good use of it, please
// send a mail to leif(at)sm5bsz.com
//
// #define USE_AFFINITY
#endif

#ifdef USE_AFFINITY
#define _GNU_SOURCE
#include <sched.h>
#endif

#include "globdef.h"

#include <ctype.h>
#include <string.h>
#include "uidef.h"
#include "thrdef.h"

#define MAX_HEAVY_THREADS 4

#ifdef USE_AFFINITY

int heavy_thread_id[MAX_HEAVY_THREADS]={THREAD_WIDEBAND_DSP,
                                        THREAD_SECOND_FFT,
                                        THREAD_TIMF2,
                                        THREAD_NARROWBAND_DSP};


void show_aff(int num,cpu_set_t *set)
{
int i, m;
fprintf( stderr,"\n%02d: ",num);
for(i=0; i<no_of_processors; i++)
  {
  m=CPU_ISSET(i, set);
  fprintf( stderr,"[%d]",m);
  }
}

void set_affinity_masks(void)
{
int i, j, k, m;
int cores;
cpu_set_t light_set, heavy_set;
int no_of_blocked, block;
int blocking_pids[32];
int blocked_cpu[32];
int blocked_cpu_ind[32];
//return;
cores=no_of_processors;
if(hyperthread_flag)cores/=2;
if(cpu_rotate >= cores)cpu_rotate-=cores;
for(i=0; i<cores; i++)
  {
  blocked_cpu[i]=-1;
  blocked_cpu_ind[i]=0;
  }
k=ui.max_blocked_cpus;
no_of_blocked=0;
if(k >MAX_HEAVY_THREADS)k=MAX_HEAVY_THREADS;
for(j=0; j<MAX_HEAVY_THREADS; j++)
  {
  for(i=0; i<THREAD_MAX; i++)
    {
    if(heavy_thread_id[j] == i)
      { 
      if(thread_command_flag[i] > THRFLAG_NOT_ACTIVE  &&
                    thread_command_flag[i] < THRFLAG_RETURNED)
        {
        blocking_pids[no_of_blocked]=thread_pid[i];
        no_of_blocked++;
        if(no_of_blocked >= k)goto nomore;
        goto block_found;
        }
      }
    }  
block_found:;
  }
nomore:;
m=cores/2;
k=0;
for(i=0; i<no_of_blocked; i++)
  {
  blocked_cpu[i]=k;
  blocked_cpu_ind[k]=1;
  k+=m;
  if(k>=cores)
    {
cpumap:;    
    k=0;
    m/=2;
    if(m==0)
      {
      no_of_blocked=i+1;
      goto cpumap_x;
      }
    }  
  while(blocked_cpu_ind[k] == 1 && k<cores)k+=m;
  if(k>=cores)goto cpumap;
  }
cpumap_x:;
if(hyperthread_flag)
  {
  for(i=0; i<no_of_blocked; i++)blocked_cpu[i+no_of_blocked]=blocked_cpu[i]+cores;
  }
i=0;
//cpu_rotate=0;  
CPU_ZERO(&light_set);
for(i=0; i<no_of_processors; i++)
  {
  j=0;
  while(blocked_cpu[j] != -1)
    {
    if(blocked_cpu[j]==i)goto skip;
    j++;
    }
  k=i+cpu_rotate;
  if(k>=no_of_processors)k-=no_of_processors;
  CPU_SET(k,&light_set);
skip:;
  }
m=0;
for(i=0; i<THREAD_MAX; i++)
  {
  for(j=0; j<no_of_blocked; j++)
    {
    if(blocking_pids[j] == thread_pid[i])
      {
      CPU_ZERO(&heavy_set);
      block=cpu_rotate+blocked_cpu[m];
      if(block>=cores)block-=cores;
      CPU_SET(block,&heavy_set);
      if(hyperthread_flag)
        {
        block+=cores;
        CPU_SET(block,&heavy_set);
        }      
      m++;
      sched_setaffinity(thread_pid[i], sizeof(cpu_set_t), &heavy_set);
show_aff(i,&heavy_set);
      goto set;
      }
    }  
  if(thread_command_flag[i] > THRFLAG_NOT_ACTIVE  &&
                    thread_command_flag[i] < THRFLAG_RETURNED)
    {
    sched_setaffinity(thread_pid[i], sizeof(cpu_set_t), &light_set);
show_aff(i,&light_set);
    }
set:;  
  }
fprintf( stderr,"\n");
}
#endif

void setup_thread_affinities(void)
{
#ifdef USE_AFFINITY
cpu_rotate=0;
thread_rotate_count=0;
if(ui.max_blocked_cpus == 0)return;
set_affinity_masks();
#endif
}

                                 
void fix_thread_affinities(void)
{
#ifdef USE_AFFINITY
if(ui.max_blocked_cpus == 0)return;
// Step through the running threads and set processor affinity.
thread_rotate_count++;
if(thread_rotate_count > 4)
  {
  thread_rotate_count=0;
  cpu_rotate++;
  set_affinity_masks();
  }
#endif
}


