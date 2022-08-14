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



extern _fft1_split_shi
extern _fft1_first_point
extern _fft1_last_point
extern _fft1_float
extern _fft1_size
extern _liminfo
extern _fft1_px
extern _fft1_block
extern _fft1_mask
extern _fft1_back_scramble
extern _fft1_mmxcosin
extern _fft1_n
extern _fft1_backbuffer
extern _timf2_shi
extern _timf2_pa
extern _timf2_input_block
extern _timf2_mask
extern _timf2_pwr_int
extern _fft1_interleave_points
extern _fft1_inverted_mmxwin
extern _rx_channels
extern _fft1back_lowgain_n
extern _fft1_lowlevel_points

global _split_two
global _split_one
global _mmx_fft1back_two
global _mmx_fft1back_one
global _fft1back_mmx_finish

section .bss
mem1 resd 1
mem2 resd 1
mem3 resd 1
mem5 resd 1

%define INCC ebp-4
%define SIZ2 ebp-8
%define M2 ebp-12
%define J ebp-16
%define N ebp-20
%define M1 ebp-24
%define SIZ8 ebp-28
section .text

; Construct timf2_shi from back transforms in fft1_backbuffer
; Calculate timf2_pwr_int
_fft1back_mmx_finish
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  mov  ebx,[_fft1_interleave_points]
  mov  edi,[_timf2_pa]
  add  edi,edi
  add  edi,[_timf2_shi]
  mov  esi,[_fft1_backbuffer]
  mov  eax,[_timf2_input_block]
  add  eax,eax
  sub  eax,8
  cmp  dword [_rx_channels],1
  jz near finish_one
  shr  eax,2
  mov  ebp,[_timf2_pa]
  shr  ebp,1
  add  ebp,[_timf2_pwr_int]
  or   ebx,ebx
  jnz  not_two_0
; ********************************************
; No window used
two_0_loop:
  movq mm1,[4*eax+esi-8]
  movq [4*eax+edi-8],mm1
  pmaddwd mm1,mm1
  psrld mm1,1
  movq mm2,mm1
  psllq mm1,32
  paddd mm1,mm2
  movq  [eax+ebp-6],mm1
  movq mm1,[4*eax+esi]
  movq [4*eax+edi],mm1
  sub  eax,4
  jns  two_0_loop
; We have destroyed one data point in timf2_pwr_int. Put it back!
  movsx  eax,word [-16+edi]
  imul eax,eax
  movsx  ebx,word [-14+edi]
  imul ebx,ebx
  add  eax,ebx
  shr  eax,1
  movsx  ecx,word [-12+edi]
  imul ecx,ecx
  movsx  ebx,word [-10+edi]
  imul ebx,ebx
  add  ecx,ebx
  shr  ecx,1
  add  eax,ecx
  mov [-4+ebp],eax
  jmp near finish_x 
; **********************************************
; sin squared window
not_two_0:
  add  ebx,ebx
  cmp  ebx,[_fft1_size]
  jnz  near not_two_2
two_2a_loop:
  movq mm1,[4*eax+esi-8]
  paddsw mm1,[4*eax+edi-8]
  movq [4*eax+edi-8],mm1
  pmaddwd mm1,mm1
  psrad mm1,1
  movq mm2,mm1
  psllq mm1,32
  paddd mm1,mm2
  movq  [eax+ebp-6],mm1
  movq mm1,[4*eax+esi]
  paddsw mm1,[4*eax+edi]
  movq [4*eax+edi],mm1
  sub  eax,4
  jns  two_2a_loop
; We have destroyed one data point in timf2_pwr_int. Put it back!
  movsx  eax,word [-16+edi]
  imul eax,eax
  movsx  ebx,word [-14+edi]
  imul ebx,ebx
  add  eax,ebx
  shr  eax,1
  movsx  ecx,word [-12+edi]
  imul ecx,ecx
  movsx  ebx,word [-10+edi]
  imul ebx,ebx
  add  ecx,ebx
  shr  ecx,1
  add  eax,ecx
  mov [-4+ebp],eax

  mov  eax,[_timf2_input_block]
  mov  edi,[_timf2_pa]
  add  edi,eax
  and  edi,[_timf2_mask]
  add  edi,edi
  add  edi,[_timf2_shi]
  mov  esi,[_fft1_backbuffer]
  add  eax,eax
  add  esi,eax
  sub  eax,8
two_2b_loop:
  movq mm1,[eax+esi]
  movq [eax+edi],mm1
  sub  eax,8
  jns  two_2b_loop
  jmp near finish_x 
; *****************************************
; window N=1 or N>2
not_two_2:
  mov  ebp,[_fft1_inverted_mmxwin]
  mov  eax,[_fft1_size]
  sal  eax,2
  mov  [ebp-4],eax
  mov  eax,[_fft1_interleave_points]   
  sal  eax,2
  mov  [ebp-8],eax
  mov  eax,[_fft1_interleave_points]   
  sal  eax,2
  mov  edx,[_timf2_mask]
  mov  ecx,[_timf2_pa]
  mov  edi,[_timf2_shi]
  shr  ecx,1
  mov  ebx,[_timf2_pwr_int]
  lea  esi,[esi+2*eax]  
  sub  ecx,4
  shr  edx,1
genwin_two_loop1:
  movq mm0,[ebp+eax]
  movq mm1,mm0
  movq mm2,mm0
  movq mm3,mm0
  pmulhw mm0,[esi]
  add  ecx,4 
  paddsw mm0,mm0    ; shift by additions so we saturate properly!!
  pmulhw mm1,[esi+8]
  paddsw mm0,mm0
  and  ecx,edx
  paddsw mm1,mm1
  pmullw mm2,[esi]
  paddsw mm1,mm1
  add  eax,8
  psraw  mm2,14
  pmullw mm3,[esi+8]
  add  esi,16
  psraw  mm3,14
  paddsw mm0,mm2
  paddsw mm1,mm3
  movq [edi+4*ecx],mm0
  movq [edi+4*ecx+8],mm1
  pmaddwd mm0,mm0
  psrad mm0,1
  movq mm2,mm0
  psrlq mm0,32
  paddd mm0,mm2
  movq [ebx+ecx],mm0
  cmp  eax,[ebp-4]
  jnz  genwin_two_loop1
  align  8
genwin_two_loop2:
  movq mm0,[ebp+eax]
  movq mm1,mm0
  movq mm2,mm0
  movq mm3,mm0
  pmulhw mm0,[esi]
  add  ecx,4
  paddsw mm0,mm0    ; shift by additions so we saturate!!
  pmulhw mm1,[esi+8]
  paddsw mm0,mm0
  and  ecx,edx
  paddsw mm1,mm1
  pmullw mm2,[esi]
  paddsw mm1,mm1
  sub  eax,8
  psraw  mm2,14
  pmullw mm3,[esi+8]
  add  esi,16
  psraw  mm3,14
  paddsw mm0,mm2
  paddsw mm1,mm3
  movq [edi+4*ecx],mm0
  movq [edi+4*ecx+8],mm1
  pmaddwd mm0,mm0
  psrad  mm0,1
  movq mm2,mm0
  psrlq mm0,32
  paddd mm0,mm2
  movq [ebx+ecx],mm0
  cmp  eax,[ebp-8]
  jns  genwin_two_loop2
finish_x:
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  pop  eax
  ret
finish_one:
  shr  eax,1
  mov  ebp,[_timf2_pa]
  add  ebp,[_timf2_pwr_int]
  or   ebx,ebx
  jnz  not_one_0
