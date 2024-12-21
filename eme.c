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


#include "osnum.h"
#include "globdef.h"
#include <ctype.h>
#include "uidef.h"
#include "screendef.h"
#include "keyboard_def.h"
#include "thrdef.h"

#if OSNUM == OSNUM_WINDOWS
#include "wscreen.h"
#endif
#if OSNUM == OSNUM_LINUX
#include "lscreen.h"
#endif

extern void users_eme(void);

#define MAX_EMEPARM 4
extern char *eme_own_info_filename;
extern char *eme_allcalls_filename;
extern char *eme_emedta_filename;
extern char *eme_dirskd_filename;
extern char *eme_dxdata_filename;
extern char *eme_call3_filename;
extern char *eme_error_report_file;

#define EGH 18
#define DX_SEARCH_LINES 2

int eg_old_y1;
int eg_old_y2;
int eg_old_x1;
int eg_old_x2;


void make_eme_graph(int clear_old);
int eme_graph_scro;
int emeparm_int[MAX_EMEPARM];
float *emeparm_float;
char *emeparm_text[MAX_EMEPARM+1]={"Auto init",        //0
                      "UTC correction",                //1
                      "Latitude (+ for N)",            //2  
                      "Longitude (+ for E)",           //3 
                      "Locator"}; 
char tmp_locator[7];                       
char dx_locator[7];
int dx_no;
FILE *locerr_file;
float cos_laref;
float sin_laref;
float cos_dxlat;
float sin_dxlat;
void locator_to_latlong(void);  
void dist_az(float *dist, float *azimuth, float lat, float lon, 
                                        float dxlat, float dxlon);
void latlong_to_locator(float lon, float lat);


float dx_lat;
float tmp_lat;
float dx_lon;
float tmp_lon;
float dx_dist;
float dx_az;
int dxflag;
#define MAX_SUGGESTED 6
int eg_move_flag;

int suggested_calls[MAX_SUGGESTED];
int suggested_calls_counter;
char dx_callsign[EG_DX_CHARS+1];


#define MAX_DXCALLS 10000
#define RAD (PI_L/180)

void check_eg_borders(void)
{
current_graph_minh=eg_vsiz;
current_graph_minw=eg_hsiz;
check_graph_placement((void*)(&eg));
set_graph_minwidth((void*)(&eg));
}


void clear_search_result_box(void)
{
lir_fillbox(eg.xleft+1,egbutt[EG_DX].y1+3+text_height,
eg.xright-eg.xleft-1,eg.ybottom-egbutt[EG_DX].y1-3-text_height,0);
settextcolor(10);
lir_pixwrite(egbutt[EG_DX].x1+text_width/2,egbutt[EG_DX].y1+2,dx_callsign);  
settextcolor(7);
}

void show_dx_location(void)
{
char s[80];
settextcolor(10);
lir_pixwrite(egbutt[EG_LOC].x1+text_width/2,egbutt[EG_LOC].y1+2,dx_locator);  
sprintf(s,"%d   ",(int)(dx_az/RAD));
s[3]=0;
lir_pixwrite(eg.xleft+text_width/2,egbutt[EG_LOC].y1+2,s);  
sprintf(s,"%d     ",(int)(dx_dist));
s[6]=0;
lir_pixwrite(egbutt[EG_LOC].x2+text_width/2,egbutt[EG_LOC].y1+2,s);  
settextcolor(7);
clear_search_result_box();
}



void new_eme_loc(void)
{
int i;
for(i=0; i<6; i++)tmp_locator[i]=numinput_txt[i];
locator_to_latlong();  
for(i=0; i<6; i++)dx_locator[i]=tmp_locator[i];
dx_locator[6]=0;
dx_lat=tmp_lat;
dx_lon=tmp_lon;
dxflag=1;
eme_time=0;
cos_dxlat=cos(RAD*dx_lat);
sin_dxlat=sin(RAD*dx_lat);
dist_az(&dx_dist, &dx_az, emeparm_float[2],emeparm_float[3], dx_lat, dx_lon);
suggested_calls_counter=1;
show_dx_location();
numinput_flag=-1;
}





void clear_dx(void)
{
char s[80];
int i;
for(i=0; i<EG_DX_CHARS; i++)s[i]=' ';
s[EG_DX_CHARS]=0;   
lir_pixwrite(egbutt[EG_DX].x1+text_width/2,egbutt[EG_DX].y1+2,s);  
dxflag=0;
}

void new_dx_result(void)
{    
char s[256];
int i, j, k, m;
int line, pos;
if(suggested_calls_counter == 0)
  {
  clear_dx();
  return;
  }
if(suggested_calls_counter == 1)
  {
  for(i=0; i<EG_DX_CHARS; i++)dx_callsign[i]=dx[suggested_calls[0]].call[i];
  tmp_lon=-dx[suggested_calls[0]].lon;
  tmp_lat=dx[suggested_calls[0]].lat;
  latlong_to_locator(tmp_lon,tmp_lat);
  for(i=0; i<6; i++)numinput_txt[i]=tmp_locator[i];
  new_eme_loc();
  }
else
  {
  for(i=0; i<EG_DX_CHARS; i++)
    {
    dx_callsign[i]=' ';
    }
  }
dx_callsign[EG_DX_CHARS]=0;   
k=0;
for(j=0; j<DX_SEARCH_LINES; j++)
  {
  for(i=0; i<EGH-1; i++)
    {
    s[k]=' ';
    k++;
    }
  s[k]=0;
  k++;
  }
if(suggested_calls_counter > 1)
  {
  line=0;
  pos=0;
  for(i=0; i<suggested_calls_counter; i++)
    {
    k=suggested_calls[i];
    j=0;
    while(dx[k].call[j] != ' ')j++;
    if(dx[k].call[j] == ' ')j--;
    if(pos+j >= EGH-1)
      {
      pos=0;
      line++;
      if(line >= DX_SEARCH_LINES)
        {
        j=0;
        goto nomore;
        }
      }
    m=0;  
    while(m <= j)
      {
      s[EGH*line+pos]=dx[k].call[m];
      m++;
      pos++;
      }
    pos++;    
    }
nomore:;
  if(j==0)settextcolor(13);
  }
clear_search_result_box();      
for(i=0; i<DX_SEARCH_LINES; i++)
  {
  lir_pixwrite(eg.xleft+text_width/2,egbutt[EG_DX].y1+3+(i+1)*text_height,
                                                         &s[i*EGH]);
  }
if(j==0)settextcolor(7);
}


void star_match_dx_a(void)
{
int i, j, k, ia, ib, ib0;
// The first character is a star.
// Find the length of the fragment.
ib0=1;
while(  ib0 < numinput_curpos && numinput_txt[ib0] != '*')ib0++;
// Search for calls containing the string.
for(i=0; i<dx_no; i++)
  {
  ia=1;
  j=0;
  ib=ib0;
get_string:;
  while(dx[i].call[j] != numinput_txt[ia] && j <  CALLSIGN_CHARS )j++;
  if(j < CALLSIGN_CHARS)
    {
// We have found the first character.
    for(k=ia+1; k<ib; k++)
      {
      j++;
      if(j >= CALLSIGN_CHARS)goto fail;
      if(dx[i].call[j] != numinput_txt[k] && numinput_txt[k] != '?')goto fail;    
      }
// The string matches.      
// If there no more '*' in the search string, decide if sucess or fail.
    if(ib==numinput_curpos)
      {
      j++;
      if(j == CALLSIGN_CHARS)goto sucess;
      if(dx[i].call[j] == ' ')goto sucess;
      goto fail;
      }
    ib++;
    if(ib==numinput_curpos)goto sucess;
    ia=ib;
    while(  ib < numinput_curpos && numinput_txt[ib] != '*')ib++;
    goto get_string;  
sucess:;
    suggested_calls[suggested_calls_counter]=i;
    suggested_calls_counter++;
    if(suggested_calls_counter >= MAX_SUGGESTED)goto maxsug;
    }
fail:;
  }
maxsug:;
new_dx_result();    
}

