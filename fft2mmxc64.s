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
extern fft2_mmxcosin
extern fft2_na
extern fft2_n
extern fft2_m2
extern fft2_inc
extern mailbox
global fft2_mmx_c2
global fft2_mmx_c1

%define TMP1 ebp-4
%define TMP2 ebp-8
%define TMP3 ebp-12
%define TMP4 ebp-16
%define LIMIT ebp-20
%define INCC ebp-24
section .text

fft2_mmx_c1
_fft2_mmx_c1
  push    rbx
  push    rcx
  push    rdx
  push    rsi
  push    rdi
  push    rbp
;ö  mov     ebp,[fft2_mmxcosin]
  mov     eax,0FFFFH
  mov     [TMP1],eax
  mov     [TMP2],eax
  mov     eax,1
  mov     [TMP3],eax
  mov     [TMP4],eax
  movq    mm7,[TMP2]
  movq    mm6,[TMP4]
  xor     eax,eax
  mov     [TMP1],eax
  mov     [TMP3],eax
  dec     eax
  mov     [TMP2],eax
  mov     eax,10001H
  mov     [TMP4],eax
  movq    mm5,[TMP2]
  movq    mm4,[TMP4]
;ö  test    word [fft2_n],1
  jz near dual_last_1
;ö  mov     eax,[fft2_size]
  sal     eax,2
  mov     [LIMIT],eax
; Base of current transform.
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
;ö  mov     eax,[fft2_size]
  sal     eax,2
;ö  imul    eax,[fft2_na]
;ö  mov     esi,[fft2_short_int]
  add     esi,eax
;ö  mov     ecx,[fft2_size]
  sal     ecx,1
  xor     edi,edi
c1b:;    
;  ss1= fft2_tab[itab].sin; 
;  cs1= fft2_tab[itab].cos;
  movq    mm0,[ebp+edi]        ;mm0 =  cs1   -ss1    ss1    cs1
;  x1=fft1_tmp[4*ia  ];
;  y1=fft1_tmp[4*ia+1];
  mov    eax,[esi+ecx]
  mov    [TMP1],eax
  mov    [TMP2],eax
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm2,[TMP2]           ;mm1 =  y1     x1     y1     x1
;      t1= c1*x1+s1*y1;      
;      t2= c1*y1-s1*x1;      
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1 
  psrad   mm2,16               ;mm2 =         t2            t1 
  packssdw mm2,mm2             ;mm2 =   t2    t1     t2     t1          
;  x0=fft1_tmp[4*ia  ];
;  y0=fft1_tmp[4*ia+1];
  mov    eax,[esi]
  mov    [TMP1],eax
  mov    [TMP2],eax
  movq   mm1,[TMP2]            ;mm1 =  y0     x0     y0     x0
  psraw   mm1,1
  pxor    mm2,mm5
  paddsw  mm2,mm4              ;mm2 = -t2    -t1     t2     t1
;        zxy[ib  ]=(x0-t1)/4;
;        zxy[ib+1]=-(y0-t2)/4;
;        zxy[ia  ]=(x0+t1)/4;
;        zxy[ia+1]=-(y0+t2)/4;
  paddsw   mm1,mm2
  movq     [TMP2],mm1
  mov      eax,[TMP2]
  mov      ebx,[TMP1]
  neg      ax
  neg      bx
  mov      [esi],ebx
  mov      [esi+ecx],eax
  add      edi,8
  add      esi,4
  cmp      edi,[LIMIT]
  jb  near  c1b
  emms
  pop  rbp
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret


dual_last_1:
  xor     eax,eax
  mov     [TMP1],eax
  mov     [TMP3],eax
  dec     eax
  mov     [TMP2],eax
  movq    mm7,[TMP2]
  movq    mm6,[TMP3]
;ö  mov     eax,[fft2_size]
  sal     eax,1
  mov     [LIMIT],eax
; Base of current transform.
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
;ö  mov     eax,[fft2_size]
  sal     eax,2
