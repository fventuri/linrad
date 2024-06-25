
#ifdef __FreeBSD__
#include <sys/types.h>
#endif

  #ifndef u_int8_t
    #ifdef uint8_t
      #define u_int8_t uint8_t
    #else 
      #define u_int8_t unsigned char
      #endif
    #endif
  #ifndef u_int16_t
    #ifdef uint16_t
      #define u_int16_t uint16_t
    #else
      #define u_int16_t unsigned short int
    #endif
  #endif
  #ifndef u_int32_t
    #ifdef uint32_t
      #define u_int32_t uint32_t
    #else
      #if(OSNUM == OSNUM_WINDOWS)
        #include <windows.h>
        #define u_int32_t DWORD
      #endif
    #endif
  #endif
