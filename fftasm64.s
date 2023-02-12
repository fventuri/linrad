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

extern lir_sched_yield
global asmbulk_of_dual_dif
global asmbulk_of_dif

TMP1 equ 0
TMP2 equ 4
TMP4 equ 8
TMP5 equ 12
TMP6 equ 16
TMP7 equ 20
TMP8 equ 24
TMP9 equ 28
TMP10 equ 32
TMP11 equ 36
section .text 



asmbulk_of_dual_dif
_asmbulk_of_dual_dif
  push   rbp
  mov    rbp,rsp  
  push   rdi
  push   rsi
  push   rdx
  push   rcx
  push   rbx
  sub    rsp,5CH
  mov    eax,[ebp+24]  ;yield_flag
  mov    [TMP11+esp],eax
  mov    eax,[ebp+8]   ;size
  mov    edi,[ebp+12]  ;n
  shl    eax,1
  mov    ebx,[ebp+16]  ;x
  sub    edi,2
  mov    ecx,[ebp+20]  ;cosin table
  mov    [TMP5+esp],eax
  shl    eax,1
  mov    [TMP4+esp],edi
  mov    [TMP1+esp],eax
  shl    eax,1
  mov    [TMP6+esp],ebx
  mov    ebp,eax
  mov    [TMP10+esp],ecx
  mov    dword[TMP2+esp],16
  mov    dword[TMP7+esp],0
  mov    [TMP4+esp],edi
L$1:
  cmp    dword [TMP11+esp],0
  jz     LNY1
  push   rbp
  call   lir_sched_yield
  pop    rbp
LNY1:
  mov    eax,[TMP5+esp]
  xor    edx,edx
  sal    eax,3
  sub    eax,ebp
  mov    dword [TMP9+esp],eax
  mov    eax,dword [TMP1+esp]
  mov    dword [TMP8+esp],eax
L$2:
  mov    esi,edx
  mov    edi,dword [TMP1+esp]
  add    esi,dword [TMP6+esp]
  add    edi,esi
  mov    eax,dword [TMP8+esp]
  mov    ebx,dword [TMP10+esp] 
  add    eax,esi
  mov    ecx,dword [TMP2+esp]
L$3:                               ;st0 st1 st2 st3 st4 st5 st6 st7      

    fld       dword [esi]          ; A   
    fsub      dword [eax]          ; U 
    fld       dword [esi]          ; A   A  
    fadd      dword [eax]          ; a   U 
    fstp      dword [esi]          ; U  
    fld       st0                  ; U   U
    fld       dword [4+esi]        ; C   U   U  
    fsub      dword [4+eax]        ; V   U   U
    fld       dword [4+esi]        ; C   V   U   U 
    fadd      dword [4+eax]        ; b   V   U   U
    fstp      dword [4+esi]        ; V   U   U
    fld       st0                  ; V   V   U   U
    fld       dword[ebx]           ;sin  V   V   U   U
    fmul      st4,st0              ;sin  V   V   U   Us
    fmulp     st2,st0              ; V   Vs  U   Us
    fld       dword[4+ebx]         ;cos  V   Vs  U   Us 
    fmul      st3,st0              ;cos  V   Vs  Uc  Us
    fmulp     st1,st0              ; Vc  Vs  Uc  Us
    faddp     st3,st0              ; Vs  Uc  (Vc+Us)
    fsubp     st1,st0
    fstp      dword[eax]
    fstp      dword[4+eax]

;***********************
    fld       dword [esi+8]          ; A   
    fsub      dword [eax+8]          ; U 
    fld       dword [esi+8]          ; A   A  
    fadd      dword [eax+8]          ; a   U 
    fstp      dword [esi+8]          ; U  
    fld       st0                  ; U   U
    fld       dword [4+esi+8]        ; C   U   U  
    fsub      dword [4+eax+8]        ; V   U   U
    fld       dword [4+esi+8]        ; C   V   U   U 
    fadd      dword [4+eax+8]        ; b   V   U   U
    fstp      dword [4+esi+8]        ; V   U   U
    fld       st0                  ; V   V   U   U
    fld       dword[ebx]           ;sin  V   V   U   U
    fmul      st4,st0              ;sin  V   V   U   Us
    fmulp     st2,st0              ; V   Vs  U   Us
    fld       dword[4+ebx]         ;cos  V   Vs  U   Us 
  add   ebx,ecx
    fmul      st3,st0              ;cos  V   Vs  Uc  Us
    fmulp     st1,st0              ; Vc  Vs  Uc  Us
  add   eax,16
    faddp     st3,st0              ; Vs  Uc  (Vc+Us)
    fsubp     st1,st0
  add   esi,16
    fstp      dword[eax-8]
    fstp      dword[4+eax-8]
  cmp   esi,edi
  jl    near   L$3
  add   edx,ebp
  cmp   edx,dword [TMP9+esp]
  jle   near  L$2
  shr   ebp,1
  shr   dword [TMP1+esp],1
  shl   dword [TMP2+esp],1
  mov   edi,dword [TMP7+esp]
  inc   edi
  mov   dword [TMP7+esp],edi
  cmp   edi,dword [TMP4+esp]
  jl    near L$1
  add   rsp,5CH
  pop   rbx
  pop   rcx
  pop   rdx
  pop   rsi
  pop   rdi
  pop   rbp
  ret       
