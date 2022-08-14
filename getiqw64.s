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

extern _rawsave_tmp
extern _rawsave_tmp_net
extern _rawsave_tmp_disk
extern _rx_read_bytes
extern _timf1_char
extern _timf1p_pa
extern _timf1p_pc_disk
extern _timf1p_pc_net
global _compress_rawdat_net
global _compress_rawdat_disk
global _expand_rawdat

section .text

_compress_rawdat_net
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  xor  rsi,rsi
mov rax,_timf1p_pc_net
mov esi,[rax]
;  mov  esi,[_timf1p_pc_net]

mov rax,_timf1_char
add rsi,[rax]
;  add  rsi,[_timf1_char]

mov rax,_rawsave_tmp_net
mov rdi,[rax]
;  mov  rdi,[_rawsave_tmp_net]

mov rax,_rx_read_bytes
mov ebx,[rax]
;  mov  ebx,[_rx_read_bytes]


; Compress 4 byte words (int) to 18 bit.
cmpr_d:
  mov  ax,[rsi+2]      ;byte 2,3  of word 0
  mov  ch,[rsi+1]      ;byte 1 of word 0
  mov  [rdi],ax
  shr  ch,2
  mov  ax,[rsi+6]      ;byte 2,3  of word 1
  mov  cl,[rsi+5]      ;byte 1 of word 1
  mov  [rdi+2],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[rsi+10]      ;byte 2,3  of word 2
  mov  cl,[rsi+ 9]      ;byte 1 of word 2
  mov  [rdi+4],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[rsi+14]      ;byte 2,3  of word 3
  mov  cl,[rsi+13]      ;byte 1 of word 3
  and  cl,11000000B
  or   ch,cl
  mov  [rdi+6],ax
  mov  [rdi+8],ch
  add  rsi,16
  add  rdi,9
  sub  ebx,16
  ja   cmpr_d
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret

_compress_rawdat_disk
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  xor  rsi,rsi

mov rax,_timf1p_pc_disk
mov esi,[rax]
;  mov  esi,[_timf1p_pc_disk]

mov rax,_timf1_char
add rsi,[rax]
;  add  rsi,[_timf1_char]

mov rax,_rawsave_tmp_disk
mov rdi,[rax]
;  mov  rdi,[_rawsave_tmp_disk]

mov rax,_rx_read_bytes
mov ebx,[rax]
;  mov  ebx,[_rx_read_bytes]

; Compress 4 byte words (int) to 18 bit.
cmpr_n:
  mov  ax,[rsi+2]      ;byte 2,3  of word 0
  mov  ch,[rsi+1]      ;byte 1 of word 0
  mov  [rdi],ax
  shr  ch,2
  mov  ax,[rsi+6]      ;byte 2,3  of word 1
  mov  cl,[rsi+5]      ;byte 1 of word 1
  mov  [rdi+2],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[rsi+10]      ;byte 2,3  of word 2
  mov  cl,[rsi+ 9]      ;byte 1 of word 2
  mov  [rdi+4],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[rsi+14]      ;byte 2,3  of word 3
  mov  cl,[rsi+13]      ;byte 1 of word 3
  and  cl,11000000B
  or   ch,cl
  mov  [rdi+6],ax
  mov  [rdi+8],ch
  add  rsi,16
  add  rdi,9
  sub  ebx,16
  ja   cmpr_n
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret


_expand_rawdat
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  xor  rsi,rsi
mov rax,_timf1p_pa
mov esi,[rax]
;  mov  esi,[_timf1p_pa]

mov rax,_timf1_char
add rsi,[rax]
;  add  rsi,[_timf1_char]

mov rax,_rawsave_tmp
mov rdi,[rax]
;  mov  rdi,[_rawsave_tmp]

mov rax,_rx_read_bytes
mov ebx,[rax]
;  mov  ebx,[_rx_read_bytes]
; Expand 18 bit packed data.
; add 0.5 bits (=bit 19) to correct for the average error
; of 0.5 bits that we introduced by truncating to 18 bits.
; This will remove the spur at frequency=0 that would otherwise
; have been introduced
expnd:
  mov  eax,[rdi-2]    
  mov  ecx,[rdi+6]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [rsi],eax
  mov  eax,[rdi]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [rsi+4],eax
  mov  eax,[rdi+2]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [rsi+8],eax
  mov  eax,[rdi+4]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [rsi+12],eax
  add  rsi,16
  add  rdi,9
  sub  ebx,16
  ja   expnd
  pop  rdi
  pop  rsi
  pop  rdx
  pop  rcx
  pop  rbx
  ret





