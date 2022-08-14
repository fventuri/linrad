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

extern _fft1_permute
extern _fft1_window
extern _timf1_int
extern _fft1_float
extern _fft1_pa
extern _timf1p_px
extern _fft1_size
extern _timf1_bytemask
extern _fft1_interleave_points
extern _yieldflag_wdsp_fft1
extern _lir_sched_yield
global _simdbulk_of_dual_dit
global _simd132_win
global _simd132_nowin
global _simd116_win
global _simd116_nowin
global _simd132_win_real
global _simd132_nowin_real
global _simd116_win_real
global _simd116_nowin_real

section .bss
%define NEG rbp-16
%define OFFS 48
%define MEM1 rsp+OFFS-8
%define MEM2 rsp+OFFS-16
%define MEM3 rsp+OFFS-24
%define MEM4 rsp+OFFS-32
%define MEM5 rsp+OFFS-40
%define MEM6 rsp+OFFS-48
%define MEM7 rsp+OFFS-56
%define MEM8 rsp+OFFS-64
%define MEM9 rsp+OFFS-72

section .text

_simd1_16_nowin
  push    rbp
  push    rdi
  push    rsi
  push    rdx
  push    rcx
  push    rbx
;ebp=_fft1_permute;
;edi=_timf1_int;
;mem3=&_fft1_float[_fft1_pa];
;ö  mov    ebp,[_fft1_permute]
;ö  mov    edi,[_timf1_int]
;ö  mov    eax,[_fft1_pa]
  sal    eax,2
;ö  add    eax,[_fft1_float]
  mov    [MEM3],eax
;ö  mov    eax,[_fft1_size]
  add    eax,eax
  mov    [MEM4],eax
;mem2=_timf1_bytemask/sizeof(int);
;mem1=_timf1p_px/sizeof(int);
;mem1=(mem1-_fft1_interleave_points*4+mem2+1)&mem2;
;ö  mov    ebx,[_timf1_bytemask]
  shr    ebx,1
  mov    [MEM2],ebx          
;ö  mov    ebx,[_timf1_bytemask]
  add    ebx,1
;ö  add    ebx,[_timf1p_px]
;ö  mov    edx,[_fft1_interleave_points]
  sal    edx,3
  sub    ebx,edx
  shr    ebx,1
  and    ebx,[MEM2]
  mov    [MEM1],ebx
;edx=0;  
  xor    edx,edx
nowin_first_16:  
;  eax=ebp[edx  ];
;  ecx=(mem1+4*eax)&mem2;
;  ebx=ebp[edx+1];
  mov      eax,[ebp+edx]
  mov      ebx,eax
  and      eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov      ecx,[MEM1]
  sal      eax,2
  add      ecx,eax         
  shr      ebx,14
  and      ecx,[MEM2]
;  a1=edi[ecx  ];
;  a2=edi[ecx+1];
;  a3=edi[ecx+2];
;  a4=edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movaps   xmm0,[MEM8]
  mov      ecx,[MEM1]
;  ecx=(mem1+4*ebx)&mem2;
  add    ecx,ebx         
  and    ecx,[MEM2]
;  b1=edi[ecx  ];
;  b2=edi[ecx+1];
;  b3=edi[ecx+2];
;  b4=edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movaps   xmm1,[MEM8]
;  eax=ebp[edx+2];
;  ebx=ebp[edx+3];
  mov    eax,[ebp+edx+4]
  mov    ebx,eax
  and    eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov    ecx,[MEM1]
  sal    eax,2
  add    ecx,eax         
  shr    ebx,14
  and    ecx,[MEM2]
;  c1=edi[ecx  ];
;  c2=edi[ecx+1];
;  c3=edi[ecx+2];
;  c4=edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movaps   xmm2,[MEM8]

;  ecx=(mem1+4*ebx)&mem2;


  mov    ecx,[MEM1]
  add    ecx,ebx         
  and    ecx,[MEM2]
;  d1=edi[ecx  ];
;  d2=edi[ecx+1];
;  d3=edi[ecx+2];
;  d4=edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movaps   xmm3,[MEM8]

;  t1=a1+b1;
;  t2=a2+b2;
;  r1=a3+b3;
;  r2=a4+b4;
;  t5=a1-b1;
;  t7=a2-b2;
;  r5=a3-b3;
;  r7=a4-b4;

  movaps xmm4,xmm0
  addps  xmm0,xmm1             ;xmm0=t1...
  subps  xmm4,xmm1             ;xmm4=t5...
  movaps xmm5,xmm2


;  t3=c1+d1;
;  t4=c2+d2;
;  r3=c3+d3;
;  r4=c4+d4;
  
  
;  t10=c1-d1;
;  t6= c2-d2;
;  r10=c3-d3;
;  r6= c4-d4;
  addps  xmm2,xmm3             ;xmm2=t3...
  subps  xmm5,xmm3             ;xmm5=t10...  
  shufps xmm5,xmm5,10110001B   ;xmm5=t6....

;  feax=mem3;
  mov    eax,[MEM3]
;  feax[4*edx  ]=t1+t3;
;  feax[4*edx+1]=t2+t4;
;  feax[4*edx+2]=r1+r3;
;  feax[4*edx+3]=r2+r4;
  
