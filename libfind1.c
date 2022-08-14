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
#include <sys/types.h>
#include <dirent.h>

char *dirs[]={"/lib/i386-linux-gnu",
              "/lib/x86_64-linux-gnu",
              "/lib",
              "/lib32",
              "/lib64",
              "/usr/lib/i386-linux-gnu",
              "/usr/lib/x86_64-linux-gnu",
              "/usr/lib32/lib",
              "/usr/lib64/lib",
              "/usr/lib32",
              "/usr/lib64",
              "/usr/local/lib",
              "/usr/local/lib32",
              "/usr/local/lib64",
              "/usr/local32/lib",
              "/usr/lib",
              "/emul/ia32-linux/lib",
              "/emul/ia32-linux/usr/lib",              
              "/usr/PCBSD/local/lib",
              "/usr/X11R6/lib", 
              "/usr/X11/lib",
              "/opt/local/lib",
              "/usr/lib/arm-linux-gnueabihf",
              "/usr/lib/aarch64-linux-gnu",
              "X"};

int main(int argc, char *argv[])
{
FILE *file;
int i,k;
char s[256], buf[256];
if(argc < 2)
  {
nothing:;  
  printf("0");
  return 0;
  }
i=0;  
while(dirs[i][0] != 'X')
  {
  sprintf(s,"%s/%s",dirs[i],argv[1]);
  file=fopen(s,"r");
  if(file != NULL)
    {
    fclose(file);
    sprintf(s,"file -L %s/%s",dirs[i],argv[1]);
    system(s); 
#if DARWIN == 1
    sprintf(s,"lipo -info %s/%s",dirs[i],argv[1]);
    system(s); 
#endif
    }
  i++;  
  }
return 1;  
}