; No window used
one_0_loop:
  movq mm1,[2*eax+esi]
  movq [2*eax+edi],mm1
  pmaddwd mm1,mm1
  psllq mm1,32
  movq  [eax+ebp-4],mm1
  sub  eax,4
  jns  one_0_loop
; We have destroyed one data point in timf2_pwr_int. Put it back!
  movsx  eax,word [-8+edi]
  imul eax,eax
  movsx  ebx,word [-6+edi]
  imul ebx,ebx
  add  eax,ebx
  mov [-4+ebp],eax
  jmp short finish_x 
not_one_0:
  add  ebx,ebx
  cmp  ebx,[_fft1_size]
  jnz  not_one_2
; Sine squared window
one_2a_loop:
  movq mm1,[2*eax+esi]
  paddsw mm1,[2*eax+edi]
  movq  [2*eax+edi],mm1
  pmaddwd mm1,mm1
  psllq mm1,32
  movq  [eax+ebp-4],mm1
  sub  eax,4
  jns  one_2a_loop
; We have destroyed one data point in timf2_pwr_int. Put it back!
  movsx  eax,word [-8+edi]
  imul eax,eax
  movsx  ebx,word [-6+edi]
  imul ebx,ebx
  add  eax,ebx
  mov [-4+ebp],eax
  mov  eax,[_timf2_input_block]
  mov  edi,[_timf2_pa]
  add  edi,eax
  and  edi,[_timf2_mask]
  add  edi,edi
  add  edi,[_timf2_shi]
  mov  esi,[_fft1_backbuffer]
  add  eax,eax
  add  esi,eax
  sub  eax,8
one_2b_loop:
  movq mm1,[eax+esi]
  movq [eax+edi],mm1
  sub  eax,8
  jns  one_2b_loop
  jmp near finish_x 
; Window N==1 or N>2
not_one_2:
  mov  ebp,[_fft1_inverted_mmxwin]
  mov  eax,[_fft1_size]
  sal  eax,2
  mov  [ebp-4],eax
  mov  eax,[_fft1_interleave_points]
  sal  eax,2
  mov  [ebp-8],eax
  mov  eax,[_fft1_interleave_points]
  sal  eax,2
  mov  ebx,[_timf2_pwr_int]
  mov  edx,[_timf2_mask]
  mov  ecx,[_timf2_pa]
  mov  edi,[_timf2_shi]
  add  esi,eax  
  sub  ecx,4
genwin_one_loop1:
  movq mm0,[ebp+eax]
  movq mm2,mm0
  pmulhw mm0,[esi]
  add  ecx,4
  pmullw mm2,[esi]
  and  ecx,edx
  paddsw mm0,mm0
  add  eax,8
  paddsw mm0,mm0
  psraw  mm2,14
  add  esi,8
  paddsw mm0,mm2
  movq [edi+2*ecx],mm0
  pmaddwd mm0,mm0
  movq  [ebx+ecx],mm0
  cmp  eax,[ebp-4]
  jnz  genwin_one_loop1
genwin_one_loop2:
  movq mm0,[ebp+eax]
  movq mm2,mm0
  pmulhw mm0,[esi]
  add  ecx,4
  pmullw mm2,[esi]
  and  ecx,edx
  paddsw mm0,mm0
  sub  eax,8
  paddsw mm0,mm0
  psraw  mm2,14
  add  esi,8
  paddsw mm0,mm2
  movq [edi+2*ecx],mm0
  pmaddwd mm0,mm0
  movq  [ebx+ecx],mm0
  cmp  eax,[ebp-8]
  jns  genwin_one_loop2
  jmp near finish_x
_mmx_fft1back_one
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  mov     ebp,[_fft1_backbuffer]
  mov     eax,[_fft1_size]
  mov     esi,[_fft1_back_scramble]
  mov     edi,[_fft1_split_shi]
  mov     ebx,0ffffH
  add     eax,eax
  add     eax,esi
  mov     [edi-16],ebx              ;[edi-14]=  1 0 1 0
  mov     [edi-12],ebx              
  mov     [edi-8],ebx
  mov     [edi-4],eax
  mov     eax,[esi]
  mov     ebx,eax
  and     eax,0ffffH
  shr     ebx,16
  mov     ecx,[esi+4]
  mov     edx,ecx
  and     ecx,0ffffH
  shr     edx,16
; The two first trivial loops are combined and
; run together with the unscrambling
fftback_one_1:
  add     esi,8
;  t1=fft1_split_shi[ia  ]+fft1_split_shi[ib  ];
;  t2=fft1_split_shi[ia+1]+fft1_split_shi[ib+1];
;  r1=fft1_split_shi[ia+2]+fft1_split_shi[ib+2];
;  r2=fft1_split_shi[ia+3]+fft1_split_shi[ib+3];

;  t5=fft1_split_shi[ia  ]-fft1_split_shi[ib  ];
;  t7=fft1_split_shi[ia+1]-fft1_split_shi[ib+1];
;  r5=fft1_split_shi[ia+2]-fft1_split_shi[ib+2];
;  r7=fft1_split_shi[ia+3]-fft1_split_shi[ib+3];
  movq    mm0,[edi+eax*8]
  movq    mm1,[edi+ecx*8]
  movq    mm2,mm0
  paddsw  mm0,[edi+ebx*8]    ;mm0=r2   r1   t2   t1
  movq    mm3,mm1
  psubsw  mm2,[edi+ebx*8]    ;mm2=r7   r5   t7   t5
  movq    mm4,mm0            ;mm4=r2   r1   t2   t1

;  t3=fft1_split_shi[ic  ]+fft1_split_shi[id  ];
;  t4=fft1_split_shi[ic+1]+fft1_split_shi[id+1];
;  r3=fft1_split_shi[ic+2]+fft1_split_shi[id+2];
;  r4=fft1_split_shi[ic+3]+fft1_split_shi[id+3];
;  t10=fft1_split_shi[ic  ]-fft1_split_shi[id  ];
;  t6= fft1_split_shi[ic+1]-fft1_split_shi[id+1];
;  r10=fft1_split_shi[ic+2]-fft1_split_shi[id+2];  
;  r6= fft1_split_shi[ic+3]-fft1_split_shi[id+3];

  paddsw  mm1,[edi+edx*8]    ;mm1=r4   r3   t4   t3
  psubsw  mm3,[edi+edx*8]    ;mm3=r6   r10  t6   t10
  mov     eax,[esi]
  mov     ecx,[esi+4]
;  fft1_tmp[k  ]=t1+t3;
;  fft1_tmp[k+1]=t2+t4;
;  fft1_tmp[k+2]=r1+r3;
;  fft1_tmp[k+3]=r2+r4;
;  fft1_tmp[k+8]=t1-t3;
;  fft1_tmp[k+9]=t2-t4;
;  fft1_tmp[k+10]=r1-r3;
;  fft1_tmp[k+11]=r2-r4;
                             ;mm0=r2   r1   t2   t1
  paddsw  mm0,mm1            ;mm0=r2+r4 r1+r3 t2+t4 t1+t3
;  t11=t5-t6
;  t9=t7+t10
;  r11=r5-r6
;  r9=r7+r10