;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

;!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


asmbulk_of_dif
_asmbulk_of_dif equ asmbulk_of_dif
  push   rbp
  mov    rbp,rsp  
  push   rdi
  push   rsi
  push   rdx
  push   rcx
  push   rbx
  sub    rsp,5CH
  mov    eax,[ebp+24]  ;yield_flag
  mov    [TMP11+esp],eax
  mov    eax,[ebp+8]   ;size
  mov    edi,[ebp+12]  ;n
  mov    ebx,[ebp+16]  ;x
  sub    edi,2
  mov    ecx,[ebp+20]  ;cosin table
  mov    [TMP5+esp],eax
  shl    eax,1 
  mov    [TMP4+esp],edi
  mov    [TMP1+esp],eax
  shl    eax,1
  mov    [TMP6+esp],ebx
  mov    ebp,eax
  mov    [TMP10+esp],ecx
  mov    dword[TMP2+esp],16
  mov    dword[TMP7+esp],0
  mov    [TMP4+esp],edi
M$1:
  cmp    dword [TMP11+esp],0
  jz     MNY1
  push   rbp
  call   lir_sched_yield
  pop    rbp
MNY1:
  mov    eax,[TMP5+esp]
  xor    edx,edx
  sal    eax,3
  sub    eax,ebp
  mov    dword [TMP9+esp],eax
  mov    eax,dword [TMP1+esp]
  mov    dword [TMP8+esp],eax
M$2:
  mov    esi,edx
  mov    edi,dword [TMP1+esp]
  add    esi,dword [TMP6+esp]
  add    edi,esi
  mov    eax,dword [TMP8+esp]
  mov    ebx,dword [TMP10+esp] 
  add    eax,esi
  mov    ecx,dword [TMP2+esp]
M$3:                               ;st0 st1 st2 st3 st4 st5 st6 st7      
    fld       dword [esi]          ; A   
    fsub      dword [eax]          ; U 
    fld       dword [esi]          ; A   A  
    fadd      dword [eax]          ; a   U 
    fstp      dword [esi]          ; U  
    fld       st0                  ; U   U
    fld       dword [4+esi]        ; C   U   U  
    fsub      dword [4+eax]        ; V   U   U
    fld       dword [4+esi]        ; C   V   U   U 
    fadd      dword [4+eax]        ; b   V   U   U
    fstp      dword [4+esi]        ; V   U   U
    fld       st0                  ; V   V   U   U
    fld       dword[ebx]           ;sin  V   V   U   U
    fmul      st4,st0              ;sin  V   V   U   Us
    fmulp     st2,st0              ; V   Vs  U   Us
    fld       dword[4+ebx]         ;cos  V   Vs  U   Us 
  add   ebx,ecx
    fmul      st3,st0              ;cos  V   Vs  Uc  Us
  add   eax,8
    fmulp     st1,st0              ; Vc  Vs  Uc  Us
    faddp     st3,st0              ; Vs  Uc  (Vc+Us)
  add   esi,8
    fsubp     st1,st0
    fstp      dword[-8+eax]
    fstp      dword[-4+eax]


  cmp   esi,edi
  jl    near   M$3
  add   edx,ebp
  cmp   edx,dword [TMP9+esp]
  jle   near  M$2
  shr   ebp,1
  shr   dword [TMP1+esp],1
  shl   dword [TMP2+esp],1
  mov   edi,dword [TMP7+esp]
  inc   edi
  mov   dword [TMP7+esp],edi
  cmp   edi,dword [TMP4+esp]
  jl    near M$1
  add   rsp,5CH
  pop   rbx
  pop   rcx
  pop   rdx
  pop   rsi
  pop   rdi
  pop   rbp

ret

