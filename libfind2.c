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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[])
{
FILE *file;
int i,j,k,m;
int flags;
if(argc < 3)
  {
nothing:;  
  printf("0");
  return 0;
  }
i=0;
flags=0;
next:;
if(argv[2][i] == 0)
  {
  if(argv[1][0] == '0')
    {
    printf("%d",flags);
    }
  return 1;
  }
if(argv[2][i  ] == '.' && 
   argv[2][i+1] == 's' && 
   argv[2][i+2] == 'o') 
  {  
  m=i+3;
  j=i+2;
  k=j;
  while(argv[2][j+1] != '/' && argv[2][j+1] !=0)j++;
  while(argv[2][k-1] != ' ' && argv[2][k-1] != 10 && k>0)k--;
findlinux:;
  if(argv[2][i  ] == ' ' && 
     argv[2][i+1] == 'E' && 
     argv[2][i+2] == 'L' && 
     argv[2][i+3] == 'F' && 
     argv[2][i+4] == ' ' &&
     argv[2][i+5] == '6' &&
     argv[2][i+6] == '4')
    {
    while(argv[2][k-1] != ' ' && argv[2][k-1] != 10 && k>0)k--;
    if(argv[1][0] == '0')
      {  
      flags |= 2;
      }
    if(argv[1][0] == '2') 
      {
      j=m-3;
      while(argv[2][j-1] != '/' && j>4)j--;
      j--;
      while(argv[2][j-1] != '/' && j>0)j--;
      if(argv[2][j-1] == '/')
        {
#ifndef __FreeBSD__        
        if(argv[2][i-3] == 'X' && 
           argv[2][i-2] == '1' && 
           argv[2][i-1] == '1')
          {
          sprintf(&argv[2][j],"include/X11");
          }
        else  
#endif
          {
          if(argv[2][j-5] == '/' && 
             argv[2][j-4] == 'l' && 
             argv[2][j-3] == 'i' && 
             argv[2][j-2] == 'b')
            {
            j-=4;
            } 
          sprintf(&argv[2][j],"include");
          }
        printf("-I%s ",&argv[2][k]);
        }
      i+=6;
      goto next;
      }
    if(argv[1][0] == '4')
      {
      argv[2][m]=0;
      printf("%s",&argv[2][k]);
      return 0;
      }
    }
  if((argv[2][i  ] == ' ' && 
      argv[2][i+1] == 'E' && 
      argv[2][i+2] == 'L' && 
      argv[2][i+3] == 'F' && 
      argv[2][i+4] == ' ' &&
      argv[2][i+5] == '3' &&
      argv[2][i+6] == '2') ||
     (argv[2][i  ] == ' ' && 
      argv[2][i+1] == 'A' && 
      argv[2][i+2] == 'S' && 
      argv[2][i+3] == 'C' && 
      argv[2][i+4] == 'I' &&
      argv[2][i+5] == 'I' &&
      argv[2][i+6] == ' ') 
      )
    {
    if(argv[1][0] == '0')
      {  
      flags |= 1;
      }
    if(argv[1][0] == '1') 
      {
      j=m-3;
      while(argv[2][j-1] != '/' && j>4)j--;
      j--;
      while(argv[2][j-1] != '/' && j>0)j--;
      if(argv[2][j-1] == '/')
        {
#ifdef __FreeBSD__
        if(argv[2][i-7] == 'X' && 
           argv[2][i-6] == '1' && 
           argv[2][i-5] == '1')
          {
          sprintf(&argv[2][j],"include/X11");
          }
        else  
#endif
          {
          if(argv[2][j-5] == '/' && 
             argv[2][j-4] == 'l' && 
             argv[2][j-3] == 'i' && 
             argv[2][j-2] == 'b')
            {
            j-=4;
            } 
          sprintf(&argv[2][j],"include");
          }
        printf("-I%s ",&argv[2][k]);
        }
      i+=6;
      goto next;
      }
    if(argv[1][0] == '3')
      {
      argv[2][m]=0;
      printf("%s",&argv[2][k]);
      return 0;
      }
    }
  if(i < j-6)
    {
    i++;
    goto findlinux;
    }
  else
    {  
    i=j;
    }  
  }
if(argv[2][i  ] == '.' &&
   argv[2][i+1] == 'd' &&
   argv[2][i+2] == 'y' &&
   argv[2][i+3] == 'l' &&
   argv[2][i+4] == 'i' &&
   argv[2][i+5] == 'b')
  {
  m=i+6; 
  j=i+5;
  k=j;
  while(argv[2][j+1] != '/' && argv[2][j+1] !=0)j++;
  while(argv[2][k-1] != ' ' && argv[2][k-1] != 10 && k>0)k--;
findmac:;
  if(argv[2][i  ] == 'x' && 
     argv[2][i+1] == '8' && 
     argv[2][i+2] == '6' && 
     argv[2][i+3] == '_' && 
     argv[2][i+4] == '6' && 
     argv[2][i+5] == '4')
    {
    if(argv[1][0] == '0')
      {  
      flags |= 2;
      }
    if(argv[1][0] == '2') 
      {
      j=m-6;
      while(argv[2][j-1] != '/' && j>4)j--;
      j--;
      while(argv[2][j-1] != '/' && j>0)j--;
      if(argv[2][j-1] == '/')
        {
        if(argv[2][i-3] == 'X' && 
           argv[2][i-2] == '1' && 
           argv[2][i-1] == '1')
          {
          sprintf(&argv[2][j],"include/X11");
          }
        else  
          {
          if(argv[2][j-5] == '/' && 
             argv[2][j-4] == 'l' && 
             argv[2][j-3] == 'i' && 
             argv[2][j-2] == 'b')
            {
            j-=4;
            } 
          sprintf(&argv[2][j],"include");
          }
        printf("-I%s ",&argv[2][k]);
        }
      i+=6;
      goto next;
      }
    if(argv[1][0] == '4')
      {
      argv[2][m]=0;
      printf("%s",&argv[2][k]);
      return 0;
      }
    }
  if(argv[2][i  ] == 'i' && 
     argv[2][i+1] == '3' && 
     argv[2][i+2] == '8' && 
     argv[2][i+3] == '6')
    {  
    if(argv[1][0] == '0')
      {  
      flags |= 1;
      }
    if(argv[1][0] == '1') 
      {
      j=m-6;
      while(argv[2][j-1] != '/' && j>4)j--;
      j--;
      while(argv[2][j-1] != '/' && j>0)j--;
      if(argv[2][j-1] == '/')
        {
        if(argv[2][i-3] == 'X' && 
           argv[2][i-2] == '1' && 
           argv[2][i-1] == '1')
          {
          sprintf(&argv[2][j],"include/X11");
          }
        else  
          {
          if(argv[2][j-5] == '/' && 
             argv[2][j-4] == 'l' && 
             argv[2][j-3] == 'i' && 
             argv[2][j-2] == 'b')
            {
            j-=4;
            } 
          sprintf(&argv[2][j],"include");
          }
        printf("-I%s ",&argv[2][k]);
        }
      return 0;
      }
    if(argv[1][0] == '3')
      {
      argv[2][m]=0;
      printf("%s",&argv[2][k]);
      return 0;
      }
    }
  if(i < j-4)
    {
    i++;
    goto findmac;
    }
  else
    {  
    i=j;
    }  
  }
i++;  
goto next;   
}