void star_match_dx_b(void)
{
int i, j, k, ia, ib, ib0;
// The first character is not a star.
// Find the length of the fragment.
ib0=1;
while(  ib0 < numinput_curpos && numinput_txt[ib0] != '*')ib0++;
// Search for calls beginning with the fragment string.
for(i=0; i<dx_no; i++)
  {
  for(k=0; k<ib0; k++)
    {
    if(dx[i].call[k] != numinput_txt[k] && numinput_txt[k] != '?')goto fail;
    }
  ib=ib0+1;
  if(ib==numinput_curpos)goto sucess;
  j=0;
get_string:;
  ia=ib;
  while(  ib < numinput_curpos && numinput_txt[ib] != '*')ib++;
  while(dx[i].call[j] != numinput_txt[ia] && j <  CALLSIGN_CHARS )j++;
  if(j < CALLSIGN_CHARS)
    {
// We have found the first character.
    for(k=ia+1; k<ib; k++)
      {
      j++;
      if(j >= CALLSIGN_CHARS)goto fail;
      if(dx[i].call[j] != numinput_txt[k] && numinput_txt[k] != '?')goto fail;    
      }
// The string matches.      
// If there no more '*' in the search string, decide if sucess or fail.
    if(ib==numinput_curpos)
      {
      j++;
      if(j == CALLSIGN_CHARS)goto sucess;
      if(dx[i].call[j] == ' ')goto sucess;
      goto fail;
      }
    ib++;
    if(ib==numinput_curpos)goto sucess;
    goto get_string;  
sucess:;
    suggested_calls[suggested_calls_counter]=i;
    suggested_calls_counter++;
    if(suggested_calls_counter >= MAX_SUGGESTED)goto maxsug;
    }
fail:;
  }
maxsug:;
new_dx_result();    
}


void new_eme_dx(void)
{
int i,j,k;
pause_thread(THREAD_SCREEN);
hide_mouse(eg.xleft,eg.xright,eg.ytop,eg.ybottom);  
// Search the data base for the call sign in numinput_txt.
// Just return if we have 2 or less characters
numinput_flag=-1;
suggested_calls_counter=0;
// There may be '*' and '?' in the call.
// Clean up the search string in case '?' are next to '*'
// or there are multiple '*'.
i=0; 
while(i<numinput_curpos)
  {
  if(numinput_txt[i]=='*')
    {
    while(i>0 && numinput_txt[i-1] == '*')i--;
    while(i>0 && numinput_txt[i-1] == '?')
      {
      i--;
      k=i;
      while(k<numinput_curpos)
        {
        numinput_txt[k]=numinput_txt[k+1];
        k++;
        }
      numinput_curpos--;
      if(i>0)i--;
      }
    }
  if(numinput_txt[i]=='*')
    {
    while(i<numinput_curpos-1 && 
                  (numinput_txt[i+1] == '?' || numinput_txt[i+1] == '*'))
      {
      k=i+1;
      while(k<numinput_curpos)
        {
        numinput_txt[k]=numinput_txt[k+1];
        k++;
        }
      numinput_curpos--;
      }
    }  
  i++;
  }
if(numinput_curpos<=2)
  {
  clear_dx();
  goto new_eme_dx_x;
  }
// If the first character is '*' call a special routine.
if(numinput_txt[0] == '*')
  {
  star_match_dx_a();
  goto new_eme_dx_x;
  }
// If there is any '*' at all, call another special routine
for(i=0; i<numinput_curpos; i++)
  {
  if(numinput_txt[i] == '*')
    {
    star_match_dx_b();
    goto new_eme_dx_x;
    }
  }
// Search for matching calls with fixed positions for the
// characters.
for(i=0; i<dx_no; i++)
  {
  j=0;
  while(j<numinput_curpos && 
         (numinput_txt[j] == '?' || numinput_txt[j]==dx[i].call[j])  )j++;
  if(j == numinput_curpos && 
        (dx[i].call[j] == ' ' || numinput_curpos >= CALLSIGN_CHARS)  )
    {
    suggested_calls[suggested_calls_counter]=i;
    suggested_calls_counter++;
    if(suggested_calls_counter >= MAX_SUGGESTED)goto maxsug;
    }
  }
maxsug:;
new_dx_result();    
new_eme_dx_x:;
resume_thread(THREAD_SCREEN);
}


void help_on_eme_graph(void)
{
int msg_no;
int event_no;
// Set msg invalid in case we are not in any select area.
msg_no=-1;
for(event_no=0; event_no<MAX_EGBUTT; event_no++)
  {
  if( egbutt[event_no].x1 <= mouse_x && 
      egbutt[event_no].x2 >= mouse_x &&      
      egbutt[event_no].y1 <= mouse_y && 
      egbutt[event_no].y2 >= mouse_y) 
    {
    switch (event_no)
      {
      case EG_TOP:
      case EG_BOTTOM:
      case EG_LEFT:
      case EG_RIGHT:
      msg_no=101;
      break;

      case EG_MINIMISE: 
      msg_no=70;
      break;
  
      case EG_LOC:
      msg_no=71;
      break;

      case EG_DX:
      msg_no=72;
      break;
      }
    }  
  }
help_message(msg_no);
}

void mouse_continue_eme_graph(void)
{
int i, j;
switch (mouse_active_flag-1)
  {
  case EG_TOP:
  if(eg.ytop!=mouse_y)goto egm;
  break;

  case EG_BOTTOM:
  if(eg.ybottom!=mouse_y)goto egm;
  break;

  case EG_LEFT:
  if(eg.xleft!=mouse_x)goto egm;
  break;

  case EG_RIGHT:
  if(eg.xright==mouse_x)break;
egm:;
  pause_screen_and_hide_mouse();
  graph_borders((void*)&eg,0);
  eg_move_flag=1;
  if(eg_oldx==-10000)
    {
    eg_oldx=mouse_x;
    eg_oldy=mouse_y;
    }
  else
    {
    i=mouse_x-eg_oldx;
    j=mouse_y-eg_oldy;  
    eg_oldx=mouse_x;
    eg_oldy=mouse_y;
    eg.ytop+=j;
    eg.ybottom+=j;      
    eg.xleft+=i;
    eg.xright+=i;
    check_eg_borders();
    }
  graph_borders((void*)&eg,15);
  resume_thread(THREAD_SCREEN);
  break;
    
  default:
  goto await_release;
  }
if(leftpressed == BUTTON_RELEASED)goto finish;
return;
await_release:;
if(leftpressed != BUTTON_RELEASED) return;
switch (mouse_active_flag-1)
  {
  case EG_MINIMISE: 
  eg.minimise^=1;
  break;
  
  case EG_LOC:
  for(i=0; i<EG_DX_CHARS; i++) dx_callsign[i]=' ';
  mouse_active_flag=1;
  numinput_chars=EG_LOC_CHARS;
  numinput_xpix=egbutt[EG_LOC].x1+text_width/2;
  numinput_ypix=egbutt[EG_LOC].y1+2;
  erase_numinput_txt();
  numinput_flag=TEXT_PARM;
  par_from_keyboard_routine=new_eme_loc;
  return;

  case EG_DX:
  mouse_active_flag=1;
  numinput_chars=EG_DX_CHARS;
  numinput_xpix=egbutt[EG_DX].x1+text_width/2;
  numinput_ypix=egbutt[EG_DX].y1+2;
  erase_numinput_txt();
  numinput_flag=TEXT_PARM;
  par_from_keyboard_routine=new_eme_dx;
  return;

  default:
  lirerr(872);
  break;
  }
finish:;
leftpressed=BUTTON_IDLE;  
mouse_active_flag=0;
make_eme_graph(TRUE);
if(eg.minimise==0)calculate_moon_data();
eg_oldx=-10000;
}

void mouse_on_eme_graph(void)
{
int event_no;
// First find out is we are on a button or border line.
for(event_no=0; event_no<MAX_EGBUTT; event_no++)
  {
  if( egbutt[event_no].x1 <= mouse_x && 
      egbutt[event_no].x2 >= mouse_x &&      
      egbutt[event_no].y1 <= mouse_y && 
      egbutt[event_no].y2 >= mouse_y) 
    {
    eg_old_y1=eg.ytop;
    eg_old_y2=eg.ybottom;
    eg_old_x1=eg.xleft;
    eg_old_x2=eg.xright;
    mouse_active_flag=1+event_no;
    current_mouse_activity=mouse_continue_eme_graph;
    return;
    }
  }
// Not button or border.
// Do nothing.
current_mouse_activity=mouse_nothing;
mouse_active_flag=1;
}


void make_eme_graph(int clear_old)
{
int xj,xi,yj,yi;
pause_thread(THREAD_SCREEN);
if(clear_old)
  {
  hide_mouse(eg_old_x1,eg_old_x2,eg_old_y1,eg_old_y2);
  lir_fillbox(eg_old_x1,eg_old_y1,eg_old_x2-eg_old_x1+1,
                                                    eg_old_y2-eg_old_y1+1,0);
  }
eg_move_flag=0;
if(eg.minimise == 0 && eme_flag == 2)
  {
  eg_hsiz=EGH*text_width;
  eg_vsiz=(4+DX_SEARCH_LINES)*text_height+3;
  eme_active_flag=1;
  cos_laref=cos(RAD*emeparm_float[2]);
  sin_laref=sin(RAD*emeparm_float[2]);
  }
else
  {  
  eg_hsiz=2*text_width-1;
  eg_vsiz=1.5*text_height-1;
  eme_active_flag=0;
  }
eg_flag=1;
check_eg_borders();
clear_button(egbutt, MAX_EGBUTT);
hide_mouse(eg.xleft,eg.xright,eg.ytop,eg.ybottom);  
scro[eme_graph_scro].no=EME_GRAPH;
scro[eme_graph_scro].x1=eg.xleft;
scro[eme_graph_scro].x2=eg.xright;
scro[eme_graph_scro].y1=eg.ytop;
scro[eme_graph_scro].y2=eg.ybottom;
egbutt[EG_LEFT].x1=eg.xleft;
egbutt[EG_LEFT].x2=eg.xleft+2;
egbutt[EG_LEFT].y1=eg.ytop;
egbutt[EG_LEFT].y2=eg.ybottom;
egbutt[EG_RIGHT].x1=eg.xright-2;
egbutt[EG_RIGHT].x2=eg.xright;
egbutt[EG_RIGHT].y1=eg.ytop;
egbutt[EG_RIGHT].y2=eg.ybottom;
egbutt[EG_TOP].x1=eg.xleft;
egbutt[EG_TOP].x2=eg.xright;
egbutt[EG_TOP].y1=eg.ytop;
egbutt[EG_TOP].y2=eg.ytop+2;
egbutt[EG_BOTTOM].x1=eg.xleft;
egbutt[EG_BOTTOM].x2=eg.xright;
egbutt[EG_BOTTOM].y1=eg.ybottom-2;
egbutt[EG_BOTTOM].y2=eg.ybottom;
// Draw the border lines
graph_borders((void*)&eg,7);
eg_oldx=-10000;
settextcolor(7);
xj=eg.xleft+text_width-1;
if(eme_active_flag != 0)
  {
  xi=xj+3.5*text_width;
  yi=eg.ytop+2*text_height;
  egbutt[EG_LOC].x1=xi;
  egbutt[EG_LOC].x2=xi+text_width*(EG_LOC_CHARS+0.5);
  egbutt[EG_LOC].y1=yi;
  yi+=text_height;
  xi-=2*text_width;
  egbutt[EG_LOC].y2=yi;
  lir_hline(egbutt[EG_LOC].x1,egbutt[EG_LOC].y1,egbutt[EG_LOC].x2,7);
  if(kill_all_flag) return;
  lir_hline(egbutt[EG_LOC].x1,egbutt[EG_LOC].y2,egbutt[EG_LOC].x2,7);
  if(kill_all_flag) return;
  lir_line(egbutt[EG_LOC].x1,egbutt[EG_LOC].y1,egbutt[EG_LOC].x1,
                                                      egbutt[EG_LOC].y2,7);
  if(kill_all_flag) return;
  lir_line(egbutt[EG_LOC].x2,egbutt[EG_LOC].y1,egbutt[EG_LOC].x2,
                                                      egbutt[EG_LOC].y2,7);
  if(kill_all_flag) return;
  yi+=2;
  yj=yi+text_height/2;
  egbutt[EG_DX].x1=xi;
  egbutt[EG_DX].x2=xi+text_width*(EG_DX_CHARS+0.5);
  egbutt[EG_DX].y1=yi;
  yi+=text_height;
  egbutt[EG_DX].y2=yi;
  yi+=2;
  if(dxflag != 0)
    {
    show_dx_location();
    }
  lir_hline(egbutt[EG_DX].x1,egbutt[EG_DX].y1,egbutt[EG_DX].x2,7);
  if(kill_all_flag) return;
  lir_hline(egbutt[EG_DX].x1,egbutt[EG_DX].y2,egbutt[EG_DX].x2,7);
  if(kill_all_flag) return;
  lir_line(egbutt[EG_DX].x1,egbutt[EG_DX].y1,egbutt[EG_DX].x1,
                                                      egbutt[EG_DX].y2,7);
  if(kill_all_flag) return;
  lir_line(egbutt[EG_DX].x2,egbutt[EG_DX].y1,egbutt[EG_DX].x2,
                                                      egbutt[EG_DX].y2,7);
  if(kill_all_flag) return;
  }
else
  {
  yj=eg.ybottom-text_height/2-1; 
  }
make_button(xj,yj,egbutt,EG_MINIMISE,'X'); 
resume_thread(THREAD_SCREEN);
make_modepar_file(GRAPHTYPE_EG);
}

void init_eme_graph(void)
{
FILE *file;
int i, j, k;
char s[80];
// In case we process saved data from disk, the real time
// moon window is useless, just skip it.
if(savefile_parname[0] != 0)return;
s[0]=0;
if (read_modepar_file(GRAPHTYPE_EG) == 0)
  {
  eg.xleft=15*text_width;
  eg.xright=17*text_width;
  eg.ybottom=0.7*screen_height;
  eg.ytop=eg.ybottom-1.5*text_height;
  eg.minimise=1;
  }
dxflag=0;
if(eme_flag == 2)
  {
  if(dx!=NULL)
    {
    lirerr(81975);
    return;
    }
  file = fopen(eme_dxdata_filename, "r");
  if(file == NULL)
    {
    lirerr(1130);
    return;
    }
  k=fread(s,1,1,file);
  j=0;
  while(k!=0 && s[j] != 10 && s[j] != 13)
    {
    j++;
    k=fread(&s[j],1,1,file);
    if(j >= 80)
      {
err1131:;      
      lirerr(1131);
      return;
      }
    }
  if(j == 0 || k == 0 || j > 79)goto err1131;
  s[j]=0;
  sscanf(s,"%d",&dx_no);
  if(dx_no > MAX_DXCALLS)goto err1131;
  dx=malloc(dx_no*sizeof(DXDATA));
  if(dx == NULL)
    {
    lirerr(1132);
    return;
    }
  i=0;
read_dx:;
  k=fread(s,1,1,file);
  j=0;
  while(k!=0 && (s[j] == 10 || s[j] == 13)) k=fread(s,1,1,file);
  if(k == 0)
    {
    lirerr(1131);
    return;
    }
  while(k!=0 && s[j] != 10 && s[j] != 13)
    {
    j++;
    k=fread(&s[j],1,1,file);
    if(j >= 80)
      {
      lirerr(1131);
      return;
      }
    }
  if(j == 0 || k == 0 || j > 79)
    {
    lirerr(1131);
    return;
    }
  s[j]=0;
  for(k=0; k<CALLSIGN_CHARS; k++)dx[i].call[k]=s[k];  
  sscanf(&s[CALLSIGN_CHARS],"%f %f",&dx[i].lat,&dx[i].lon);
  i++;
  if(i<dx_no)goto read_dx;
  fclose(file);
  }
eme_graph_scro=no_of_scro;
make_eme_graph(FALSE);
no_of_scro++;
if(no_of_scro >= MAX_SCRO)lirerr(89);
}


void dist_az(float *dist, float *azimuth, float lat, float lon, 
                                        float dxlat, float dxlon)
{
float sin_dxl, cos_dxl;
float sin_lat, cos_lat;
float t1,t2,t3;
cos_dxl=cos(PI_L*dxlat/180);
sin_dxl=sin(PI_L*dxlat/180);
cos_lat=cos(PI_L*lat/180);
sin_lat=sin(PI_L*lat/180);
t1=PI_L*(lon-dxlon)/180;
t2=sin_dxl*sin_lat+cos_dxl*cos_lat*cos(t1);
if(fabs(t2)>1)t2=1;
t2=atan2(sqrt(1-t2*t2),t2);
dist[0]=t2*6366;
if(dist[0] < 5)
  {
  dist[0]=0;
  azimuth[0]=0;
  return;
  }
if(t1<-PI_L)t1+=2*PI_L;
if(t1> PI_L)t1-=2*PI_L;
t3=(sin_dxl-sin_lat*cos(t2))/(cos_lat*sin(t2));
if(fabs(t3) > 1)t3=1;
azimuth[0]=atan2(sqrt(1-t3*t3),t3);
if(t1 == 0)
  {
  if(dxlat < lat)
    {
    azimuth[0]=PI_L;
    }
  else
    {
    azimuth[0]=2*PI_L;
    }
  }
else
  {      
  if(t1 > 0)azimuth[0]=2*PI_L-azimuth[0];
  }
}

void check_latest_dx(void)
{
char s[80];
int i, j, k;
float dist, az;
// Check that the latest call sign is not already in the data base.
for(i=0; i<dx_no; i++)
  {
  j=0;
  while(j<CALLSIGN_CHARS && dx[i].call[j] == dx[dx_no].call[j])j++;
  if(j==CALLSIGN_CHARS)
    {
// We have located a duplicate call sign.
// Just ignore it in case the location is the same (or close to)
// If longitude or latitude is 1000 for one, use the other. 
    if(dx[i].lon == 1000)dx[i].lon=dx[dx_no].lon;
    if(dx[i].lat == 1000)dx[i].lat=dx[dx_no].lat;
    if(dx[dx_no].lon == 1000)dx[dx_no].lon=dx[i].lon;
    if(dx[dx_no].lat == 1000)dx[dx_no].lat=dx[i].lat;
    if(fabs(dx[i].lon-dx[dx_no].lon) > 0.1 || 
       fabs(dx[i].lat-dx[dx_no].lat) > 0.1)
      {
      dist_az(&dist, &az, dx[i].lon,dx[i].lat,dx[dx_no].lon,dx[dx_no].lat);
      if(dist > 200)
        {
        if(locerr_file == NULL)
          {
          locerr_file = fopen(eme_error_report_file, "w");
          if(locerr_file == NULL)
            {
            lirerr(1133);
            return;
            }
          }
        for(k=0; k<CALLSIGN_CHARS; k++)s[k]=dx[i].call[k];
        s[CALLSIGN_CHARS]=0;
        fprintf(locerr_file,
              "%s %d km   lon %.2f lat %.2f          [lon %.2f lat %.2f]\n",
                                   s, (int)(dist), dx[i].lon, dx[i].lat,
                                         dx[dx_no].lon, dx[dx_no].lat);
        }
      }
    dx[i].lon=0.5*(dx[i].lon+dx[dx_no].lon);  
    dx[i].lat=0.5*(dx[i].lat+dx[dx_no].lat);  
    dx_no--;
    }
  }  
}



void latlong_to_locator(float lon, float lat)
{
int i;
float t1,t2;
t1=lon;
t2=lat;
t1+=180;
i=t1/20;
t1=t1-20*i;
tmp_locator[0]=i+'A';
i=t1/2;
t1=t1-2*i;
tmp_locator[2]=i+'0';
t1*=12;
tmp_locator[4]=t1+'a';
t2+=90;
i=t2/10;
t2=t2-10*i;
tmp_locator[1]=i+'A';
i=t2;
tmp_locator[3]=i+'0';
t2-=i;
t2*=24;
tmp_locator[5]=t2+'a';
}






  
void locator_to_latlong(void)
{
int flg;
float lat, lon;
#ifdef __NetBSD__
tmp_locator[0]=toupper((int)(unsigned char)tmp_locator[0]);
tmp_locator[1]=toupper((int)(unsigned char)tmp_locator[1]);
tmp_locator[4]=tolower((int)(unsigned char)tmp_locator[4]);
tmp_locator[5]=tolower((int)(unsigned char)tmp_locator[5]);
#else
tmp_locator[0]=toupper(tmp_locator[0]);
tmp_locator[1]=toupper(tmp_locator[1]);
tmp_locator[4]=tolower(tmp_locator[4]);
tmp_locator[5]=tolower(tmp_locator[5]);
#endif  
flg=0;
if(tmp_locator[0]<'A')
  {
  tmp_locator[0]='A';
  flg=1;
  }
if(tmp_locator[0]>'R')
  {
  tmp_locator[0]='R';
  flg=1;
  }
if(tmp_locator[1]<'A')
  {
  tmp_locator[1]='A';
  flg=1;
  }
if(tmp_locator[1]>'R')
  {
  tmp_locator[1]='R';
  flg=1;
  }
if(tmp_locator[2]<'0' || tmp_locator[2]>'9')flg=1;
if(tmp_locator[3]<'0' || tmp_locator[3]>'9')flg=1;
if(flg != 0)
  {
  tmp_locator[2]='4';
  tmp_locator[3]='4';
  tmp_locator[4]='a';
  tmp_locator[5]='a';
  }
if( tmp_locator[4]<'a' || 
    tmp_locator[4]>'x' ||
    tmp_locator[5]<'a' || 
    tmp_locator[5]>'x' )
  {
  tmp_locator[4]='l';
  tmp_locator[5]='l';
  }
lon=tmp_locator[4]-'a'+0.5;
lon/=12;
lon-=180;
lon+=(tmp_locator[2]-'0')*2;
lon+=(tmp_locator[0]-'A')*20;
if(fabs(lon)>180.001)lon=0;
lat=tmp_locator[5]-'a'+0.5;
lat/=24;
lat-=90;
lat+=(tmp_locator[3]-'0');
lat+=(tmp_locator[1]-'A')*10;
if(fabs(lat)>90.001)lat=0;
tmp_lon=lon;
tmp_lat=lat;
}  


// eme_flag=0  Nothing loaded. Load all data in case the first parameter =1
// eme_flag=1  Nothing loaded. Load all data unconditionally. 
// eme_flag=2  Database loaded. Ask if it should be changed.

void read_eme_database(void)
{
int i, j, k, m;
char s[80];
FILE *own_file;
char *tmpbuf;
emeparm_float=(float*)((void*)(emeparm_int));
// Set flag to zero if file is empty
own_file = fopen(eme_own_info_filename, "r");
if(own_file == NULL)
  {
  eme_flag=0;
  return;
  }
// Allocate a lot of scratch memory.
// No processing arrays are allocated this early in program
// execution so there should be plenty of memory available.
tmpbuf=malloc(131072);
for(i=0;i<4096; i++)tmpbuf[i]=0;
if(tmpbuf == NULL)
  {
  lirerr(1075);
  return;
  }
m=fread(tmpbuf,1,4096,own_file);
fclose(own_file);
if(m == 4096)
  {
// Set flag to zero if the data is incorrect.  
daterr:;
  eme_flag=0;
  clear_screen();    
  sprintf(s,"Data error in file %s",eme_own_info_filename);
  lir_text(5,5,s);
  lir_text(8,8,"PRESS ANY KEY");
  clear_await_keyboard();
  if(kill_all_flag) goto read_eme_x;
  process_current_lir_inkey();
  goto read_eme_x;
  }
k=0;
for(i=0; i<MAX_EMEPARM; i++)
  {
  while( (tmpbuf[k]==' ' || tmpbuf[k]== '\n') && k<m)k++;
  j=0;
  while(tmpbuf[k]== emeparm_text[i][j])
    {
    k++;
    j++;
    } 
  if(emeparm_text[i][j] != 0)
    {
    tmpbuf=chk_free(tmpbuf);
    goto daterr;
    }
  while(tmpbuf[k]!='['&&k<m-1)k++;
  k++;
  if(i<2)
    {
    sscanf(&tmpbuf[k],"%d",&emeparm_int[i]);
    }
  else
    {
    sscanf(&tmpbuf[k],"%f",&emeparm_float[i]);
    }
// If the first parameter is zero, do not read database now.
// The user wants to save time, memory or screen space.
// Just set flag and exit. 
  if( eme_flag == 0 && i == 0 && emeparm_int[0] == 0)
    {
    eme_flag=1;
    goto read_eme_x;
    }
  while(tmpbuf[k]!='\n'&&k<m)k++;
  }
latlong_to_locator(emeparm_float[3], emeparm_float[2]);
eme_flag=2;  
read_eme_x:;
free(tmpbuf);
}