;  t12=t5+t6
;  t8=t7-t10
;  r12=r5+r6
;  r8=r7-r10
                                 ;mm2=r7   r5   t7   t5
                                 ;mm3=r6   r10  t6   t10
  movq    mm5,mm3                ;mm5=r6   r10  t6   t10
  psllq   mm3,16                 ;mm3=r10  t6    t10    0
                                 ;mm4=r2   r1   t2   t1
  psubsw  mm4,mm1                ;mm4=r2-r4 r1-r3 t2-t4 t1-t3
  psrlq   mm5,16                 ;mm5= 0    r6   r10    t6
                                 ;[edi-14]=  1 0 1 0 
  pand    mm3,[edi-14]           ;mm3=r10   0   t10   0
  pand    mm5,[edi-12]           ;mm5= 0   r6    0   t6
  mov     ebx,eax
  movq    mm6,mm2                ;mm6=r7   r5   t7   t5
  psubsw  mm3,mm5                ;mm3=r10  -r6  t10  -t6
  mov     edx,ecx
  movq    [ebp],mm0
;  fft1_tmp[k+4]=t12;
;  fft1_tmp[k+5]=t8;
;  fft1_tmp[k+6]=r12;
;  fft1_tmp[k+7]=r8;
;  fft1_tmp[k+12]=t11;
;  fft1_tmp[k+13]=t9;
;  fft1_tmp[k+14]=r11;
;  fft1_tmp[k+15]=r9;
  psubsw  mm2,mm3                     ;mm2=r8  r12  t8  t12
  paddsw  mm6,mm3                     ;mm6=r9  r11  t9  t11
  and     ecx,0ffffH
  and     eax,0ffffH
  shr     ebx,16
  movq    [ebp+8],mm2
  movq    [ebp+24],mm6
  shr     edx,16
  movq    [ebp+16],mm4
  add     ebp,32
  cmp     esi,[edi-4]
  jl near fftback_one_1
; 
; Loops are run pairwise to save memory access.
; This also makes fft fit well to the mmx architecture.
; Use reserved memory (8*dwords) before fft1_mmxcosin as scratch area
; The first locations of this array are used in every iteration and these
; variables will be in the cash.
;m1=4;
;m2=16;
;inc=fft1_size/16;
;for(n=2; n<fft1_n-1; n+=2)
;  {
;  for(j=0; j<fft1_size; j+=m2)
;    {
;    itab=0;
;    k=j;
;lp1:;    
  mov     ebp,[_fft1_mmxcosin]
  mov     eax,[_fft1back_lowgain_n] ;eax=number of loops with g=0.5
  mov     ebx,eax
  shr     eax,1
  mov     [mem2],eax         ;mem2 is no of loops with g=0.25
  add     eax,eax
  sub     ebx,eax      
  mov     [mem1],ebx         ;mem1 is no of loops with g=0.5
  mov     eax,[_fft1_n]
  sub     eax,ebx
  sub     eax,2                    ;eax is the number of loops left
  sub     eax,[_fft1back_lowgain_n] ;eax=number of loops left with g=1
  mov     ecx,eax
  shr     eax,1
  mov     [N],eax                  ;[N]= number of dual loops width g=1
  add     eax,eax
  sub     ecx,eax                  ;ecx=1 if a single loop g=1 is needed
  mov     [mem3],ecx               ;one final loop width g=1
  mov     dword [M1],8*4           ; [M1]=m1*8
  mov     eax,[_fft1_size]
  mov     dword [M2],8*16          ; [M2]=m2*8
  shr     eax,1
  mov     [INCC],eax               ; [INCC]=inc*8
  shl     eax,2
  mov     [SIZ2],eax               ; [SIZ2]=fft1_size*2
  shl     eax,2
  mov     [SIZ8],eax
;***************************************************
;***************************************************
;***************************************************
; Full gain double loops
; There must be at least one and we make that one the last double loop.
  dec     dword [N]
  mov     eax,dword [N]
  or      eax,eax
  jz  near skip_fullgain_one
fftback_one_2:
  mov     dword [J],0             ;[J]=j*8
fftback_one_3:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8  
  mov     ebx,[_fft1_backbuffer]   ;ebx=*x
  mov     ecx,[M1]
fftback_one_4:
  lea     esi,[ebx+edx]           ;[esi]=x[ia] 
;      ia=k;
;//s1=fft1_mmxcosin[2*itab].s2p;
;//s1/=0x7fff;
;      s1= fft1tab[2*itab].sin; 
;      c1= fft1tab[2*itab].cos;
;      s2= fft1tab[itab].sin; 
;      c2= fft1tab[itab].cos;
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
;      x0=fft1_tmp[4*ia  ];
;      y0=fft1_tmp[4*ia+1];
;      u0=fft1_tmp[4*ia+2];
;      w0=fft1_tmp[4*ia+3];
;      ia+=m1;
;      x1=fft1_tmp[4*ia  ];
;      y1=fft1_tmp[4*ia+1];
;      u1=fft1_tmp[4*ia+2];
;      w1=fft1_tmp[4*ia+3];
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
;      t1= c1*x1+s1*y1;      
;      t2= c1*y1-s1*x1;      
;      r1= c1*u1+s1*w1;      
;      r2= c1*w1-s1*u1;      
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;      x4=x0+t1;
;      y4=y0+t2;
;      u4=u0+r1;
;      w4=w0+r2;
;
;      x6=x0-t1;
;      y6=y0-t2;
;      u6=u0-r1;
;      w6=w0-r2;
;      ia+=m1;
;      x2=fft1_tmp[4*ia  ];
;      y2=fft1_tmp[4*ia+1];
;      u2=fft1_tmp[4*ia+2];
;      w2=fft1_tmp[4*ia+3];
;      ia+=m1;
;      x3=fft1_tmp[4*ia  ];
;      y3=fft1_tmp[4*ia+1];
;      u3=fft1_tmp[4*ia+2];
;      w3=fft1_tmp[4*ia+3];
;mm0 =  c1    -s1     s1     c1
;mm1 =  w0     u0     y0     x0
;mm2 =  r2     r1     t2     t1
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 = ?
;mm6 =  w0     u0     y0     x0
;mm7 = ?
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
                               ;mm3 =  w2     u2     y2     x2
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
;      t3=c1*x3+s1*y3;      
;      t4=c1*y3-s1*x3;      
;      r3=c1*u3+s1*w3;      
;      r4=c1*w3-s1*u3;      
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
                               ;mm7 =  t4            t3
                               ;mm5 =  r4            r3
  sub     esi,ecx
;      x5=x2+t3;
;      y5=y2+t4;
;      u5=u2+r3;
;      w5=w2+r4;
;
;      x7=x2-t3;
;      y7=y2-t4;
;      u7=u2-r3;
;      w7=w2-r4;
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 =  r4            r3 
;mm6 =  w6     u6     y6     x6
;mm7 =  t4            t3 
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w5     u5     y5     x5
;mm4 = ?
;mm5 =  w7     u7     y7     x7 
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
;      t5=c2*x5+s2*y5;
;      t6=c2*y5-s2*x5;
;      r5=c2*u5+s2*w5;
;      r6=c2*w5-s2*u5;
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4 
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
;      ia=k;
;      ib=ia+m1;
;      ic=ia+2*m1;
;      id=ia+3*m1;
;      fft1_tmp[4*ia  ]=x4+t5;
;      fft1_tmp[4*ia+1]=y4+t6;
;      fft1_tmp[4*ia+2]=u4+r5;
;      fft1_tmp[4*ia+3]=w4+r6;
;      fft1_tmp[4*ic  ]=x4-t5;
;      fft1_tmp[4*ic+1]=y4-t6;
;      fft1_tmp[4*ic+2]=u4-r5;
;      fft1_tmp[4*ic+3]=w4-r6;
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
;mm0 =  c1    -s1     s1     c1
;mm1 = ?
;mm2 = ? 
;mm3 = ?
;mm4 = ?
;mm5 =  w7     u7     y7     x7
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
;      t8=c2*x7+s2*y7;
;      t7=c2*y7-s2*x7;
;      r8=c2*u7+s2*w7;
;      r7=c2*w7-s2*u7;
;      fft1_tmp[4*ib  ]=x6+t7;
;      fft1_tmp[4*ib+1]=y6-t8;
;      fft1_tmp[4*ib+2]=u6+r7;
;      fft1_tmp[4*ib+3]=w6-r8;
;
;      fft1_tmp[4*id  ]=x6-t7;
;      fft1_tmp[4*id+1]=y6+t8;
;      fft1_tmp[4*id+2]=u6-r7;
;      fft1_tmp[4*id+3]=w6+r8;

  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
                               ;mm5 =  t7            t8
                               ;mm2 =  r7            r8
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near  fftback_one_4
;
;      itab+=inc;
;      k++; 
;    if(itab<fft1_size/4)goto lp1;
;    }
;  inc/=4;
;  m1*=4;
;  m2*=4;
;  }
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_one_3
  shr     dword [INCC],2
  shl     dword [M1],2
  shl     dword [M2],2
  dec     dword [N]
  jnz near    fftback_one_2
