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


// Define in screendef, but repeat here so we will detect
// any mistakes immediately
#include "osnum.h"
#include "globdef.h"
#include "screendef.h"
#define MAX_SVGA_PALETTE 59

/*
// Black and white palette for (publishing)
int color_scale[]={ 
 0, 17, 18, 19, 20, 21, 22, 23, 24, 25,
 26, 27, 28, 29, 30, 15, 15, 15, 15, 15,
 15, 15, 15};
*/

unsigned char color_scale[]={ 
0,36,37,38,39,40,41,42,43,44,
45,46,47,48,49,50,51,52,53,54,
55,15,15};


unsigned char svga_palette[]={
//    0       |      1       |     2        |     3
0x00,0x00,0x00,0x2a,0x00,0x00,0x00,0x2a,0x00,0x2a,0x2a,0x00,
//    4       |      5       |     6        |     7
0x00,0x00,0x2a,0x2a,0x00,0x2a,0x00,0x15,0x2a,0x2a,0x2a,0x2a,
//    8       |      9       |     10       |     11
0x15,0x15,0x15,0x3f,0x15,0x15,0x15,0x3f,0x15,0x3f,0x3f,0x15,
//    12      |      13      |     14       |     15
0x15,0x15,0x3f,0x3f,0x15,0x3f,0x15,0x3f,0x3f,0x3f,0x3f,0x3f,
//    16      |      17      |     18       |     19
0x00,0x17,0x00,0x05,0x05,0x05,0x08,0x08,0x08,0x0b,0x0b,0x0b,
//    20      |      21      |     22       |     23
0x0e,0x0e,0x0e,0x11,0x11,0x11,0x14,0x14,0x14,0x18,0x18,0x18,
//    24      |      25      |     26       |     27
0x1c,0x1c,0x1c,0x20,0x20,0x20,0x24,0x24,0x24,0x28,0x28,0x28,
//    28      |      29      |     30       |     31
0x2d,0x2d,0x2d,0x32,0x32,0x32,0x38,0x38,0x38,0x08,0x18,0x1a,
//    32      |      33      |     34       |     35
0x20,0x3f,0x20,0x12,0x00,0x00,0x17,0x00,0x00,0x00,0x00,0x17,
//    36      |      37      |     38       |     39
0x08,0x00,0x00,0x10,0x00,0x00,0x18,0x00,0x00,0x1a,0x0a,0x00,
//    40      |      41      |     42       |     43
0x1c,0x0f,0x00,0x1d,0x16,0x00,0x17,0x1c,0x00,0x14,0x21,0x00,
//    44      |      45      |     46       |     47
0x05,0x23,0x00,0x00,0x28,0x0a,0x00,0x28,0x1e,0x00,0x23,0x25,
//    48      |      49      |     50       |     51
0x00,0x21,0x30,0x00,0x1c,0x32,0x00,0x16,0x34,0x00,0x0f,0x3a,
//    52      |      53      |     54       |     55
0x0a,0x0a,0x3f,0x14,0x14,0x3f,0x1f,0x1f,0x3f,0x2b,0x2b,0x3f,
//    56      |      57      |     58       |     59
0x0e,0x29,0x22,0x12,0x12,   0,0x30,0x30,0x3f,0x15,0x15,   0,
0,0,0,0,0,0,0,0,0,0,0,0,
// do not forget to update MAX_SVGA_PALETTE when adding more colours

0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0};