;ö  imul    eax,[fft2_na]
;ö  mov     esi,[fft2_short_int]
  add     esi,eax
;ö  mov     ecx,[fft2_size]
  xor     edi,edi
c1a:;    
;    ss1= fft2_tab[2*itab].sin; 
;    cs1= fft2_tab[2*itab].cos;
  movq    mm0,[ebp+2*edi]        ;mm0 =  cs1   -ss1    ss1    cs1
;    ib=ia+m2;
;    x1=zxy[ib  ];
;    y1=zxy[ib+1];
  mov     eax,[esi+ecx]          ;ib
  mov     [TMP1],eax
  mov     [TMP2],eax
  movq    mm1,[TMP2]             ;mm1 =   y1     x1     y1      x1           
;    t1=cs1*x1+ss1*y1;      
;    t2=cs1*y1-ss1*x1; 
  pmaddwd mm1,mm0                ;mm1 = c1*y1-s1*x1   c1*x1+s1*y1 
  add     esi,ecx
                                 ;mm1 =     t2            t1  
  psrad   mm1,16                 ;mm1 =   .     t2     .    t1     
  packssdw mm1,mm1               ;mm1 =   t2    t1    t2    t1                      
  pand     mm1,mm7               ;mm1 =    0     0    t2    t1 
;    x3=zxy[id  ];
;    y3=zxy[id+1];
  mov     eax,[esi+2*ecx]        ;id
  mov     [TMP1],eax
  mov     [TMP2],eax
  movq    mm2,[TMP2]             ;mm2 =   y3     x3     y3      x3           
;    t3=cs1*x3+ss1*y3;      
;    t4=cs1*y3-ss1*x3;      
  pmaddwd mm2,mm0                ;mm2 = c1*y3-s1*x3   c1*x3+s1*y3 
  sub     esi,ecx
  psrad   mm2,16                 ;mm2 =   .     t4     .    t3
  packssdw mm2,mm2               ;mm2 =   t4    t3    t4    t3
  pand    mm2,mm6                ;mm2 =   t4    t3     0     0
  paddsw  mm1,mm2                ;mm1 =   t4    t3    t2    t1
;    x0=zxy[ia  ];
;    y0=zxy[ia+1];
;    x2=zxy[ic  ];
;    y2=zxy[ic+1];
  mov     eax,[esi]              ;ia
  mov     [TMP2],eax
  mov     ebx,[esi+2*ecx]        ;ic
  mov     [TMP1],ebx
  movq    mm0,[TMP2]             ;mm0 = y2   x2   y0   x0
  psraw   mm0,1
  movq    mm2,mm0
;    x4=x0+t1;
;    y4=y0+t2;
;    x5=x2+t3;
;    y5=y2+t4;
;  
;    x6=x0-t1;
;    y6=y0-t2;
;    x7=x2-t3;
;    y7=y2-t4;
  paddsw  mm0,mm1                ;mm0 =  y5   x5   y4   x4
  psubsw  mm2,mm1                ;mm2 =  y7   x7   y6   x6
;    ss2= fft2_tab[itab].sin; 
;    cs2= fft2_tab[itab].cos;
  movq    mm1,[ebp+edi]          ;mm1 =  cs2   -ss2    ss2    cs2
;    t5=cs2*x5+ss2*y5;
;    t6=cs2*y5-ss2*x5;
  movq    mm3,mm0                ;mm3 =  y5   x5   y4   x4
  movq    mm5,mm0                ;mm5 =  y5   x5   y4   x4                
  psraw   mm5,1  
  pand    mm3,mm6                ;mm3 =  y5   x5    0    0
  psrlq   mm0,32                 ;mm0 =   0    0   y5   x5
  paddsw  mm0,mm3                ;mm0 =  y5   x5   y5   x5
  pmaddwd mm0,mm1                ;mm0 =     t6       t5 
  movq    mm4,mm5
  psrad   mm0,16                 ;mm0 =       t6        t5
  packssdw mm0,mm0               ;mm1 =  t6   t5   t6   t5