skip_fullgain_one:
;***************************************************
;***************************************************
;***************************************************
; g=0.5 double loop
  cmp     dword [mem1],0
  jz      near fftback_one_20
  mov     dword [J],0             ;[J]=j*8
fftback_one_13:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8
  mov     ebx,[_fft1_backbuffer]   ;ebx=*x
  mov     ecx,[M1]
fftback_one_14:
  lea     esi,[ebx+edx]           ;[esi]=x[ia] 
;      ia=k;
;//s1=fft1_mmxcosin[2*itab].s2p;
;//s1/=0x7fff;
;      s1= fft1tab[2*itab].sin; 
;      c1= fft1tab[2*itab].cos;
;      s2= fft1tab[itab].sin; 
;      c2= fft1tab[itab].cos;
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
;      x0=fft1_tmp[4*ia  ];
;      y0=fft1_tmp[4*ia+1];
;      u0=fft1_tmp[4*ia+2];
;      w0=fft1_tmp[4*ia+3];
;      ia+=m1;
;      x1=fft1_tmp[4*ia  ];
;      y1=fft1_tmp[4*ia+1];
;      u1=fft1_tmp[4*ia+2];
;      w1=fft1_tmp[4*ia+3];
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
;      t1= c1*x1+s1*y1;      
;      t2= c1*y1-s1*x1;      
;      r1= c1*u1+s1*w1;      
;      r2= c1*w1-s1*u1;      
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;      x4=x0+t1;
;      y4=y0+t2;
;      u4=u0+r1;
;      w4=w0+r2;
;
;      x6=x0-t1;
;      y6=y0-t2;
;      u6=u0-r1;
;      w6=w0-r2;
;      ia+=m1;
;      x2=fft1_tmp[4*ia  ];
;      y2=fft1_tmp[4*ia+1];
;      u2=fft1_tmp[4*ia+2];
;      w2=fft1_tmp[4*ia+3];
;      ia+=m1;
;      x3=fft1_tmp[4*ia  ];
;      y3=fft1_tmp[4*ia+1];
;      u3=fft1_tmp[4*ia+2];
;      w3=fft1_tmp[4*ia+3];
;mm0 =  c1    -s1     s1     c1
;mm1 =  w0     u0     y0     x0
;mm2 =  r2     r1     t2     t1
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 = ?
;mm6 =  w0     u0     y0     x0
;mm7 = ?
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
                               ;mm3 =  w2     u2     y2     x2
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
;      t3=c1*x3+s1*y3;      
;      t4=c1*y3-s1*x3;      
;      r3=c1*u3+s1*w3;      
;      r4=c1*w3-s1*u3;      
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
                               ;mm7 =  t4            t3 
                               ;mm5 =  r4            r3 
  sub     esi,ecx
;      x5=x2+t3;
;      y5=y2+t4;
;      u5=u2+r3;
;      w5=w2+r4;
;
;      x7=x2-t3;
;      y7=y2-t4;
;      u7=u2-r3;
;      w7=w2-r4;
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 =  r4            r3 
;mm6 =  w6     u6     y6     x6
;mm7 =  t4            t3 
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w5     u5     y5     x5
;mm4 = ?
;mm5 =  w7     u7     y7     x7 
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
;      t5=c2*x5+s2*y5;
;      t6=c2*y5-s2*x5;
;      r5=c2*u5+s2*w5;
;      r6=c2*w5-s2*u5;
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
;      ia=k;
;      ib=ia+m1;
;      ic=ia+2*m1;
;      id=ia+3*m1;
;      fft1_tmp[4*ia  ]=x4+t5;
;      fft1_tmp[4*ia+1]=y4+t6;
;      fft1_tmp[4*ia+2]=u4+r5;
;      fft1_tmp[4*ia+3]=w4+r6;
;      fft1_tmp[4*ic  ]=x4-t5;
;      fft1_tmp[4*ic+1]=y4-t6;
;      fft1_tmp[4*ic+2]=u4-r5;
;      fft1_tmp[4*ic+3]=w4-r6;
  psraw   mm1,1
  psraw   mm7,1
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
;mm0 =  c1    -s1     s1     c1
;mm1 = ?
;mm2 = ? 
;mm3 = ?
;mm4 = ?
;mm5 =  w7     u7     y7     x7
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
;      t8=c2*x7+s2*y7;
;      t7=c2*y7-s2*x7;
;      r8=c2*u7+s2*w7;
;      r7=c2*w7-s2*u7;
;      fft1_tmp[4*ib  ]=x6+t7;
;      fft1_tmp[4*ib+1]=y6-t8;
;      fft1_tmp[4*ib+2]=u6+r7;
;      fft1_tmp[4*ib+3]=w6-r8;
;
;      fft1_tmp[4*id  ]=x6-t7;
;      fft1_tmp[4*id+1]=y6+t8;
;      fft1_tmp[4*id+2]=u6-r7;
;      fft1_tmp[4*id+3]=w6+r8;

  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
                               ;mm5 =  t7            t8
                               ;mm2 =  r7            r8
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  psraw     mm6,1
  psraw     mm7,1
  add       esi,ecx
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near fftback_one_14
;
;      itab+=inc;
;      k++; 
;    if(itab<fft1_size/4)goto lp1;
;    }
;  inc/=4;
;  m1*=4;
;  m2*=4;
;  }
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_one_13
  shr     dword [INCC],2
  shl     dword [M1],2
  shl     dword [M2],2
;***************************************************
;***************************************************
;***************************************************
; g=0.25 double loops
fftback_one_20:
  mov     eax,[mem2]
  or      eax,eax
  jz near fftback_one_30
  mov     [N],eax
  cmp     dword [mem1],0
fftback_one_22:
  mov     dword [J],0             ;[J]=j*8
fftback_one_23:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8
  mov     ebx,[_fft1_backbuffer]   ;ebx=*x
  mov     ecx,[M1]
fftback_one_24:
  lea     esi,[ebx+edx]           ;[esi]=x[ia]
