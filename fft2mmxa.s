; Copyright (c) <2012> <Leif Asbrink>
;
; Permission is hereby granted, free of charge, to any person 
; obtaining a copy of this software and associated documentation 
; files (the "Software"), to deal in the Software without restriction, 
; including without limitation the rights to use, copy, modify, 
; merge, publish, distribute, sublicense, and/or sell copies of 
; the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be 
; included in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
; HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
; WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
; OR OTHER DEALINGS IN THE SOFTWARE.
section .note.GNU-stack

extern fft2_short_int
extern fft2_size
extern fft2_bigpermute
extern fft2_na
extern timf2_shi
extern timf2_mask
extern timf2_px
extern fft2_mmxwin
extern mailbox
global fft2mmx_a2_nowin
global fft2mmx_a1_nowin
global fft2mmx_a2_win
global fft2mmx_a1_win

%define NEGODD_XOR ebp-8
%define NEGODD_INC ebp-16
%define NEGEVEN_XOR ebp-24
%define NEGEVEN_INC ebp-32
%define NEGMID_XOR ebp-40
%define NEGMID_INC ebp-48
%define MASK ebp-52
%define P0 ebp-56
%define LIMIT ebp-60
section .text


fft2mmx_a1_win
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp
  mov     ebp,[fft2_bigpermute]
  mov     eax,[timf2_mask]
  sal     eax,1
  mov     [MASK],eax
  mov     eax,[timf2_px]
  sal     eax,1
  mov     [P0],eax
  mov     eax,[fft2_size]
  sal     eax,1
  mov     [LIMIT],eax
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
  sal     eax,1
  imul    eax,[fft2_na]
  mov     edx,[fft2_short_int]
  add     edx,eax
  mov     ebx,[timf2_shi]
; The two first trivial loops are combined and
; run together with the unscrambling process
  xor     eax,eax
  mov     ecx,0ffffffffh
  mov     [edx+4],eax
  mov     [edx],ecx
  movq    mm7,[edx]
  movq    mm6,[edx]
  psllq   mm6,32
  mov     ecx,[fft2_mmxwin]

win_one:
;for(j=0; j<siz; j+=4)
;  {
;ia=(p0+fft2_bigpermute[j  ])&mask;
;ib=(p0+fft2_bigpermute[j+1])&mask;
  movq    mm5,[ecx+eax]         ;mm5 = win4  win3  win2  win1
  psrlq   mm5,16                ;mm5 =  0    win4  win3  win2
  movq    mm4,[ecx+eax]
  punpcklwd  mm4,mm4            ;mm4 =  .      .   win1  win1
  movq    mm3,mm5               ;mm3 =  0    win4  win3  win2
  punpcklwd  mm3,mm3            ;mm3 =  .      .   win2  win2
  punpckldq  mm4,mm3            ;mm4 = win2  win2  win1  win1

  mov     esi,[2*eax+ebp]     ;esi=fft2_bigpermute[j]
  mov     edi,[2*eax+ebp+4]   ;edi=fft2_bigpermute[j+1]
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  a1=fft2_window[ja]*(timf2_shi[ia  ]+timf2_shi[ia+2]);
;  a2=fft2_window[ja]*(timf2_shi[ia+1]+timf2_shi[ia+3]);
;  b1=fft2_window[jb]*(timf2_shi[ib  ]+timf2_shi[ib+2]);
;  b2=fft2_window[jb]*(timf2_shi[ib+1]+timf2_shi[ib+3]);
  movq    mm0,[esi+ebx]     ;mm0=   ia+3  ia+2  ia+1  ia+0 
  movq    mm1,[esi+ebx]     ;mm1=   ia+3  ia+2  ia+1  ia+0 
  movq    mm2,[edi+ebx]     ;mm2=   ib+3  ib+2  ib+1  ib+0
  movq    mm3,[edi+ebx]     ;mm3=   ib+3  ib+2  ib+1  ib+0
  psrlq   mm1,32            ;mm1=     0     0   ia+3  ia+2
  psllq   mm3,32            ;mm3=   ib+1  ib+0   0     0
  pand    mm0,mm7           ;mm0=     0     0   ia+1  ia+0

  pand    mm2,mm6           ;mm2=   ib+3  ib+2   0     0
  paddsw  mm0,mm1              
  paddsw  mm0,mm2
  paddsw  mm0,mm3           ;mm0= sums
  movq    mm1,mm4
  pmulhw  mm4,mm0
  paddsw  mm4,mm4
  pmullw  mm0,mm1
  psraw   mm0,13
  paddsw  mm0,mm4           ;mm0=   b2    b1    a2    a1 

  psrlq   mm5,16            ;mm5 =  0     0    win4  win3
  movq    mm4,mm5
  psrlq   mm4,16            ;mm4 =  0     0     0    win4   
  punpcklwd  mm5,mm5        ;mm5 =  .      .   win3  win3
  punpcklwd  mm4,mm4        ;mm3 =  .      .   win4  win4
  punpckldq  mm5,mm4        ;mm5 = win4  win4  win3  win3
  mov     esi,[2*eax+ebp+8]     ;esi=fft2_bigpermute[j+2]
  mov     edi,[2*eax+ebp+12]    ;edi=fft2_bigpermute[j+3]
;ic=(p0+fft2_bigpermute[j+2])&mask;
;id=(p0+fft2_bigpermute[j+3])&mask;
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  c1=fft2_window[jc]*(timf2_shi[ic  ]+timf2_shi[ic+2]);
;  c2=fft2_window[jc]*(timf2_shi[ic+1]+timf2_shi[ic+3]);
;  d1=fft2_window[jd]*(timf2_shi[id  ]+timf2_shi[id+2]);
;  d2=fft2_window[jd]*(timf2_shi[id+1]+timf2_shi[id+3]);
  movq    mm1,[esi+ebx]     ;mm1=   ic+3  ic+2  ic+1  ic+0 
  movq    mm2,[esi+ebx]     ;mm2=   ic+3  ic+2  ic+1  ic+0 
  movq    mm3,[edi+ebx]     ;mm3=   id+3  id+2  id+1  id+0
  movq    mm4,[edi+ebx]     ;mm4=   id+3  id+2  id+1  id+0
  psrlq   mm2,32            ;mm2=     0     0   ic+3  ic+2
  psllq   mm4,32            ;mm4=   id+1  id+0   0     0
  pand    mm1,mm7           ;mm1=     0     0   ic+1  ic+0
  pand    mm3,mm6           ;mm3=   id+3  id+2   0     0
  paddsw  mm1,mm2
  paddsw  mm1,mm3
  paddsw  mm1,mm4           ;mm1= sums
  movq    mm2,mm5
  pmulhw  mm5,mm1
  paddsw  mm5,mm5
  pmullw  mm1,mm2
  psraw   mm1,13
  paddsw  mm1,mm5           ;mm1=     d2    d1    c2    c1 
;  t1=a1+b1;
;  t2=a2+b2;
;  t5=-(a1-b1);
;  t7=-(a2-b2);
                            ;mm0=     b2    b1    a2    a1 
  movq    mm2,mm0
  movq    mm3,mm0
  psrlq   mm2,32            ;mm2=      0     0    b2    b1
  psllq   mm3,32            ;mm3=      a2   a1     0     0 
  paddsw  mm0,mm2           ;mm0=      b2   b1   a2+b2 a1+b1 
  psubsw  mm0,mm3           ;mm0=      t7   t5    t2    t1
;  t3=c1+d1;
;  t4=c2+d2;
;  t10=-(c1-d1);
;  t6=-( c2-d2);
  movq    mm2,mm1
  movq    mm3,mm1
  psrlq   mm2,32            ;mm2=      0     0    d2    d1
  psllq   mm3,32            ;mm3=      c2   c1     0     0 
  paddsw  mm1,mm2           ;mm1=      d2   d1   c2+d2 c1+d1 
  psubsw  mm1,mm3           ;mm1=      t6   t10    t4    t3
  movq    mm4,mm1
  movq    mm3,mm1
  pxor    mm0,[NEGMID_XOR]
  paddsw  mm0,[NEGMID_INC]  ;mm0=      t7   -t5   -t2   t1
                            ;mm1=      t6   t10    t4    t3
  movq    mm2,mm1
  movq    mm3,mm1
  psrlq   mm2,16            ;mm2=       0   t6     .     .
  pslld   mm3,16            ;mm3=      t10   0     .     .
  paddsw  mm2,mm3
  pxor    mm1,[NEGODD_XOR]
  paddsw  mm1,[NEGODD_INC]  ;mm1=       .    .    -t4    t3
  pand    mm1,mm7
  pand    mm2,mm6
  paddsw  mm1,mm2
  movq    mm3,mm1
;  x1= t1+t3
;  x2=-t2-t4
;  x3=-t5+t6
;  x4= t7+t10
;  y1= t1-t3
;  y2=-t2+t4
;  y3=-t5-t6
;  y4= t7-t10
  paddsw  mm1,mm0           ;mm1=     t10   t6    -t4    t3
  movq    [edx+2*eax],mm1
  psubsw  mm0,mm3           
  movq    [edx+2*eax+8],mm0
  add     eax,8
  cmp     eax,[LIMIT]
  jl near win_one
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret

fft2mmx_a1_nowin
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp
  mov     ebp,[fft2_bigpermute]
  mov     eax,[timf2_mask]
  sal     eax,1
  mov     [MASK],eax
  mov     eax,[timf2_px]
  sal     eax,1
  mov     [P0],eax
  mov     eax,[fft2_size]
  sal     eax,2
  mov     [LIMIT],eax
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
  imul    eax,[fft2_na]
  mov     edx,[fft2_short_int]
  add     edx,eax
  mov     ebx,[timf2_shi]
; The two first trivial loops are combined and
; run together with the unscrambling
  xor     eax,eax
  mov     ecx,0ffffffffh
  mov     [edx+4],eax
  mov     [edx],ecx
  movq    mm7,[edx]
  movq    mm6,[edx]
  psllq   mm6,32
nowin_one:
;for(j=0; j<siz; j+=4)
;  {
;ia=(p0+fft2_bigpermute[j  ])&mask;
;ib=(p0+fft2_bigpermute[j+1])&mask;
  mov     esi,[eax+ebp]     ;esi=fft2_bigpermute[j]
  mov     edi,[eax+ebp+4]   ;edi=fft2_bigpermute[j+1]
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  a1=timf2_shi[ia  ]+timf2_shi[ia+2];
;  a2=timf2_shi[ia+1]+timf2_shi[ia+3];

;  b1=timf2_shi[ib  ]+timf2_shi[ib+2];
;  b2=timf2_shi[ib+1]+timf2_shi[ib+3];

;  t1=a1+b1;
;  t2=a2+b2;
;  t5=a1-b1;
;  t7=a2-b2;

  movq    mm0,[esi+ebx]     ;mm0=   ia+3  ia+2  ia+1  ia+0 
  movq    mm1,[esi+ebx]     ;mm1=   ia+3  ia+2  ia+1  ia+0 
  movq    mm2,[edi+ebx]     ;mm2=   ib+3  ib+2  ib+1  ib+0
  movq    mm3,[edi+ebx]     ;mm3=   ib+3  ib+2  ib+1  ib+0
  psrlq   mm1,32            ;mm1=     0     0   ia+3  ia+2
  psrlq   mm3,32            ;mm3=     0     0   ib+3  ib+2
  paddsw  mm0,mm1           ;mm0=     ?     ?    a2    a1
  pand    mm0,mm7           ;mm0=     0     0    a2    a1
  paddsw  mm2,mm3           ;mm2=     ?     ?    b2    b1 
  pand    mm2,mm7           ;mm2=     0     0    b2    b1 
  movq    mm1,mm0           ;mm1=     0     0    a2    a1
  psllq   mm1,32            ;mm1=    a2    a1     0    0
  paddsw  mm0,mm1           ;mm0=    a2    a1    a2    a1
  paddsw  mm0,mm2           ;mm0=    a2    a1   a2+b2 a1+b1
  psllq   mm2,32            ;mm2=    b2    b1     0    0
  psubsw  mm0,mm2           ;mm0=   a2-b2 a1-b1 a2+b2 a1+b1
                            ;mm0=    t7    t5    t2   t1 
  mov     esi,[eax+ebp+8]     ;esi=fft2_bigpermute[j+2]
  mov     edi,[eax+ebp+12]    ;edi=fft2_bigpermute[j+3]
;ic=(p0+fft2_bigpermute[j+2])&mask;
;id=(p0+fft2_bigpermute[j+3])&mask;
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
  movq    mm1,[esi+ebx]     ;mm1=   ic+3  ic+2  ic+1  ic+0 
  movq    mm2,[esi+ebx]     ;mm2=   ic+3  ic+2  ic+1  ic+0 
  movq    mm3,[edi+ebx]     ;mm3=   id+3  id+2  id+1  id+0
  movq    mm4,[edi+ebx]     ;mm4=   id+3  id+2  id+1  id+0
  psrlq   mm2,32            ;mm2=     0     0   ic+3  ic+2
  psrlq   mm4,32            ;mm4=     0     0   id+3  id+2
  paddsw  mm1,mm2           ;mm1=     ?     ?    c2    c1
  pand    mm1,mm7           ;mm1=     0     0    c2    c1
  paddsw  mm3,mm4           ;mm3=     ?     ?    d2    d1 
  pand    mm3,mm7           ;mm3=     0     0    d2    d1 
  movq    mm2,mm1           ;mm2=     0     0    c2    c1
  psllq   mm2,32            ;mm2=    c2    c1     0    0
  paddsw  mm1,mm2           ;mm1=    c2    c1    c2    c1
  paddsw  mm1,mm3           ;mm1=    c2    c1   c2+d2 c1+d1
  psllq   mm3,32            ;mm3=    d2    d1     0    0
  psubsw  mm1,mm3           ;mm1=   c2-d2 c1-d1 c2+d2 c1+d1
                            ;mm1=    t6    t10   t4   t3 
  movq    mm2,mm1           ;mm1=    t6    t10   t4   t3 
  psrlq   mm2,16            ;mm2=     0    t6     ?    ?
  movq    mm3,mm1           ;mm1=    t6    t10   t4   t3 
  psllq   mm3,16            ;mm3=   t10    ?     ?     ?
  pand    mm3,[NEGODD_XOR]  ;mm3=   t10    0     ?     0
  paddsw  mm2,mm3           ;mm2=   t10    t6    ?     ? 
  pand    mm2,mm6           ;mm2=    t10    t6    0     0
  pxor    mm1,[NEGEVEN_XOR]
  paddsw  mm1,[NEGEVEN_INC]
  pand    mm1,mm7           ;mm1=     0     0    t4   t3 
  paddsw  mm1,mm2           ;mm1=    t10    t6   t4   t3 
  pxor    mm0,[NEGODD_XOR]
  paddsw  mm0,[NEGODD_INC]
  movq    mm2,mm0
  psubsw  mm0,mm1
  paddsw  mm2,mm1
  movq    [edx+eax],mm0
  movq    [edx+eax+8],mm2
  add     eax,16
  cmp     eax,[LIMIT]
  jl near nowin_one
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret




fft2mmx_a2_nowin
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp
  mov     ebp,[fft2_bigpermute]
  mov     eax,[timf2_mask]
  sal     eax,1
  mov     [MASK],eax
  mov     eax,[timf2_px]
  sal     eax,1
  mov     [P0],eax
  mov     eax,[fft2_size]
  sal     eax,2
  mov     [LIMIT],eax
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
  sal     eax,1
  imul    eax,[fft2_na]
  mov     edx,[fft2_short_int]
  add     edx,eax
  mov     ebx,[timf2_shi]
; The two first trivial loops are combined and
; run together with the unscrambling
  xor     eax,eax
nowin_two:
;for(j=0; j<siz; j+=4)
;  {
;ia=(p0+fft2_bigpermute[j  ])&mask;
;ib=(p0+fft2_bigpermute[j+1])&mask;
  mov     esi,[eax+ebp]     ;esi=fft2_bigpermute[j]
  mov     edi,[eax+ebp+4]   ;edi=fft2_bigpermute[j+1]
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  t1=timf2_shi[ia  ]+timf2_shi[ib  ]+
;     timf2_shi[ia+4]+timf2_shi[ib+4];
;  t2=timf2_shi[ia+1]+timf2_shi[ib+1]+
;     timf2_shi[ia+5]+timf2_shi[ib+5];
;  r1=timf2_shi[ia+2]+timf2_shi[ib+2]+
;     timf2_shi[ia+6]+timf2_shi[ib+6];
;  r2=timf2_shi[ia+3]+timf2_shi[ib+3]+
;     timf2_shi[ia+7]+timf2_shi[ib+7];
;  t5=timf2_shi[ia  ]-timf2_shi[ib  ]+
;     timf2_shi[ia+4]-timf2_shi[ib+4];
;  t7=timf2_shi[ia+1]-timf2_shi[ib+1]+
;     timf2_shi[ia+5]-timf2_shi[ib+5];
;  r5=timf2_shi[ia+2]-timf2_shi[ib+2]+
;     timf2_shi[ia+6]-timf2_shi[ib+6];
;  r7=timf2_shi[ia+3]-timf2_shi[ib+3]+
;      timf2_shi[ia+7]-timf2_shi[ib+7];
  movq    mm0,[esi+ebx]
  paddsw  mm0,[esi+ebx+8]
  movq    mm1,[edi+ebx]
  paddsw  mm1,[edi+ebx+8]
  mov     esi,[eax+ebp+8]     ;esi=fft2_bigpermute[j+2]
  mov     edi,[eax+ebp+12]    ;edi=fft2_bigpermute[j+3]
  movq    mm2,mm0
  paddsw  mm0,mm1            ;mm0=  r2    r1    t2    t1  
  psubsw  mm2,mm1            ;mm2=  r7    r5    t7    t5
  movq    mm5,mm0
;ic=(p0+fft2_bigpermute[j+2])&mask;
;id=(p0+fft2_bigpermute[j+3])&mask;
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  t3=timf2_shi[ic  ]+timf2_shi[id  ]+
;     timf2_shi[ic+4]+timf2_shi[id+4];
;  t4=timf2_shi[ic+1]+timf2_shi[id+1]+
;     timf2_shi[ic+5]+timf2_shi[id+5];
;  r3=timf2_shi[ic+2]+timf2_shi[id+2]+
;     timf2_shi[ic+6]+timf2_shi[id+6];
;  r4=timf2_shi[ic+3]+timf2_shi[id+3]+
;     timf2_shi[ic+7]+timf2_shi[id+7];
;  
;  
;  t10=timf2_shi[ic  ]-timf2_shi[id  ]+
;      timf2_shi[ic+4]-timf2_shi[id+4];
;  t6= timf2_shi[ic+1]-timf2_shi[id+1]+
;      timf2_shi[ic+5]-timf2_shi[id+5];
;  r10=timf2_shi[ic+2]-timf2_shi[id+2]+
;      timf2_shi[ic+6]-timf2_shi[id+6];  
;  r6= timf2_shi[ic+3]-timf2_shi[id+3]+
;      timf2_shi[ic+7]-timf2_shi[id+7];
  movq    mm3,[esi+ebx]
  paddsw  mm3,[esi+ebx+8]
  movq    mm1,[edi+ebx]
  paddsw  mm1,[edi+ebx+8]
  movq    mm4,mm3
  paddsw  mm3,mm1            ;mm3=  r4    r3    t4    t3  
  psubsw  mm4,mm1            ;mm4=  r6    r10   t6    t10
                             ;mm0=  r2    r1    t2    t1  
                             ;mm2=  r7    r5    t7    t5
                             ;mm5=  r2    r1    t2    t1  
;  a1=t1+t3
;  a2=t2+t4
;  a3=r1+r3
;  a4=r2+r4
;  b1=t1-t3
;  b2=t2-t4
;  b3=r1-r3
;  b4=r2-r4
;  a2^=0xffff
;  a2++
;  a4^=0xffff
;  a4++
;  b2^=0xffff
;  b2++
;  b4^=0xffff
;  b4++
  paddsw  mm0,mm3
  psubsw  mm5,mm3
  pxor    mm0,[NEGODD_XOR]
  pxor    mm5,[NEGODD_XOR]
  paddsw  mm0,[NEGODD_INC]
  paddsw  mm5,[NEGODD_INC]
;  zxy[4*j  ]=a1;
;  zxy[4*j+1]=a2;
;  zxy[4*j+2]=a3;
;  zxy[4*j+3]=a4;
;  zxy[4*j+8]=b1;
;  zxy[4*j+9]=b2;
;  zxy[4*j+10]=b3;
;  zxy[4*j+11]=b4;
  movq    mm1,mm4
  movq    [edx+2*eax],mm0
  movq    [edx+2*eax+16],mm5
                               ;mm1=  r6    r10   t6    t10
                               ;mm4=  r6    r10   t6    t10
                               ;mm2=  r7    r5    t7    t5
  psllq   mm1,16               ;mm1=  r10   t6    t10    0
  psrlq   mm4,16               ;mm4=   0    r6    r10   t6
  pand    mm1,[NEGODD_XOR]     ;mm1=  r10    0    t10    0
  pand    mm4,[NEGEVEN_XOR]    ;mm4=   0    r6     0    t6
  movq    mm5,mm2              ;mm5=  r7    r5    t7    t5
  paddsw  mm1,mm4              
;  c1=t5+t6
;  c2=t7+t10
;  c3=r5+r6
;  c4=r7+r10 
;  d1=t5-t6
;  d2=t7-t10
;  d3=r5-r6
;  d4=r7-r10
  paddsw  mm2,mm1
  psubsw  mm5,mm1
;  c2^=0xffff
;  c2++
;  c4^=0xffff
;  c4++
;  d2^=0xffff
;  d2++
;  d4^=0xffff
;  d4++
  pxor    mm2,[NEGODD_XOR]
  pxor    mm5,[NEGODD_XOR]
  paddsw  mm2,[NEGODD_INC]
  paddsw  mm5,[NEGODD_INC]
                                      ;mm2=  c4   c3   c2   c1
                                      ;mm5=  d4   d3   d2   d1
  movq    mm0,mm2                     ;mm0=  c4   c3   c2   c1
  pand    mm0,[NEGODD_XOR]            ;mm0=  c4    0   c2    0
  movq    mm1,mm5                     ;mm1=  d4   d3   d2   d1
  pand    mm1,[NEGODD_XOR]            ;mm1=  d4    0   d2    0
  pand    mm2,[NEGEVEN_XOR]           ;mm2=   0   c3    0   c1
  pand    mm5,[NEGEVEN_XOR]           ;mm5=   0   d3    0   d1
  paddsw  mm2,mm1
  paddsw  mm5,mm0
;  zxy[4*j+4]=d1;
;  zxy[4*j+5]=c2;
;  zxy[4*j+6]=d3;
;  zxy[4*j+7]=c4;
;  zxy[4*j+12]=c1;
;  zxy[4*j+13]=d2;
;  zxy[4*j+14]=c3;
;  zxy[4*j+15]=d4;
  movq    [edx+2*eax+8],mm5
  movq    [edx+2*eax+24],mm2
  add     eax,16
  cmp     eax,[LIMIT]
  jl near nowin_two
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret

fft2mmx_a2_win
  push    ebx
  push    ecx
  push    edx
  push    esi
  push    edi
  push    ebp
  mov     ebp,[fft2_bigpermute]
  mov     eax,[timf2_mask]
  sal     eax,1
  mov     [MASK],eax
  mov     eax,[timf2_px]
  sal     eax,1
  mov     [P0],eax
  mov     eax,[fft2_size]
  sal     eax,1
  mov     [LIMIT],eax
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
  sal     eax,2
  imul    eax,[fft2_na]
  mov     edx,[fft2_short_int]
  add     edx,eax
  mov     ebx,[timf2_shi]
  mov     ecx,[fft2_mmxwin]
; The two first trivial loops are combined and
; run together with the unscrambling
  xor     eax,eax
win_two:
;for(j=0; j<siz; j+=4)
;  {
  movq    mm7,[ecx+eax]         ;mm7 = win4  win3  win2  win1
  movq    mm6,[ecx+eax]
  punpcklwd  mm6,mm6            ;mm6 =  .      .   win1  win1
  punpckldq  mm6,mm6            ;mm6 = win1  win1  win1  win1
;ia=(p0+fft2_bigpermute[j  ])&mask;
;ib=(p0+fft2_bigpermute[j+1])&mask;
  mov     esi,[2*eax+ebp]     ;esi=fft2_bigpermute[j]
  mov     edi,[2*eax+ebp+4]   ;edi=fft2_bigpermute[j+1]
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  t1=timf2_shi[ia  ]+timf2_shi[ib  ]+
;     timf2_shi[ia+4]+timf2_shi[ib+4];
;  t2=timf2_shi[ia+1]+timf2_shi[ib+1]+
;     timf2_shi[ia+5]+timf2_shi[ib+5];
;  r1=timf2_shi[ia+2]+timf2_shi[ib+2]+
;     timf2_shi[ia+6]+timf2_shi[ib+6];
;  r2=timf2_shi[ia+3]+timf2_shi[ib+3]+
;     timf2_shi[ia+7]+timf2_shi[ib+7];
;  t5=timf2_shi[ia  ]-timf2_shi[ib  ]+
;     timf2_shi[ia+4]-timf2_shi[ib+4];
;  t7=timf2_shi[ia+1]-timf2_shi[ib+1]+
;     timf2_shi[ia+5]-timf2_shi[ib+5];
;  r5=timf2_shi[ia+2]-timf2_shi[ib+2]+
;     timf2_shi[ia+6]-timf2_shi[ib+6];
;  r7=timf2_shi[ia+3]-timf2_shi[ib+3]+
;      timf2_shi[ia+7]-timf2_shi[ib+7];
  movq    mm0,[esi+ebx]
  paddsw  mm0,[esi+ebx+8]
; Multiply by first window value in mm6
  movq    mm1,mm6
  pmulhw  mm1,mm0
  paddsw  mm1,mm1  ; shift by addition so we saturate properly!!
  paddsw  mm1,mm1
  pmullw  mm0,mm6
  psraw   mm0,14
  paddsw  mm0,mm1
  psrlq   mm7,16
  movq    mm6,mm7
  punpcklwd  mm6,mm6
  punpckldq  mm6,mm6            ;mm6 = win2
  movq    mm1,[edi+ebx]
  paddsw  mm1,[edi+ebx+8]
; multiply by second window value
  movq    mm2,mm6
  pmulhw  mm2,mm1
  paddsw  mm2,mm2  
  paddsw  mm2,mm2  
  pmullw  mm1,mm6
  psraw   mm1,14
  paddsw  mm1,mm2
  mov     esi,[2*eax+ebp+8]     ;esi=fft2_bigpermute[j+2]
  mov     edi,[2*eax+ebp+12]    ;edi=fft2_bigpermute[j+3]
  movq    mm2,mm0
  paddsw  mm0,mm1            ;mm0=  r2    r1    t2    t1  
  psubsw  mm2,mm1            ;mm2=  r7    r5    t7    t5
  movq    mm5,mm0            ;mm5=  r2    r1    t2    t1  
;ic=(p0+fft2_bigpermute[j+2])&mask;
;id=(p0+fft2_bigpermute[j+3])&mask;
  add     esi,[P0]
  add     edi,[P0]
  and     esi,[MASK]   
  and     edi,[MASK]
;  t3=timf2_shi[ic  ]+timf2_shi[id  ]+
;     timf2_shi[ic+4]+timf2_shi[id+4];
;  t4=timf2_shi[ic+1]+timf2_shi[id+1]+
;     timf2_shi[ic+5]+timf2_shi[id+5];
;  r3=timf2_shi[ic+2]+timf2_shi[id+2]+
;     timf2_shi[ic+6]+timf2_shi[id+6];
;  r4=timf2_shi[ic+3]+timf2_shi[id+3]+
;     timf2_shi[ic+7]+timf2_shi[id+7];
;  
;  
;  t10=timf2_shi[ic  ]-timf2_shi[id  ]+
;      timf2_shi[ic+4]-timf2_shi[id+4];
;  t6= timf2_shi[ic+1]-timf2_shi[id+1]+
;      timf2_shi[ic+5]-timf2_shi[id+5];
;  r10=timf2_shi[ic+2]-timf2_shi[id+2]+
;      timf2_shi[ic+6]-timf2_shi[id+6];  
;  r6= timf2_shi[ic+3]-timf2_shi[id+3]+
;      timf2_shi[ic+7]-timf2_shi[id+7];
  psrlq   mm7,16
  movq    mm6,mm7
  punpcklwd  mm6,mm6
  punpckldq  mm6,mm6            ;mm6 = win3
  movq    mm3,[esi+ebx]
  paddsw  mm3,[esi+ebx+8]
  movq    mm4,mm6
  pmulhw  mm4,mm3
  paddsw  mm4,mm4  
  paddsw  mm4,mm4  
  pmullw  mm3,mm6
  psraw   mm3,14
  paddsw  mm3,mm4

  psrlq   mm7,16
  punpcklwd  mm7,mm7
  punpckldq  mm7,mm7            ;mm7 = win4
  movq    mm1,[edi+ebx]
  paddsw  mm1,[edi+ebx+8]
  movq    mm6,mm7
  pmulhw  mm6,mm1
  paddsw  mm6,mm6
  paddsw  mm6,mm6
  pmullw  mm1,mm7
  psraw   mm1,14
  paddsw  mm1,mm6

  movq    mm4,mm3
  paddsw  mm3,mm1            ;mm3=  r4    r3    t4    t3  
  psubsw  mm4,mm1            ;mm4=  r6    r10   t6    t10
                             ;mm0=  r2    r1    t2    t1  
                             ;mm2=  r7    r5    t7    t5
                             ;mm5=  r2    r1    t2    t1  
;  a1=t1+t3
;  a2=t2+t4
;  a3=r1+r3
;  a4=r2+r4
;  b1=t1-t3
;  b2=t2-t4
;  b3=r1-r3
;  b4=r2-r4
;  a2^=0xffff
;  a2++
;  a4^=0xffff
;  a4++
;  b2^=0xffff
;  b2++
;  b4^=0xffff
;  b4++
  paddsw  mm0,mm3
  psubsw  mm5,mm3
  pxor    mm0,[NEGODD_XOR]
  pxor    mm5,[NEGODD_XOR]
  paddsw  mm0,[NEGODD_INC]
  paddsw  mm5,[NEGODD_INC]
;  zxy[4*j  ]=a1;
;  zxy[4*j+1]=a2;
;  zxy[4*j+2]=a3;
;  zxy[4*j+3]=a4;
;  zxy[4*j+8]=b1;
;  zxy[4*j+9]=b2;
;  zxy[4*j+10]=b3;
;  zxy[4*j+11]=b4;
  movq    mm1,mm4
  movq    [edx+4*eax],mm0
  movq    [edx+4*eax+16],mm5
                               ;mm1=  r6    r10   t6    t10
                               ;mm4=  r6    r10   t6    t10
                               ;mm2=  r7    r5    t7    t5
  psllq   mm1,16               ;mm1=  r10   t6    t10    0
  psrlq   mm4,16               ;mm4=   0    r6    r10   t6
  pand    mm1,[NEGODD_XOR]     ;mm1=  r10    0    t10    0
  pand    mm4,[NEGEVEN_XOR]    ;mm4=   0    r6     0    t6
  movq    mm5,mm2              ;mm5=  r7    r5    t7    t5
  paddsw  mm1,mm4              
;  c1=t5+t6
;  c2=t7+t10
;  c3=r5+r6
;  c4=r7+r10 
;  d1=t5-t6
;  d2=t7-t10
;  d3=r5-r6
;  d4=r7-r10
  paddsw  mm2,mm1
  psubsw  mm5,mm1
;  c2^=0xffff
;  c2++
;  c4^=0xffff
;  c4++
;  d2^=0xffff
;  d2++
;  d4^=0xffff
;  d4++
  pxor    mm2,[NEGODD_XOR]
  pxor    mm5,[NEGODD_XOR]
  paddsw  mm2,[NEGODD_INC]
  paddsw  mm5,[NEGODD_INC]
                                      ;mm2=  c4   c3   c2   c1
                                      ;mm5=  d4   d3   d2   d1
  movq    mm0,mm2                     ;mm0=  c4   c3   c2   c1
  pand    mm0,[NEGODD_XOR]            ;mm0=  c4    0   c2    0
  movq    mm1,mm5                     ;mm1=  d4   d3   d2   d1
  pand    mm1,[NEGODD_XOR]            ;mm1=  d4    0   d2    0
  pand    mm2,[NEGEVEN_XOR]           ;mm2=   0   c3    0   c1
  pand    mm5,[NEGEVEN_XOR]           ;mm5=   0   d3    0   d1
  paddsw  mm2,mm1
  paddsw  mm5,mm0
;  zxy[4*j+4]=d1;
;  zxy[4*j+5]=c2;
;  zxy[4*j+6]=d3;
;  zxy[4*j+7]=c4;
;  zxy[4*j+12]=c1;
;  zxy[4*j+13]=d2;
;  zxy[4*j+14]=c3;
;  zxy[4*j+15]=d4;
  movq    [edx+4*eax+8],mm5
  movq    [edx+4*eax+24],mm2
  add     eax,8
  cmp     eax,[LIMIT]
  jl near win_two
zz:
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret


