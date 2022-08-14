
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


section .text
;

     global check_mmx

check_mmx: 

 push ebx
 push ecx
 push edx
 pushfd
 pop  eax           ;get EFLAGS into eax
 mov  edx,eax
;         10987654321098765432109876543210
 xor  eax,00000000001000000000000000000000B
 push eax
 popfd
 pushfd
 pop  eax
 xor  eax,edx
 and  eax,00000000001000000000000000000000B
 jz   chkcpux    ; if we can not change ID flag, cpuid not supported 
 xor  eax,eax
 cpuid
 dec  eax
 jns  id_ok
 xor  eax,eax    ; Not Pentium or above
 jmp  chkcpux
id_ok: mov  eax,1
 cpuid
 and  edx,11100000000000000000000000B
 mov  eax,edx
 shr  eax,23 
chkcpux: pop edx
 pop  ecx
 pop  ebx
 ret