;      ia=k;
;//s1=fft1_mmxcosin[2*itab].s2p
;//s1/=0x7fff
;      s1= fft1tab[2*itab].sin
;      c1= fft1tab[2*itab].cos
;      s2= fft1tab[itab].sin
;      c2= fft1tab[itab].cos
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
;      x0=fft1_tmp[4*ia  ]
;      y0=fft1_tmp[4*ia+1]
;      u0=fft1_tmp[4*ia+2]
;      w0=fft1_tmp[4*ia+3]
;      ia+=m1
;      x1=fft1_tmp[4*ia  ]
;      y1=fft1_tmp[4*ia+1]
;      u1=fft1_tmp[4*ia+2]
;      w1=fft1_tmp[4*ia+3]
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
;      t1= c1*x1+s1*y1
;      t2= c1*y1-s1*x1
;      r1= c1*u1+s1*w1
;      r2= c1*w1-s1*u1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;      x4=x0+t1;
;      y4=y0+t2;
;      u4=u0+r1;
;      w4=w0+r2;
;
;      x6=x0-t1;
;      y6=y0-t2;
;      u6=u0-r1;
;      w6=w0-r2;
;      ia+=m1;
;      x2=fft1_tmp[4*ia  ]
;      y2=fft1_tmp[4*ia+1]
;      u2=fft1_tmp[4*ia+2]
;      w2=fft1_tmp[4*ia+3]
;      ia+=m1;
;      x3=fft1_tmp[4*ia  ]
;      y3=fft1_tmp[4*ia+1]
;      u3=fft1_tmp[4*ia+2]
;      w3=fft1_tmp[4*ia+3]
;mm0 =  c1    -s1     s1     c1
;mm1 =  w0     u0     y0     x0
;mm2 =  r2     r1     t2     t1
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 = ?
;mm6 =  w0     u0     y0     x0
;mm7 = ?
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
                               ;mm3 =  w2     u2     y2     x2
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
;      t3=c1*x3+s1*y3
;      t4=c1*y3-s1*x3
;      r3=c1*u3+s1*w3
;      r4=c1*w3-s1*u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
                               ;mm7 =  t4            t3 
                               ;mm5 =  r4            r3 
  sub     esi,ecx
;      x5=x2+t3;
;      y5=y2+t4;
;      u5=u2+r3;
;      w5=w2+r4;
;
;      x7=x2-t3;
;      y7=y2-t4;
;      u7=u2-r3;
;      w7=w2-r4;
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 =  r4            r3 
;mm6 =  w6     u6     y6     x6
;mm7 =  t4            t3 
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w5     u5     y5     x5
;mm4 = ?
;mm5 =  w7     u7     y7     x7
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
;      t5=c2*x5+s2*y5;
;      t6=c2*y5-s2*x5;
;      r5=c2*u5+s2*w5;
;      r6=c2*w5-s2*u5;
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
;      ia=k;
;      ib=ia+m1;
;      ic=ia+2*m1;
;      id=ia+3*m1;
;      fft1_tmp[4*ia  ]=x4+t5;
;      fft1_tmp[4*ia+1]=y4+t6;
;      fft1_tmp[4*ia+2]=u4+r5;
;      fft1_tmp[4*ia+3]=w4+r6;
;      fft1_tmp[4*ic  ]=x4-t5;
;      fft1_tmp[4*ic+1]=y4-t6;
;      fft1_tmp[4*ic+2]=u4-r5;
;      fft1_tmp[4*ic+3]=w4-r6;
  psraw   mm1,2
  psraw   mm7,2
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
;mm0 =  c1    -s1     s1     c1
;mm1 = ?
;mm2 = ? 
;mm3 = ?
;mm4 = ?
;mm5 =  w7     u7     y7     x7
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
;      t8=c2*x7+s2*y7;
;      t7=c2*y7-s2*x7;
;      r8=c2*u7+s2*w7;
;      r7=c2*w7-s2*u7;
;      fft1_tmp[4*ib  ]=x6+t7;
;      fft1_tmp[4*ib+1]=y6-t8;
;      fft1_tmp[4*ib+2]=u6+r7;
;      fft1_tmp[4*ib+3]=w6-r8;
;
;      fft1_tmp[4*id  ]=x6-t7;
;      fft1_tmp[4*id+1]=y6+t8;
;      fft1_tmp[4*id+2]=u6-r7;
;      fft1_tmp[4*id+3]=w6+r8;

  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
                               ;mm5 =  t7            t8
                               ;mm2 =  r7            r8
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  psraw     mm6,2
  psraw     mm7,2
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add     edi,[INCC]
  add     edx,8
  cmp     edi,[SIZ2]
  jb  near fftback_one_24
;
;      itab+=inc;
;      k++; 
;    if(itab<fft1_size/4)goto lp1;
;    }
;  inc/=4;
;  m1*=4;
;  m2*=4;
;  }
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_one_23
  shr     dword [INCC],2
  shl     dword [M1],2
  shl     dword [M2],2
  dec     dword [N]
  jnz near    fftback_one_22
;***************************************************
;***************************************************
;***************************************************
; The last dual loop has g=1, we do not want to hide overflows
  mov     dword [J],0             ;[J]=j*8
fftback_one_3x:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8  
  mov     ebx,[_fft1_backbuffer]   ;ebx=*x
  mov     ecx,[M1]
fftback_one_4x:
  lea     esi,[ebx+edx]           ;[esi]=x[ia] 
;      ia=k;
;//s1=fft1_mmxcosin[2*itab].s2p;
;//s1/=0x7fff;
;      s1= fft1tab[2*itab].sin; 
;      c1= fft1tab[2*itab].cos;
;      s2= fft1tab[itab].sin; 
;      c2= fft1tab[itab].cos;
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
;      x0=fft1_tmp[4*ia  ];
;      y0=fft1_tmp[4*ia+1];
;      u0=fft1_tmp[4*ia+2];
;      w0=fft1_tmp[4*ia+3];
;      ia+=m1;
;      x1=fft1_tmp[4*ia  ];
;      y1=fft1_tmp[4*ia+1];
;      u1=fft1_tmp[4*ia+2];
;      w1=fft1_tmp[4*ia+3];
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
;      t1= c1*x1+s1*y1;      
;      t2= c1*y1-s1*x1;      
;      r1= c1*u1+s1*w1;      
;      r2= c1*w1-s1*u1;      
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;      x4=x0+t1;
;      y4=y0+t2;
;      u4=u0+r1;
;      w4=w0+r2;
;
;      x6=x0-t1;
;      y6=y0-t2;
;      u6=u0-r1;
;      w6=w0-r2;
;      ia+=m1;
;      x2=fft1_tmp[4*ia  ];
;      y2=fft1_tmp[4*ia+1];
;      u2=fft1_tmp[4*ia+2];
;      w2=fft1_tmp[4*ia+3];
;      ia+=m1;
;      x3=fft1_tmp[4*ia  ];
;      y3=fft1_tmp[4*ia+1];
;      u3=fft1_tmp[4*ia+2];
;      w3=fft1_tmp[4*ia+3];
;mm0 =  c1    -s1     s1     c1
;mm1 =  w0     u0     y0     x0
;mm2 =  r2     r1     t2     t1
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 = ?
;mm6 =  w0     u0     y0     x0
;mm7 = ?
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
                               ;mm3 =  w2     u2     y2     x2
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
;      t3=c1*x3+s1*y3;      
;      t4=c1*y3-s1*x3;      
;      r3=c1*u3+s1*w3;      
;      r4=c1*w3-s1*u3;      
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
                               ;mm7 =  t4            t3
                               ;mm5 =  r4            r3
  sub     esi,ecx