;  feax[4*edx+8]=t1-t3;
;  feax[4*edx+9]=t2-t4;
;  feax[4*edx+10]=r1-r3;
;  feax[4*edx+11]=r2-r4;
  movaps xmm1,xmm0
  addps  xmm0,xmm2
  subps  xmm1,xmm2
  movaps [eax+8*edx],xmm0
  movaps [eax+8*edx+32],xmm1    
  
;  t12=t5+t6;
;  t9=t7+t10;
;  r12=r5+r6;
;  r9=r7+r10;

;  t11=t5-t6;
;  t8=t7-t10;
;  r11=r5-r6;
;  r8=r7-r10;
  movaps xmm7,xmm4
  addps  xmm4,xmm5
  subps  xmm7,xmm5  
  movaps xmm0,xmm4
  shufps xmm4,xmm7,01111000B
  shufps xmm7,xmm0,01111000B
  shufps xmm4,xmm4,10011100B
  shufps xmm7,xmm7,10011100B

;  feax[4*edx+4]=t12;
;  feax[4*edx+5]=t8;
;  feax[4*edx+6]=r12;
;  feax[4*edx+7]=r8;
  
;  feax[4*edx+12]=t11;
;  feax[4*edx+13]=t9;
;  feax[4*edx+14]=r11;
;  feax[4*edx+15]=r9;
  movaps [eax+8*edx+16],xmm4    
  movaps [eax+8*edx+48],xmm7    
  add    edx,8
  cmp    edx,[MEM4]
  jl near nowin_first_16  
  pop      rbx
  pop      rcx
  pop      rdx
  pop      rsi
  pop      rdi
  pop      rbp
  ret       


_simd1_32_nowin
  push    rbp
  push    rdi
  push    rsi
  push    rdx
  push    rcx
  push    rbx
;ebp=_fft1_permute;
;edi=_timf1_int;
;mem3=&_fft1_float[_fft1_pa];
;ö  mov    ebp,[_fft1_permute]
;ö  mov    edi,[_timf1_int]
;ö  mov    eax,[_fft1_pa]
  sal    eax,2
;ö  add    eax,[_fft1_float]
  mov    [MEM3],eax
;ö  mov    eax,[_fft1_size]
  add    eax,eax
  mov    [MEM4],eax
;mem2=_timf1_bytemask/sizeof(int);
;mem1=_timf1p_px/sizeof(int);
;mem1=(mem1-_fft1_interleave_points*4+mem2+1)&mem2;
;ö  mov    ebx,[_timf1_bytemask]
  shr    ebx,2
  mov    [MEM2],ebx          
;ö  mov    ebx,[_timf1_bytemask]
  add    ebx,1
;ö  add    ebx,[_timf1p_px]
;ö  mov    edx,[_fft1_interleave_points]
  sal    edx,4
  sub    ebx,edx
  shr    ebx,2
  and    ebx,[MEM2]
  mov    [MEM1],ebx
;edx=0;  
  xor    edx,edx
nowin_first_32:  
;  eax=ebp[edx  ];
;  ecx=(mem1+4*eax)&mem2;
;  ebx=ebp[edx+1];
  mov      eax,[ebp+edx]
  mov      ebx,eax
  and      eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov      ecx,[MEM1]
  sal      eax,2
  add      ecx,eax         
  shr      ebx,14
  and      ecx,[MEM2]
;  a1=edi[ecx  ];
;  a2=edi[ecx+1];
;  a3=edi[ecx+2];
;  a4=edi[ecx+3];
  cvtpi2ps xmm0,[edi+4*ecx+8]
  shufps   xmm0,xmm0,01000000B
  cvtpi2ps xmm0,[edi+4*ecx]
  mov      ecx,[MEM1]
;  ecx=(mem1+4*ebx)&mem2;
  add    ecx,ebx         
  and    ecx,[MEM2]
;  b1=edi[ecx  ];
;  b2=edi[ecx+1];
;  b3=edi[ecx+2];
;  b4=edi[ecx+3];
  cvtpi2ps xmm1,[edi+4*ecx+8]
  shufps   xmm1,xmm1,01000000B
  cvtpi2ps xmm1,[edi+4*ecx]
;  eax=ebp[edx+2];
;  ebx=ebp[edx+3];
  mov    eax,[ebp+edx+4]
  mov    ebx,eax
  and    eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov    ecx,[MEM1]
  sal    eax,2
  add    ecx,eax         
  shr    ebx,14
  and    ecx,[MEM2]
;  c1=edi[ecx  ];
;  c2=edi[ecx+1];
;  c3=edi[ecx+2];
;  c4=edi[ecx+3];
  cvtpi2ps xmm2,[edi+4*ecx+8]
  shufps   xmm2,xmm2,01000000B
  cvtpi2ps xmm2,[edi+4*ecx]
;  ecx=(mem1+4*ebx)&mem2;


  mov    ecx,[MEM1]
  add    ecx,ebx         
  and    ecx,[MEM2]
;  d1=edi[ecx  ];
;  d2=edi[ecx+1];
;  d3=edi[ecx+2];
;  d4=edi[ecx+3];
  cvtpi2ps xmm3,[edi+4*ecx+8]
  shufps   xmm3,xmm3,01000000B
  cvtpi2ps xmm3,[edi+4*ecx]

;  t1=a1+b1;
;  t2=a2+b2;
;  r1=a3+b3;
;  r2=a4+b4;
;  t5=a1-b1;
;  t7=a2-b2;
;  r5=a3-b3;
;  r7=a4-b4;
movaps xmm4,xmm0
  addps  xmm0,xmm1             ;xmm0=t1...
  subps  xmm4,xmm1             ;xmm4=t5...
  movaps xmm5,xmm2