void init_eme_database(void)
{
int i, j, k, m, hr, min;
char s[384];
float t1;
FILE *own_file, *dx_file;
clear_screen();
if(eme_flag != 0)
  {
  read_eme_database();
  if(kill_all_flag) return;
  if(eme_flag == 2)
    {
begin:;
    clear_screen();    
    lir_text(5,5,"Press M to update, F1 for help.");
    lir_text(5,6,"Any other key to use old data.");
    clear_await_keyboard();
    if(kill_all_flag) return;
    process_current_lir_inkey();
    if(kill_all_flag) return;
    if(lir_inkey == F1_KEY || lir_inkey == '!')
      {
      help_message(301);
      if(kill_all_flag) return;
      goto begin;
      }
    else
      {  
      if(lir_inkey != 'M')return;
      }
    }
  }  
if(eme_flag == 0)
  {
  for(i=0; i<MAX_EMEPARM; i++)emeparm_int[i]=0;
  tmp_locator[0]=0;
  }
new_lin:;
tmp_locator[6]=0;
clear_screen();
sprintf(s,"Locator: %6s (%d to enter new)",tmp_locator,MAX_EMEPARM);
lir_text(10,0,s);
t1=lir_get_epoch_seconds();
t1+=3600*emeparm_int[1];
min=t1/60;
hr=min/60;
hr%=24;
min%=60;
sprintf(s,"UTC time is %d.%02d (set correction if needed)",hr,min);
lir_text(10,1,s);
for(i=0; i<MAX_EMEPARM; i++)
  {
  if(i<2)
    {
    sprintf(s,"%d  %s  [%d]",i,emeparm_text[i],emeparm_int[i]);
    }
  else
    {
    sprintf(s,"%d  %s  [%f]",i,emeparm_text[i],emeparm_float[i]);
    }
  lir_text(0,i+2,s);
  }    
lir_text(8,MAX_EMEPARM+5,"Enter line number to change.");
lir_text(8,MAX_EMEPARM+6,"(9 to save and exit)");
clear_await_keyboard();
if(kill_all_flag) return;
if(lir_inkey != '9')
  {
  i=lir_inkey-'0';
  if(i<0 || i>MAX_EMEPARM)goto new_lin;
  sprintf(s,"Enter new value for %s",emeparm_text[i]);
  lir_text(8,MAX_EMEPARM+8,s);
  j=0;
  while(s[j]!=0)j++;
  j=lir_get_filename(j+10,MAX_EMEPARM+8,s);
  if(kill_all_flag) return;
  if(j==0)goto new_lin;
  switch (i)
    {
    case 0:
    sscanf(s,"%d",&emeparm_int[0]);
    if(emeparm_int[0] != 0)emeparm_int[0]=1;
    break;

    case 1:
    sscanf(s,"%d",&emeparm_int[1]);
    if(abs(emeparm_int[1])>23)emeparm_int[1]=0;
    break;

    case 2:
    sscanf(s,"%f",&emeparm_float[2]);
    if(fabs(emeparm_float[2])>90)emeparm_float[2]=0;
    latlong_to_locator(emeparm_float[3], emeparm_float[2]);
    break;

    case 3:
    sscanf(s,"%f",&emeparm_float[3]);
    if(fabs(emeparm_float[3])>180)emeparm_float[3]=0;
    latlong_to_locator(emeparm_float[3], emeparm_float[2]);
    break;

    case 4:
    j=0;
    while(s[j]==0)j++;
    i=0;
    while(i<6)
      {
      tmp_locator[i]=s[j];
      i++;
      j++;
      if(s[j] == 0)
        {
        while(i<6)
          {
          tmp_locator[i]=' ';
          i++;
          }
        }
      }    
    tmp_locator[6]=0; 
    locator_to_latlong();  
    emeparm_float[3]=tmp_lon;
    emeparm_float[2]=tmp_lat;
    break;
    }
  goto new_lin;
  }  
// No processing memory is yet allocated. Store station data
// in a non-economic way that is easy to manipulate.
dx=malloc(MAX_DXCALLS*sizeof(DXDATA));
if(dx == NULL)
  {
  lirerr(1075);
  return;
  }
dx_file = fopen(eme_allcalls_filename, "r");
dx_no=0;
clear_screen();
locerr_file=NULL;
if(dx_file != NULL)
  {
  k=fread(s,1,1,dx_file);
allcall_a:;
  j=0;
  while(k!=0 && s[j] != 10 && s[j] != 13)
    {
    j++;
    k=fread(&s[j],1,1,dx_file);
    if(j >= CALLSIGN_CHARS)
      {
      lirerr(1122);
      return;
      }
    }
  if(j == 0)goto allcall_x;
  for(i=0; i<j; i++)dx[dx_no].call[i]=s[i];
  for(i=j; i<CALLSIGN_CHARS; i++)dx[dx_no].call[i]=' ';  
  dx[dx_no].lon=1000;
  dx[dx_no].lat=1000;
  k=fread(s,1,1,dx_file);
  while(k!=0 && (s[0] == 10 || s[0] == 13))
    {
    k=fread(s,1,1,dx_file);
    }
  check_latest_dx();
  dx_no++;  
  if(dx_no>=MAX_DXCALLS)
    {
    lirerr(1123);
    return;
    }
  if(k > 0)goto allcall_a;
allcall_x:;
  fclose(dx_file);
  }
dx_file = fopen(eme_emedta_filename, "r");
if(dx_file != NULL)
  {
emedta_a:;
// Read this file with a fixed record length.
  k=fread(s,1,277,dx_file);
  if(k<277 || s[0]==92)goto emedta_x;
  j=0;
  while(j<11 && s[j]!=' ' && s[j]!='(' && s[j]<'a')j++;
  for(i=0; i<j; i++)dx[dx_no].call[i]=s[i];
  for(i=j; i<CALLSIGN_CHARS; i++)dx[dx_no].call[i]=' ';  
  sscanf(&s[166],"%f",&dx[dx_no].lat);
  i=167;
#ifdef __NetBSD__
  while(i<175 && toupper((int)(unsigned char)s[i]) != 'S' && toupper((int)(unsigned char)s[i]) != 'N')i++;
#else
  while(i<175 && toupper(s[i]) != 'S' && toupper(s[i]) != 'N')i++;
#endif  
  if(i==175)
    {
    dx[dx_no].lat=1000;
    dx[dx_no].lon=1000;
    }
  else
    {

#ifdef __NetBSD__
    if(toupper((int)(unsigned char)s[i]) == 'S')
#else
    if(toupper(s[i]) == 'S')
#endif  
      {
      dx[dx_no].lat=-dx[dx_no].lat;
      }
    i++;  
    sscanf(&s[i],"%f",&dx[dx_no].lon);

#ifdef __NetBSD__
    while(i<181 && toupper((int)(unsigned char)s[i]) != 'E' && toupper((int)(unsigned char)s[i]) != 'W')i++;
#else
    while(i<181 && toupper(s[i]) != 'E' && toupper(s[i]) != 'W')i++;
#endif  
    if(i==181)
      {
      dx[dx_no].lat=1000;
      dx[dx_no].lon=1000;
      }
    else
      {
#ifdef __NetBSD__
      if(toupper((int)(unsigned char)s[i]) == 'E')
#else
      if(toupper(s[i]) == 'E')
#endif  
        {
        dx[dx_no].lon=-dx[dx_no].lon;
        }
      }
    }
  check_latest_dx();
  dx_no++;  
  if(dx_no>=MAX_DXCALLS)
    {
    lirerr(1123);
    return;
    }
  goto emedta_a;
emedta_x:;
  fclose(dx_file);
  }
dx_file = fopen(eme_dirskd_filename, "r");
if(dx_file != NULL)
  {
  k=fread(s,1,1,dx_file);
dirskd_a:;
  j=0;
  while(k!=0 && s[j] != ',')
    {
    j++;
    k=fread(&s[j],1,1,dx_file);
    if(j >= CALLSIGN_CHARS)
      {
      lirerr(1124);
      return;
      }
    }
  if(j == 0)goto dirskd_x;
  for(i=0; i<j; i++)dx[dx_no].call[i]=s[i];
  for(i=j; i<CALLSIGN_CHARS; i++)dx[dx_no].call[i]=' ';  
  k=fread(s,1,1,dx_file);
  j=0;
  while(k!=0 && s[j] != ',')
    {
    j++;
    k=fread(&s[j],1,1,dx_file);
    if(j >= 10)
      {
      lirerr(1124);
      return;
      }
    }
  sscanf(s,"%f",&dx[dx_no].lat);
#ifdef __NetBSD__
  while(j > 0 && toupper((int)(unsigned char)s[j])!='S' && toupper((int)(unsigned char)s[j])!='N')j--;
  if(toupper((int)(unsigned char)s[j]) == 'S')
#else
  while(j > 0 && toupper(s[j])!='S' && toupper(s[j])!='N')j--;
  if(toupper(s[j]) == 'S')
#endif  
    {
    dx[dx_no].lat=-dx[dx_no].lat;
    }
  k=fread(s,1,1,dx_file);
  j=0;
  while(k!=0 && s[j] != ',')
    {
    j++;
    k=fread(&s[j],1,1,dx_file);
    if(j >= 10)
      {
      lirerr(1124);
      return;
      }
    }
  if(k == 0)goto dirskd_x;
  sscanf(s,"%f",&dx[dx_no].lon);
#ifdef __NetBSD__
  while(j > 0 && toupper((int)(unsigned char)s[j])!='E' && toupper((int)(unsigned char)s[j])!='W')j--;
  if(toupper((int)(unsigned char)s[j]) == 'E')
#else
  while(j > 0 && toupper(s[j])!='E' && toupper(s[j])!='W')j--;
  if(toupper(s[j]) == 'E')
#endif  
    {
    dx[dx_no].lon=-dx[dx_no].lon;
    }
  if(dx[dx_no].lon == 0 && dx[dx_no].lat == 0)
    {
    dx[dx_no].lat=1000;
    dx[dx_no].lon=1000;
    }
  check_latest_dx();
  dx_no++;  
  if(dx_no>=MAX_DXCALLS)
    {
    lirerr(1123);
    return;
    }
  while(k!=0 && s[0] != 10 && s[0] != 13)
    {
    k=fread(s,1,1,dx_file);
    }
  while(k!=0 && (s[0] == 10 || s[0] == 13))
    {
    k=fread(s,1,1,dx_file);
    }
  if(k > 0)goto dirskd_a;
dirskd_x:;
  fclose(dx_file);
  }
dx_file = fopen(eme_call3_filename, "r");
if(dx_file != NULL)
  {
  k=fread(s,1,1,dx_file);
  while(k!=0 && s[0]=='/')
    {
    while(k!=0 && s[0] != 10 && s[0] != 13)
      {
      k=fread(s,1,1,dx_file);
      }
    while(k!=0 && (s[0] == 10 || s[0] == 13))
      {
      k=fread(s,1,1,dx_file);
      }
    }
  if(k==0)goto call3_x;
call3_a:;
  j=0;
  while(k!=0 && s[j] != ',')
    {
    j++;
    k=fread(&s[j],1,1,dx_file);
    if(j >= CALLSIGN_CHARS)
      {
      clear_screen();
      s[j+1]=0;
      lir_text(0,0,"This line is in error (field too long)");
      lir_text(0,2,s);
      lir_text(0,4,press_any_key);
      await_keyboard();
      lirerr(1230);
      return;
      }
    }
  if(j == 0)goto call3_x;
  if(j==6)
    {
    i=0;
    while(s[i]=='Z')i++;
    if(i == 6)goto call3_x;
    }
  for(i=0; i<j; i++)dx[dx_no].call[i]=s[i];
  for(i=j; i<CALLSIGN_CHARS; i++)dx[dx_no].call[i]=' ';  
  k=fread(s,1,1,dx_file);
  j=0;
  while(k!=0 && s[j] != ',')
    {
    j++;
    k=fread(&s[j],1,1,dx_file);
    if(j > 6)
      {
      clear_screen();
      dx[dx_no].call[CALLSIGN_CHARS-1]=0;
      s[j+1]=0;
      lir_text(0,0,dx[dx_no].call);
      lir_text(0,2,"field too long");
      lir_text(0,4,s);
      lir_text(0,6,press_any_key);
      await_keyboard();
      lirerr(1230);
      return;
      }
    }
  for(i=0; i<j; i++)tmp_locator[i]=s[i];
  for(i=j; i<6; i++)tmp_locator[i]=' ';
  locator_to_latlong();  
  dx[dx_no].lat=tmp_lat;
  dx[dx_no].lon=-tmp_lon;
// locator to lat/lon
//  sscanf(s,"%f",&dx[dx_no].lat);
  if(dx[dx_no].lon == 0 && dx[dx_no].lat == 0)
    {
    dx[dx_no].lat=1000;
    dx[dx_no].lon=1000;
    }
  check_latest_dx();
  dx_no++;  
  if(dx_no>=MAX_DXCALLS)
    {
    lirerr(1123);
    return;
    }
  while(k!=0 && s[0] != 10 && s[0] != 13)
    {
    k=fread(s,1,1,dx_file);
    }
  while(k!=0 && (s[0] == 10 || s[0] == 13))
    {
    k=fread(s,1,1,dx_file);
    }
  if(k > 0)goto call3_a;
call3_x:;
  fclose(dx_file);
  }
if(locerr_file != NULL)
  {
  fclose(locerr_file);
  } 
// Sort the dx data in alphabetical order.
if(dx_no >= 2)
  {
  m=dx_no-1;
  for(i=0; i<m; i++)
    {
    for(k=i+1; k<dx_no; k++)
      {
      for(j=0; j<CALLSIGN_CHARS; j++)
        {
        if(dx[i].call[j] > dx[k].call[j])goto swap;
        if(dx[i].call[j] < dx[k].call[j])goto noswap;
        }
      lirerr(1129);
      return;
swap:;
      for(j=0; j<CALLSIGN_CHARS; j++)
        {
        s[0]=dx[i].call[j];
        dx[i].call[j]=dx[k].call[j];
        dx[k].call[j]=s[0];
        }
      t1=dx[i].lat;
      dx[i].lat=dx[k].lat;  
      dx[k].lat=t1;  
      t1=dx[i].lon;
      dx[i].lon=dx[k].lon;  
      dx[k].lon=t1;  
noswap:;
      }
    }
  }    
// Store call sign and geographical position in linrad's own format:
dx_file = fopen(eme_dxdata_filename, "w");
if(dx_file == NULL)
  {
  lirerr(1128);
  return;
  }
else  
  {
  fprintf(dx_file,"%d\n",dx_no);
  for(i=0; i<dx_no; i++)
    {
    for(j=0; j<CALLSIGN_CHARS; j++)s[j]=dx[i].call[j];
    s[CALLSIGN_CHARS]=0;
    fprintf(dx_file,"%s  %.2f %.2f\n",s,dx[i].lat,dx[i].lon);
    }
  }
fprintf(dx_file,"%d\n",dx_no);
fclose(dx_file);
own_file = fopen(eme_own_info_filename, "w");
if(own_file == NULL)
  {
  lirerr(1121);
  return;
  }
for(i=0; i<2; i++)
  {
  fprintf(own_file,"%s [%d]\n",emeparm_text[i],emeparm_int[i]);
  }
for(i=2; i<MAX_EMEPARM; i++)
  {
  fprintf(own_file,"%s [%f]\n",emeparm_text[i],emeparm_float[i]);
  }
fclose(own_file);
free(dx);
dx=NULL;
}