;      x5=x2+t3;
;      y5=y2+t4;
;      u5=u2+r3;
;      w5=w2+r4;
;
;      x7=x2-t3;
;      y7=y2-t4;
;      u7=u2-r3;
;      w7=w2-r4;
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 =  r4            r3 
;mm6 =  w6     u6     y6     x6
;mm7 =  t4            t3 
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
;mm0 =  c1    -s1     s1     c1
;mm1 =  w4     u4     y4     x4
;mm2 = ? 
;mm3 =  w5     u5     y5     x5
;mm4 = ?
;mm5 =  w7     u7     y7     x7 
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
;      t5=c2*x5+s2*y5;
;      t6=c2*y5-s2*x5;
;      r5=c2*u5+s2*w5;
;      r6=c2*w5-s2*u5;
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4 
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
;      ia=k;
;      ib=ia+m1;
;      ic=ia+2*m1;
;      id=ia+3*m1;
;      fft1_tmp[4*ia  ]=x4+t5;
;      fft1_tmp[4*ia+1]=y4+t6;
;      fft1_tmp[4*ia+2]=u4+r5;
;      fft1_tmp[4*ia+3]=w4+r6;
;      fft1_tmp[4*ic  ]=x4-t5;
;      fft1_tmp[4*ic+1]=y4-t6;
;      fft1_tmp[4*ic+2]=u4-r5;
;      fft1_tmp[4*ic+3]=w4-r6;
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
;mm0 =  c1    -s1     s1     c1
;mm1 = ?
;mm2 = ? 
;mm3 = ?
;mm4 = ?
;mm5 =  w7     u7     y7     x7
;mm6 =  w6     u6     y6     x6
;mm7 = ? 
;      t8=c2*x7+s2*y7;
;      t7=c2*y7-s2*x7;
;      r8=c2*u7+s2*w7;
;      r7=c2*w7-s2*u7;
;      fft1_tmp[4*ib  ]=x6+t7;
;      fft1_tmp[4*ib+1]=y6-t8;
;      fft1_tmp[4*ib+2]=u6+r7;
;      fft1_tmp[4*ib+3]=w6-r8;
;
;      fft1_tmp[4*id  ]=x6-t7;
;      fft1_tmp[4*id+1]=y6+t8;
;      fft1_tmp[4*id+2]=u6-r7;
;      fft1_tmp[4*id+3]=w6+r8;

  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
                               ;mm5 =  t7            t8
                               ;mm2 =  r7            r8
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near  fftback_one_4x
;
;      itab+=inc;
;      k++; 
;    if(itab<fft1_size/4)goto lp1;
;    }
;  inc/=4;
;  m1*=4;
;  m2*=4;
;  }
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_one_3x
;***************************************************
;***************************************************
;***************************************************
; Single loop with g=1
fftback_one_30:
  cmp     dword [mem3],0
  jz  near    fft1back_one_x
;     ib=fft1_size/2;
;     for(ia=0; ia<fft1_size/2; ia++)
;       {
  mov     esi,[_fft1_backbuffer]
  mov     eax,[_fft1_size]
  sal     eax,2
  lea     edi,[eax+esi]
  mov     edx,edi  
  mov     ebp,[_fft1_mmxcosin]
fftback_one_5:
;       s1= fft1_backtab[ia].sin;
;       c1= fft1_backtab[ia].cos;
  movq    mm0,[ebp]        ;cos   -sin    sin    cos
;       x0=fft1_tmp[4*ia  ];
;       y0=fft1_tmp[4*ia+1];
;       u0=fft1_tmp[4*ia+2];
;       w0=fft1_tmp[4*ia+3];
;       x1=fft1_tmp[4*ib  ];
;       y1=fft1_tmp[4*ib+1];
;       u1=fft1_tmp[4*ib+2];
;       w1=fft1_tmp[4*ib+3];
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[edi]            ;mm2 =  w1     u1     y1     x1
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
;      t1= c1*x1+s1*y1;
;      t2= c1*y1-s1*x1;
;      r1= c1*u1+s1*w1;
;      r2= c1*w1-s1*u1;
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;       fft1_tmp[4*ib  ]=x0-t1;
;       fft1_tmp[4*ib+1]=y0-t2;
;       fft1_tmp[4*ib+2]=u0-r1;
;       fft1_tmp[4*ib+3]=w0-r2;
;       fft1_tmp[4*ia  ]=x0+t1;
;       fft1_tmp[4*ia+1]=y0+t2;
;       fft1_tmp[4*ia+2]=u0+r1;
;       fft1_tmp[4*ia+3]=w0+r2;
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    [esi],mm1
  movq    [edi],mm6
;       ib++;
;       }
  add     ebp,8
  add     edi,8
  add     esi,8
  cmp     esi,edx
  jb      fftback_one_5
fft1back_one_x:
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  pop  eax
  ret

_mmx_fft1back_two
  push eax
  push ebx
  push ecx
  push edx
  push esi
  push edi
  push ebp
  mov     ebp,[_fft1_backbuffer]
  mov     eax,[_fft1_size]
  mov     esi,[_fft1_back_scramble]
  mov     edi,[_fft1_split_shi]
  mov     ebx,0ffffH
  add     eax,eax 
  add     eax,esi
  mov     [edi-16],ebx              ;[edi-14]=  1 0 1 0
  mov     [edi-12],ebx
  mov     [edi-8],ebx
  mov     [edi-4],eax
  mov     eax,[esi]
  mov     ebx,eax
  and     eax,0ffffH
  shr     ebx,16
  mov     ecx,[esi+4]
  mov     edx,ecx
  and     ecx,0ffffH
  shr     edx,16
  add     eax,eax
  add     ebx,ebx
  add     ecx,ecx
  add     edx,edx
; The two first trivial loops are combined and
; run together with the unscrambling
fftback_two_1:
  add     esi,8
  movq    mm0,[edi+eax*8]
  movq    mm1,[edi+ecx*8]
  movq    mm2,mm0
  paddsw  mm0,[edi+ebx*8]
  movq    mm3,mm1
  psubsw  mm2,[edi+ebx*8]
  movq    mm4,mm0
  paddsw  mm1,[edi+edx*8]
  psubsw  mm3,[edi+edx*8]
  paddsw  mm0,mm1
  movq    mm5,mm3
  psllq   mm3,16
  psubsw  mm4,mm1
  movq    [ebp],mm0
  movq    [ebp+32],mm4
  movq    mm0,[edi+eax*8+8]
  movq    mm1,[edi+ecx*8+8]
  movq    mm6,mm0
  paddsw  mm0,[edi+ebx*8+8]
  movq    mm7,mm1
  psubsw  mm6,[edi+ebx*8+8]
  movq    mm4,mm0
  paddsw  mm1,[edi+edx*8+8]
  psubsw  mm7,[edi+edx*8+8]
  paddsw  mm0,mm1
  psubsw  mm4,mm1
  movq    [ebp+8],mm0
  movq    [ebp+40],mm4
  mov     eax,[esi]
  mov     ecx,[esi+4]
  movq    mm4,mm7
  psllq   mm7,16
  psrlq   mm4,16
  pand    mm7,[edi-14]
  pand    mm4,[edi-12]
  movq    mm0,mm6
  psubsw  mm7,mm4
  paddsw  mm6,mm7
  psubsw  mm0,mm7
  movq    [ebp+24],mm0
  movq    [ebp+56],mm6
  psrlq   mm5,16
  pand    mm5,[edi-12]
  pand    mm3,[edi-14]
  mov     ebx,eax
  mov     edx,ecx
  movq    mm0,mm2
  psubsw  mm3,mm5
  paddsw  mm2,mm3
  psubsw  mm0,mm3
  and     eax,0ffffH
  and     ecx,0ffffH
  shr     ebx,16
  shr     edx,16
  add     eax,eax
  add     ebx,ebx
  add     ecx,ecx
  add     edx,edx
  movq    [ebp+16],mm0
  movq    [ebp+48],mm2
  add     ebp,64
  cmp     esi,[edi-4]
  jl near fftback_two_1
;*********************************************
  mov     ebp,[_fft1_mmxcosin]
  mov     eax,[_fft1back_lowgain_n] ;eax=number of loops with g=0.5
  mov     ebx,eax
  shr     eax,1
  mov     [mem2],eax         ;mem2 is no of loops with g=0.25
  add     eax,eax
  sub     ebx,eax      
  mov     [mem1],ebx         ;mem1 is no of loops with g=0.5
  mov     eax,[_fft1_n]
  sub     eax,ebx
  sub     eax,2                    ;eax is the number of loops left
  sub     eax,[_fft1back_lowgain_n] ;eax=number of loops left with g=1
  mov     ecx,eax
  shr     eax,1
  mov     [N],eax                  ;[N]= number of dual loops width g=1
  add     eax,eax
  sub     ecx,eax                  ;ecx=1 if a single loop g=1 is needed
  mov     [mem3],ecx               ;one final loop width g=1
  mov     dword [M1],16*4         ; [M1]=m1*16
  mov     eax,[_fft1_size]
  mov     dword [M2],8*16        ; [M2]=m2*8
  shr     eax,1
  mov     [INCC],eax              ; [INCC]=inc*8
  shl     eax,2
  mov     [SIZ2],eax             ; [SIZ2]=fft1_size*2
  shl     eax,2
  mov     [SIZ8],eax
;***************************************************
;***************************************************
;***************************************************
; Full gain double loops
; There must be at least one and we make that one the last double loop.
  dec     dword [N]
  mov     eax,dword [N]
  or      eax,eax
  jz  near skip_fullgain_two
fftback_two_2:
  mov     dword [J],0             ;[J]=j*8
fftback_two_3:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8  
  mov     ebx,[_fft1_backbuffer]                ;ebx=*x
  mov     ecx,[M1]
fftback_two_4:
  lea     esi,[ebx+2*edx]           ;[esi]=x[ia] 
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
;*********************************************************************
  add     esi,8  
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  add       esi,ecx
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       esi,8
;*********************************************************************
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near fftback_two_4
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_two_3
  shr     dword [INCC],2
  shl     dword [M1],2
  shl     dword [M2],2
  dec     dword [N]
  jnz near    fftback_two_2
skip_fullgain_two:
;***************************************************
;***************************************************
;***************************************************
; g=0.5 double loop
  cmp     dword [mem1],0
  jz      near fftback_two_20
  mov     dword [J],0             ;[J]=j*8
fftback_two_13:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8  
  mov     ebx,[_fft1_backbuffer]                ;ebx=*x
  mov     ecx,[M1]
fftback_two_14:
  lea     esi,[ebx+2*edx]           ;[esi]=x[ia] 
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5    
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  psraw   mm1,1
  psraw   mm7,1
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  psraw     mm6,1
  psraw     mm7,1
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
;*********************************************************************
  add     esi,8  
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  psraw   mm1,1
  psraw   mm7,1
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  add       esi,ecx
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  psraw     mm6,1
  psraw     mm7,1
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       esi,8
;*********************************************************************
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near fftback_two_14
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_two_13
  shr     dword [INCC],2
  shl     dword [M1],2
  shl     dword [M2],2
;***************************************************
;***************************************************
;***************************************************
; g=0.25 double loops
fftback_two_20:
  mov     eax,[mem2]
  or      eax,eax
  jz near fftback_two_30
  mov     [N],eax
fftback_two_22:
  mov     dword [J],0             ;[J]=j*8
fftback_two_23:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8  
  mov     ebx,[_fft1_backbuffer]                ;ebx=*x
  mov     ecx,[M1]
fftback_two_24:
  lea     esi,[ebx+2*edx]           ;[esi]=x[ia] 
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  psraw   mm1,2
  psraw   mm7,2
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  psraw     mm6,2
  psraw     mm7,2
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
;*********************************************************************
  add     esi,8  
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  psraw   mm1,2
  psraw   mm7,2
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  add       esi,ecx
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  psraw     mm6,2
  psraw     mm7,2
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       esi,8
;*********************************************************************
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near fftback_two_24
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_two_23
  shr     dword [INCC],2
  shl     dword [M1],2
  shl     dword [M2],2
  dec     dword [N]
  jnz near    fftback_two_22
;***************************************************
;***************************************************
;***************************************************
; The last dual loop has g=1, we do not want to hide overflows
  mov     dword [J],0             ;[J]=j*8
fftback_two_3x:
  mov     edx,dword[J]            ;edx=k*8
  xor     edi,edi                 ;edi=itab*8  
  mov     ebx,[_fft1_backbuffer]                ;ebx=*x
  mov     ecx,[M1]
fftback_two_4x:
  lea     esi,[ebx+2*edx]           ;[esi]=x[ia] 
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
;*********************************************************************
  add     esi,8  
  movq    mm0,[ebp+2*edi]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[esi+ecx  ]      ;mm2 =  w1     u1     y1     x1
  movq    mm3,[esi+2*ecx]      ;mm3 =  w2     u2     y2     x2
  add     esi,ecx
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm2 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3
  sub     esi,ecx
  psrad   mm7,15               ;mm7 =         t4            t3
  psrad   mm5,15               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
  movq    mm5,mm3              ;mm5 =  w2     u2     y2     x2
  paddsw  mm3,mm7              ;mm3 =  w5     u5     y5     x5
  psubsw  mm5,mm7              ;mm5 =  w7     u7     y7     x7
  movq    mm0,[ebp+edi]        ;mm0 =  c2    -s2     s2     c2
  movq    mm2,mm3              ;mm2 =  w5     u5     y5     x5
  punpckldq  mm3,mm3           ;mm3 =  y5     x5     y5     x5
  punpckhdq  mm2,mm2           ;mm2 =  w5     u5     w5     u5
  pmaddwd mm3,mm0              ;mm3 = c2*y5-s2*x5   c2*x5+s2*y5
  pmaddwd mm2,mm0              ;mm2 = c2*w5-s2*u5   c2*u5+s2*w5
  movq    mm4,mm3              ;mm3 =  t6            t5
  movq    mm7,mm2              ;mm2 =  r6            r5
  psrad   mm3,15               ;mm3 =         t6            t5
  psrad   mm2,15               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  movq    [esi],mm1
  movq    [esi+2*ecx],mm7
  add       esi,ecx
  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7
  psrad   mm5,15               ;mm5 =         t7            t8
  psrad   mm2,15               ;mm2 =         r7            r8
  movq    mm4,mm5
  movq    mm3,mm2
  pxor    mm0,mm0            
  punpckldq mm5,mm0            ;mm5 =                       t8
  punpckldq mm2,mm0            ;mm2 =                       r8
  punpckhdq mm4,mm0            ;mm4 =                       t7
  punpckhdq mm3,mm0            ;mm3 =                       r7
  packssdw  mm4,mm3            ;mm4 =         r7            t7
  packssdw  mm5,mm2            ;mm5 =         r8            t8
  psllq     mm5,16             ;mm5 =   r8           t8
  psubsw    mm4,mm5            ;mm4 =  -r8    r7    -t8     t7
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
  add       esi,8
;*********************************************************************
  add       edi,[INCC]
  add       edx,8
  cmp       edi,[SIZ2]
  jb  near fftback_two_4x
  mov     eax,[J]
  add     eax,[M2]
  mov     dword [J],eax
  cmp     eax,[SIZ8]
  jb near fftback_two_3x