;  t3=c1+d1;
;  t4=c2+d2;
;  r3=c3+d3;
;  r4=c4+d4;
  
  
;  t10=c1-d1;
;  t6= c2-d2;
;  r10=c3-d3;
;  r6= c4-d4;
  addps  xmm2,xmm3             ;xmm2=t3...
  subps  xmm5,xmm3             ;xmm5=t10...  
  shufps xmm5,xmm5,10110001B   ;xmm5=t6....

;  feax=mem3;
  mov    eax,[MEM3]
;  feax[4*edx  ]=t1+t3;
;  feax[4*edx+1]=t2+t4;
;  feax[4*edx+2]=r1+r3;
;  feax[4*edx+3]=r2+r4;
  
;  feax[4*edx+8]=t1-t3;
;  feax[4*edx+9]=t2-t4;
;  feax[4*edx+10]=r1-r3;
;  feax[4*edx+11]=r2-r4;
  movaps xmm1,xmm0
  addps  xmm0,xmm2
  subps  xmm1,xmm2
  movaps [eax+8*edx],xmm0
  movaps [eax+8*edx+32],xmm1    
  
;  t12=t5+t6;
;  t9=t7+t10;
;  r12=r5+r6;
;  r9=r7+r10;

;  t11=t5-t6;
;  t8=t7-t10;
;  r11=r5-r6;
;  r8=r7-r10;
  movaps xmm7,xmm4
  addps  xmm4,xmm5
  subps  xmm7,xmm5  
  movaps xmm0,xmm4
  shufps xmm4,xmm7,01111000B
  shufps xmm7,xmm0,01111000B
  shufps xmm4,xmm4,10011100B
  shufps xmm7,xmm7,10011100B

;  feax[4*edx+4]=t12;
;  feax[4*edx+5]=t8;
;  feax[4*edx+6]=r12;
;  feax[4*edx+7]=r8;
  
;  feax[4*edx+12]=t11;
;  feax[4*edx+13]=t9;
;  feax[4*edx+14]=r11;
;  feax[4*edx+15]=r9;
  movaps [eax+8*edx+16],xmm4    
  movaps [eax+8*edx+48],xmm7    
  add    edx,8
  cmp    edx,[MEM4]
  jl near nowin_first_32;  
  pop      rbx
  pop      rcx
  pop      rdx
  pop      rsi
  pop      rdi
  pop      rbp
  ret       

_simd1_16_win
  push    rbp
  push    rdi
  push    rsi
  push    rdx
  push    rcx
  push    rbx
;ebp=_fft1_permute;
;esi=_fft1_window;
;edi=_timf1_int;
;mem3=&_fft1_float[_fft1_pa];
;ö  mov    ebp,[_fft1_permute]
;ö  mov    esi,[_fft1_window]
;ö  mov    edi,[_timf1_int]
;ö  mov    eax,[_fft1_pa]
  sal    eax,2
;ö  add    eax,[_fft1_float]
  mov    [MEM3],eax
;ö  mov    eax,[_fft1_size]
  add    eax,eax
  mov    [MEM4],eax
;mem2=_timf1_bytemask/sizeof(int);
;mem1=_timf1p_px/sizeof(int);
;mem1=(mem1-_fft1_interleave_points*4+mem2+1)&mem2;
;ö  mov    ebx,[_timf1_bytemask]
  shr    ebx,1


  mov    [MEM2],ebx          
;ö  mov    ebx,[_timf1_bytemask]
  add    ebx,1
;ö  add    ebx,[_timf1p_px]
;ö  mov    edx,[_fft1_interleave_points]
  sal    edx,3
  sub    ebx,edx
  shr    ebx,1
  and    ebx,[MEM2]
  mov    [MEM1],ebx
;edx=0;  
  xor    edx,edx
win_first_16:  
;  eax=ebp[edx  ];
;  ecx=(mem1+4*eax)&mem2;
;  ebx=ebp[edx+1];
  mov      eax,[ebp+edx]
  mov      ebx,eax
  and      eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov      ecx,[MEM1]
  sal      eax,2
  add      ecx,eax         
  shr      ebx,14
  and      ecx,[MEM2]
;  a1=esi[eax]*edi[ecx  ];
;  a2=esi[eax]*edi[ecx+1];
;  a3=esi[eax]*edi[ecx+2];
;  a4=esi[eax]*edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movss    xmm1,[esi+eax]
  movaps   xmm0,[MEM8]
  mov      ecx,[MEM1]
  shufps   xmm1,xmm1,0
  mulps    xmm0,xmm1               ;xmm0=a
;  ecx=(mem1+4*ebx)&mem2;
  add    ecx,ebx         
  and    ecx,[MEM2]
;  b1=esi[ebx]*edi[ecx  ];
;  b2=esi[ebx]*edi[ecx+1];
;  b3=esi[ebx]*edi[ecx+2];
;  b4=esi[ebx]*edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movss    xmm2,[esi+ebx]
  movaps   xmm1,[MEM8]
  shufps   xmm2,xmm2,0
  mulps    xmm1,xmm2               ;xmm1=b
;  eax=ebp[edx+2];
;  ebx=ebp[edx+3];
  mov    eax,[ebp+edx+4]
  mov    ebx,eax
  and    eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov    ecx,[MEM1]
  sal    eax,2
  add    ecx,eax         
  shr    ebx,14
  and    ecx,[MEM2]
;  c1=esi[eax]*edi[ecx  ];
;  c2=esi[eax]*edi[ecx+1];
;  c3=esi[eax]*edi[ecx+2];
;  c4=esi[eax]*edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movss    xmm3,[esi+eax]
  movaps   xmm2,[MEM8]
  shufps   xmm3,xmm3,0
  mulps    xmm2,xmm3               ;xmm2=c

;  ecx=(mem1+4*ebx)&mem2;


  mov    ecx,[MEM1]
  add    ecx,ebx         
  and    ecx,[MEM2]
;  d1=esi[ebx]*edi[ecx  ];
;  d2=esi[ebx]*edi[ecx+1];
;  d3=esi[ebx]*edi[ecx+2];
;  d4=esi[ebx]*edi[ecx+3];
  fild     word[edi+2*ecx]
  fstp   dword  [MEM8]
  fild     word[edi+2*ecx+2]
  fstp   dword  [MEM7]
  fild     word[edi+2*ecx+4]
  fstp   dword  [MEM6]
  fild     word[edi+2*ecx+6]
  fstp   dword  [MEM5]
  movss    xmm4,[esi+ebx]
  movaps   xmm3,[MEM8]
  shufps   xmm4,xmm4,0
  mulps    xmm3,xmm4               ;xmm3=d

;  t1=a1+b1;
;  t2=a2+b2;
;  r1=a3+b3;
;  r2=a4+b4;
;  t5=a1-b1;
;  t7=a2-b2;
;  r5=a3-b3;
;  r7=a4-b4;

  movaps xmm4,xmm0
  addps  xmm0,xmm1             ;xmm0=t1...
  subps  xmm4,xmm1             ;xmm4=t5...
  movaps xmm5,xmm2


;  t3=c1+d1;
;  t4=c2+d2;
;  r3=c3+d3;
;  r4=c4+d4;
  
  
;  t10=c1-d1;
;  t6= c2-d2;
;  r10=c3-d3;
;  r6= c4-d4;
  addps  xmm2,xmm3             ;xmm2=t3...
  subps  xmm5,xmm3             ;xmm5=t10...  
  shufps xmm5,xmm5,10110001B   ;xmm5=t6....

;  feax=mem3;
  mov    eax,[MEM3]
;  feax[4*edx  ]=t1+t3;
;  feax[4*edx+1]=t2+t4;
;  feax[4*edx+2]=r1+r3;
;  feax[4*edx+3]=r2+r4;
  
;  feax[4*edx+8]=t1-t3;
;  feax[4*edx+9]=t2-t4;
;  feax[4*edx+10]=r1-r3;
;  feax[4*edx+11]=r2-r4;
  movaps xmm1,xmm0
  addps  xmm0,xmm2
  subps  xmm1,xmm2
  movaps [eax+8*edx],xmm0
  movaps [eax+8*edx+32],xmm1    
  
;  t12=t5+t6;
;  t9=t7+t10;
;  r12=r5+r6;
;  r9=r7+r10;

;  t11=t5-t6;
;  t8=t7-t10;
;  r11=r5-r6;
;  r8=r7-r10;
  movaps xmm7,xmm4
  addps  xmm4,xmm5
  subps  xmm7,xmm5  
  movaps xmm0,xmm4
  shufps xmm4,xmm7,01111000B
  shufps xmm7,xmm0,01111000B
  shufps xmm4,xmm4,10011100B
  shufps xmm7,xmm7,10011100B

;  feax[4*edx+4]=t12;
;  feax[4*edx+5]=t8;
;  feax[4*edx+6]=r12;
;  feax[4*edx+7]=r8;
  
;  feax[4*edx+12]=t11;
;  feax[4*edx+13]=t9;
;  feax[4*edx+14]=r11;
;  feax[4*edx+15]=r9;
  movaps [eax+8*edx+16],xmm4    
  movaps [eax+8*edx+48],xmm7    
  add    edx,8
  cmp    edx,[MEM4]
  jl near win_first_16  
  pop      rbx
  pop      rcx
  pop      rdx
  pop      rsi
  pop      rdi
  pop      rbp
  ret       

_simd1_32_win 
  push    rbp
  push    rdi
  push    rsi
  push    rdx
  push    rcx
  push    rbx
;ebp=_fft1_permute;
;esi=_fft1_window;
;edi=_timf1_int;
;mem3=&_fft1_float[_fft1_pa];
;ö  mov    ebp,[_fft1_permute]
;ö  mov    esi,[_fft1_window]
;ö  mov    edi,[_timf1_int]
;ö  mov    eax,[_fft1_pa]
  sal    eax,2
;ö  add    eax,[_fft1_float]
  mov    [MEM3],eax
;ö  mov    eax,[_fft1_size]
  add    eax,eax
  mov    [MEM4],eax
;mem2=_timf1_bytemask/sizeof(int);
;mem1=_timf1p_px/sizeof(int);
;mem1=(mem1-_fft1_interleave_points*4+mem2+1)&mem2;
;ö  mov    ebx,[_timf1_bytemask]
  shr    ebx,2
  mov    [MEM2],ebx          
;ö  mov    ebx,[_timf1_bytemask]
  add    ebx,1
;ö  add    ebx,[_timf1p_px]
;ö  mov    edx,[_fft1_interleave_points]
  sal    edx,4
  sub    ebx,edx
  shr    ebx,2
  and    ebx,[MEM2]
  mov    [MEM1],ebx
;edx=0;  
  xor    edx,edx
win_first_32:  
;  eax=ebp[edx  ];
;  ecx=(mem1+4*eax)&mem2;
;  ebx=ebp[edx+1];
  mov      eax,[ebp+edx]
  mov      ebx,eax
  and      eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov      ecx,[MEM1]
  sal      eax,2
  add      ecx,eax         
  shr      ebx,14
  and      ecx,[MEM2]
;  a1=esi[eax]*edi[ecx  ];
;  a2=esi[eax]*edi[ecx+1];
;  a3=esi[eax]*edi[ecx+2];
;  a4=esi[eax]*edi[ecx+3];
  cvtpi2ps xmm0,[edi+4*ecx+8]
  shufps   xmm0,xmm0,01000000B
  cvtpi2ps xmm0,[edi+4*ecx]
  mov      ecx,[MEM1]
  movss    xmm1,[esi+eax]
  shufps   xmm1,xmm1,0
  mulps    xmm0,xmm1               ;xmm0=a
;  ecx=(mem1+4*ebx)&mem2;
  add    ecx,ebx         
  and    ecx,[MEM2]
;  b1=esi[ebx]*edi[ecx  ];
;  b2=esi[ebx]*edi[ecx+1];
;  b3=esi[ebx]*edi[ecx+2];
;  b4=esi[ebx]*edi[ecx+3];
  cvtpi2ps xmm1,[edi+4*ecx+8]
  shufps   xmm1,xmm1,01000000B
  cvtpi2ps xmm1,[edi+4*ecx]
  movss    xmm2,[esi+ebx]
  shufps   xmm2,xmm2,0
  mulps    xmm1,xmm2               ;xmm1=b
;  eax=ebp[edx+2];
;  ebx=ebp[edx+3];
  mov    eax,[ebp+edx+4]
  mov    ebx,eax
  and    eax,0FFFFH
;  ecx=(mem1+4*eax*sizeof(int))&mem2;
  mov    ecx,[MEM1]
  sal    eax,2
  add    ecx,eax         
  shr    ebx,14
  and    ecx,[MEM2]
;  c1=esi[eax]*edi[ecx  ];
;  c2=esi[eax]*edi[ecx+1];
;  c3=esi[eax]*edi[ecx+2];
;  c4=esi[eax]*edi[ecx+3];
  cvtpi2ps xmm2,[edi+4*ecx+8]
  shufps   xmm2,xmm2,01000000B
  cvtpi2ps xmm2,[edi+4*ecx]
  movss    xmm3,[esi+eax]
  shufps   xmm3,xmm3,0
  mulps    xmm2,xmm3               ;xmm2=c

;  ecx=(mem1+4*ebx)&mem2;


  mov    ecx,[MEM1]
  add    ecx,ebx         
  and    ecx,[MEM2]
;  d1=esi[ebx]*edi[ecx  ];
;  d2=esi[ebx]*edi[ecx+1];
;  d3=esi[ebx]*edi[ecx+2];
;  d4=esi[ebx]*edi[ecx+3];
  cvtpi2ps xmm3,[edi+4*ecx+8]
  shufps   xmm3,xmm3,01000000B
  cvtpi2ps xmm3,[edi+4*ecx]
  movss    xmm4,[esi+ebx]
  shufps   xmm4,xmm4,0
  mulps    xmm3,xmm4               ;xmm3=d

;  t1=a1+b1;
;  t2=a2+b2;
;  r1=a3+b3;
;  r2=a4+b4;
;  t5=a1-b1;
;  t7=a2-b2;
;  r5=a3-b3;
;  r7=a4-b4;

  movaps xmm4,xmm0
  addps  xmm0,xmm1             ;xmm0=t1...
  subps  xmm4,xmm1             ;xmm4=t5...
  movaps xmm5,xmm2


;  t3=c1+d1;
;  t4=c2+d2;
;  r3=c3+d3;
;  r4=c4+d4;
  
  
;  t10=c1-d1;
;  t6= c2-d2;
;  r10=c3-d3;
;  r6= c4-d4;
  addps  xmm2,xmm3             ;xmm2=t3...
  subps  xmm5,xmm3             ;xmm5=t10...  
  shufps xmm5,xmm5,10110001B   ;xmm5=t6....

;  feax=mem3;
  mov    eax,[MEM3]
;  feax[4*edx  ]=t1+t3;
;  feax[4*edx+1]=t2+t4;
;  feax[4*edx+2]=r1+r3;
;  feax[4*edx+3]=r2+r4;
  
;  feax[4*edx+8]=t1-t3;
;  feax[4*edx+9]=t2-t4;
;  feax[4*edx+10]=r1-r3;
;  feax[4*edx+11]=r2-r4;
  movaps xmm1,xmm0
  addps  xmm0,xmm2
  subps  xmm1,xmm2
  movaps [eax+8*edx],xmm0
  movaps [eax+8*edx+32],xmm1    
  
;  t12=t5+t6;
;  t9=t7+t10;
;  r12=r5+r6;
;  r9=r7+r10;

;  t11=t5-t6;
;  t8=t7-t10;
;  r11=r5-r6;
;  r8=r7-r10;
  movaps xmm7,xmm4
  addps  xmm4,xmm5
  subps  xmm7,xmm5  
  movaps xmm0,xmm4
  shufps xmm4,xmm7,01111000B
  shufps xmm7,xmm0,01111000B
  shufps xmm4,xmm4,10011100B
  shufps xmm7,xmm7,10011100B

;  feax[4*edx+4]=t12;
;  feax[4*edx+5]=t8;
;  feax[4*edx+6]=r12;
;  feax[4*edx+7]=r8;
  
;  feax[4*edx+12]=t11;
;  feax[4*edx+13]=t9;
;  feax[4*edx+14]=r11;
;  feax[4*edx+15]=r9;
  movaps [eax+8*edx+16],xmm4    
  movaps [eax+8*edx+48],xmm7    
  add    edx,8
  cmp    edx,[MEM4]
  jl near win_first_32;  
  pop      rbx
  pop      rcx
  pop      rdx
  pop      rsi
  pop      rdi
  pop      rbp
  ret       

_simdbulk_of_dual_dit
  push    rbp
  push    rdi
  push    rsi
  push    rdx
  push    rcx
  push    rbx
  sub     rsp,OFFS
;MEM5=n-2;
;MEM6=4*size;
;ecx=size/16;
;edi=size/4;
;esi=16;
;ebp=tab;
  mov    rax,rdi   ;size
  mov    rbx,rdx   ;x
  mov    rdx,rsi   ;n
  mov    rbp,rcx   ;cosin table
  mov    rdi,rax       ;
  mov    rcx,rax       ; 
  mov    [MEM8],rax    ;MEM8=size
  sub    rdx,2         ;edx=n-2 
  shl    rax,4         ;eax=size*16
  add    rdi,rdi       ;edi=size*sizeof(COSIN_TABLE)/4=size*2
  mov    [MEM5],rdx    ;[MEM5]=n-2  
  shr    rcx,1         ;ecx=size*sizeof(COSIN_TABLE)/16=size/2
  mov    [MEM6],rax    ;[MEM6]=size*sizeof(float)*4=size*16
  mov    rsi,16*4      ;esi=16*sizeof(float)
  mov    [MEM7],rbx    ;[MEM7]=x
loop1:
;    edx=0;
  xor    rdx,rdx
;ö  cmp    edx,dword[_yieldflag_wdsp_fft1]
  jz     no_yield
  push   rcx
  push   rbp
  push   rsi
  push   rdi
  call   _lir_sched_yield
  pop    rdi
  pop    rsi
  pop    rbp
  pop    rcx
no_yield:
loop2:
;    eax=0;
;    ebx=x+edx;
  xor    rax,rax
  mov    rbx,[MEM7]
  add    rbx,rdx
loop3:    
;    ss1= ebp[2*eax].sin; 
;    cs1= ebp[2*eax].cos;
;    b0=ebx[+esi  ];
;    b1=ebx[+esi+1];
;    b2=ebx[+esi+2];
;    b3=ebx[+esi+3];
;
;    c0=cs1*b0+ss1*b1;      
;    c1=cs1*b1-ss1*b0;      
;    c2=cs1*b2+ss1*b3;      
;    c3=cs1*b3-ss1*b2;      
  movaps   xmm1,[rbp+2*rax]
  movaps   xmm0,xmm1
  shufps   xmm1,xmm1,0           ;xmm1=sin
  shufps   xmm0,xmm0,01010101B   ;xmm0=cos
  xorps    xmm1,[NEG]            ;signs adjusted 
  movaps   xmm4,[rbx+rsi]

  movaps   xmm2,xmm0
  movaps   xmm3,xmm1
  shufps   xmm4,xmm4,10110001B
  mulps    xmm0,[rbx+rsi]        ;xmm1=cs1*b
  mulps    xmm1,xmm4             ;xmm0=ss1*b
  addps    xmm0,xmm1             ;xmm0=c  xmm1=?   

;    a0=ebx[ 0];
;    a1=ebx[+1];
;    a2=ebx[+2];
;    a3=ebx[+3];
;
;    d0=a0+c0;
;    d1=a1+c1;
;    d2=a2+c2;
;    d3=a3+c3;
;
;    e0=a0-c0;
;    e1=a1-c1;
;    e2=a2-c2;
;    e3=a3-c3;
  movaps  xmm1,[rbx]
  subps   xmm1,xmm0              ;xmm1=e
  addps   xmm0,[rbx]             ;xmm0=d 
;    ebx+=esi;
;    g0=ebx[2*esi  ];
;    g1=ebx[2*esi+1];
;    g2=ebx[2*esi+2];
;    g3=ebx[2*esi+3];
;    ebx-=esi;
;    h0=cs1*g0+ss1*g1;      
;    h1=cs1*g1-ss1*g0;      
;    h2=cs1*g2+ss1*g3;      
;    h3=cs1*g3-ss1*g2; 
  add     rbx,rsi
  movaps   xmm4,[rbx+2*rsi]
  shufps   xmm4,xmm4,10110001B
     
  mulps  xmm2,[rbx+2*rsi]        ;cs1*g
  mulps  xmm3,xmm4               ;ss1*g
  sub    rbx,rsi
  addps  xmm2,xmm3               ;xmm2=h  xmm3=?
;    f0=ebx[2*esi  ];
;    f1=ebx[2*esi+1];
;    f2=ebx[2*esi+2];
;    f3=ebx[2*esi+3];
;
;    p0=f0+h0;
;    p1=f1+h1;
;    p2=f2+h2;
;    p3=f3+h3;
; 
;    z0=f0-h0;
;    z1=f1-h1;
;    z2=f2-h2;
;    z3=f3-h3;
;    ss2= ebp[eax].sin; 
;    cs2= ebp[eax].cos;

  movaps   xmm3,[rbp+rax]
  movaps   xmm6,xmm3
  shufps   xmm3,xmm3,0           ;xmm3=sin
  shufps   xmm6,xmm6,01010101B   ;xmm6=cos
  xorps    xmm3,[NEG]            ;signs adjusted 
  movaps   xmm4,[rbx+2*rsi]      ;xmm4=f
  movaps   xmm7,xmm6             ;xmm7=cos
  subps    xmm4,xmm2             ;xmm4=z
  addps    xmm2,[rbx+2*rsi]      ;xmm2=p  
;    t5=cs2*p0+ss2*p1;
;    t6=cs2*p1-ss2*p0;
;    r5=cs2*p2+ss2*p3;
;    r6=cs2*p3-ss2*p2;
  mulps    xmm7,xmm2
  add     rax,rcx
  shufps   xmm2,xmm2,10110001B
  mulps    xmm2,xmm3
  addps    xmm2,xmm7             ;xmm2=t5  xmm3=sin  xmm7=?
;    t8=cs2*z0+ss2*z1;
;    t7=cs2*z1-ss2*z0;
;    r8=cs2*z2+ss2*z3;
;    r7=cs2*z3-ss2*z2;
  mulps    xmm6,xmm4  
  shufps   xmm4,xmm4,10110001B
  mulps    xmm3,xmm4
  addps    xmm3,xmm6             ;xmm3=t8
;    ebx[ 0]=d0+t5;
;    ebx[+1]=d1+t6;
;    ebx[+2]=d2+r5;
;    ebx[+3]=d3+r6;
  movaps   xmm4,xmm0             ;xmm0=d
  addps    xmm0,xmm2
  xorps    xmm3,[NEG]      
  movaps   [rbx],xmm0
;    ebx[2*esi  ]=d0-t5;
;    ebx[2*esi+1]=d1-t6;
;    ebx[2*esi+2]=d2-r5;
;    ebx[2*esi+3]=d3-r6;
  subps   xmm4,xmm2
  add      rbx,rsi
  shufps   xmm3,xmm3,10110001B
  movaps   [rbx+rsi],xmm4
;    ebx[+esi  ]=e0+t7;
;    ebx[+esi+1]=e1-t8;
;    ebx[+esi+2]=e2+r7;
;    ebx[+esi+3]=e3-r8;
;
;    ebx+=esi;
;    ebx[+2*esi  ]=e0-t7;
;    ebx[+2*esi+1]=e1+t8;
;    ebx[+2*esi+2]=e2-r7;
;    ebx[+2*esi+3]=e3+r8;
;    ebx-=esi;
  movaps   xmm5,xmm1         ;e
  addps    xmm1,xmm3
  subps    xmm5,xmm3
  movaps   [rbx],xmm5
  movaps   [rbx+2*rsi],xmm1
  sub      rbx,rsi
  add      rbx,16
;    eax+=ecx;
;    ebx+=4; 
;    if(eax<edi)goto loop3;
  cmp      rax,rdi
  jl near  loop3 
;    edx+=4*esi;
;    if( edx<MEM6)goto loop2; 
  lea      rdx,[rdx+4*rsi]
  cmp      rdx,[MEM6]
  jl near  loop2
;    ecx/=4;
;    esi*=4;
;    edx=MEM5;
;    edx-=2;
;    MEM5=edx;
;    if(edx>2)goto loop1;
  shr      rcx,2
  shl      rsi,2
  mov      rdx,[MEM5]
  sub      rdx,2
  mov      [MEM5],rdx
  cmp      rdx,2
  ja near  loop1
; The last loop is single or dual.
; The dual loop is treated separately because frequency=0 is shifted from
; the mid point to the first point.
  jz near last_dual



;  ebx=size/2;
;  m=size/2;
;  for(eax=0; eax<m; eax++)
;    {
  mov      rdi,[MEM7]          ;edi=x
  xor      rax,rax             ;eax=0 

  mov      rcx,[MEM8]           
  sal      rcx,2               ;ecx=siz*sizeof(COSIN_TABLE)/2=4*siz 
  mov      rbx,rcx
  lea      rbx,[2*rbx+rdi]
ls1:
;    s1= tab[eax].sin; 
;    c1= tab[eax].cos;
; ** a **
;    x0=x[4*eax  ];
;    y0=x[4*eax+1];
;    u0=x[4*eax+2];
;    w0=x[4*eax+3];
; ** b **
;    x1=x[4*ebx  ];
;    y1=x[4*ebx+1];
;    u1=x[4*ebx+2];
;    w1=x[4*ebx+3];
; ** c **
;    t1=c1*x1+s1*y1;
;    t2=c1*y1-s1*x1;
;    r1=c1*u1+s1*w1;
;    r2=c1*w1-s1*u1;
  movups   xmm1,[rbp+rax]
  movaps   xmm0,xmm1
  shufps   xmm1,xmm1,0           ;xmm1=sin
  shufps   xmm0,xmm0,01010101B   ;xmm0=cos
  xorps    xmm1,[NEG]            ;signs adjusted 
  movaps   xmm4,[rbx]
  movaps   xmm2,xmm0
  movaps   xmm3,xmm1
  shufps   xmm4,xmm4,10110001B
  mulps    xmm0,[rbx]            ;xmm1=cs1*b
  mulps    xmm1,xmm4             ;xmm0=ss1*b
  addps    xmm0,xmm1             ;xmm0=c  xmm1=?   
; ** d **
;    d0=x0+t1
;    d1=y0+t2
;    d2=u0+r1
;    d3=w0+r2
; ** e **
;    e0=x0-t1;
;    e1=y0-t2;
;    e2=u0-r1;
;    e3=w0-r2;
  movaps  xmm1,[2*rax+rdi]
  subps   xmm1,xmm0                    ;xmm1=e
  addps   xmm0,[2*rax+rdi]             ;xmm0=d 
  xorps   xmm1,[NEG]
  xorps   xmm0,[NEG]
  movaps  [2*rax+rdi],xmm1
  movaps  [rbx],xmm0
;    x[4*eax  ]=x0-t1;
;    x[4*eax+1]=-(y0-t2);
;    x[4*eax+2]=u0-r1;
;    x[4*eax+3]=-(w0-r2);
;    x[4*ebx  ]=x0+t1;
;    x[4*ebx+1]=-(y0+t2);
;    x[4*ebx+2]=u0+r1;
;    x[4*ebx+3]=-(w0+r2);
;    ebx++;
  add      rax,8
  add      rbx,16
  cmp      rax,rcx
  jb       ls1  
  jmp  near simdasm_x
; The same as dual loops with output points exchanged
; Note that sin/cos data is no longer aligned so we use movups 
; rather than movaps for one fetch here (we only step by 2 fp numbers now)
last_dual:
  xor    rdx,rdx
ld2:
  xor    rax,rax
  mov    rbx,[MEM7]
  add    rbx,rdx
ld3:    
  movaps   xmm1,[rbp+2*rax]
  movaps   xmm0,xmm1
  shufps   xmm1,xmm1,0           ;xmm1=sin
  shufps   xmm0,xmm0,01010101B   ;xmm0=cos
  xorps    xmm1,[NEG]            ;signs adjusted 
  movaps   xmm4,[rbx+rsi]
  movaps   xmm2,xmm0
  movaps   xmm3,xmm1
  shufps   xmm4,xmm4,10110001B
  mulps    xmm0,[rbx+rsi]        ;xmm1=cs1*b
  mulps    xmm1,xmm4             ;xmm0=ss1*b
  addps    xmm0,xmm1             ;xmm0=c  xmm1=?   
  movaps   xmm1,[rbx]
  subps    xmm1,xmm0              ;xmm1=e
  addps    xmm0,[rbx]             ;xmm0=d 
  add      rbx,rsi
  movaps   xmm4,[rbx+2*rsi]
  shufps   xmm4,xmm4,10110001B
  mulps    xmm2,[rbx+2*rsi]        ;cs1*g
  mulps    xmm3,xmm4               ;ss1*g
  sub      rbx,rsi
  addps    xmm2,xmm3               ;xmm2=h  xmm3=?
  movups   xmm3,[rbp+rax]
  movaps   xmm6,xmm3
  shufps   xmm3,xmm3,0           ;xmm3=sin
  shufps   xmm6,xmm6,01010101B   ;xmm6=cos
  xorps    xmm3,[NEG]            ;signs adjusted 
  movaps   xmm4,[rbx+2*rsi]      ;xmm4=f
  movaps   xmm7,xmm6             ;xmm7=cos
  subps    xmm4,xmm2             ;xmm4=z
  addps    xmm2,[rbx+2*rsi]      ;xmm2=p  
  mulps    xmm7,xmm2
  add      rax,rcx
  shufps   xmm2,xmm2,10110001B
  mulps    xmm2,xmm3
  addps    xmm2,xmm7             ;xmm2=t5  xmm3=sin  xmm7=?
  mulps    xmm6,xmm4  
  shufps   xmm4,xmm4,10110001B
  mulps    xmm3,xmm4
  addps    xmm3,xmm6             ;xmm3=t8
  movaps   xmm4,xmm0             ;xmm0=d
  addps    xmm0,xmm2
  xorps    xmm3,[NEG]      
  xorps    xmm0,[NEG]         
  movaps   [rbx+2*rsi],xmm0    
  subps    xmm4,xmm2
  shufps   xmm3,xmm3,10110001B
  xorps    xmm4,[NEG]        
  movaps   [rbx],xmm4                
  movaps   xmm5,xmm1         ;e
  addps    xmm1,xmm3
  add      rbx,rsi
  subps    xmm5,xmm3
  xorps    xmm1,[NEG]             
  xorps    xmm5,[NEG]             
;; since all 4 outputs are sign reversed some time
;; can be saved by reversing sign of sin and cos
;; not important - loop is not often used.
  movaps   [rbx],xmm1             
  movaps   [rbx+2*rsi],xmm5    
  sub      rbx,rsi
  add      rbx,16
  cmp      rax,rdi
  jl near  ld3 
  lea      rdx,[rdx+4*rsi]
  cmp      rdx,[MEM6]
  jl near  ld2
simdasm_x:
  add      rsp,OFFS
  pop      rbx
  pop      rcx
  pop      rdx
  pop      rsi
  pop      rdi
  pop      rbp
  ret       

;; Dummy entries
_simd1_32_win_real:
_simd1_32_nowin_real:
_simd1_16_win_real:
_simd1_16_nowin_real:
  ret