double to_first_period(double x)
{
return (x-(int)(x))*2*PI_L;
}


void calculate_moon_data()
{
char s[80];
int k, day;
float dec,dec1,ra,pol,zd,sinref;
float sdtim,gha,uha;
float uhadx;
float elsin,elcos,el,azsin,azcos,az;
float elsindx,elcosdx,eldx,azsindx,azcosdx,azdx,poldx;
double dt1, timofday, a, tottim, f1, f2, f3, f4, dlon, dlat;
double f5,f6,f7;
if(suggested_calls_counter > 1 || eg_move_flag !=0 || eme_flag != 2)return;
// lir_get_epoch_seconds is the number of seconds since 1970
dt1=lir_get_epoch_seconds();
tottim=(dt1-946731602)/(24*3600);
dt1+=3600*emeparm_int[1];
day=dt1/(24*3600)+7305;
timofday=dt1/(24*3600);
timofday-=(int)(timofday);
a=.0657098232*day;
sdtim=6.67170278+(a-24*(int)(a/24))+1.0027379093*timofday*24;
f1=to_first_period(.374897+.03629164709*tottim);
f2=to_first_period(.259091+.0367481952*tottim);
f3=to_first_period(.827362+.03386319198*tottim);
f4=to_first_period(.347343-.00014709391*tottim);
f5=to_first_period(.779072+.00273790931*tottim);
f6=to_first_period(.993126+.0027377785*tottim);
f7=to_first_period(.505498+.00445046867*tottim);
dlon=22640*sin(f1)
    -4586*sin(f1-2*f3)
    +2370*sin(2*f3)
    +769*sin(2*f1)
    -668*sin(f6)
    -412*sin(2*f2)
    -212*sin(2*f1-2*f3)
    -206*sin(f1-2*f3+f6)
    +192*sin(f1+2*f3)
    +165*sin(2*f3-f6)+
    148*sin(f1-f6)
    -125*sin(f3)
    -110*sin(f1+f6)
    -55*sin(2*f2-2*f3)
    -45*sin(f1+2*f2)
    +40*sin(f1-2*f2)
    -38*sin(f1-4*f3)
    +36*sin(3*f1)
    -31*sin(2*f1-4*f3)
    +28*sin(f1-2*f3-f6)
    -24*sin(2*f3+f6)
    +19*sin(f1-f3)
    +18*sin(f3+f6)
    +15*sin(f1+2*f3-f6)
    +14*(sin(2*f1+2*f3)+sin(4*f3))
    -13*sin(3*f1-2*f3)
    -11*sin(f1+16*f5-18*f7)
    +10*sin(2*f1-f6)
    +9*(sin(f1-2*f2-2*f3)+cos(f1+16*f5-18*f7)-sin(2*f1-2*f3+f6))
    +8*(sin(2*f3-2*f6)-sin(f1+f3)-sin(2*f1+f6))
    +7*(sin(f4)-sin(2*f6)-sin(f1-2*f3+2*f6))
    -6*(sin(f1-2*f2+2*f3)+sin(2*f2+2*f3))
    +4*(tottim/36525+1)*(cos(f1+16*f5-18*f7)+sin(f1+16*f5-18*f7))
    -4*(sin(f1-4*f3+f6)+sin(2*f1+2*f2))
    +3*(sin(f1-3*f3)+sin(f1-2*f6)+sin(f1-2*f3-2*f6)-
                                     sin(f1+2*f3+f6)-sin(2*f1-4*f3+f6))
    +2*(sin(f1+4*f3)+sin(4*f1)+sin(4*f3-f6)+sin(2*f1-f3)-
                                     sin(2*f1-2*f3-f6)-sin(2*f2-2*f3+f6));
dlon=2*PI_L*(dlon/1296000+.606434+.03660110129*tottim);
dlat=  18461*sin(f2)
      +1010*sin(f1+f2)
      +1000*sin(f1-f2)
      -624*sin(f2-2*f3)
      -199*sin(f1-f2-2*f3)
      -167*sin(f1+f2-2*f3)
      +117*sin(f2+2*f3)
      +62*sin(2*f1+f2)
      +33*sin(f1-f2+2*f3)
      +32*sin(2*f1-f2)
      -30*sin(f2-2*f3+f6)
      -16*sin(2*f1+f2-2*f3)
      +15*sin(f1+f2+2*f3)
      +12*sin(f2-2*f3-f6)
      -9*sin(f1-f2-2*f3+f6)
      +8*(sin(f2+2*f3-f6)-sin(f2+f4))
      +7*(sin(f1+f2-f6)-sin(f1+f2-2*f3+f6)-sin(f1+f2-4*f3))
      +6*(sin(f1-f2-f6)-sin(f2+f6)-sin(3*f2))
      +5*(sin(f2-f6)+sin( f2-f3)-sin(f2+f3)-sin(f1+f2+f6)-sin(f1-f2+f6))
      +4*(sin(3*f1+f2)-sin(f2-4*f3))
      +3*(sin(f1-3*f2)-sin(f1-f2-4*f3))
      +2*(sin(2*f1-f2+2*f3)+sin(f1-f2+2*f3-f6)+sin(2*f1-f2-2*f3)+
                            sin(3*f1-f2)-sin(2*f1-f2-4*f3)-sin(3*f2-2*f3));
dlat=2*PI_L*dlat/1296000;
dec1=cos(dlat)*sin(dlon)*.397821+sin(dlat)*.917463;
dec=asin(dec1);
ra=atan2(cos(dlat)*sin(dlon)*.917463-sin(dlat)*.397821, cos(dlat)*cos(dlon));
if(ra < 0)ra+=2*PI_L;
gha=2*PI_L*(sdtim+.00029*sin(0.211417-0.00092422*(day+timofday)))/24-ra;
uha=-RAD*emeparm_float[3]-gha;
elsin=cos_laref*cos(uha)*cos(dec)+dec1*sin_laref;
elcos=sqrt(1-elsin*elsin);
el=atan2(elsin,elcos);
azcos=dec1/(cos_laref*elcos)-(sin_laref/cos_laref)*(elsin/elcos);
azsin=sin_laref*dec1+cos_laref*cos(dec)*cos(uha);
azsin=sin(uha)*cos(dec)/sqrt(1-azsin*azsin);
az=atan2(azsin,azcos);
if(az < 0)az+=2*PI_L;
el-=atan2(cos(el),60.4-sin(el));
moon_az=az/RAD;
moon_el=el/RAD;
sprintf(s,"az=%.1f el=%.1f      ",az/RAD,el/RAD);
s[EGH-1]=0;
lir_pixwrite(eg.xleft+text_width/2,eg.ytop+2,s);
if(dxflag != 0)
  {
  uhadx=-RAD*dx_lon-gha;
  elsindx=cos_dxlat*cos(uhadx)*cos(dec)+dec1*sin_dxlat;
  elcosdx=sqrt(1-elsindx*elsindx);
  eldx=atan2(elsindx,elcosdx);
  azcosdx=dec1/(cos_dxlat*elcosdx)-(sin_dxlat/cos_dxlat)*(elsindx/elcosdx);
  azsindx=sin_dxlat*dec1+cos_dxlat*cos(dec)*cos(uhadx);
  azsindx=sin(uhadx)*cos(dec)/sqrt(1-azsindx*azsindx);
  azdx=atan2(azsindx,azcosdx);
  if(azdx < 0)azdx+=2*PI_L;
  eldx-=atan2(cos(eldx),60.4-sin(eldx));
  if(eldx > 0 && eldx < .27925)
    {
    zd=PI_L/2-eldx;
    sinref=.9986047*sin(.9967614*zd);
    sinref=atan2(sinref,sqrt(1-sinref*sinref));
    eldx-=0.00104329*(196.5411*(zd-sinref)-.6393802*zd);
    }
  settextcolor(10);
  sprintf(s,"az=%.1f el=%.1f      ",azdx/RAD,eldx/RAD);
  s[EGH-1]=0;
  lir_pixwrite(eg.xleft+text_width/2,eg.ytop+1+text_height,s);
  settextcolor(7);
  if(fabs(cos_laref*azsin) <  0.0001*
                           fabs(sin_laref*cos(el)-cos_laref*cos(az)*sin(el)))
    {
    pol=PI_L/2;
    }
  else
    {          
    pol=(sin_laref*cos(el)-cos_laref*cos(az)*sin(el))/(cos_laref*azsin);
    pol=atan(pol);
    }
  if(fabs(cos_dxlat*azsindx) < 0.0001*
                     fabs(sin_dxlat*cos(eldx)-cos_dxlat*cos(azdx)*sin(eldx)))
    {
    poldx=PI_L/2;
    }
  else
    {
    poldx=(sin_dxlat*cos(eldx)-cos_dxlat*cos(azdx)*sin(eldx))/
                                                      (cos_dxlat*azsindx);
    poldx=atan(poldx);
    }
  k=2*(poldx-pol)/RAD+pg_pol_angle;
  while(k<0)k+=180;
  while(k>180)k-=180;
  sprintf(s,"Tx pol %d   ",k);
  lir_pixwrite(eg.xleft+1.5*text_width,egbutt[EG_DX].y2+text_height/2,s);
  }
users_eme();
}