;***************************************************
;***************************************************
;***************************************************
; Single loop with g=1
fftback_two_30:
  cmp     dword [mem3],0
  jz  near    fft1back_two_x
  mov     esi,[_fft1_backbuffer]
  mov     eax,[_fft1_size]
  sal     eax,3
  lea     edi,[eax+esi]
  mov     edx,edi  
fft1back_two_5:
  movq    mm0,[ebp]        ;cos   -sin    sin    cos
  movq    mm1,[esi]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[edi]            ;mm2 =  w1     u1     y1     x1
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1 
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1 
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1 
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    [esi],mm1
  movq    [edi],mm6
  movq    mm1,[esi+8]            ;mm1 =  w0     u0     y0     x0
  movq    mm2,[edi+8]            ;mm2 =  w1     u1     y1     x1
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1 
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1 
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,15               ;mm2 =         t2            t1 
  psrad   mm4,15               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
  movq    [esi+8],mm1
  movq    [edi+8],mm6
  add     ebp,8
  add     edi,16
  add     esi,16
  cmp     esi,edx
  jb      fft1back_two_5
fft1back_two_x:
  emms
  pop  ebp
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  pop  eax
  ret






;**************************************************************
;**************************************************************
;Use the information in _liminfo to split the fourier transforms
;stored in fft1_float as floating point numbers into
;two 16 bit integer fourier transforms in fft1_split_shi
;The first group contains noise and weak signals while
;the second group contains strong, sinewave like components.
;This routine is in assembly because that was necessary in
;Watcom C under DOS. Maybe gnu C is ok - I have not tried.

_split_one
  push   eax
  push   ebx
  push   ecx
  push   edx
  push   esi
  push   edi
  push   ebp

; First fill ends with zeroes (skip signals at ends)
  xor    eax,eax
  mov    ecx,[_fft1_first_point]
  or     ecx,ecx
  jz     low_one_ok
  mov    edi,[_fft1_split_shi]
  shl    ecx,1
  cld
  rep    stosd
  
low_one_ok:
  mov    edi,[_fft1_size]
  dec    edi
  mov    ecx,edi
  sub    ecx,[_fft1_last_point]
  jz     high_one_ok
  shl    ecx,1
  shl    edi,3
  add    edi,[_fft1_split_shi]
  std
  rep    stosd
  cld
high_one_ok:
;***********************************************************
  mov    esi,[_fft1_first_point]
  mov    ebx,esi
  shl    ebx,1           
  mov    edx,ebx
  add    ebx,[_fft1_px]
  shl    ebx,2
  add    ebx,[_fft1_float]
  shl    edx,2
  add    edx,[_fft1_split_shi]
  xor    edi,edi
  mov    ecx,esi
  sal    ecx,2
  add    ecx,[_liminfo]
  fldz
  xor    ebp,ebp
split_one_loop:
  fcom   dword [ecx]
  fnstsw    ax
  sahf
  je  near   flag_zero_one
  jb  near   flag_pos_one
; _liminfo is negative so we just convert from float to 16 bit integers.
; and store among large signals
  fld    dword [ebx]
  mov    dword [edx],edi
  fistp  word [edx+4]
  fld    dword [ebx+4]
  fistp  word [edx+6]
split_one_inc:
  inc    esi
  add    ecx,4
  add    ebx,8
  add    edx,8
  cmp    esi,[_fft1_last_point]
  jle    split_one_loop
splitx: 
  fcomp  dword [ecx-4]
  mov    ebx,[_fft1_px]
  add    ebx,[_fft1_block]
  and    ebx,[_fft1_mask]
  mov    [_fft1_px],ebx
  mov    [_fft1_lowlevel_points],ebp
  pop    ebp
  pop    edi
  pop    esi
  pop    edx
  pop    ecx
  pop    ebx
  pop    eax
  ret
flag_pos_one:
; _liminfo is positive so we have to multiply by it.
  fld    dword [ebx]
  fmul   dword [ecx]
  mov    dword [edx],edi
  fistp  word [edx+4]
  fld    dword [ebx+4]
  fmul   dword [ecx]
  fistp  word [edx+6]
  jmp near split_one_inc
flag_zero_one:
; _liminfo is zero so we just convert from float to 16 bit integers.
; and store among small signals
  fld    dword [ebx]
  mov    dword [edx+4],edi
  fistp  word [edx]
  fld    dword [ebx+4]
  fistp  word [edx+2]
  inc    ebp
  jmp near split_one_inc

_split_two
  push   eax
  push   ebx
  push   ecx
  push   edx
  push   esi
  push   edi
  push   ebp
; First fill ends with zeroes (skip signals at ends)
  xor    eax,eax
  mov    ecx,[_fft1_first_point]
  or     ecx,ecx
  jz     low_two_ok
  mov    edi,[_fft1_split_shi]
  shl    ecx,2
  cld
  rep    stosd
low_two_ok:
  mov    edi,[_fft1_size]
  dec    edi
  mov    ecx,edi
  sub    ecx,[_fft1_last_point]
  jz     high_two_ok
  shl    ecx,2
  shl    edi,4
  add    edi,[_fft1_split_shi]
  std
  rep    stosd
  cld
high_two_ok:
;***********************************************************
  mov    esi,[_fft1_first_point]
  mov    ebx,esi
  shl    ebx,2
  mov    edx,ebx
  add    ebx,[_fft1_px]
  shl    ebx,2
  add    ebx,[_fft1_float]
  shl    edx,2
  add    edx,[_fft1_split_shi]
  xor    edi,edi
  mov    ecx,esi
  sal    ecx,2
  add    ecx,[_liminfo]
  fldz
  xor    ebp,ebp
split_two_loop:
  fcom   dword [ecx]
  fnstsw    ax
  sahf
  je  near   flag_zero_two
  jb  near   flag_pos_two
; _liminfo is negative so we just convert from float to 16 bit integers.
; and store among large signals
  fld    dword [ebx]
  mov    dword [edx],edi
  mov    dword [edx+4],edi
  fistp  word [edx+8]
  fld    dword [ebx+4]
  fistp  word [edx+10]
  fld    dword [ebx+8]
  fistp  word [edx+12]
  fld    dword [ebx+12]
  fistp  word [edx+14]
split_two_inc:
  inc    esi
  add    ecx,4
  add    ebx,16
  add    edx,16
  cmp    esi,[_fft1_last_point]
  jle    split_two_loop
  jmp near splitx  
flag_pos_two:
; _liminfo is positive so we have to multiply by it.
  fld    dword [ebx]
  fmul   dword [ecx]
  mov    dword [edx],edi
  mov    dword [edx+4],edi
  fistp  word [edx+8]
  fld    dword [ebx+4]
  fmul   dword [ecx]
  fistp  word [edx+10]
  fld    dword [ebx+8]
  fmul   dword [ecx]
  fistp  word [edx+12]
  fld    dword [ebx+12]
  fmul   dword [ecx]
  fistp  word [edx+14]
  jmp near split_two_inc
flag_zero_two:
; _liminfo is zero so we just convert from float to 16 bit integers.
; and store among small signals
  fld    dword [ebx]
  mov    dword [edx+8],edi
  mov    dword [edx+12],edi
  fistp  word [edx]
  fld    dword [ebx+4]
  fistp  word [edx+2]
  fld    dword [ebx+8]
  fistp  word [edx+4]
  fld    dword [ebx+12]
  fistp  word [edx+6]
  inc    ebp
  jmp near split_two_inc