;    zxy[ia  ]=x4+t5;
;    zxy[ia+1]=y4+t6;
;    zxy[ic  ]=x4-t5;
;    zxy[ic+1]=y4-t6;
  paddsw  mm5,mm0
  psubsw  mm4,mm0
  movq    [TMP2],mm5
  movq    [TMP4],mm4
  mov     eax,[TMP2]
  mov     ebx,[TMP4]
  neg     ax
  neg     bx
  mov     [esi],eax
  mov     [esi+2*ecx],ebx
;    t8=cs2*x7+ss2*y7;
;    t7=cs2*y7-ss2*x7;
  movq    mm5,mm2                ;mm5 =  y7   x7   y6   x6
  movq    mm4,mm2                ;mm4 =  y7   x7   y6   x6
  psraw   mm4,1
  pand    mm5,mm6                ;mm5 =  y7   x7    0    0
  psrlq   mm2,32                 ;mm2 =   0    0   y7   x7 
  paddsw  mm2,mm5                ;mm2 =  y7   x7   y7   x7
  pmaddwd mm2,mm1                ;mm2 =     t7       t8 
  add     esi,ecx
  psrad   mm2,16                 ;mm2 =   .   t7    .   t8
  movq    mm3,mm4                ;mm3 =  y7   x7   y6   x6
  movq    mm1,mm2                ;mm1 =   .   t7   .    t8
  psrlq   mm1,16                 ;mm1 =   0   .    t7    .
  psrld   mm1,16                 ;mm1 =   0    0    0   t7
  pslld   mm2,16                 ;mm2 =   .    .   t8    0 
  psubsw  mm1,mm2                ;mm1 =   .   .    t7  -t8              
;    zxy[ib  ]=x6+t7;
;    zxy[ib+1]=y6-t8;
;    zxy[id  ]=x6-t7;
;    zxy[id+1]=y6+t8;
  paddsw  mm4,mm1
  psubsw  mm3,mm1
  movq    [TMP2],mm4
  movq    [TMP4],mm3
  mov     eax,[TMP2]
  mov     ebx,[TMP4]
  neg     ax
  neg     bx
  mov     [esi],eax
  mov     [esi+2*ecx],ebx
  sub     esi,ecx
;    itab+=inc;
;    ia+=2; 
;    if(itab<siz_d4)goto xlp1;
  add       edi,8
  add       esi,4
  cmp       edi,[LIMIT]
  jb  near  c1a
zz:
  emms
  pop  rbp
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret

fft2_mmx_c2
_fft2_mmx_c2
  push    rbx
  push    rcx
  push    rdx
  push    rsi
  push    rdi
  push    rbp
;ö  mov     ebp,[fft2_mmxcosin]
  mov     eax,0FFFFH
  mov     [TMP1],eax
  mov     [TMP2],eax
  mov     eax,1
  mov     [TMP3],eax
  mov     [TMP4],eax
;ö  test    word [fft2_n],1
  jz near dual_last_2
;ö  mov     eax,[fft2_size]
  sal     eax,2
  mov     [LIMIT],eax
; Base of current transform.
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
;ö  mov     eax,[fft2_size]
  sal     eax,3
;ö  imul    eax,[fft2_na]
;ö  mov     esi,[fft2_short_int]
  add     esi,eax
;ö  mov     ecx,[fft2_size]
  sal     ecx,2
  xor     edi,edi
c2b:;    
;  ss1= fft2_tab[itab].sin; 
;  cs1= fft2_tab[itab].cos;
  movq    mm0,[ebp+edi]        ;mm0 =  cs1   -ss1    ss1    cs1
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
  movq    mm4,mm2              ;mm4 =  w1     u1     y1     x1
