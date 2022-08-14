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

extern rawsave_tmp
extern rawsave_tmp_net
extern rawsave_tmp_disk
extern fft1_char 
extern fft1_bytes
extern rx_read_bytes
extern timf1_char
extern timf1p_pa
extern timf1p_pc_disk
extern timf1p_pc_net

global compress_rawdat_net
global compress_rawdat_disk
global expand_rawdat

section .text

compress_rawdat_disk
  push ebx
  push ecx
  push edx
  push esi
  push edi
  mov  esi,[timf1_char]
  mov  edi,[rawsave_tmp_disk]
  add  esi,[timf1p_pc_disk]
  mov  ebx,[rx_read_bytes]
; Compress 4 byte words (int) to 18 bit.
cmpr_d:
  mov  ax,[esi+2]      ;byte 2,3  of word 0
  mov  ch,[esi+1]      ;byte 1 of word 0
  mov  [edi],ax
  shr  ch,2
  mov  ax,[esi+6]      ;byte 2,3  of word 1
  mov  cl,[esi+5]      ;byte 1 of word 1
  mov  [edi+2],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[esi+10]      ;byte 2,3  of word 2
  mov  cl,[esi+ 9]      ;byte 1 of word 2
  mov  [edi+4],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[esi+14]      ;byte 2,3  of word 3
  mov  cl,[esi+13]      ;byte 1 of word 3
  and  cl,11000000B
  or   ch,cl
  mov  [edi+6],ax
  mov  [edi+8],ch
  add  esi,16
  add  edi,9
  sub  ebx,16
  ja   cmpr_d
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret

compress_rawdat_net
_compress_rawdat_net
  push ebx
  push ecx
  push edx
  push esi
  push edi
  mov  esi,[timf1_char]
  mov  edi,[rawsave_tmp_net]
  add  esi,[timf1p_pc_net]
  mov  ebx,[rx_read_bytes]
; Compress 4 byte words (int) to 18 bit.
cmpr_n:
  mov  ax,[esi+2]      ;byte 2,3  of word 0
  mov  ch,[esi+1]      ;byte 1 of word 0
  mov  [edi],ax
  shr  ch,2
  mov  ax,[esi+6]      ;byte 2,3  of word 1
  mov  cl,[esi+5]      ;byte 1 of word 1
  mov  [edi+2],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[esi+10]      ;byte 2,3  of word 2
  mov  cl,[esi+ 9]      ;byte 1 of word 2
  mov  [edi+4],ax
  and  cl,11000000B
  or   ch,cl
  shr  ch,2
  mov  ax,[esi+14]      ;byte 2,3  of word 3
  mov  cl,[esi+13]      ;byte 1 of word 3
  and  cl,11000000B
  or   ch,cl
  mov  [edi+6],ax
  mov  [edi+8],ch
  add  esi,16
  add  edi,9
  sub  ebx,16
  ja   cmpr_n
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret


expand_rawdat
  push ebx
  push ecx
  push edx
  push esi
  push edi
  mov  esi,[timf1_char]
  mov  edi,[rawsave_tmp]
  add  esi,[timf1p_pa]
  mov  ebx,[rx_read_bytes]
; Expand 18 bit packed data.
; add 0.5 bits (=bit 19) to correct for the average error
; of 0.5 bits that we introduced by truncating to 187 bits.
; This will remove the spur at frequency=0 that would otherwise
; have been introduced
expnd:
  mov  eax,[edi-2]    
  mov  ecx,[edi+6]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi],eax
  mov  eax,[edi]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi+4],eax
  mov  eax,[edi+2]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi+8],eax
  mov  eax,[edi+4]
  xor  cx,cx  
  shr  ecx,2
  mov  ax,cx
  add  eax,0x2000
  mov  [esi+12],eax
  add  esi,16
  add  edi,9
  sub  ebx,16
  ja   expnd
  pop  edi
  pop  esi
  pop  edx
  pop  ecx
  pop  ebx
  ret





