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


// ********************* sockaddr and sockaddr_in **********************
// struct sockaddr {
// unsigned short sa_family;    // address family, AF_xxx
// char sa_data[14];            // 14 bytes of protocol address
// sa_family can be a variety of things, 
// but it'll be AF_INET for everything we do 
// in this document. sa_data contains a destination 
// address and port number for the socket. 
// This is rather unwieldy since you don't 
// want to tediously pack the address in 
// the sa_data by hand.
//    
// To deal with struct sockaddr, programmers 
// created a parallel structure: 
// struct sockaddr_in ("in" for "Internet".)
// struct sockaddr_in {
// short int sin_family;         // Address family
// unsigned short int sin_port;  // Port number
// struct in_addr sin_addr;      // Internet address
// unsigned char sin_zero[8];    // Same size as struct sockaddr
// **********************************************************************                                                                

#include "osnum.h"
#include "globdef.h"
#include <ctype.h>
#ifndef _MSC_VER
  #include <unistd.h>
#endif

#if(OSNUM == OSNUM_WINDOWS)
#ifdef _MSC_VER
  #include <winsock.h> 
#else
  #include <ws2tcpip.h> 
#endif
#include <windows.h>
#include "wscreen.h"
#define RECV_FLAG 0
#if !defined __MINGW64_VERSION_MAJOR || __MINGW64_VERSION_MAJOR != 11
typedef struct{
struct in_addr imr_multiaddr;   /* IP multicast address of group */
struct in_addr imr_interface;   /* local IP address of interface */
}IP_MREQ;
#endif
extern WSADATA wsadata;
#define INVSOCK INVALID_SOCKET
#define CLOSE_FD closesocket
#define SOCKLEN_T int
#endif

#if(OSNUM == OSNUM_LINUX)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <netdb.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include "lscreen.h"
#ifdef MSG_WAITALL
#define RECV_FLAG MSG_WAITALL
#else
#define RECV_FLAG 0
#endif
#define IP_MREQ struct ip_mreq
#define INVSOCK -1
#define CLOSE_FD close
#define SOCKLEN_T socklen_t
#endif

#include "uidef.h"
#include "fft1def.h"
#include "fft2def.h"
#include "vernr.h"
#include "caldef.h"
#include "screendef.h"
#include "thrdef.h"
#include "keyboard_def.h"
#include "seldef.h"
#include "options.h"
#include "sigdef.h"


void openhpsdr_input(void)
{
}

void display_openhpsdr_parm_info(void)
{
}

#define DISCOVERY_PACKET_BYTES 60

void init_openhpsdr(void)
{
FD fd;
ssize_t net_written;
char buf[DISCOVERY_PACKET_BYTES];
struct sockaddr_in addr_discovery;
int err;
char broadcast;
broadcast=1;
clear_screen();
#if(OSNUM == OSNUM_WINDOWS)
if(WSAStartup(2, &wsadata) != 0)
  {
  lirerr(341263);
  return;   
  }
#endif
memset(buf,0,DISCOVERY_PACKET_BYTES);
buf[4]=0x02;
fd=socket(AF_INET,SOCK_DGRAM,0);
if (fd == INVSOCK)
  {
  lirerr(341249);
  return;
  }                  
addr_discovery.sin_family=AF_INET;
addr_discovery.sin_addr.s_addr=inet_addr("255.255.255.255");
addr_discovery.sin_port=htons(1024);
// Broadcast a discovery packet to 255.255.255.255
//setsockopt(fd, SO_BROADCAST, 1);
err=setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);
if ( err == -1) 
  {
  lirerr(344669);
  goto errexit;
  }
net_written=sendto(fd, buf, DISCOVERY_PACKET_BYTES, 0 , 
               (struct sockaddr *) &addr_discovery, sizeof(addr_discovery));
if(net_written != DISCOVERY_PACKET_BYTES)
  {
  lirerr(344679);
errexit:;  
  CLOSE_FD(fd);
  return;
  }
                    
fprintf( stderr,"\nOK\n");
CLOSE_FD(fd);
}