;      t1= c1*x1+s1*y1;      
;      t2= c1*y1-s1*x1;      
;      r1= c1*u1+s1*w1;      
;      r2= c1*w1-s1*u1;      
  punpckldq  mm2,mm2           ;mm2 =  y1     x1     y1     x1
  punpckhdq  mm4,mm4           ;mm4 =  w1     u1     w1     u1
  pmaddwd mm2,mm0              ;mm2 = c1*y1-s1*x1   c1*x1+s1*y1 
  pmaddwd mm4,mm0              ;mm4 = c1*w1-s1*u1   c1*u1+s1*w1 
  psraw   mm1,1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,16               ;mm2 =         t2            t1 
  psrad   mm4,16               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;        zxy[ib  ]=(x0-t1)/4;
;        zxy[ib+1]=-(y0-t2)/4;
;        zxy[ib+2]=(u0-r1)/4;
;        zxy[ib+3]=-(w0-r2)/4;
;        zxy[ia  ]=(x0+t1)/4;
;        zxy[ia+1]=-(y0+t2)/4;
;        zxy[ia+2]=(u0+r1)/4;
;        zxy[ia+3]=-(w0+r2)/4;
  paddsw   mm1,mm2
  psubsw   mm6,mm2
  pxor    mm1,[TMP2]
  paddsw  mm1,[TMP4]
  pxor    mm6,[TMP2]
  paddsw  mm6,[TMP4]
  movq    [esi],mm1
  movq    [esi+ecx],mm6
  add     edi,8
  add     esi,8
  cmp     edi,[LIMIT]
  jb  near  c2b
  emms
  pop  rbp
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret


dual_last_2:
;ö  mov     eax,[fft2_size]
  sal     eax,1
  mov     [LIMIT],eax
; Base of current transform.
;zxy=&fft2_short_int[2*ui.rx_channels*fft2_na*fft2_size];
;ö  mov     eax,[fft2_size]
  sal     eax,3
;ö  imul    eax,[fft2_na]
;ö  mov     esi,[fft2_short_int]
  add     esi,eax
;ö  mov     ecx,[fft2_size]
  sal     ecx,1
  xor     edi,edi
c2a:;
;jmp c2a    
;  ss1= fft2_tab[2*itab].sin; 
;  cs1= fft2_tab[2*itab].cos;
  movq    mm0,[ebp+2*edi]        ;mm0 =  cs1   -ss1    ss1    cs1
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
;  x2=zxy[ic  ];
;  y2=zxy[ic+1];
;  u2=zxy[ic+2];
;  w2=zxy[ic+3];
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
  psraw   mm1,1
  movq    mm6,mm1              ;mm6 =  w0     u0     y0     x0
  psrad   mm2,16               ;mm2 =         t2            t1 
  psrad   mm4,16               ;mm4 =         r2            r1
  packssdw mm2,mm4             ;mm2 =  r2     r1     t2     t1
;  x3=zxy[id  ];
;  y3=zxy[id+1];
;  u3=zxy[id+2];
;  w3=zxy[id+3];
  movq    mm7,[esi+2*ecx]      ;mm7 =  w3     u3     y3     x3
;      x4=x0+t1;
;      y4=y0+t2;
;      u4=u0+r1;
;      w4=w0+r2;
;
;      x6=x0-t1;
;      y6=y0-t2;
;      u6=u0-r1;
;      w6=w0-r2;
;mm0 =  c1    -s1     s1     c1
;mm1 =  w0     u0     y0     x0
;mm2 =  r2     r1     t2     t1
;mm3 =  w2     u2     y2     x2
;mm4 = ?
;mm5 = ?
;mm6 =  w0     u0     y0     x0
;mm7 =  w3     u3     y3     x3
  paddsw  mm1,mm2              ;mm1 =  w4     u4     y4     x4
  psubsw  mm6,mm2              ;mm6 =  w6     u6     y6     x6
;      t3=c1*x3+s1*y3;      
;      t4=c1*y3-s1*x3;      
;      r3=c1*u3+s1*w3;      
;      r4=c1*w3-s1*u3;      
                               ;mm0 =  c1    -s1     s1     c1
  movq    mm5,mm7              ;mm5 =  w3     u3     y3     x3
  punpckldq  mm7,mm7           ;mm7 =  y3     x3     y3     x3
  punpckhdq  mm5,mm5           ;mm5 =  w3     u3     w3     u3
  pmaddwd mm7,mm0              ;mm7 = c1*y3-s1*x3   c1*x3+s1*y3 
  pmaddwd mm5,mm0              ;mm5 = c1*w3-s1*u3   c1*u3+s1*w3 
                               ;mm7 =  t4            t3 
                               ;mm5 =  r4            r3 
  sub     esi,ecx
  psrad   mm7,16               ;mm7 =         t4            t3 
  psrad   mm5,16               ;mm5 =         r4            r3
  packssdw mm7,mm5             ;mm7 =  r4     r3     t4     t3
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
  psraw   mm3,1
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
  psrad   mm3,16               ;mm3 =         t6            t5 
  psrad   mm2,16               ;mm2 =         r6            r5
  packssdw mm3,mm2             ;mm3 =  r6     r5     t6     t5
  psraw   mm1,1
  movq    mm7,mm1              ;mm7 =  w4     u4     y4     x4 
  paddsw  mm1,mm3              ;mm1 = w4+r6  u4+r5  y4+t6  x4+t5
  psubsw  mm7,mm3              ;mm7 = w4-r6  u4-r5  y4-t6  x4-t5
  pxor    mm1,[TMP2]
  paddsw  mm1,[TMP4]
  pxor    mm7,[TMP2]
  paddsw  mm7,[TMP4]

; zxy[ia  ]=(x4+t5);
; zxy[ia+1]=-(y4+t6);
; zxy[ia+2]=(u4+r5);
; zxy[ia+3]=-(w4+r6);

; zxy[ic  ]=(x4-t5);
; zxy[ic+1]=-(y4-t6);
; zxy[ic+2]=(u4-r5);
; zxy[ic+3]=-(w4-r6);
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

;  t7=cs2*y7-ss2*x7;
;  t8=cs2*x7+ss2*y7;
;  r7=cs2*w7-ss2*u7;
;  r8=cs2*u7+ss2*w7;

;  zxy[ib  ]=(x6+t7);
;  zxy[ib+1]=-(y6-t8);
;  zxy[ib+2]=(u6+r7);
;  zxy[ib+3]=-(w6-r8);

;  zxy[id  ]=(x6-t7);
;  zxy[id+1]=-(y6+t8);
;  zxy[id+2]=(u6-r7);
;  zxy[id+3]=-(w6+r8);

  movq    mm2,mm5              ;mm2 =  w7     u7     y7     x7
  punpckldq  mm5,mm5           ;mm5 =  y7     x7     y7     x7
  punpckhdq  mm2,mm2           ;mm2 =  w7     u7     w7     u7
  pmaddwd mm5,mm0              ;mm5 = c2*y7-s2*x7   c2*x7+s2*y7 
  pmaddwd mm2,mm0              ;mm2 = c2*w7-s2*u7   c2*u7+s2*w7 
                               ;mm5 =  t7            t8    
                               ;mm2 =  r7            r8
  psrad   mm5,16               ;mm5 =         t7            t8 
  psrad   mm2,16               ;mm2 =         r7            r8
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
  psraw     mm6,1
  movq      mm7,mm6            ;mm7 =  w6     u6     y6     x6
  paddsw    mm6,mm4
  psubsw    mm7,mm4
  add       esi,ecx
  pxor    mm6,[TMP2]
  paddsw  mm6,[TMP4]
  pxor    mm7,[TMP2]
  paddsw  mm7,[TMP4]
  movq      [esi],mm6
  movq      [esi+2*ecx],mm7
  sub       esi,ecx
;    itab+=inc;
;    ia+=2; 
;    if(itab<siz_d4)goto xlp1;
  add       edi,8
  add       esi,8
  cmp       edi,[LIMIT]
  jb  near  c2a
  emms
  pop  rbp
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret

