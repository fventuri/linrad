AC_PREREQ(2.50)
setterm -term linux -back white
setterm -term linux -fore black
AC_INIT(xmain.c)
AC_MSG_CHECKING([OS])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([ft245.c])
BITS=`(getconf LONG_BIT)`
AC_SUBST(BITS)
OS=`uname`
AC_SUBST(OS, "${OS}")
MACHINE=$(uname -m)
CPU="CPU_INTEL"
if test "$MACHINE" = "armv5l"; then
  CPU="CPU_ARM"
fi
if test "$MACHINE" = "armv6l"; then
  CPU="CPU_ARM"
fi
if test "$MACHINE" = "armv7l"; then
  CPU="CPU_ARM"
fi
if test "$MACHINE" = "aarch64"; then
  CPU="CPU_ARM"
fi
if test "$MACHINE" = "armv8l"; then
  echo "The architecture is armv8l which should support 64 bit code."
  echo "Please send a mail to me: <leif at sm5bsz.com>"
  echo "I will send some test packages to figure out how to"
  echo "add whatever might be needed to compile Linrad"
  exit
  CPU="CPU_ARM"
fi
if test "$MACHINE" = "arm64"; then
# Apple ARM64 architecture
  CPU="CPU_ARM"
fi
AC_SUBST(CPU)
if test "$CPU" = "CPU_INTEL"; then
  ASMLIB32='fftasm.ol getiq.ol fft2mmxa.ol fft2mmxb.ol fft2mmxc.ol simdasm.ol hware.ol split.ol'
  ASMLIB64='fftasm.om getiq.om fft2mmxa.om fft2mmxb.om fft2mmxc.om simdasm.om split.om'
else
  ASMLIB32="csplit.ol"
  ASMLIB64="csplit.om"
fi
AC_SUBST(ASMLIB32)
AC_SUBST(ASMLIB64)
# *******************************************************************
if test "$BITS" = "32"; then
  TXT32=""
else
  TXT32="(32bit)"
fi
ELF32=""
ELF64=""
if test ${OS} = "Darwin"; then
  S_OS_TYPE="OS_WINDOWS"
  LIBEXT="dylib"
  DARWIN="-DDARWIN=1 -D__STDC_VERSION__=0L"
else
  if test ${OS} != "FreeBSD"; then
    ldconfig /lib32 /lib64 /usr/lib32 /usr/lib64 /usr/lib32/lib /usr/local/lib \
    /usr/local32/lib /emul/ia32-linux/lib /emul/ia32-linux/usr/lib /usr/X11/lib \
    /usr/PCBSD/local/lib /usr/lib/i386-linux-gnu /usr/lib/x86_64-linux-gnu \
    /usr/X11R6/lib /opt/local/lib /usr/lib/arm-linux-gnueabihf \
    /usr/local/lib64
  fi
  ELF64="-felf64"
  LIBEXT="so"
  S_OS_TYPE="OS_LINUX"
  DARWIN="-DDARWIN=0"
  if test "$BITS" = "32"; then
    ELF32=-felf
  else
    ELF32=-felf32
  fi
fi
AC_SUBST(S_OS_TYPE)
# Find out whether we have gcc
if test "${CC}" = ""; then
  CC=gcc
fi
AC_PATH_PROG(HAVE_GCC, ${CC} )
if test "$HAVE_GCC" = ""; then
  echo -------------------------------------------------------
  echo Can not find ${CC} , the GNU C compiler.
  echo Install gcc and development libraries. Then run configure again.
  echo Debian, Ubuntu: apt-get install gcc
  echo Old Fedora: yum install gcc
  echo Fedora 22 and later: dnf install gcc
  echo Mandriva, Mageia: urpmi gcc
  echo SuSE: zypper install gcc
  echo Sabayon: equo install sys-devel/gcc
  exit
fi
${CC} $DARWIN libfind1.c -c -o libfind1.ol 2> x.x
chmod 777 x.x
Y=`cat x.x`
if test "$Y" != ""; then
  DARWIN="-DDARWIN=1"
  ${CC} $DARWIN libfind1.c -c -o libfind1.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    echo -------------------------------------------------------
    echo $Y
    echo -------------------------------------------------------
    echo The gcc compiler is installed but does not work.
    echo The reason is probably missing header files.
    echo Debian, Ubuntu: apt-get install libc6-dev
    echo Old Fedora: yum install gcc
    echo Fedora 22 and later: dnf install gcc
    echo SuSE: zypper install gcc
    echo Sabayon: equo install sys-devel/gcc
    echo -------------------------------------------------------
    exit
  fi
fi
AC_SUBST(DARWIN)
M32="-m32"
M64="-m64"
# Check that we have -m32 and -m64
if test "$BITS" = "32"; then
  ${CC} -m32 csplit.c -c -o x.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    M32=""
  fi
else
  ${CC} -m64 csplit.c -c -o x.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    M64=""
  fi
fi
NOPIE="-no-pie"
${CC} csplit.c -no-pie -c -o x.ol 2> x.x
chmod 777 x.x
Y=`cat x.x`
if test "$Y" != ""; then
  NOPIE=""
fi
AC_SUBST(M32)
AC_SUBST(M64)
${CC} lxsys.c -DHAVE_PARPORT=1 -c -o x.ol 2> x.x
HAVE_PARPORT="-DHAVE_PARPORT=1"
chmod 777 x.x
Y=`cat x.x`
if test "$Y" != ""; then
  HAVE_PARPORT="-DHAVE_PARPORT=0"
fi
AC_SUBST(HAVE_PARPORT)
# ***********************************************************************
# Make the libfind files that we use to figure out whether libraries
# are 32 bit ELF or 64 bit ELF.
${CC} $DARWIN -o libfind1 libfind1.ol -lm 2>x.x
Y=`cat x.x`
chmod 777 x.x
if test "$Y" != ""; then
  echo -------------------------------------------------------
  echo $Y
  echo -------------------------------------------------------
  echo The gcc compiler is installed and works, but the linker does not work.
  echo a symbolic limk may help: 
  echo ln /lib/libgcc_s.${LIBEXT}.1 /lib/libgcc_s.${LIBEXT}
  echo -------------------------------------------------------
  exit
fi
${CC} libfind2.c -c -o libfind2.ol 2> x.x
chmod 777 x.x
Y=`cat x.x`
if test "$Y" != ""; then
  echo -------------------------------------------------------
  echo $Y
  echo -------------------------------------------------------
  exit
fi 
${CC} -o libfind2 libfind2.ol -lm 2>x.x
chmod 777 x.x
Y=`cat x.x`
if test "$Y" != ""; then
  echo -------------------------------------------------------
  echo $Y
  echo -------------------------------------------------------
  exit
fi 
# *******************************************************************
# Find out whether we have nasm
if test "$CPU" = "CPU_INTEL"; then
  AC_PATH_PROG(NASM, nasm)
  if test "$NASM" = ""; then
    echo -------------------------------------------------------
    echo Can not find NASM, the Netwide Assembler.
    echo Install nasm and run configure again.
    echo Debian, Ubuntu,: apt-get install nasm
    echo PClinuxOS: apt-get install nasm
    echo Old Fedora: yum install nasm
    echo Fedora 22 and later: dnf install nasm
    echo SuSE: zypper install nasm
    echo Mandriva,Mageia: urpmi nasm
    echo Gentoo: emerge nasm
    echo Sabayon: equo install nasm
    echo You may also get it from the NASM Internet site.
    echo -------------------------------------------------------
    exit
  fi
else
  ELF32="UNUSED"
  ELF64="UNUSED"
fi
if test "$LIBEXT" = "dylib"; then
  ELF32=""
  nasm -w-orphan-labels -felf -s -D OS_WINDOWS getiq.s -o getiq.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    ELF32=-felf
  fi
  nasm -w-orphan-labels -fmacho -s -D OS_WINDOWS getiq.s -o getiq.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    ELF32=-fmacho
  fi
  nasm -w-orphan-labels -fmacho32 -s -D OS_WINDOWS getiq.s -o getiq.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    ELF32=-fmacho32
  fi
  ELF64=""
  nasm -w-orphan-labels -fmacho64 -s -D OS_WINDOWS getiq64.s -o getiq64.om 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    ELF64=-fmacho64
  fi
fi
AC_SUBST(ELF32)
AC_SUBST(ELF64)
# *******************************************************************
# Find out whether we have make
AC_PATH_PROG(MAKE, make)
if test "$MAKE" = ""; then
  echo -------------------------------------------------------
  echo Can not find make which will be needed to build Linrad.
  echo Install make and run configure again.
  echo Debian, Ubuntu: apt-get install make
  echo oldFedora: yum install make
  echo Fedora 22 and later: dnf install make
  echo Mandriva: urpmi make.
  echo SuSE: zypper install make
  echo -------------------------------------------------------
  exit
fi
AC_PATH_PROG(HAVE_FILE, file)
if test "$HAVE_FILE" = ""; then
  echo -------------------------------------------------------
  echo Can not find file which will be needed to find libraries for hardware.
  echo Install file and run configure again.
  echo Debian, Ubuntu: apt-get install file
  echo -------------------------------------------------------
  exit
fi
AC_PATH_PROG(HAVE_WGET, wget)
AC_PATH_PROG(HAVE_PKG_CONFIG, pkg-config)
AC_PATH_PROG(HAVE_LIBTOOL, libtoolize)
AC_PATH_PROG(HAVE_AUTOMAKE, automake)
AC_PATH_PROG(HAVE_CMAKE, cmake)
AC_PATH_PROG(HAVE_GIT, git)
AC_PATH_PROG(HAVE_SUBVERSION, svn)
AC_PATH_PROG(HAVE_XXD, xxd)
AC_PATH_PROG(HAVE_CPP, g++)
AC_PATH_PROG(HAVE_RPM, rpm)
AC_PATH_PROG(HAVE_REALPATH, realpath)
AC_PATH_PROG(HAVE_PANDOC, pandoc)
AC_PATH_PROG(HAVE_LIBUSB_DEV, libusb-dev)
HELP="0"
AC_ARG_WITH(help,,HELP="1")
AC_ARG_WITH(x11-32,,HELP="2")
AC_ARG_WITH(xext-32,,HELP="3")
AC_ARG_WITH(svgalib-32,,HELP="4")
AC_ARG_WITH(ALSA-32,,HELP="5")
AC_ARG_WITH(libusb1-32,,HELP="6")
AC_ARG_WITH(perseus-32,,HELP="7")
AC_ARG_WITH(rtlsdr-32,,HELP="8")
AC_ARG_WITH(mirics-32,,HELP="9")
AC_ARG_WITH(bladerf-32,,HELP="10")
AC_ARG_WITH(libwd_dask-32,,HELP="11")
AC_ARG_WITH(FDM-S1-32,,HELP="12")
AC_ARG_WITH(airspy-32,,HELP="13")
AC_ARG_WITH(soft66-32,,HELP="14")
AC_ARG_WITH(udev-32,,HELP="15")
AC_ARG_WITH(portaudio-32,,HELP="16")
AC_ARG_WITH(airspyhf-32,,HELP="17")
AC_ARG_WITH(libusb0-32,,HELP="18")
AC_ARG_WITH(cufft-32,,HELP="19")
AC_ARG_WITH(clfft-header-32,,HELP="20")
AC_ARG_WITH(clfft-32,,HELP="21")
AC_ARG_WITH(opencl-32,,HELP="22")
AC_ARG_WITH(libftdi-32,,HELP="23")
AC_ARG_WITH(sdrplay2-32,,HELP="24")
AC_ARG_WITH(sdrplay3-32,,HELP="25")

AC_ARG_WITH(gcc32,,HELP="999")

AC_ARG_WITH(x11-64,,HELP="1002")
AC_ARG_WITH(xext-64,,HELP="1003")
AC_ARG_WITH(svgalib-64,,HELP="1004")
AC_ARG_WITH(ALSA-64,,HELP="1005")
AC_ARG_WITH(libusb1-64,,HELP="1006")
AC_ARG_WITH(perseus-64,,HELP="1007")
AC_ARG_WITH(rtlsdr-64,,HELP="1008")
AC_ARG_WITH(mirics-64,,HELP="1009")
AC_ARG_WITH(bladerf-64,,HELP="1010")
AC_ARG_WITH(libwd_dask-64,,HELP="1011")
AC_ARG_WITH(FDM-S1-64,,HELP="1012")
AC_ARG_WITH(airspy-64,,HELP="1013")
AC_ARG_WITH(soft66-64,,HELP="1014")
AC_ARG_WITH(udev-64,,HELP="1015")
AC_ARG_WITH(portaudio-64,,HELP="1016")
AC_ARG_WITH(airspyhf-64,,HELP="1017")
AC_ARG_WITH(libusb0-64,,HELP="1018")
AC_ARG_WITH(cufft-64,,HELP="1019")
AC_ARG_WITH(clfft-header-64,,HELP="1020")
AC_ARG_WITH(clfft-64,,HELP="1021")
AC_ARG_WITH(opencl-64,,HELP="1022")
AC_ARG_WITH(libftdi-64,,HELP="1023")
AC_ARG_WITH(sdrplay2-64,,HELP="1024")
AC_ARG_WITH(sdrplay3-64,,HELP="1025")

MAKE_SERVER="0"
AC_ARG_WITH(server,,MAKE_SERVER="1")
AC_SUBST(MAKE_SERVER)
LLINRAD_EXE="linrad.exe"
AC_SUBST(LLINRAD_EXE)
LLINRAD64_EXE="linrad64.exe"
AC_SUBST(LLINRAD64_EXE)
WLINRAD_EXE="for_Windows"
AC_SUBST(WLINRAD_EXE)
WLINRAD64_EXE="for_Windows64"
AC_SUBST(WLINRAD64_EXE)
rm -f Makefile
rm -f config.cache
SDEVPATH=""
OSSN="0"
OSSINC32=""
if test ${OS} = "Linux"; then
  # ************************************************************
  # Find out where 4Front OSS is installed and set path to the
  # device files for OSS and legacy sound.
  AC_CHECK_FILE( /usr/lib/oss/soundcard.h, OSSINC32="/usr/lib/oss")
  AC_CHECK_FILE( /usr/lib/oss/include/sys/soundcard.h, OSSINC32="/usr/lib/oss/include/sys")
  AC_CHECK_FILE( /usr/local/lib/oss/soundcard.h, OSSINC32="/usr/local/lib/oss")
  AC_CHECK_FILE( /usr/local/lib/oss/include/sys/soundcard.h, OSSINC32="/usr/local/lib/oss/include/sys")
  AC_CHECK_FILE( /lib/oss/soundcard.h, OSSINC32="/lib/oss")
  AC_CHECK_FILE( /lib/oss/include/sys/soundcard.h, OSSINC32="/lib/oss/include/sys/soundcard.h")
  AC_CHECK_FILE( /var/lib/oss/soundcard.h, OSSINC32="/var/lib/oss")
  AC_CHECK_FILE( /var/lib/oss/include/sys/soundcard.h, OSSINC32="/var/lib/oss/include/sys")
  AC_CHECK_FILE( /opt/oss/soundcard.h, OSSINC32="/opt/oss")
  AC_CHECK_FILE( /opt/oss/include/sys/soundcard.h, OSSINC32="/opt/oss/include/sys")
  AC_CHECK_FILE( /root/oss/soundcard.h, OSSINC32="/root/oss")
  AC_CHECK_FILE( /root/oss/include/sys/soundcard.h, OSSINC32="/root/oss/include/sys")
  AC_CHECK_FILE( /dev/dsp, SDEVPATH="/dev/dsp")
  AC_CHECK_FILE( /dev/sound/dsp, SDEVPATH="/dev/sound/dsp")
  # ********************************************************************
  # Find out where to list the init scripts to use when booting Linux
  # These will be used to install svgalib_helper, the SDR-14 driver 
  # and possibly other hardware related drivers to be added.
  INIT="/etc/rc.d"
  AC_CHECK_FILE( /etc/rc3.d/., INIT="/etc")
  AC_CHECK_FILE( /etc/init.d/rc3.d/., INIT="/etc/init.d")
  AC_CHECK_FILE( /etc/rc.d/rc3.d/., INIT="/etc/rc.d")
  AC_SUBST(INIT)
fi
OSSINC64=OSSINC32
# ***********************************************************************
# Find out if the user has any files of his own that we should include.
LUSERHWDR="-DLUSERS_ROUTINES_PRESENT=0"
AC_CHECK_FILE( ./users_hwaredriver.c, LUSERHWDR="-DLUSERS_ROUTINES_PRESENT=1")
AC_SUBST(LUSERHWDR)
AC_CHECK_FILE(./users_hwaredriver.c, LDEPS1="users_hwaredriver.c")
AC_SUBST(LDEPS1)
WUSERHWDR="-DWUSERS_ROUTINES_PRESENT=0"
AC_CHECK_FILE(./wusers_hwaredriver.c, WUSERHWDR="-DWUSERS_ROUTINES_PRESENT=1")
AC_SUBST(WUSERHWDR)
AC_CHECK_FILE(./wusers_hwaredriver.c, WDEPS1="wusers_hwaredriver.c")
AC_SUBST(WDEPS1)
LUSERHWDEF="-DLUSERS_DEF=0"
AC_CHECK_FILE(./users_hwaredef.h, LUSERHWDEF="-DLUSERS_DEF=1")
AC_SUBST(LUSERHWDEF)
AC_CHECK_FILE(./users_hwaredef.h, DEPS2="users_hwaredef.h")
AC_SUBST(DEPS2)
WUSERHWDEF="-DWUSERS_DEF=0"
AC_CHECK_FILE( ./wusers_hwaredef.h, WUSERHWDEF="-DWUSERS_DEF=1")
AC_SUBST(WUSERHWDEF)
AC_CHECK_FILE( ./wusers_hwaredef.h, WDEPS4="wusers_hwaredef.h")
AC_SUBST(WDEPS4)
USEREXTRA="-DUSERS_EXTRA_PRESENT=0"
AC_CHECK_FILE( ./users_extra.c, USEREXTRA="-DUSERS_EXTRA_PRESENT=1")
AC_SUBST(USEREXTRA)
AC_CHECK_FILE( ./users_extra.c, DEPS3="users_extra.c")
AC_SUBST(DEPS3)
#!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# Things we need to cross compile for 32 bit Windows under Linux.
WLIN32_OK="1"
MINGWPATH=" "
MINGWBIN="mingw32"
#*********************************************************************
#********************************************************************
# Check whether we have mingw32, or at least some of it. 
MINGW32_OK="0"
AC_CHECK_FILE(/usr/bin/i686-w64-mingw32-gcc,MINGW32_DEB="1" , MINGW32_DEB="0")
if test "$MINGW32_DEB" = "1"; then
  AC_CHECK_FILE(/usr/i686-w64-mingw32/include/math.h,DI="1",DI="0")
  if test "$DI" = "1"; then
    MINGWBIN="/usr/bin/i686-w64-mingw32-gcc"
    MINGWPATH="/usr/i686-w64-mingw32/"
    MINGW32_OK="1"
  fi
fi
AC_CHECK_FILE(/usr/bin/i586-mingw32msvc-gcc,MINGW32_DEB="1" , MINGW32_DEB="0")
if test "$MINGW32_DEB" = "1"; then
  AC_CHECK_FILE(/usr/i586-mingw32msvc/include/math.h,DI="1",DI="0")
  if test "$DI" = "1"; then
    MINGWBIN="/usr/bin/i586-mingw32msvc-gcc"
    MINGWPATH="/usr/i586-mingw32msvc/"
    MINGW32_OK="1"
    fi 
  fi
AC_CHECK_FILE(/usr/bin/i586-pc-mingw32-gcc,MINGW32_MDV="1" , MINGW32_MDV="0")
if test "$MINGW32_MDV" = "1"; then
  AC_CHECK_FILE(/usr/i586-pc-mingw32/sys-root/mingw/include/math.h,MI="1",MI="0")
  if test "$MI" = "1"; then
  MINGWBIN="/usr/bin/i586-pc-mingw32-gcc"
  MINGWPATH="/usr/i586-pc-mingw32/sys-root/mingw/"
  MINGW32_OK="1"
  fi
fi
AC_CHECK_FILE(/usr/bin/i686-pc-mingw32-gcc,MINGW32_FD1="1" , MINGW32_FD1="0")
if test "$MINGW32_FD1" = "1"; then
  AC_CHECK_FILE(/usr/i686-pc-mingw32/sys-root/mingw/include/math.h,FI1="1",FI1="0")
  if test "$FI1" = "1"; then
    MINGWBIN="/usr/bin/i686-pc-mingw32-gcc"
    MINGWPATH="/usr/i686-pc-mingw32/sys-root/mingw/"
    MINGW32_OK="1"
  fi
fi
AC_CHECK_FILE(/usr/bin/i686-w64-mingw32-gcc,MINGW32_FD2="1" , MINGW32_FD2="0")
if test "$MINGW32_FD2" = "1"; then
  AC_CHECK_FILE(/usr/i686-w64-mingw32/sys-root/mingw/include/math.h,FI2="1",FI2="0")
  if test "$FI2" = "1"; then
    MINGWBIN=/usr/bin/i686-w64-mingw32-gcc;
    MINGWPATH="/usr/i686-w64-mingw32/sys-root/mingw/"
    MINGW32_OK="1"
  fi
fi
if test "$MINGW32_OK" = "0"; then
# Everything failed.
  WLIN32_OK="0"
fi
#*********************************************************************
AC_SUBST(MINGWPATH)
AC_SUBST(MINGWBIN)
AC_SUBST(WLIN32_OK)  
AC_SUBST(MINGW32_OK)
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
# The remaining tests are for Linux and/or FreeBSD
# *************************************************************************
# This section sets defines we need to maintain backwards compatibility.
XFLUSH_OK=1
LIBRT=""
if test ${OS} = "Linux"; then
  LIBRT=-lrt
  kerntot=`(uname -r) | cut -d- -f-1`
  kernver=`(echo $kerntot) | cut -d. -f-3`
  case `(uname -r) | cut -d. -f-2` in
  "2.2")
  SDR1422=sdr14
  SDR1424=no_use
  SDR1426=no_use2
  SDR1430=no_use3
  MODEXT=o
  ;;
  "2.4")
  SDR1422=no_use2
  SDR1424=sdr14
  SDR1426=no_use
  SDR1430=no_use3
  MODEXT=o
  OSVER=1
  ;;
  "2.6")
  SDR1426=sdr14
  SDR1424=no_use
  SDR1422=no_use2
  SDR1430=no_use3
  MODEXT=ko
  case `(echo $kerntot) | cut -d. -f-3` in
  "2.6.1")
  FT245PGM=ERROR
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.2")
  FT245PGM=ERROR
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.3")
  FT245PGM=ERROR
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.4")
  OSVER=2
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.5")
  OSVER=2
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.6")
  OSVER=4
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.7")
  OSVER=4
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.8")
  KERNEL_SUBDIRS=SUBDIRS
  case `(uname -r)` in
  "2.6.8-24-default")
  OSVER=4
  ;;
  *)
  OSVER=2
  ;;
  esac
  ;;
  "2.6.9")
  OSVER=4
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.10")
  OSVER=4
  KERNEL_SUBDIRS=SUBDIRS
  ;;
  "2.6.11")
  OSVER=4
  KERNEL_SUBDIRS=SUBDIRS

# skip older versions!!
  ;;
  "2.6.12")
  OSVER=4
  KERNEL_SUBDIRS=M
  ;;
  "2.6.13")
  OSVER=4
  KERNEL_SUBDIRS=M
  ;;
  "2.6.14")
  OSVER=4
  KERNEL_SUBDIRS=M
  ;;
  "2.6.15")
  OSVER=4
  KERNEL_SUBDIRS=M
  ;;
  "2.6.16")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.17")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.18")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.19")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.20")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.21")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.22")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.23")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.24")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.25")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.26")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.27")
  OSVER=8
  KERNEL_SUBDIRS=M
  ;;
  "2.6.28")
  OSVER=8
  KERNEL_SUBDIRS=M
  XFLUSH_OK=0
  LIBRT=""
  ;;
  "2.6.29")
  OSVER=8
  KERNEL_SUBDIRS=M
  XFLUSH_OK=0
  LIBRT=""
  ;;
  "2.6.30")
  OSVER=8
  KERNEL_SUBDIRS=M
  XFLUSH_OK=0
  LIBRT=""
  ;;
  "2.6.31")
  OSVER=8
  KERNEL_SUBDIRS=M
  XFLUSH_OK=0
  LIBRT=""
  ;;
  "2.6.32")
  OSVER=8
  KERNEL_SUBDIRS=M
  XFLUSH_OK=0
  LIBRT=""
  ;;
  *)
  OSVER=9
  KERNEL_SUBDIRS=M
  XFLUSH_OK=0
  LIBRT=""
  ;;
  esac
  ;;
  *)
  LIBRT=""
  SDR1426=no_use3
  SDR1424=no_use
  SDR1422=no_use2
  SDR1430=sdr14
  ;;
  esac
  AC_SUBST(SDR1430)
  AC_SUBST(SDR1426)
  AC_SUBST(SDR1424)
  AC_SUBST(SDR1422)
  AC_SUBST(MODEXT)
  AC_SUBST(KERNEL_SUBDIRS)
  AC_CHECK_FILE( /usr/src/linux, LINDIR="linux")
  AC_CHECK_FILE( /usr/src/linux-2.4, LINDIR="linux-2.4")
  AC_SUBST(LINDIR)
  AC_SUBST(FT245PGM) 
  AC_SUBST(OSVER) 
else
  AC_CHECK_FILE( /dev/dsp, SDEVPATH="/dev/dsp")
  LIBRT=""
fi
AC_SUBST(XFLUSH_OK)
AC_SUBST(SDEVPATH)
AC_SUBST(LIBRT)
# *******************************************************************
# Symbol tables make the executable about 3 times bigger. 
# There is an option in options.h to disable them.
# That is the option -s to the gcc linker.
# The -s option is obsolete in Mac OSX and generates a warning
# message which is avoided by not setting the -s option under Darwin
if test "$DARWIN" = "-DDARWIN=0"; then
  grep SYMBOL_TABLES options.h > x.x
  X=`grep TRUE x.x | cut -dd -f-1`
  if test "$X" = "#"; then
    ST=""
  else
    ST="-s $NOPIE"
  fi
else
  ST="-Wl,$NOPIE"
fi
AC_SUBST(ST)
# ********************************************************************
# Check if we have svgalib
SVGALIB32_OK="0"
SVGALIB64_OK="0"
SVGA32_LD=""
SVGA64_LD=""
X=`./libfind1 libvga.${LIBEXT}`
SVGA32_INC=`./libfind2 [1] ["$X"]`
SVGA64_INC=`./libfind2 [2] ["$X"]`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  SVGALIB32_OK="1"
  SVGA32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  SVGALIB64_OK="1"
  SVGA64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  SVGALIB32_OK="1"
  SVGA32_LD=`./libfind2 [3] ["$X"]`
  SVGALIB64_OK="1"
  SVGA64_LD=`./libfind2 [4] ["$X"]`
fi
SVGAGL32_LD=""
SVGAGL64_LD=""
X=`./libfind1 libvgagl.${LIBEXT}`
SVGAGL32_INC=`./libfind2 [1] ["$X"]`
SVGAGL64_INC=`./libfind2 [2] ["$X"]`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  SVGAGL32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  SVGAGL64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  SVGAGL32_LD=`./libfind2 [3] ["$X"]`
  SVGAGL64_LD=`./libfind2 [4] ["$X"]`
fi
SVGALIB32_H="0"
SVGALIB64_H="0"
if test "$SVGALIB32_OK" = "1"; then
  ${CC} ${M32} verify_svgalib.c -DCPU="$CPU" "$SVGA32_INC" "$SVGAGL32_INC" \
	"$SVGA32_LD" "$SVGAGL32_LD" -o verify_svgalib.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    SVGALIB32_H="1"
  else
    SVGALIB32_OK="0"
    SVGAGL32_LD=""
    SVGA32_LD=""
  fi
fi
if test "$SVGALIB64_OK" = "1"; then
  ${CC} ${M64} verify_svgalib.c -DCPU=${CPU} ${SVGA64_INC} ${SVGAGL64_INC} \
	${SVGA64_LD} ${SVGAGL64_LD} -o verify_svgalib.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    SVGALIB64_H="1"
  else
    SVGALIB64_OK="0"
    SVGALIB64_H="0"
    SVGAGL64_LD=""
    SVGA64_LD=""
  fi
fi
AC_SUBST(SVGA32_LD)
AC_SUBST(SVGA64_LD)
AC_SUBST(SVGAGL32_LD)
AC_SUBST(SVGAGL64_LD)
AC_SUBST(SVGA32_INC)
AC_SUBST(SVGA64_INC)
AC_SUBST(SVGAGL32_INC)
AC_SUBST(SVGAGL64_INC)
# *********************************************************************
# Set variables that will create verbose error messages from Makefile.
CONSOLE32_OK="$SVGALIB32_OK"
CONSOLE64_OK="$SVGALIB64_OK"
# here we can set CONSOLExx_OK to zero when something is missing.
if test "$BITS" = "64"; then
  if test "$M32" = ""; then
    CONSOLE32_OK="0"
  fi
fi
AC_SUBST(CONSOLE32_OK)
AC_SUBST(CONSOLE64_OK)
# ********************************************************************
# Check if we have X11
X11_32_OK="0"
X11_64_OK="0"
X11_32_LD=""
X11_64_LD=""
X=`./libfind1 libX11.${LIBEXT}`
X11_32_INC=`./libfind2 [1] ["$X"]`
X11_64_INC=`./libfind2 [2] ["$X"]`
X11_32_INC="$X11_32_INC -I/usr/include/"
X11_64_INC="$X11_64_INC -I/usr/include/"
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  X11_32_OK="1"
  X11_32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  X11_64_OK="1"
  X11_64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  X11_32_OK="1"
  X11_64_OK="1"
  X11_32_LD=`./libfind2 [3] ["$X"]`
  X11_64_LD=`./libfind2 [4] ["$X"]`
fi
X11_32_H="0"
X11_64_H="0"
XKBLIB_32_OK="0"
XKBLIB_64_OK="0"
if test "$X11_32_OK" = "1"; then
  ${CC} ${M32} verify_x11.c $X11_32_INC $X11_32_LD -o verify_x11.ol  2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    X11_32_H="1"
    ${CC} ${M32} verify_xkblib.c $X11_32_INC $X11_32_LD -o verify_xkblib.ol 2> x.x
    chmod 777 x.x
    Y=`cat x.x`
    if test "$Y" = ""; then
      XKBLIB_32_OK="1"
    fi
  else
    X11_32_OK="0"
    X11_32_LD=""
    echo $Y
  fi
fi
if test "$X11_64_OK" = "1"; then
  ${CC} ${M64} verify_x11.c $X11_64_INC $X11_64_LD -o verify_x11.om 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    X11_64_H="1"
    ${CC} ${M64} verify_xkblib.c $X11_64_INC $X11_64_LD -o verify_xkblib.om 2> x.x
    chmod 777 x.x
    Y=`cat x.x`
    if test "$Y" = ""; then
      XKBLIB_64_OK="1"
    fi
  else
    X11_64_H="0"
    X11_64_LD=""
  fi
fi
X11_32_ALL_OK="$X11_32_H"
X11_64_ALL_OK="$X11_64_H"
if test "$X11_32_OK" = "0"; then
  X11_32_ALL_OK="0"
 fi
if test "$X11_64_OK" = "0"; then
  X11_64_ALL_OK="0"
fi
if test "$SVGALIB_32_OK" = "1"; then
  X11_32_ALL_OK="1"
fi
if test "$SVGALIB_64_OK" = "1"; then
  X11_64_ALL_OK="1"
fi
if test "$BITS" = "64"; then
  if test "$M32" = ""; then
    X11_32_ALL_OK="0"
  fi
fi
AC_SUBST(SVGALIB32_OK)
AC_SUBST(SVGALIB64_OK)
AC_SUBST(X11_32_INC)
AC_SUBST(X11_64_INC)
AC_SUBST(X11_32_OK)
AC_SUBST(X11_64_OK)
AC_SUBST(X11_32_LD)
AC_SUBST(X11_64_LD)
AC_SUBST(X11_32_ALL_OK)
AC_SUBST(X11_64_ALL_OK)
AC_SUBST(XKBLIB_32_OK)
AC_SUBST(XKBLIB_64_OK)
# ********************************************************************
# Check if we can use MIT-SHM, the shared memory fast graphics for X11.
LIBXEXT32_OK="0"
LIBXEXT64_OK="0"
LIBXEXT32_LD=""
LIBXEXT64_LD=""
X=`./libfind1 libXext.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBXEXT32_OK="1"
  LIBXEXT32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBXEXT64_OK="1"
  LIBXEXT64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBXEXT32_OK="1"
  LIBXEXT32_LD=`./libfind2 [3] ["$X"]`
  LIBXEXT64_OK="1"
  LIBXEXT64_LD=`./libfind2 [4] ["$X"]`
fi
LIBXEXT32_H="0"
LIBXEXT64_H="0"
${CC} ${M32} verify_xext.c $X11_32_INC $X11_32_LD -o verify_xext.ol 2> x.x
chmod 777 x.x
Y=`cat x.x`
if test "$Y" = ""; then
  LIBXEXT32_H="1"
else
  LIBXEXT32_OK="0"
  LIBXEXT32_LD=""
fi
if test "$BITS" = "64"; then
  ${CC} ${M64} verify_xext.c $X11_64_INC  $X11_64_LD -o verify_xext.ol 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    LIBXEXT64_H="1"
  else
    LIBXEXT64_OK="0"
    LIBXEXT64_LD=""
  fi
fi
AC_SUBST(LIBXEXT32_LD)
AC_SUBST(LIBXEXT64_LD)
AC_SUBST(LIBXEXT32_OK)
AC_SUBST(LIBXEXT64_OK)
#********************************************************************
# Look for the Perseus libraries that we need to compile for Linux.
PERSEUS32_OK="0"
PERSEUS64_OK="0"
PERSEUS32_LD=libperseus-sdr.${LIBEXT}
PERSEUS64_LD=libperseus-sdr.${LIBEXT}
X=`./libfind1 libperseus-sdr.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  PERSEUS32_OK="1"
  PERSEUS32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  PERSEUS64_OK="1"
  PERSEUS64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  PERSEUS32_OK="1"
  PERSEUS32_LD=`./libfind2 [3] ["$X"]`
  PERSEUS64_OK="1"
  PERSEUS64_LD=`./libfind2 [4] ["$X"]`
fi
#********************************************************************
# Look for the libusb-1.0 package that we need for Perseus.
LIBUSB1_32_OK="0"
LIBUSB1_64_OK="0"
LIBUSB1_32_LD="libusb-1.0.${LIBEXT}"
LIBUSB1_64_LD="libusb-1.0.${LIBEXT}"
X=`./libfind1 libusb-1.0.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBUSB1_32_OK="1"
  LIBUSB1_32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBUSB1_64_OK="1"
  LIBUSB1_64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBUSB1_32_OK="1"
  LIBUSB1_32_LD=`./libfind2 [3] ["$X"]`
  LIBUSB1_64_OK="1"
  LIBUSB1_64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(PERSEUS32_LD)
AC_SUBST(PERSEUS64_LD)
AC_SUBST(LIBUSB1_32_LD)
AC_SUBST(LIBUSB1_64_LD)
# ********************************************************************
# Look for (the old) libusb package that we need for si570 and for
# USB2LPT (usb2lpt.c) to use a parallel port on USB.
LIBUSB32_OK="0"
LIBUSB64_OK="0"
LIBUSB32_LD="libusb.${LIBEXT}"
LIBUSB64_LD="libusb.${LIBEXT}"
X=`./libfind1 libusb.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBUSB32_OK="1"
  LIBUSB32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBUSB64_OK="1"
  LIBUSB64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBUSB32_OK="1"
  LIBUSB32_LD=`./libfind2 [3] ["$X"]`
  LIBUSB64_OK="1"
  LIBUSB64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBUSB32_LD)
AC_SUBST(LIBUSB64_LD)
# ********************************************************************
# Look for the libftdi package that we need for Soft66 and Elektor
LIBFTDI32_OK="0"
LIBFTDI64_OK="0"
LIBFTDI32_LD="libftdi.${LIBEXT}"
LIBFTDI64_LD="libftdi.${LIBEXT}"
X=`./libfind1 libftdi.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBFTDI32_OK="1"
  LIBFTDI32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBFTDI64_OK="1"
  LIBFTDI64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBFTDI32_OK="1"
  LIBFTDI32_LD=`./libfind2 [3] ["$X"]`
  LIBFTDI64_OK="1"
  LIBFTDI64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBFTDI32_LD)
AC_SUBST(LIBFTDI64_LD)
# Look for the soft66 package that we need for Soft66 receiver support
SOFT66_32_OK="0"
SOFT66_64_OK="0"
SOFT66_32_LD="libsoft66.so"
SOFT66_64_LD="libsoft66.so"
X=`./libfind1 libsoft66.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  SOFT66_32_OK="1"
  SOFT66_32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  SOFT66_64_OK="1"
  SOFT66_64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  SOFT66_32_OK="1"
  SOFT66_32_LD=`./libfind2 [3] ["$X"]`
  SOFT66_64_OK="1"
  SOFT66_64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(SOFT66_32_LD)
AC_SUBST(SOFT66_64_LD)
# ********************************************************************
# Look for the librtlsdr package that we need for Realtek RTL2832 support
LIBRTLSDR32_OK="0"
LIBRTLSDR64_OK="0"
LIBRTLSDR32_LD="librtlsdr.${LIBEXT}"
LIBRTLSDR64_LD="librtlsdr.${LIBEXT}"
X=`./libfind1 librtlsdr.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBRTLSDR32_OK="1"
  LIBRTLSDR32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBRTLSDR64_OK="1"
  LIBRTLSDR64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBRTLSDR32_OK="1"
  LIBRTLSDR32_LD=`./libfind2 [3] ["$X"]`
  LIBRTLSDR64_OK="1"
  LIBRTLSDR64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBRTLSDR32_LD)
AC_SUBST(LIBRTLSDR64_LD)
# Look for libwd_dask.so that we need for PCIe-9842
LIBWD_DASK_32_OK="0"
LIBWD_DASK_64_OK="0"
LIBWD_DASK_32_LD="libwd_dask.${LIBEXT}"
LIBWD_DASK_64_LD="libwd_dask64.${LIBEXT}"
if test "$CPU" = "CPU_INTEL"; then
  X=`./libfind1 libwd_dask.${LIBEXT}`
  Y=`./libfind2 [0] ["$X"]`
  if test "$Y" = "1"; then
    LIBWD_DASK_32_OK="1"
    LIBWD_DASK_32_LD=`./libfind2 [3] ["$X"]`
  fi
  X=`./libfind1 libwd_dask64.${LIBEXT}`
  Y=`./libfind2 [0] ["$X"]`
  if test "$Y" = "2"; then
    LIBWD_DASK_64_OK="1"
    LIBWD_DASK_64_LD=`./libfind2 [4] ["$X"]`
  fi
fi
AC_SUBST(LIBWD_DASK_32_LD)
AC_SUBST(LIBWD_DASK_64_LD)
# ********************************************************************
# Look for libtecla.so that we need to build libbladeRF
HAVE_LIBTECLA32=""
HAVE_LIBTECLA64=""
X=`./libfind1 libtecla.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  HAVE_LIBTECLA32="1"
fi
if test "$Y" = "2"; then
  HAVE_LIBTECLA64="1"
fi
if test "$Y" = "3"; then
  HAVE_LIBTECLA32="1"
  HAVE_LIBTECLA64="1"
fi
# ********************************************************************
# Look for libbladeRF.so that we need for bladeRF from Nuand.
LIBBLADERF32_OK="0"
LIBBLADERF64_OK="0"
LIBBLADERF32_LD="libbladeRF.${LIBEXT}"
LIBBLADERF64_LD="libbladeRF.${LIBEXT}"
X=`./libfind1 libbladeRF.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBBLADERF32_OK="1"
  LIBBLADERF32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBBLADERF64_OK="1"
  LIBBLADERF64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBBLADERF32_OK="1"
  LIBBLADERF32_LD=`./libfind2 [3] ["$X"]`
  LIBBLADERF64_OK="1"
  LIBBLADERF64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBBLADERF32_LD)
AC_SUBST(LIBBLADERF64_LD)
# ********************************************************************
# Look for libOpenCL.so that we need to run FFT on the GPU.
LIBOPENCL32_OK="0"
LIBOPENCL64_OK="0"
LIBOPENCL32_LD=""
LIBOPENCL64_LD=""
X=`./libfind1 libOpenCL.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBOPENCL32_OK="1"
  LIBOPENCL32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBOPENCL64_OK="1"
  LIBOPENCL64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBOPENCL32_OK="1"
  LIBOPENCL32_LD=`./libfind2 [3] ["$X"]`
  LIBOPENCL64_OK="1"
  LIBOPENCL64_LD=`./libfind2 [4] ["$X"]`
fi
#********************************************************************
# Look for the libclFFT in the linux distribution.
CLFFT32_OK="0"
CLFFT64_OK="0"
CLFFT32_LD=""
CLFFT64_LD=""
X=`./libfind1 libclFFT.${LIBEXT}`
CLFFT32_INC=`./libfind2 [1] ["$X"]`
CLFFT64_INC=`./libfind2 [2] ["$X"]`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  CLFFT32_OK="2"
  CLFFT32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  CLFFT64_OK="2"
  CLFFT64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  CLFFT32_OK="2"
  CLFFT32_LD=`./libfind2 [3] ["$X"]`
  CLFFT64_OK="2"
  CLFFT64_LD=`./libfind2 [4] ["$X"]`
fi
CLFFTPATH32=""
CLFFTPATH64=""
if test "$CLFFT32_OK" = "2"; then
  CLFFTPATH32="${CLFFT32_LD} -lstdc++" 
fi
if test "$CLFFT64_OK" = "2"; then
  CLFFTPATH64="${CLFFT64_LD} -lstdc++" 
fi
if test "$HAVE_REALPATH" != ""; then  
  if test "$BITS" = "64"; then
    if test "$CLFFT64_OK" = "0"; then
      AC_CHECK_FILE( $(realpath ./)/build/extlibs/libclFFT.a, \
         CLFFTPATH64="$(realpath ./)/build/extlibs/libclFFT.a -lstdc++") 
      if test "$CLFFTPATH64" != ""; then    
        AC_CHECK_FILE( $(realpath ./)/extlibs/clFFT/src/include/clFFT.h, \
          CLFFT64_INC="-I$(realpath ./)/extlibs/freeocl/include \
          -I$(realpath ./)/extlibs/clFFT/src/include \
          -I$(realpath ./)/extlibs/clFFT/src/build/include/")
        if test "$CLFFT64_INC" != ""; then
          ldconfig $(realpath ./)/build/extlibs/
          CLFFT64_OK="1"
        fi
      fi
    fi
  else
    if test "$CLFFT32_OK" = "0"; then
      AC_CHECK_FILE( $(realpath ./)/build/extlibs/libclFFT.a, \
         CLFFTPATH32="$(realpath ./)/build/extlibs/libclFFT.a -lstdc++") 
      if test "$CLFFTPATH32" != ""; then    
        AC_CHECK_FILE( $(realpath ./)/extlibs/clFFT/src/include/clFFT.h, \
          CLFFT32_INC="-I$(realpath ./)/extlibs/freeocl/include \
          -I$(realpath ./)/extlibs/clFFT/src/include \
          -I$(realpath ./)/extlibs/clFFT/src/build/include/")
        if test "$CLFFT32_INC" != ""; then
          ldconfig $(realpath ./)/build/extlibs/
          CLFFT32_OK="1"
        fi
      fi
    fi
  fi
fi
if test "$CLFFT64_OK" == "1"; then
  ${CC} ${M64} oclprogs.c -DCPU=${CPU} -DOPENCL_PRESENT=1 -DOSNUM=1 \
       -DCL_USE_DEPRECATED_OPENCL_1_2_APIS ${CLFFT64_INC} -c 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    CLFFT64_OK="0"
  fi
fi
if test "$CLFFT32_OK" == "1"; then
  ${CC} ${M32} oclprogs.c -DCPU=${CPU} -DOPENCL_PRESENT=1 -DOSNUM=1 \
       -DCL_USE_DEPRECATED_OPENCL_1_2_APIS ${CLFFT32_INC} -c 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    CLFFT32_OK="0"
  fi
fi
OCLPROGS32="-DOPENCL_PRESENT=0"
OCLPROGS64="-DOPENCL_PRESENT=0"
if test "$CLFFTPATH32" != ""; then
  if test "$CLFFT32_INC" != ""; then
    if test "$LIBOPENCL32_OK" = "1"; then
      if test "$CLFFT32_OK" != "0"; then
        OCLPROGS32="-DOPENCL_PRESENT=1"
      else
        LIBOPENCL32_LD=""
        CLFFTPATH32=""
      fi
    fi
  fi
fi 
if test "$CLFFTPATH64" != ""; then
  if test "$CLFFT64_INC" != ""; then
    if test "$LIBOPENCL64_OK" = "1"; then
      if test "$CLFFT64_OK" != "0"; then
        OCLPROGS64="-DOPENCL_PRESENT=1"
      else
        LIBOPENCL64_LD=""
        CLFFTPATH64=""
      fi
    fi
  fi
fi 
if test "$CLFFTPATH32" == ""; then
  LIBOPENCL32_LD=""
  CLFFTPATH32=""
fi
if test "$CLFFTPATH64" == ""; then
  LIBOPENCL64_LD=""
  CLFFTPATH64=""
fi
if test "$CLFFT32_INC" == ""; then
  LIBOPENCL32_LD=""
  CLFFTPATH32=""
fi
if test "$CLFFT64_INC" == ""; then
  LIBOPENCL64_LD=""
  CLFFTPATH64=""
fi
AC_SUBST(CLFFTPATH32)
AC_SUBST(CLFFTPATH64)
AC_SUBST(CLFFT32_INC)
AC_SUBST(CLFFT64_INC)
AC_SUBST(OCLPROGS32)
AC_SUBST(OCLPROGS64)
AC_SUBST(LIBOPENCL32_LD)
AC_SUBST(LIBOPENCL64_LD)
#********************************************************************
# Look for libcufft
CUFFT32_OK="0"
CUFFT64_OK="0"
CUFFT32_LD=""
CUFFT64_LD=""
X=`./libfind1 libcufft.${LIBEXT}`
CUFFT32_INC=`./libfind2 [1] ["$X"]`
CUFFT64_INC=`./libfind2 [2] ["$X"]`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  CUFFT32_OK="1"
  CUFFT32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  CUFFT64_OK="1"
  CUFFT64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  CUFFT32_OK="1"
  CUFFT32_LD=`./libfind2 [3] ["$X"]`
  CUFFT64_OK="1"
  CUFFT64_LD=`./libfind2 [4] ["$X"]`
fi
CUFFTPATH32=""
CUFFTPATH64=""
if test "$CUFFT32_OK" = "1"; then
  CUFFTPATH32="${CUFFT32_LD} -lcudart" 
fi
if test "$CUFFT64_OK" = "1"; then
  CUFFTPATH64="${CUFFT64_LD} -lcudart" 
fi

if test "$CUFFT32_OK" = "1"; then
  ${CC} ${M32} verify_cuda.c -DCPU="$CPU" "$CUFFT32_INC" \
	"$CUFFTPATH32" -lm -o verify_cuda32 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    CUFFT32_OK="0"
  fi
fi
if test "$CUFFT64_OK" = "1"; then
  ${CC} ${M64} verify_cuda.c -DCPU=${CPU} ${CUFFT64_INC} \
	${CUFFTPATH64} -lm -o verify_cuda64 2> x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    CUFFT64_OK="0"
  fi
fi
AC_SUBST(CUFFT32_INC)
AC_SUBST(CUFFT64_INC)
AC_SUBST(CUFFT32_OK)
AC_SUBST(CUFFT64_OK)
AC_SUBST(CUFFTPATH32)
AC_SUBST(CUFFTPATH64)
# ********************************************************************
# Look for libmirisdr.so this is for  www.mirics.com USB dongles.
LIBMIRISDR32_OK="0"
LIBMIRISDR64_OK="0"
LIBMIRISDR32_LD="libmirisdr.${LIBEXT}"
LIBMIRISDR64_LD="libmirisdr.${LIBEXT}"
X=`./libfind1 libmirisdr.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBMIRISDR32_OK="1"
  LIBMIRISDR32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBMIRISDR64_OK="1"
  LIBMIRISDR64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBMIRISDR32_OK="1"
  LIBMIRISDR32_LD=`./libfind2 [3] ["$X"]`
  LIBMIRISDR64_OK="1"
  LIBMIRISDR64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBMIRISDR32_LD)
AC_SUBST(LIBMIRISDR64_LD)

# ********************************************************************
# Look for sdrplay2
LIBSDRPLAY232_OK="0"
LIBSDRPLAY232_LD="libmirsdrapi-rsp.${LIBEXT}"
LIBSDRPLAY264_OK="0"
LIBSDRPLAY264_LD="libmirsdrapi-rsp.${LIBEXT}"
X=`./libfind1 libmirsdrapi-rsp.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBSDRPLAY232_OK="1"
  LIBSDRPLAY232_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBSDRPLAY264_OK="1"
  LIBSDRPLAY264_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBSDRPLAY232_OK="1"
  LIBSDRPLAY232_LD=`./libfind2 [3] ["$X"]`
  LIBSDRPLAY264_OK="1"
  LIBSDRPLAY264_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBSDRPLAY232_LD)
AC_SUBST(LIBSDRPLAY264_LD)

# ********************************************************************
# Look for sdrplay3
LIBSDRPLAY332_OK="0"
LIBSDRPLAY332_LD="libsdrplay_api.${LIBEXT}"
LIBSDRPLAY364_OK="0"
LIBSDRPLAY364_LD="libsdrplay_api.${LIBEXT}"
X=`./libfind1 libsdrplay_api.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBSDRPLAY332_OK="1"
  LIBSDRPLAY332_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBSDRPLAY364_OK="1"
  LIBSDRPLAY364_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBSDRPLAY332_OK="1"
  LIBSDRPLAY332_LD=`./libfind2 [3] ["$X"]`
  LIBSDRPLAY364_OK="1"
  LIBSDRPLAY364_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBSDRPLAY332_LD)
AC_SUBST(LIBSDRPLAY364_LD)

# ********************************************************************
# Look for libairspy.so 
LIBAIRSPY32_OK="0"
LIBAIRSPY64_OK="0"
LIBAIRSPY32_LD="libairspy.${LIBEXT}"
LIBAIRSPY64_LD="libairspy.${LIBEXT}"
X=`./libfind1 libairspy.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBAIRSPY32_OK="1"
  LIBAIRSPY32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBAIRSPY64_OK="1"
  LIBAIRSPY64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBAIRSPY32_OK="1"
  LIBAIRSPY32_LD=`./libfind2 [3] ["$X"]`
  LIBAIRSPY64_OK="1"
  LIBAIRSPY64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBAIRSPY32_LD)
AC_SUBST(LIBAIRSPY64_LD)
# ********************************************************************
# Look for libairspyhf.so 
LIBAIRSPYHF32_OK="0"
LIBAIRSPYHF64_OK="0"
LIBAIRSPYHF32_LD="libairspyhf.${LIBEXT}"
LIBAIRSPYHF64_LD="libairspyhf.${LIBEXT}"
X=`./libfind1 libairspyhf.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBAIRSPYHF32_OK="1"
  LIBAIRSPYHF32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBAIRSPYHF64_OK="1"
  LIBAIRSPYHF64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBAIRSPYHF32_OK="1"
  LIBAIRSPYHF32_LD=`./libfind2 [3] ["$X"]`
  LIBAIRSPYHF64_OK="1"
  LIBAIRSPYHF64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBAIRSPYHF32_LD)
AC_SUBST(LIBAIRSPYHF64_LD)
# ********************************************************************
# Look for libfdms1_hw_ctrl.so.1.0 this is for  ELAD FDM-S1.
LIBFDMS1_32_OK="0"
LIBFDMS1_64_OK="0"
LIBFDMS1_32_LD="libfdms1_hw_ctrl.${LIBEXT}.1.0"
LIBFDMS1_64_LD="libfdms1_hw_ctrl.${LIBEXT}.1.0"
X=`./libfind1 libfdms1_hw_ctrl.${LIBEXT}.1.0`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBFDMS1_32_OK="1"
  LIBFDMS1_32_LD=`./libfind2 [3] ["$X"]`.1.0
fi
if test "$Y" = "2"; then
  LIBFDMS1_64_OK="1"
  LIBFDMS1_64_LD=`./libfind2 [4] ["$X"]`.1.0
fi
if test "$Y" = "3"; then
  LIBFDMS1_32_OK="1"
  LIBFDMS1_32_LD=`./libfind2 [3] ["$X"]`.1.0
  LIBFDMS1_64_OK="1"
  LIBFDMS1_64_LD=`./libfind2 [4] ["$X"]`.1.0
fi
AC_SUBST(LIBFDMS1_32_LD)
AC_SUBST(LIBFDMS1_64_LD)
# ********************************************************************
# Look for the libudev package that we need for Funcube Dongle ProPlus 
# and Afedri USB support
LIBUDEV_32_OK="0"
LIBUDEV_64_OK="0"
LIBUDEV_32_LD="libudev.${LIBEXT}"
LIBUDEV_64_LD="libudev.${LIBEXT}"
X=`./libfind1 libudev.${LIBEXT}`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  LIBUDEV_32_OK="1"
  LIBUDEV_32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  LIBUDEV_64_OK="1"
  LIBUDEV_64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  LIBUDEV_32_OK="1"
  LIBUDEV_32_LD=`./libfind2 [3] ["$X"]`
  LIBUDEV_64_OK="1"
  LIBUDEV_64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(LIBUDEV_32_LD)
AC_SUBST(LIBUDEV_64_LD)
# ********************************************************************
# Check whether we have ALSA and if it is modern enough.
ALSA32_OK="0"
ALSA64_OK="0"
if test ${OS} = "Darwin"; then
  ALSA32_LD=""
  ALSA64_LD=""
else
  ALSA32_LD="libasound.${LIBEXT}"
  ALSA64_LD="libasound.${LIBEXT}"
  X=`./libfind1 libasound.${LIBEXT}`
  Y=`./libfind2 [0] ["$X"]`
  if test "$Y" = "1"; then
    ALSA32_OK="1"
    ALSA32_LD=`./libfind2 [3] ["$X"]`
  fi
  if test "$Y" = "2"; then
    ALSA64_OK="1"
    ALSA64_LD=`./libfind2 [4] ["$X"]`
  fi
  if test "$Y" = "3"; then
    ALSA32_OK="1"
    ALSA32_LD=`./libfind2 [3] ["$X"]`
    ALSA64_OK="1"
    ALSA64_LD=`./libfind2 [4] ["$X"]`
  fi
#In case libasound.so was not found, look for libasound.so.2
  if test "$ALSA32_OK"="0"; then
    X=`./libfind1 libasound.${LIBEXT}.2`
    Y=`./libfind2 [0] ["$X"]`
    if test "$Y" = "1"; then
      ALSA32_OK="1"
      ALSA32_LD="`./libfind2 [3] ["$X"]`.2"
    fi
    if test "$Y" = "3"; then
      ALSA32_OK="1"
      ALSA32_LD="`./libfind2 [3] ["$X"]`.2"
    fi
  fi
  if test "$ALSA64_OK"="0"; then
    X=`./libfind1 libasound.${LIBEXT}.2`
    Y=`./libfind2 [0] ["$X"]`
    if test "$Y" = "2"; then
      ALSA64_OK="1"
      ALSA64_LD="`./libfind2 [4] ["$X"]`.2"
    fi
    if test "$Y" = "3"; then
      ALSA64_OK="1"
      ALSA64_LD="`./libfind2 [4] ["$X"]`.2"
    fi
  fi
fi
AC_SUBST(ALSA32_LD)
AC_SUBST(ALSA64_LD)
OSS32_H="0"
OSS64_H="0"
if test "$OSSINC32" == ""; then
  OSSHDR32="1"
else
  OSSHDR32="0"
  OSSINC32=$OSSINC
fi
if test "$OSSINC64" == ""; then
  OSSHDR64="1"
else
  OSSHDR64="0"
  OSSINC64=$OSSINC
fi
${CC} ${M32} -DOSSD=$OSSHDR32 $OSSINC32 verify_oss.c -c -o verify_oss.ol 2>x.x
chmod 777 x.x 
Y=`cat x.x`
if test "$Y" == ""; then
  if test "$SDEVPATH" != "";then
    OSS32_H="1"
  else
    OSSINC32=""
  fi
fi
if test "$BITS" = "64"; then
  ${CC} ${M64} -DOSSD=$OSSHDR64 $OSSINC64 verify_oss.c -c -o verify_oss.ol 2>x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    if test "$SDEVPATH" != "";then
      OSS64_H="1"
    else
      OSSINC64=""
    fi
  fi
fi
AC_SUBST(OSS32_H)
AC_SUBST(OSS64_H)
AC_SUBST(OSSHDR32)
AC_SUBST(OSSHDR64)
AC_SUBST(OSSINC32)
AC_SUBST(OSSINC64)
# *******************************************************************
# Look for Portaudio
PORTAUDIO32_OK="0"
PORTAUDIO64_OK="0"
PORTAUDIO32_LD="libportaudio.${LIBEXT}"
PORTAUDIO64_LD="libportaudio.${LIBEXT}"
X=`./libfind1 libportaudio.${LIBEXT}`
PORTAUDIO32_INC=`./libfind2 [1] ["$X"]`
PORTAUDIO64_INC=`./libfind2 [2] ["$X"]`
Y=`./libfind2 [0] ["$X"]`
if test "$Y" = "1"; then
  PORTAUDIO32_OK="1"
  PORTAUDIO32_LD=`./libfind2 [3] ["$X"]`
fi
if test "$Y" = "2"; then
  PORTAUDIO64_OK="1"
  PORTAUDIO64_LD=`./libfind2 [4] ["$X"]`
fi
if test "$Y" = "3"; then
  PORTAUDIO32_OK="1"
  PORTAUDIO32_LD=`./libfind2 [3] ["$X"]`
  PORTAUDIO64_OK="1"
  PORTAUDIO64_LD=`./libfind2 [4] ["$X"]`
fi
AC_SUBST(PORTAUDIO32_OK)
AC_SUBST(PORTAUDIO32_LD)
AC_SUBST(PORTAUDIO64_OK)
AC_SUBST(PORTAUDIO64_LD)
# *********************************************************************
AC_OUTPUT
SHOW_MISSING="0"
if test "$SVGALIB32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$X11_32_ALL_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBXEXT32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$PERSEUS32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBUSB32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$SOFT66_32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBRTLSDR32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBBLADERF32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBOPENCL32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$CLFFTPATH32" == ""; then
  SHOW_MISSING="1"
fi
if test "$CUFFT32_OK" == "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBMIRISDR32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBSDRPLAY232_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBSDRPLAY332_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBAIRSPY32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBAIRSPYHF32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBFDMS1_32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBWD_DASK_32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$LIBUDEV_32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$ALSA32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$PORTAUDIO32_OK" = "0"; then
  SHOW_MISSING="1"
fi
if test "$BITS" = "64"; then
  if test "$SVGALIB64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$X11_64_ALL_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBXEXT64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$PERSEUS64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBUSB64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$SOFT66_64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBRTLSDR64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBBLADERF64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBOPENCL64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBMIRISDR64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBSDRPLAY264_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBSDRPLAY364_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$CLFFTPATH64" == ""; then
    SHOW_MISSING="1"
  fi
  if test "$CUFFT64_OK" == "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBAIRSPY64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBAIRSPYHF64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBFDMS1_64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBWD_DASK_64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$LIBUDEV_64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$ALSA64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
  if test "$PORTAUDIO64_OK" = "0"; then
    SHOW_MISSING="1"
  fi
fi
echo " "
if test "$ST" = "-s"; then
  echo "Symbol tables disabled. See options.h how to enable."
else
  echo "Symbol tables enabled. See options.h how to disable."
fi
install_libusb1_32(){
if test "$BITS" = "32"; then
  echo "Debian, Ubuntu: apt-get install libusb-1.0-0-dev"
  echo "SuSE: zypper install libusb-1_0-devel"
  echo "Mageia: urpmi libusb1.0-devel"
  echo "Old Fedora: yum install libusbx-devel"
  echo "Fedora 22 and later: dnf install libusbx-devel"
  echo "Then run ./configure again."
else
  echo "Debian, Ubuntu: Copy from a 32 bit installation."
  echo "Mageia: urpmi libusb1.0-devel"
  echo "Very old Fedora: yum install libusb1-devel.i586"
  echo "Older Fedora: yum install libusb1-devel.i686"
  echo "Old Fedora: yum install libusbx-devel.i686"
  echo "Fedora 22 and later: dnf install libusbx.i686 libusbx-devel.i686"
  echo "PCLinuxOS: apt-get install libusb1.0-devel.32bit"
  echo "SuSE: zypper install libusb-1_0-devel-32bit"
  echo "A symbolic link may be missing. Something like this:"
  echo "ln -s /usr/lib/libusb-1.0.${LIBEXT}.0.0.0 /usr/lib/libusb-1.0.${LIBEXT}"
  echo "Then run ./configure again."
fi
}
install_libusb1_64(){
echo "Needed for most USB hardware."
echo "To install, use one of these commands:"
echo "Debian, Ubuntu: apt-get install libusb-1.0-0-dev"
echo "Older Fedora: yum install libusb1-devel"
echo "Old Fedora: yum install libusbx-devel"
echo "Fedora 22 and later: dnf install libusbx-devel"
echo "Mageia: urpmi lib64usb1.0-devel"
echo "SuSE: zypper install libusb-1_0-devel"
echo "PCLinuxOS: apt-get install lib64usb1.0-devel"
echo "Sabayon: ln -s /lib64/libusb-1.0.so.0 /lib64/libusb-1.0.so"
echo "Then run ./configure again."
echo " "
}
install_libwd_dask(){
echo "Needed for PCIe-9842"
echo "Get it from the ADLINK Internet site"
echo "www.adlinktech.com"
echo "Register with your email account. Go to:"
echo "Products > PXI & Modular Instruments >"
echo "Digitizers/Oscilloscopes > Drivers"
if test "$BITS" = "32"; then
  echo "Unpack the wd-dask .tgz archive and copy the libwd_dask.${LIBEXT}"
else
  echo "Unpack the wd-dask .tgz archive and copy the libwd_dask64.${LIBEXT}"
fi
echo "file to a suitable location." 
echo "Note that you will have to install some old Linux distro"
echo "Go back to the linrad directory and run ./configure again."
echo " "
}
install_cuda(){
echo "This package allows CUDA for FFT on NVIDIA graphics cards."      
echo "Status Oct 10 2017"
echo "Debian stable (stretch) from ftp.uk.debian.org has version 8.0.44-4"
echo "Debian sid from ftp.se.debian.org has version 8.0.61-1"
echo "Both work fine with with this command:"
echo "apt-get install libcufft8.0 libcudart8.0"
echo "The Debian packages from ftp.se.debian.org have the same"
echo "version numbers, but symbolic links and header files are missing."
echo "Install the header files like this:"
echo "  cd /usr/include"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cufft.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cufftXt.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cuComplex.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/vector_types.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/host_defines.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/builtin_types.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/device_types.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/driver_types.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/surface_types.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/texture_types.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cudalibxt.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cuda_runtime.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/host_config.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/channel_descriptor.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cuda_runtime_api.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/cuda_device_runtime_api.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/driver_functions.h"
echo "  wget https://raw.githubusercontent.com/cheichler/RemoteRendering/master/inc/vector_functions.h"
echo "Go back to the linrad directory and run ./configure again."
}
install_automake(){
echo First install automake like this:
echo Old Fedora: yum install automake
echo Fedora 22 and later: dnf install automake
echo Debian, Ubuntu: apt-get install automake
echo PCLinuxOS: apt-get install automake
echo SuSE: zypper install automake
echo "Mageia: urpmi automake"
echo Then run ./configure again.
}
install_cpp(){
echo "First install the C++ compiler"
echo "Debian, Ubuntu: apt-get install g++"
echo "SuSE: zypper install gcc-c++"
echo "Old Fedora: yum install gcc-c++"
echo "Fedora 22 and later: dnf install gcc-c++"
echo "Then run ./configure again."
}
install_soft66(){
echo "   cd /usr/src"
echo "   rm -f soft66-0.1.3-test.tbz"
echo "   wget http://www.sm5bsz.com/linuxdsp/archive/mingw/soft66-0.1.3-test-bsz.tbz"
echo "   tar xvfj soft66-0.1.3-test-bsz.tbz"
echo "   cd soft66-0.1.3-test"
echo "   ./configure"
echo "   make"
echo "   make install"
echo "Go back to the linrad directory and run ./configure again."
}
install_libtecla(){
echo "First install libtecla:"
echo "   cd /usr/src"
echo "   wget --no-check-certificate http://www.astro.caltech.edu/~mcs/tecla/libtecla.tar.gz"
echo "   tar xzf libtecla.tar.gz"
echo "   cd libtecla"
echo "   ./configure"
echo "   make"
echo "   make install"
echo "Go back to the linrad directory and run ./configure again."
}
install_pkg_config(){
echo "First install pkg-config like this:"
echo "Fedora: yum install pkg-config"
echo "Debian, Ubuntu: apt-get install pkg-config"
echo "PCLinuxOS: apt-get install pkg-config"
echo "SuSE: zypper install pkg-config"
echo "Then run ./configure again."
}
install_xxd(){
echo First install xxd.
echo This package is included in mainstream Linux distributions.
echo Search the Internet for help on how to install xxd or
echo install from source https://aur.archlinux.org/packages/xxd/
echo Debian, Ubuntu: apt-get install xxd
echo old Debian, Ubuntu: apt-get install bsdmainutils
echo Old Fedora: yum install vim-common
echo Fedora 22 and later: dnf install vim-common
echo Sabayon: equo install app-editors/vim
echo Then run ./configure again.
}
install_libtool() {
echo First install libtool like this:
echo Old Fedora: yum install libtool
echo Fedora 22 and later: dnf install libtool
echo Debian, Ubuntu: apt-get install libtool
echo PCLinuxOS: apt-get install libtool
echo SuSE: zypper install libtool
echo Mageia: urpmi libtool
echo Then run ./configure again.
}
install_git(){
echo "You have to install git first"
echo "Debian, Ubuntu: apt-get install git-core"
echo "Old Fedora: yum install git"
echo "Fedora 22 and later: dnf install git"
echo "Mageia: urpmi git"
echo "SuSE: zypper install git"
echo "PCLinuxOS: apt-get install git"
echo "Sabayon: equo install dev-vcs/git"
echo "Then run ./configure again."
}
install_subversion(){
echo "First install subversion like this:"
echo "Debian, Ubuntu: apt-get install subversion"
echo Old Fedora: yum install subversion
echo Fedora 22 and later: dnf install subversion
echo "Then run ./configure again."
}
install_wget(){
echo First install wget like this:
echo Old Fedora: yum install wget
echo Fedora 22 and later: dnf install wget
echo Debian, Ubuntu: apt-get install wget
echo PCLinuxOS: apt-get install wget
echo SuSE: zypper install wget
echo Then run ./configure again.
}
install_cmake(){
echo "You have to install cmake first."
echo "Debian, Ubuntu: apt-get install cmake"
echo "Old Fedora: yum install cmake"
echo "Fedora 22 and later: dnf install cmake"
echo "Mageia: urpmi cmake"
echo "PCLinuxOS: apt-get install cmake"
echo "SuSE: zypper install cmake"
echo "Gentoo: emerge cmake"
echo "Sabayon: equo install dev-util/cmake" 
echo "Then run ./configure again."
}
install_rpm(){
echo "You have to install rpm first."
echo "Debian, Ubuntu: apt-get install rpm"
}
install_svgalib_source(){
echo "You can install svgalib from source like this:"
if test "$HAVE_WGET" = ""; then
  install_wget
else
  echo "   cd /usr/src"
  echo "   wget http://www.sm5bsz.com/linuxdsp/archive/svgalib/svga1925-4bsz.tbz"
  echo "   tar xvfj svga1925-4bsz.tbz"
  echo "   cd svgalib-1.9.25-4bsz"
  echo "   make install"
  echo "(.../utils may fail but that does not matter)"
  echo "  Go back to the linrad directory and run ./configure again."
  echo " "
fi
}
install_clfft(){
echo "Install from your Linux distribution (must be very modern)"
echo "Debian, Ubuntu: apt-get install libclfft-dev"
echo "You can also install from the Linrad extlibs files:"
if test "$HAVE_CPP" = ""; then
  install_cpp
else
  if test "$HAVE_CMAKE" = ""; then
    install_cmake
  else
    echo "Install like this:"
    echo "  cd extlibs/clFFT/src"
    echo "  mkdir build"
    echo "  cd build"
    echo "  cmake .."
    echo "  make clFFT"
    echo "  cp library/libclFFT.so.2.10.0 /usr/local/lib"
    echo "  ln -s /usr/local/lib/libclFFT.so.2.10.0 /usr/local/lib/libclFFT.so.2"
    echo "  ln -s /usr/local/lib/libclFFT.so.2.10.0 /usr/local/lib/libclFFT.so"
    echo "  cd .."
    echo "  cp include/clFFT.h /usr/local/include"
    echo "  cd build/include"
    echo "  autoconf clFFT.version.h "
    echo "  cp clFFT.version.h /usr/local/include"
    echo "  cd ../../../../.."
    echo ""
    echo "As an alternative you can use these commands:"
    if test "$HAVE_REALPATH" = ""; then  
      echo "First install realpath:"
      echo "Debian, Ubuntu: apt-get install realpath"
      echo "Run ./configure again to get the actual commands."
      echo " "
    else
      echo "  mkdir build"
      echo "  cd build"
      echo "  cmake .."
      echo "  make"
      echo "  cd .."
      echo ""
    fi
    echo "In either case you must run ./configure again."
    echo "if bits/c++config.h is missing install gcc-multilib (Debian)"
    echo "and add symbolic links as required."
    echo "Maybe make dir /usr/local/include/bits and copy"
    echo "/usr/include/c++/6.3.1/i686-redhat-linux/bits/* into it (Fedora)"
    echo " "
  fi
fi
}
install_clfft_header(){
echo "Install from your Linux distribution (must be very modern)"
echo "Debian, Ubuntu: apt-get install libclfft-dev"
echo "You can also install the Linrad extlibs files:"
echo "  svn checkout https://svn.code.sf.net/p/linrad/code/trunk linrad-latest"
echo "If you just want clFFT and not the latest Linrad from the repo:"
echo "  mkdir extlibs"
echo "  cp -r linrad-latest/extlibs/* extlibs"
echo "Then run ./configure again."
echo " "
}

install_libusb32(){
if test "$BITS" = "32"; then
  echo "Debian, Ubuntu: apt-get install libusb-dev"
  echo "Old Fedora: yum install libusb-devel"
  echo  Fedora 22 and later: dnf install libusb-devel
  echo "Mageia: urpmi libusb-compat0.1-devel"
  echo "SuSE: zypper install libusb-compat-devel"
  echo "PCLinuxOS: apt-get install libusb-compat0.1-devel"
  echo "Then run ./configure again."
else
  echo "Debian, Ubuntu, run these commands:"
  echo "dpkg --add-architecture i386"
  echo "apt-get update"
  echo "apt-get install libusb-dev:i386"
  echo "Debian,Ubuntu: 
  From source code or a 32 bit system."
  echo "Older Fedora: yum install libusb-devel.i586"
  echo "Old Fedora: yum install libusb-devel.i686"
  echo "Fedora 22 and later: dnf install libusb.i686 libusb-devel.i686"
  echo "SuSE: zypper install libusb-0_1-4-32bit"
  echo "----: ln -s /usr/lib/libusb-0.1.so.4.4.4 /usr/lib/libusb.so"
  echo "Mageia: urpmi libusb-compat0.1-devel"
  echo "Sabayon: ln -s /lib64/libusb-0.1.so.4 /lib64/libusb-0.1.so"
  echo "PCLinuxOS: apt-get install libusb-compat0.1-devel.32bit"
  echo "Then run ./configure again."
fi
}
install_pandoc(){
echo "You have to install pandoc first."
echo "Debian, Ubuntu: apt-get install pandoc"
echo "SuSE: zypper install pandoc"
echo "Fedora 22 and later: dnf install pandoc"
}
install_libusb64(){
echo "To install old libusb, use one of these commands:"
echo "Debian,Ubuntu: apt-get install libusb-dev"
echo "Modern Debian: ln -s /lib/x86_64-linux-gnu/libusb-0.1.so.4.4.4 /lib/x86_64-linux-gnu/libusb.so"
echo "If command fails, remove the old libusb.so which is a text file."
echo "Old Fedora: yum install libusb-devel"
echo "Fedora 22 and later: dnf install libusb-devel"
echo "SuSE: zypper install libusb-compat-devel"
echo "Mageia: urpmi lib64usb-compat0.1-devel"
echo "PCLinuxOS: apt-get install lib64usb-compat0.1-devel"
echo "Sabayon: ln -s /lib64/libusb-0.1.so.4 /lib64/libusb.so"
echo "Then run ./configure again."
}
install_ftdi32_from_source(){
echo " "
echo "If Soft66 fails, install libftdi-0.18 from source:"
if test "$HAVE_WGET" = ""; then
  install_wget
else
  HAVE_LIBUSB="0" 
  if test "$BITS" = "64"; then
    if test "$LIBUSB64_OK" = "1"; then
      HAVE_LIBUSB="1" 
    fi
  else
    if test "$LIBUSB32_OK" = "1"; then
      HAVE_LIBUSB="1" 
    fi
  fi
  if test "$HAVE_LIBUSB" = "0"; then
    echo "First install libusb.so like this:"
    install_libusb32
  else
    if test "HAVE_LIBUSB_DEV" = "0"; then
      echo "The package libusb-dev is needed. Install like this:"
      echo "Debian,Ubuntu: apt-get install libusb-dev"
      echo "Fedora: dnf install libusb-devel"
    else
      echo "   cd /usr/src"
      echo "   rm -f libftdi-0.18.tar.gz"
      echo "   wget http://www.sm5bsz.com/linuxdsp/archive/libftdi-0.18.tar.gz"
      echo "   tar xvfz libftdi-0.18.tar.gz"
      echo "   cd libftdi-0.18"
      echo "   ./configure"
      echo "   make"
      echo "   make install"
      echo "  Go back to the linrad directory and run ./configure again."
    fi
  fi
  echo " "
fi
}
install_ftdi64_from_source(){
echo " "
echo "If Soft66 fails, install libftdi-0.18 from source."
if test "$HAVE_WGET" = ""; then
  install_wget
else
  HAVE_LIBUSB="0" 
  if test "$BITS" = "64"; then
    if test "$LIBUSB64_OK" = "1"; then
      HAVE_LIBUSB="1" 
    fi
  else
    if test "$LIBUSB32_OK" = "1"; then
      HAVE_LIBUSB="1" 
    fi
  fi
  if test "$HAVE_LIBUSB" = "0"; then
    install_libusb64
  else
    echo "   cd /usr/src"
    echo "   rm -f libftdi-0.18.tar.gz"
    echo "   wget http://www.sm5bsz.com/linuxdsp/archive/libftdi-0.18.tar.gz"
    echo "   tar xvfz libftdi-0.18.tar.gz"
    echo "   cd libftdi-0.18"
    echo "   ./configure"
    echo "   make"
    echo "   make install"
    echo "  Go back to the linrad directory and run ./configure again."
  fi
  echo " "
fi
}
install_bladerf(){
if test "$HAVE_GIT" = ""; then
  install_git
else
  if test "$HAVE_CPP" = ""; then
    install_cpp
  else
    if test "$HAVE_PKG_CONFIG" = ""; then
      install_pkg_config
    else
      if test "$HAVE_CMAKE" = ""; then
        install_cmake
      else
        if test "$HAVE_PANDOC" = ""; then
          install_pandoc
        else
          echo "Dependencies are described here:"
          echo "https://github.com/Nuand/bladeRF/wiki"
          echo "Install the library like this:"
          X=`(cmake /V) |cut -d. -f1`
          echo "$X"
          echo "If version is below 3 your cmake is probably too old."
          echo "   cd /usr/src/"
          echo "   git clone https://github.com/Nuand/bladeRF.git"
          echo "   cd bladeRF/host"
          echo "   mkdir build"
          echo "   cd build"
          echo "   cmake -DINSTALL_UDEV_RULES=ON ../"
          echo "   make"
          echo "   make install"
          echo "Go back to the linrad directory and run ./configure again."
          echo "if bits/c++config.h is missing install gcc-multilib (Debian)"
          echo "and add symbolic links as required."
          echo "Maybe make dir /usr/local/include/bits and copy"
          echo "/usr/include/c++/6.3.1/i686-redhat-linux/bits/* into it (Fedora)"
        fi
      fi
    fi
  fi
fi
}
install_libairspyhf(){
echo "This package is needed for Airspy HF+."
if test "$HAVE_CPP" = ""; then
  install_cpp
  return
fi
if test "$HAVE_PKG_CONFIG" = ""; then
  install_pkg_config
  return
fi
if test "$HAVE_CMAKE" = ""; then
  install_cmake
  return
fi
if test "$HAVE_SUBVERSION" = ""; then
  install_subversion
  return
fi
if test "$BITS" = "32"; then
  if test "$LIBUSB1_32_OK" = "0"; then
    echo "First install libusb-1.0 like this:"
    install_libusb1_32
    return
  fi
else
  if test "$LIBUSB1_64_OK" = "0"; then
    echo "First install libusb-1.0 like this:"
    install_libusb1_64
    return  
  fi
fi
echo "Install the library like this:"
echo "   cd /usr/src/"
echo "   svn checkout https://github.com/airspy/airspyhf.git libairspyhf"
echo "   cd libairspyhf/trunk"
echo "   mkdir build"
echo "   cd build"
if test ${OS} = "Darwin"; then
  echo "   cmake .. -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0"
else
  echo "   cmake ../ -DINSTALL_UDEV_RULES=ON"
fi
echo "   make"
echo "   make install"
echo "Go back to the linrad directory and run ./configure again."
}
install_libairspy(){
echo "This package is needed for Airspy."
if test "$HAVE_CPP" = ""; then
  install_cpp
else
  if test "$HAVE_PKG_CONFIG" = ""; then
    install_pkg_config
  else
    if test "$HAVE_CMAKE" = ""; then
      install_cmake
    else
    if test "$HAVE_SUBVERSION" = ""; then
        install_subversion
      else
        echo "Install the library like this:"
        echo "   cd /usr/src/"
        echo "   svn checkout https://github.com/airspy/host libairspy"
        echo "   cd libairspy/trunk"
        echo "   mkdir build"
        echo "   cd build"
        echo "   cmake ../ -DINSTALL_UDEV_RULES=ON"
        echo "   make"
        echo "   make install"
        echo "Go back to the linrad directory and run ./configure again."
      fi
    fi
  fi
fi
}

if test "$SHOW_MISSING" = "1"; then
  echo " "
  echo " |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
  if test "$HELP" = "0"; then
    echo "For information on how to install all un-installed libraries,"
    echo "type ./configure --with-help"
  echo " "
  fi
  echo "Missing or not working libraries (non fatal.)"
  echo " "
fi
SHOW32="1"
if test "$BITS" = "64"; then
  ${CC} -m32 libfind1.c -o libfind1 -lm 2>x.x
  chmod 777 x.x
  Y=`cat x.x`
  if test "$Y" = ""; then
    ${CC} -m32 verify_cdefs.c -o cdefs -lm 2>x.x
    Y=`cat x.x`
    if test "$Y" != ""; then
      echo "32 bit sys/cdefs not found"
      if test "$HELP" = "1"; then
        echo "Install gcc-multilib"
        echo "Debian, Ubuntu: apt-get install gcc-multilib"
      fi
    fi 
    if test "$HELP" = "1"; then
      echo 32 bit libraries for ${CC} available.
    fi
  else
    SHOW32="0"
    echo "The 32 bit libraries for ${CC} are not in place"
    if test "$HELP" = "1" -o "$HELP" = "999"; then
      setterm -term linux -fore red
      echo "Debian, Ubuntu: apt-get install libc6-dev-i386"
      echo "Old Debian, Ubuntu: apt-get install ia32-libs-dev"
      echo "New Debian,Ubunty: apt-get install lib32bz2-dev"
      echo "Debian (perhaps): apt-get install gcc-multilib"
      echo "Debian (perhaps): apt-get install lib32z1-dev"
      echo "Vy old Fedora: yum install glibc-devel.i586"
      echo "Older Fedora: yum groupinstall 'Legacy Software Development' "
      echo "Old Fedora: yum install glibc-devel.i686"
      echo "Fedora 22 and later: dnf install glibc-devel.i686 libstdc++-devel.i686"
      echo "SuSE: zypper install gcc-32bit"
    else
      echo "For install instructions run ./configure --with-gcc32"
    fi
    setterm -term linux -fore black
  fi
  echo " "
fi
if test "$SHOW32" = "1"; then
# 32 bit ------------------------- Portaudio -----------------------
  if test "$PORTAUDIO32_OK" = "0"; then
    echo "Not present: libportaudio.${LIBEXT} $TXT32 (./configure --with-portaudio-32)"
    if test "$HELP" = "1" -o "$HELP" = "16"; then
      setterm -term linux -fore red
      echo "Portaudio may have to be installed from source code."
      if test ${OS} != "Darwin"; then
        echo "The purpose of installing Portaudio is to be able to use Jack."
      fi
      echo "The versions included in the distributions may not support Jack"
      echo "Make sure to remove any old non-working version first."
      if test "$BITS" = "32"; then
        echo "Install Portaudio from source like this:"
        if test "$HAVE_WGET" = ""; then
          install_wget
        else  
          echo "(Note that you may have to change the date 20140130 to something else.)"
          echo "   cd /usr/src"
          echo "   wget http://www.portaudio.com/archives/pa_stable_v190600_20161030.tgz"
          echo "   tar xvfz pa_stable_v190600_20161030.tgz"
          echo "   cd portaudio"
          echo "   ./configure" 
          echo "   make"
          echo "   make install"
          echo "Go back to the linrad directory and run ./configure again."
          echo " "
        fi
        echo "To install the distribution version use one of these commands"
        echo "Old Fedora: yum install portaudio-devel"
        echo "Fedora 22 and later: dnf install portaudio-devel"
        echo "Debian, Ubuntu: apt-get install libportaudio-dev"
        echo "Debian 8: ln -s /usr/lib/i386-linux-gnu/libportaudio.so.2.0.0 /usr/lib/i386-linux-gnu/libportaudio.so" 
        echo "Debian 9: apt-get install portaudio19-dev"
        echo "PCLinuxOS: apt-get install libportaudio2-devel"
        echo "SuSE: zypper install portaudio-devel"
        echo "Mageia: urpmi libportaudio-devel"
      else
        echo "Copy from a 32 bit installation with Jack support"
        echo "To install the distribution version use one of these commands:"
        echo "old SuSE: zypper install portaudio-devel"
        echo "old SuSE: zypper install portaudio-32bit"
        echo "new SuSE: zypper install libportaudio2-32bit"
        echo "--------: ln -s /usr/lib/libportaudio.so.2 /usr/lib/libportaudio.so"
        echo "Mandriva: urpmi libportaudio2"
        echo "Mageia: urpmi libportaudio2 libportaudio0 libportaudio0-devel"     
        echo "Older Fedora: yum install portaudio-devel.i586, portaudio-devel"
        echo "Old Fedora: yum install portaudio-devel.i686"
        echo "Fedora 22 and later: dnf install portaudio.i686 portaudio-devel.i686"
        echo "A symbolic link may be needed:"
        echo "ln -s ..../lib/libportaudio.${LIBEXT}.2 libportaudio.${LIBEXT}"
      fi
      echo "Then run ./configure again."
      echo " "
    setterm -term linux -fore black
    fi
  fi
# 32 bit ------------------ Soft66 ------------------------------------
  if test "$SOFT66_32_OK" = "0"; then
    echo "Not present: libsoft66.${LIBEXT} $TXT32 (./configure --with-soft66-32)"
    if test "$HELP" = "1" -o "$HELP" = "14"; then
      setterm -term linux -fore red
      echo "This package is needed for the Soft66 series receivers."
      if test "$LIBFTDI32_OK" = "0"; then
        echo "Install libftdi before libsoft66"
      else
        if test "$BITS" = "32"; then
          echo "Install from source code (http://www.infowares.com/soft66)"
          echo "Here are the instructions (run as root or with sudo)"
          echo "If install fails, install libftdi-0.18 from source"
          if test "$HAVE_WGET" = ""; then
            install_wget
          else 
            if test "$HAVE_PKG_CONFIG" = ""; then
              install_pkg_config
            else 
              install_soft66
            fi
          fi
        else
          echo "Copy from a 32 bit Linux installation"
          echo "Then run ./configure again."
        fi
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ------------------ Libftdi ------------------------------------
  if test "$LIBFTDI32_OK" = "0"; then
    echo "Not present: libftdi.${LIBEXT} $TXT32  (./configure --with-libftdi-32)"
    if test "$HELP" = "1" -o "$HELP" = "23"; then
      setterm -term linux -fore red
      echo "Needed for Soft66 and Elektor"
      if test "$BITS" = "32"; then
        echo "Debian, Ubuntu: apt-get install libftdi-dev"
        echo "Mandriva, Mageia: urpmi libftdi-devel (ftdi.h may be misplaced)"
        echo "----------------: cp /usr/include/libftdi/ftdi.h /usr/include"
        echo "PCLinuxOS: apt-get install libftdi-devel"
        echo "Old Fedora: yum install libftdi-devel"
        echo "Fedora 22 and later: problematic. Install from source?"
        echo "SuSE: zypper install libftdi1-devel"
        echo "Then run ./configure again."
        install_ftdi32_from_source
      else
        echo "Fedora: yum install libftdi-devel.i686"
        echo "Or copy from a 32 bit Linux installation"
        echo "Then run ./configure again."
      fi  
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ----------------- RTL 2832 Dongles --------------------------
  if test "$LIBRTLSDR32_OK" = "0"; then
    echo "Not present: librtlsdr.${LIBEXT} $TXT32 (./configure --with-rtlsdr-32)"
    if test "$HELP" = "1" -o "$HELP" = "8"; then
      setterm -term linux -fore red
      echo "This package is needed for the Realtek RTL2832 based DVB dongle"      
      if test "$BITS" = "32"; then
        if test "$LIBUSB1_32_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_32
        else
          echo "Modern Linux distributions may have a librtlsdr-dev(el) package"
          echo "You may install the standard osmocom package from source"
          echo "http://sdr.osmocom.org/trac/wiki/rtl-sdr"
          echo "To get the improved library for Linrad use the following commands:"
          if test "$HAVE_WGET" = ""; then
            install_wget
          else
            if test "$HAVE_CMAKE" = ""; then
              install_cmake
            else
              echo "   cd /usr/src"
              echo "   rm -r -f librtlsdr"
              echo "   wget http://sm5bsz.com/linuxdsp/hware/rtlsdr/rtl-sdr-linrad5.tbz"
              echo "   tar xvfj rtl-sdr-linrad5.tbz"
              echo "   cd librtlsdr"
              echo "   mkdir build"
              echo "   cd build"
              echo "   cmake .."
              echo "   make"
              echo "   make install"
              echo "Go back to the linrad directory and run ./configure again."
            fi
          fi
        fi 
      else
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ----------------- bladeRF --------------------------
  if test "$LIBBLADERF32_OK" = "0"; then
    echo "Not present: libbladeRF.${LIBEXT} $TXT32 (./configure --with-bladerf-32)"
    if test "$HELP" = "1" -o "$HELP" = "10"; then
      setterm -term linux -fore red
      echo "This package is needed for the nuand.com bladeRF"      
      if test "$BITS" = "32"; then
        if test "$LIBUSB1_32_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_32
        else
          if test "$HAVE_LIBTECLA32" = ""; then
            install_libtecla
          else
            install_bladerf
          fi
        fi
      else
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ----------------- OpenCL and clFFT --------------------------
  if test "$LIBOPENCL32_OK" = "0"; then
    echo "Not present: libOpenCL.${LIBEXT} $TXT32 (./configure --with-opencl-32)"
    if test "$HELP" = "1" -o "$HELP" = "22"; then
      setterm -term linux -fore red
      if test "$BITS" = "32"; then
        echo "This package allows usage of OpenCL and clFFT on graphics card."      
        echo "Debian: apt-get install nvidia-driver"
        echo "Debian: apt-get install ocl-icd-libopencl1"
        echo "Debian: apt-get install nvidia-libopencl1"
        echo "Debian: apt-get install mesa-opencl-icd"
        echo "Debian, Ubuntu: apt-get install amd-opencl-dev"
        echo "Debian: ln -s /usr/lib/i386-linux-gnu/libOpenCL.so.1.0.0 /usr/lib/i386-linux-gnu/libOpenCL.so"
        echo "Fedora 22 and later: dnf install opencl-utils-devel"
        echo "---- Note that non-free has to be specified in /etc/apt/sources.list"
        echo "Then run ./configure again."
      else
        echo "Copy from a 32 bit installation"
      fi
      setterm -term linux -fore black
      echo " "
    fi
  else
    if test "$CLFFTPATH32" == ""; then
      echo "Not present: libclFFT.${LIBEXT} ${TXT32} (./configure --with-clfft-32)"
      if test "$HELP" = "1" -o "$HELP" = "21"; then
        setterm -term linux -fore red
        echo "This file is needed for fast FFT with Opencl on the graphics card."
        if test "$BITS" = "32"; then
          if test "$HAVE_CPP" = ""; then
            install_cpp
          else
            install_clfft
          fi
        else
          echo "Copy from a 32 bit installation"
          echo " "
        fi
        setterm -term linux -fore black
      fi
    else
      if test "$CLFFT32_INC" == ""; then
        echo "Not present: clFFT.h (./configure --with-clfft-header-32)"
        if test "$HELP" = "1" -o "$HELP" = "20"; then
          setterm -term linux -fore red
          echo "This file is needed for fast FFT with OpenCL on the graphics card."
          if test "$BITS" = "32"; then
            install_clfft_header
          else
            echo "Copy from a 32 bit installation"
          fi
          setterm -term linux -fore black
        fi
      fi 
    fi
  fi
# 32 bit ----------------- CUDA  --------------------------
  if test "$CPU" = "CPU_INTEL" ; then
    if test "$CUFFT32_OK" = "0"; then
      echo "Not present: libcufft.${LIBEXT} $TXT32 (./configure --with-cufft-32)"
      if test "$HELP" = "1" -o "$HELP" = "19"; then
        setterm -term linux -fore red
        if test "$BITS" = "32"; then
          install_cuda
          echo "Add symbolic links like this (change the version number as required)"
          echo "ln -s /usr/lib/i386-linux-gnu/libcudart.so.8.0.61 /usr/lib/x86_64-linux-gnu/libcudart.so"
          echo "ln -s /usr/lib/i386-linux-gnu/libcufft.so.8.0.61 /usr/lib/x86_64-linux-gnu/libcufft.so"
          echo " "
        else
          echo "Copy from a 32 bit installation"
        fi
        setterm -term linux -fore black
       fi
    fi
  fi
# 32 bit ----------------- Mirics USB dongles --------------------------
  if test "$LIBMIRISDR32_OK" = "0"; then
    echo "Not present: libmirisdr.${LIBEXT} ${TXT32} (./configure --with-mirics-32)"
    if test "$HELP" = "1" -o "$HELP" = "9"; then
      setterm -term linux -fore red
      echo "This package is needed for the Mirics USB dongles and SDRPlay"
      if test "$BITS" = "32"; then
        if test "$LIBUSB1_32_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_32
        else
          if test "$HAVE_CMAKE" = ""; then
            install_cmake
          else
            if test "$HAVE_GIT" = ""; then
              install_git
            else
              echo "Install the library like this:"
              echo "   cd /usr/src/"
              echo "   git clone https://github.com/f4exb/libmirisdr-4.git"
              echo "   cd libmirisdr-4"
              echo "   mkdir build"
              echo "   cd build"
              echo "   cmake .."
              echo "   make"
              echo "   make install"
              echo "Go back to the linrad directory and run ./configure again."
            fi
          fi
        fi
      else
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
  # 32 bit ----------------- SDRPLAY devices --------------------------
  if test "$LIBSDRPLAY232_OK" = "0"; then
    echo "Not present: libmirsdrapi-rsp.${LIBEXT} ${TXT32} (./configure --with-sdrplay2-32)"
    if test "$HELP" = "1" -o "$HELP" = "24"; then
      setterm -term linux -fore red
      echo "This package is needed for the SDRPlay USB devices"
      if test "$BITS" = "32"; then
            echo "Download the binary from www.sdrplay.com"
            echo "Go back to the linrad directory and run ./configure again."
      else
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
  # 32 bit ----------------- SDRPLAY devices - API v3 --------------------------
  if test "$LIBSDRPLAY332_OK" = "0"; then
    echo "Not present: libsdrplay_api.${LIBEXT} ${TXT32} (./configure --with-sdrplay3-32)"
    if test "$HELP" = "1" -o "$HELP" = "25"; then
      setterm -term linux -fore red
      echo "This package is needed for the SDRPlay USB devices (API v3.X)"
      if test "$BITS" = "32"; then
            echo "Download the binary from www.sdrplay.com"
            echo "Go back to the linrad directory and run ./configure again."
      else
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ----------------- Airspy --------------------------
  if test "$LIBAIRSPY32_OK" = "0"; then
    echo "Not present: libairspy.${LIBEXT} ${TXT32} (./configure --with-airspy-32)" 
    if test "$HELP" = "1" -o "$HELP" = "13"; then
      setterm -term linux -fore red
      if test "$BITS" = "32"; then
        install_libairspy
      else
        echo "This package is needed for the Airspy receiver."
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ----------------- Airspy HF+ --------------------------
  if test "$LIBAIRSPYHF32_OK" = "0"; then
    echo "Not present: libairspyhf.${LIBEXT} ${TXT32} (./configure --with-airspyhf-32)"
    if test "$HELP" = "1" -o "$HELP" = "17"; then
      setterm -term linux -fore red
      if test "$BITS" = "32"; then
        install_libairspyhf
      else
        echo "Copy from a 32 bit installation."
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ----------------- ELAD FDM-S1 --------------------------
  if test "$CPU" = "CPU_INTEL" ; then
    if test "$LIBFDMS1_32_OK" = "0"; then
      echo "Not present: libfdms1.${LIBEXT}.1.0 ${TXT32} (./configure --with-FDM-S1-32)"
      if test "$HELP" = "1" -o "$HELP" = "12"; then
        setterm -term linux -fore red
        echo "This package is needed for the ELAD FDM-S1 receiver"      
        if test "$HAVE_WGET" = ""; then
          install_wget
        else  
          echo "Install the libfdms1 library like this:"
          echo "(copy and paste into the command line)"
          echo "  cd /usr/src"
          echo "  mkdir libfdms1"
          echo "  cd libfdms1"
          echo "  wget http://www.sm5bsz.com/linuxdsp/archive/libfdms1-1.0.tgz"
          echo "  tar xvfz libfdms1-1.0.tgz"
          if test "$BITS" = "32"; then
            echo "  cp usr/local/lib/* /usr/local/lib"
          else
            echo "  cp usr/local/lib/* /usr/lib32"
            echo "or copy to some other suitable location."
            echo "For SuSE, /usr/lib" 
          fi
          echo "  cp usr/local/lib/* /usr/local/lib"
          echo "Go back to the linrad directory and run ./configure again."
        fi
        setterm -term linux -fore black
        echo " "
      fi
    fi
  fi
# 32 bit ------------------ Libudev ------------------------------------
  if test "$LIBUDEV_32_OK" = "0"; then
    echo "Not present: libudev.${LIBEXT} $TXT32 (./configure --with-udev-32)"
    if test "$HELP" = "1" -o "$HELP" = "15"; then
      setterm -term linux -fore red
      echo "For Funcube ProPlus and Afedri USB"
      if test "$BITS" = "32"; then
        echo "Debian, Ubuntu: apt-get install libudev-dev"
        echo "Mandriva: urpmi libudev-devel (udev.h misplaced)"
        echo "Mageia: urpmi libudev-devel"
        echo "PCLinuxOS: apt-get install libudev-devel"
        echo "Old Fedora: yum install libudev-devel"
        echo "Fedora 22 and later: dnf install systemd-devel"
        echo "SuSE: zypper install libudev-devel"
        echo "Then run ./configure again."
        echo "Udev is believed to be part of most Linux distributions"
      else
        echo "Debian, Ubuntu:"
        echo "   dpkg --add-architecture i386"
        echo "   apt-get update"
        echo "   apt-get install libudev-dev:i386"
        echo "Fedora: yum install systemd-devel.i686"
        echo "Fedora: ln -s /usr/lib/libudev.so.1.2.1 /usr/lib/libudev.so"
        echo "Fedora 22 and later: dnf install systemd.i686 systemd-compat-libs.i686"
        echo "old SuSE: zypper install libudev0-32bit"
        echo "--------: ln -s /lib/libudev.so.0.10.0 /lib/libudev.so"
        echo "new SuSE: zypper install libudev1-32bit"
        echo "--------: ln -s /usr/lib/libudev.so.1.6.4 /usr/lib/libudev.so"
        echo "Mageia: urpmi libudev-devel-208-10.mga4.i586 (change version)"
        echo "PCLinuxOS: apt-get install libudev0-devel.32bit"
        echo "Maybe copy from a 32 bit installation"
        echo "Then run ./configure again."
      fi  
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ------------------ Libwd_dask ------------------------------------
  if test "$CPU" = "CPU_INTEL"; then  
    if test "$LIBWD_DASK_32_OK" = "0"; then
      echo "Not present: libwd_dask.${LIBEXT} $TXT32 (./configure --with-libwd_dask-32)"
      if test "$HELP" = "1" -o "$HELP" = "11"; then
        setterm -term linux -fore red
        install_libwd_dask
        setterm -term linux -fore black
      fi
    fi
  fi
# 32 bit -------------------- Perseus  -------------------------------
  if test "$PERSEUS32_OK" = "0"; then
    echo "Not present: libperseus-sdr.${LIBEXT} $TXT32 (./configure --with-perseus-32)"
    if test "$HELP" = "1" -o "$HELP" = "7"; then
      setterm -term linux -fore red
      if test "$BITS" = "32"; then
      echo "Only for the Perseus HF receiver. Install from source!"
        if test "$LIBUSB1_32_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_32
        else
          echo "Here are the instructions (run as root or with sudo)"
          if test "$HAVE_GIT" = ""; then
            install_git
          else
            if test "$HAVE_AUTOMAKE" = ""; then
              install_automake
            else
              if test "$HAVE_LIBTOOL" = ""; then
                install_libtool
              else
                if test "$HAVE_XXD" = ""; then
                  install_xxd
                else 
                  echo "   cd /usr/src"
                  echo "   rm -r libperseus-sdr"
                  echo "   git clone https://github.com/amontefusco/libperseus-sdr"
                  echo "   cd libperseus-sdr"
                  echo "   libtoolize"
                  echo "   autoconf"
                  echo "   autoreconf -i"
                  echo "   ./configure"
                  echo "   make"
                  echo "   make install"
                  echo "Go back to the linrad directory and run ./configure again."
                  echo "Note that you need root privileges."
                fi
              fi
            fi
          fi
        fi
      else
        echo "Only for the Perseus HF receiver."
        echo "Copy 32 bit libraries from a 32 bit Linux system"
        echo "Then run ./configure again."
      fi
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ---------------- libusb-1.0 ------------------------
  if test "$LIBUSB1_32_OK" = "0"; then
    echo "Not present: libusb-1.0.${LIBEXT} $TXT32 (./configure --with-libusb1-32)"
    if test "$HELP" = "1" -o "$HELP" = "6"; then
      setterm -term linux -fore red
      echo "Needed for most USB hardware."
      install_libusb1_32
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit -------------------- libusb ---------------------------------
  if test "$LIBUSB32_OK" = "0"; then
    echo "Not present: libusb.${LIBEXT} $TXT32 (./configure --with-libusb0-32)"
    if test "$HELP" = "1" -o "$HELP" = "18"; then
      setterm -term linux -fore red
      echo "This package is needed only for installing libftdi for soft66."
      echo "For the Si570 (Softrock, IQ+, Fifi SDR and others) and USB2LPT"
      echo "it is obsolete and libusb-1.0 is a replacement."
      echo "To install old libusb, use one of these commands:"
      install_libusb32
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit --------------------- MIT SHM -------------------------------
  if test "$X11_32_ALL_OK" = "1"; then
    if test "$LIBXEXT32_OK" = "0"; then
      echo "Not present: libXext.${LIBEXT} $TXT32 or headers (./configure --with-xext-32)"
      if test "$HELP" = "1" -o "$HELP" = "3"; then
        setterm -term linux -fore red
        echo "The MIT-SHM package allows faster graphics with X11"
        echo "If you want to compile for terminal mode with svgalib there is"
        echo "no need for it."
        if test "$LIBXEXT32_H" = "0"; then
          echo "Header file(s) missing: sys/ipc.h, X11/extensions/XShm.h or X11/Xfuncproto.h"
        fi
        if test "$BITS" = "32"; then
          echo "Debian, Ubuntu: apt-get install libxext-dev"
          echo "SuSE: zypper install xorg-x11-libXext-devel"
          echo "Old Fedora: yum install libXext-devel"
          echo "Fedora 22 and later: dnf install libXext-devel"
          echo "PCLinuxOS: apt-get install libxext-devel"
          echo "Mandriva: urpmi libxext6-devel"
          echo "Mageia: urpmi libxext-devel"
        else
          echo "Debian, Ubuntu, run these commands:"
          echo "dpkg --add-architecture i386"
          echo "apt-get update"
          echo "apt-get install libxext-dev:i386"
          echo "old Debian, Ubuntu: apt-get install ia32-libs-dev"
          echo "  If ia32-libs-dev is missing install ia32-libs and add a symbolic link"
          echo "  If that fails, copy from a 32 bit system"
          echo "Older Fedora: yum install libXext-devel.i586"
          echo "Old Fedora: yum install libXext-devel.i686"
          echo "Fedora 22 and later: dnf install libXext.i686 libXext-devel.i686"
          echo "Mandriva: urpmi libxext6-devel"
          echo "Mageia: urpmi libxext-devel-1.3.2-3.mga4.i586 (change version)"
          echo "SuSE: zypper install xorg-x11-libXext-devel-32bit"
          echo "PCLinuxOS: apt-get install libxext-devel.32bit"
        fi
        echo "Some development packages may be incomplete."    
        echo "A symbolic link may help:"
        echo "ln -s ..../libXext.${LIBEXT}.6.4.0 ..../libXext.${LIBEXT}"
        echo "Then run ./configure again."
        setterm -term linux -fore black
        echo " "
      fi
    fi
  fi
# 32 bit -------------------svgalib---------------------------
  if test "$SVGALIB32_OK" = "0"; then
    echo "Not present: libvga.${LIBEXT} $TXT32 or headers (./configure --with-svgalib-32)"
    if test "$HELP" = "1" -o "$HELP" = "4"; then
      setterm -term linux -fore red
      echo "svgalib is only needed for running Linrad in terminal mode."
      echo "If you want to use X11, there is no need to install svgalib."
      if test "$CPU" = "CPU_ARM"; then
        echo "To use svgalib, install the distribution package."
        echo "Raspberry Pi does not work with a svgalib screen and FBDEV does"
        echo "not work with the distribution package. To use the front buffer device"
        echo "you must install svgalib-1.9.25-3bsz from source as described below"
        echo "Note that svgalib-1.9.25-3bsz contains inline assembly for x86"
        echo "use it with FBDEV only. Attempts to use a svgalib screen will crash."
        install_svgalib_source
      else
        echo "The versions supplied by the distributions may support very old"
        echo "videocards only."
        echo "Obsolete svgalib versions can be used in linrad to provide a"
        echo "mouse to use with the framebuffer device."
        if test "$SVGALIB32_H" = "0"; then
          echo "Header file vga.h vgagl.h or vgamouse.h missing or misplaced"
          echo "or libx86.so.1 missing"
        fi
        if test "$BITS" = "32"; then
          echo "Debian, Ubuntu: apt-get install libsvga1-dev"
          echo "SuSE: Install from source."
          echo "Mandriva: urpmi libsvgalib1-devel"
          echo "Mageia: urpmi libsvgalib-devel"
          echo "PCLinuxOS: apt-get install libsvgalib1-devel"
          echo "Old Fedora: yum install svgalib-devel"
          echo  Fedora 22 and later: dnf install svgalib-devel
          echo "Sabayon, Gentoo: emerge svgalib"
          install_svgalib_source
        else
          echo "Copy from a 32 bit installation if the package is missing."
          echo "Files: libvga.so libvgagl.so vga.h vgagl.h vgamouse.h libx86.so.1"
          echo "Older Fedora: yum install svgalib-devel.i586"
          echo "Old: yum install svgalib-devel.i686"
          echo "Fedora 22 and later: dnf install svgalib svgalib-devel.i686"
          echo "Debian, Ubuntu: apt-get install libsvga1-dev"
          echo "PCLinuxOS: apt-get install libsvgalib1-devel.32bit" 
          echo "--------------: (64 libraries, some contain 32 bit also)"
          echo "--------------: Add symbolic links in /usr/lib32"
          echo "--------------: (libvga.so and libvgagl.so missing.)"
          echo "Mageia: urpmi libsvgalib-devel-1.9.25-10.mga4.i586 (change version)"
          echo "Symbolic links may be needed due to incomplete development packages."
          echo "ln -s ..../libvga.${LIBEXT}.1.9.25 ..../libvga.${LIBEXT}"
          echo "ln -s ..../libvgagl.${LIBEXT}.1.9.25 ..../libvgagl.${LIBEXT}"
          echo "Then run ./configure again."
          echo " "
        fi
      setterm -term linux -fore black
      fi
    fi   
  fi
# 32 bit --------------------------X11-----------------------------
  if test "$X11_32_ALL_OK" = "0"; then
    echo "Not present: libX11.${LIBEXT} $TXT32 or headers (./configure --with-x11-32)"
    if test "$HELP" = "1" -o "$HELP" = "2"; then
      setterm -term linux -fore red
      echo "The X11 package is only needed for running Linrad with X11"
      echo "If you want to compile for terminal mode with svgalib there is"
      echo "no need for it."
      if test "$X11_32_OK" = "1"; then
        echo "Header file X11/Xlib.h missing."
      fi
      if test "$BITS" = "32"; then
        echo "Debian, Ubuntu: apt-get install libx11-dev"
        echo "Mandriva: urpmi libx11_6-devel"
        echo "Mageia: urpmi libx11-devel"
        echo "SuSE: zypper install xorg-x11-libX11-devel"
        echo "Old Fedora: yum install libX11-devel"
        echo  Fedora 22 and later: dnf install libX11-devel
        echo "PCLinuxOS: apt-get install libx11-devel"
        echo "Mac OSX: Install XQuartz http://xquartz.macosforge.org/landing"
      else
        echo "Debian, Ubuntu, run these commands:" 
        echo "dpkg --add-architecture i386; apt-get update"
        echo "apt-get install libc6:i386 libx11-6:i386 libdbus-1-dev:i386"
        echo "ln -s /usr/lib/i386-linux-gnu/libX11.so.6.4.0 /usr/lib/i386-linux-gnu/libX11.so"
        echo "Note that you may have another version than 6.4.0"
        echo "old Debian, Ubuntu: apt-get install ia32-libs-dev libc6-dev-i386"
        echo "old--------: If ia32-libs-dev is missing, install ia32-libs and add a symbolic link:"
        echo "old--------: ln -s /usr/lib/i386-linux-gnu/libX11.so.6.4.0 /usr/lib/i386-linux-gnu/libX11.so"
        echo "old--------: Copy libX11.so from a 32 bit installation if ia32-libs fails."
        echo "old--------: Use ldd to find what more 32 bit libraries to copy."
        echo "SuSE: zypper install xorg-x11-libX11-devel-32bit"
        echo "On Red Hat base Linux distros (RHEL, CentOS, Scientific Linux):"
        echo "    yum install glibc.i686 libgcc.i686 libX11.i686"
        echo "Older Fedora: yum install libX11-devel.i586"
        echo "Old Fedora: yum install libX11-devel.i686"
        echo "Fedora 22 and later: dnf install libX11.i686 libX11-devel.i686"
        echo "Mageia: urpmi libx11-devel-1.6.2-2.mga4.i586  (change version number)"
        echo "Mandriva: urpmi libx11_6-devel"
        echo "PCLinuxOS: apt-get install libx11-devel.32bit" 
        echo "--------: Note that bugs currently make this install impossible"
        echo "--------: so install lib64x11_6-devel instead (summer 2010)."
        echo "--------: and make a symbolic link:"
        echo "--------: ln -s /usr/lib/libX11.${LIBEXT}.6.2.0 /usr/lib/libX11.${LIBEXT}"
      fi
      echo "Then run ./configure again."
      setterm -term linux -fore black
      echo " "
    fi
  fi
# 32 bit ------------------ ALSA ---------------------------------
  if test ${OS} != "Darwin"; then
    if test "$ALSA32_OK" = "0"; then
      echo "Not present: libasound.${LIBEXT} $TXT32 (./configure --with-ALSA-32)"
      if test "$HELP" = "1" -o "$HELP" = "5"; then
        setterm -term linux -fore red
        echo "Install the ALSA package like this:" 
        if test "$BITS" = "64"; then
          echo "Debian, Ubuntu Run these commands:"
          echo "dpkg --add-architecture i386"
          echo "apt-get update"
          echo "apt-get install libasound2-dev:i386"
          echo "Old Debian, Ubuntu: apt-get install libasound2-dev lib32asound2"
          echo "Old Debian, Ubuntu: apt-get install lib32asound2-dev"
          echo "Mandriva: urpmi libalsa-oss-devel"
          echo "Mageia: urpmi libalsa2-devel"
          echo "SuSE: zypper install alsa-devel-32bit"
          echo "PCLinuxOS: apt-get install libalsa-oss-devel.32bit"
          echo "Old Fedora: yum install alsa-lib-devel.i686"  
          echo "Fedora 22 and later: dnf install alsa-lib-devel.i686"  
          echo "A symbolic link may be needed on 64 bit systems."
          echo "The development package might not include the 32 bit .${LIBEXT}"
          echo "ln -s ..../libasound.${LIBEXT}.2.0.0 ..../libasound.${LIBEXT}"
        else
          echo "Debian, Ubuntu: apt-get install libasound2-dev"
          echo "Old Fedora: yum install alsa-lib-devel"  
          echo "Fedora 22 and later: dnf install alsa-lib.i686 alsa-lib-devel.i686"
          echo "Mandriva,Mageia: urpmi libalsa-oss-devel"
          echo "PClinuxOS: apt-get install libalsa-oss-devel"
          echo "SuSE: zypper install alsa-devel"
        fi
        echo "Then run ./configure again."
        setterm -term linux -fore black
        echo " "
      fi
    fi
  fi
fi
#||||||||||||||||||||||||| 64 bit compilation ||||||||||||||||||||||||||||||||
if test "$BITS" = "64"; then
  if test "$ELF64" = ""; then
    echo "Your nasm compiler can not generate 64 bit code."
    echo "To compile for 64 bit code you must install a more recent version"
  else
# 64 bit ------------------------- Portaudio -----------------------
    if test "$PORTAUDIO64_OK" = "0"; then
      echo "Not present: libportaudio.${LIBEXT} (64bit) (./configure --with-portaudio-64)"
      if test "$HELP" = "1" -o "$HELP" = "1016"; then
        setterm -term linux -fore red
        echo "Portaudio may have to be installed from source code."
        if test ${OS} != "Darwin"; then
          echo "The purpose of installing Portaudio is to be able to use Jack."
        fi
        echo "The versions included in the distributions do not always support Jack"
        echo "Make sure to remove any old non-working version first."
        echo "Install Portaudio from source like this:"
        if test "$HAVE_WGET" = ""; then
          install_wget
        else 
          echo "(Note that you may have to change the date 20140130 to something else.)"
          echo "   cd /usr/src"
          echo "   wget http://www.portaudio.com/archives/pa_stable_v190600_20161030.tgz"
          echo "   tar xvfz pa_stable_v190600_20161030.tgz"
          echo "   cd portaudio"
          echo "   ./configure" 
          echo "   make"
          echo "   make install"
          echo "Go back to the linrad directory and run ./configure again."
          echo " "
        fi
        echo "To install the distribution version use one of these commands"
        echo "Old Fedora: yum install portaudio-devel"
        echo "Fedora 22 and later: dnf install portaudio-devel"
        echo "Debian, Ubuntu: apt-get install libportaudio-dev"
        echo "Debian 8: apt-get install libportaudio2"
        echo "Debian 8: ln -s /usr/lib/x86_64-linux-gnu/libportaudio.so.2.0.0 /usr/lib/x86_64-linux-gnu/libportaudio.so" 
        echo "SuSE: zypper install portaudio-devel"
        echo "Gentoo: emerge portaudio"
        echo "Sabayon: equo install media-libs/portaudio"
        echo "Mageia: urpmi libportaudio-devel"
        echo "PCLinuxOS: apt-get install lib64portaudio2-devel"
        echo "Then run ./configure again."
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ------------------ Soft66 ------------------------------------
    if test "$SOFT66_64_OK" = "0"; then
      echo "Not present: libsoft66.${LIBEXT} (64bit) (./configure --with-soft66-64)"
      if test "$HELP" = "1" -o "$HELP" = "1014"; then
        setterm -term linux -fore red
        echo "This package is needed for the Soft66 series receivers"
        if test "$LIBFTDI64_OK" = "0"; then
          echo "Install libftdi before libsoft66"
        else
          echo "Install from source code (http://www.infowares.com/soft66)"
          echo "If install fails, install libftdi-0.18 from source"
          echo "Here are the instructions (run as root or with sudo)"
          if test "$HAVE_WGET" = ""; then
            install_wget
          else  
            if test "$HAVE_PKG_CONFIG" = ""; then
              install_pkg_config
            else 
              install_soft66
            fi
          fi
        fi
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ------------------ Libftdi ------------------------------------
    if test "$LIBFTDI64_OK" = "0"; then
      echo "Not present: libftdi.${LIBEXT} (64bit) (./configure --with-libftdi-64)"
      if test "$HELP" = "1" -o "$HELP" = "1023"; then
        setterm -term linux -fore red
        echo "Needed for Soft66 and Elektor"
        echo "Debian,Ubuntu: apt-get install libftdi-dev"
        echo "Mandriva, Mageia: urpmi libftdi-devel (ftdi.h may be misplaced)"
        echo "----------------: cp /usr/include/libftdi/ftdi.h /usr/include"
        echo "PCLinuxOS: apt-get install libftdi-devel"
        echo "Old Fedora: yum install libftdi-devel"
        echo "Fedora 22 and later: dnf install libftdi-devel"
        echo "SuSE: zypper install libftdi1-devel"
        echo "Gentoo: emerge libftdi"
        echo "Mageia: urpmi libftdi-devel"
        echo "Then run ./configure again."
        install_ftdi64_from_source
        setterm -term linux -fore black
      fi
    fi
# 64 bit ----------------- RTL 2832 Dongles --------------------------
    if test "$LIBRTLSDR64_OK" = "0"; then
      echo "Not present: librtlsdr.${LIBEXT} (64bit) (./configure --with-rtlsdr-64)"
      if test "$HELP" = "1" -o "$HELP" = "1008"; then
        setterm -term linux -fore red
        echo "This package is needed for the Realtek RTL2832 based DVB dongle"      
        if test "$LIBUSB1_64_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_64
        else
          echo "You may install the standard osmocom package from source"
          echo "http://sdr.osmocom.org/trac/wiki/rtl-sdr"
          echo "To get the improved library for Linrad use the following commands:"
          if test "$HAVE_WGET" = ""; then
            install_wget
          else
            if test "$HAVE_CMAKE" = ""; then
              install_cmake
            else
              echo "   cd /usr/src"
              echo "   rm -r -f librtlsdr"
              echo "   wget http://sm5bsz.com/linuxdsp/hware/rtlsdr/rtl-sdr-linrad5.tbz"
              echo "   tar xvfj rtl-sdr-linrad5.tbz"
              echo "   cd librtlsdr"
              echo "   mkdir build"
              echo "   cd build"
              echo "   cmake .."
              echo "   make"
              echo "   make install"
              echo "Go back to the linrad directory and run ./configure again."
            fi
          fi
        fi  
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ------------------ Libudev ------------------------------------
    if test "$LIBUDEV_64_OK" = "0"; then
      echo "Not present: libudev.${LIBEXT} (64bit) (./configure --with-udev-64)"
      if test "$HELP" = "1" -o "$HELP" = "1015"; then
        setterm -term linux -fore red
        echo "For Funcube ProPlus and Afedri USB"
        echo "Debian, Ubuntu: apt-get install libudev-dev"
        echo "Mandriva: urpmi libudev-devel (udev.h misplaced)"
        echo "PCLinuxOS: apt-get install lib64udev0-devel"
        echo "Very old Fedora: yum install systemd-devel"
        echo "Very old Fedora: ln -s /usr/lib64/libudev.so.1.2.1 /usr/lib64/libudev.so"
        echo "Old Fedora: yum install libudev-devel"
        echo "Fedora 22 and later: dnf install systemd-devel"
        echo "SuSE: zypper install libudev-devel"
        echo "Mageia: urpmi lib64udev-devel"
        echo "Then run ./configure again."
        echo "Udev is believed to be part of most Linux distributions"
        echo " "
        setterm -term linux -fore black
      fi
    fi
# 64 bit ------------------ Libwd_dask ------------------------------------
    if test "$CPU" = "CPU_INTEL"; then
      if test "$LIBWD_DASK_64_OK" = "0"; then
        echo "Not present: libwd_dask64.${LIBEXT} (64bit) (./configure --with-libwd_dask-64)"
        if test "$HELP" = "1" -o "$HELP" = "1011"; then
          setterm -term linux -fore red
          install_libwd_dask
          setterm -term linux -fore black
        fi
      fi
    fi
# 64 bit ----------------- bladeRF --------------------------
    if test "$LIBBLADERF64_OK" = "0"; then
      echo "Not present: libbladeRF.${LIBEXT} (64bit) (./configure --with-bladerf-64)"
      if test "$HELP" = "1" -o "$HELP" = "1010"; then
        setterm -term linux -fore red
        echo "This package is needed for the nuand.com bladeRF"      
        if test "$LIBUSB1_64_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_64
        else
          if test "$HAVE_LIBTECLA64" = ""; then
            install_libtecla
          else
            install_bladerf
          fi
        fi
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ----------------- OpenCL and clFFT --------------------------
    if test "$LIBOPENCL64_OK" = "0"; then
      echo "Not present: libOpenCL.${LIBEXT} (64 bit) (./configure --with-opencl-64)"
      if test "$HELP" = "1" -o "$HELP" = "1022"; then
        setterm -term linux -fore red
        if test "$HAVE_REALPATH" = ""; then  
          echo "First install realpath:"
          echo "Debian, Ubuntu: apt-get install realpath"
          echo "Run ./configure again to get the actual commands."
        else
          echo "One or the other of these is needed to use the graphics card for fast FFT"      
          echo "Debian, Ubuntu: apt-get install nvidia-opencl-dev"        
          echo "Debian, Ubuntu: apt-get install amd-opencl-dev"
          echo "----: Note that non-free has to be specified in /etc/apt/sources.list"
          echo "Fedora 22 and later: dnf install ocl-icd-devel opencl-utils-devel"
          echo "Mageia: urpmi lib64opencl1 lib64opencl-devel"
          echo Then run ./configure again.
        fi 
        setterm -term linux -fore black
        echo " "
      fi
    else
      if test "$CLFFTPATH64" == ""; then
          echo "Not present: libclFFT.${LIBEXT} (64bit) (./configure --with-clfft-64)"
        if test "$HELP" = "1" -o "$HELP" = "1021"; then
          setterm -term linux -fore red
          if test "$HAVE_REALPATH" = ""; then  
            echo "First install realpath:"
            echo "Debian, Ubuntu: apt-get install realpath"
            echo "Run ./configure again to get the actual commands."
            echo " "
          else
            echo "This package allows usage of OpenCL and clFFT on graphics card."      
            if test "$HAVE_CPP" = ""; then
              install_cpp
            else
              install_clfft
            fi
            setterm -term linux -fore black
          fi
        fi
      else 
        if test "$CLFFT64_INC" == ""; then
          echo "Not present: clFFT.h (./configure --with-clfft-header-64)"
          if test "$HELP" = "1" -o "$HELP" = "1020"; then
            setterm -term linux -fore red
            echo "This file is needed for fast FFT with OpenCL on the graphics card."
            install_clfft_header
            setterm -term linux -fore black
          fi
        fi
      fi
    fi
# 64 bit ----------------- CUDA  --------------------------
    if test "$CPU" = "CPU_INTEL" ; then
      if test "$CUFFT64_OK" = "0"; then
        echo "Not present: libcufft.${LIBEXT} (64bit) (./configure --with-cufft-64)"
        if test "$HELP" = "1" -o "$HELP" = "1019"; then
          setterm -term linux -fore red
          install_cuda
          echo "Add symbolic links like this (change the version number as required)"
          echo "ln -s /usr/lib/x86_64-linux-gnu/libcudart.so.8.0.61 /usr/lib/x86_64-linux-gnu/libcudart.so"
          echo "ln -s /usr/lib/x86_64-linux-gnu/libcufft.so.8.0.61 /usr/lib/x86_64-linux-gnu/libcufft.so"
          setterm -term linux -fore black
          echo " "
        fi
      fi
    fi
# 64 bit ----------------- Mirics USB dongles --------------------------
    if test "$LIBMIRISDR64_OK" = "0"; then
      echo "Not present: libmirisdr.${LIBEXT} (64bit) (./configure --with-mirics-64)"
      if test "$HELP" = "1" -o "$HELP" = "1009"; then
        setterm -term linux -fore red
        echo "This package is needed for the Mirics USB dongles and SDRPlay"      
        if test "$LIBUSB1_64_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_64
        else
          if test "$HAVE_CMAKE" = ""; then
            install_cmake
          else
            if test "$HAVE_GIT" = ""; then
              install_git
            else
              echo "Install the library like this:"
              echo "   cd /usr/src/"
              echo "   git clone https://github.com/f4exb/libmirisdr-4.git"
              echo "   cd libmirisdr-4"
              echo "   mkdir build"
              echo "   cd build"
              echo "   cmake .."
              echo "   make"
              echo "   make install"
              echo "Go back to the linrad directory and run ./configure again."
            fi
          fi
        fi
        setterm -term linux -fore black
        echo " "
      fi
    fi
    # 64 bit ----------------- SDRPlay devices --------------------------
    if test "$LIBSDRPLAY264_OK" = "0"; then
      echo "Not present: libmirsdrapi-rsp.${LIBEXT} (64bit) (./configure --with-sdrplay2-64)"
      if test "$HELP" = "1" -o "$HELP" = "1024"; then
        setterm -term linux -fore red
        echo "This package is needed for the SDRPlay devices"
        echo "Download from www.sdrplay.com"
        echo "Go back to the linrad directory and run ./configure again."
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ----------------- SDRPlay devices - API v3 ------------------------
    if test "$LIBSDRPLAY364_OK" = "0"; then
      echo "Not present: libsdrplay_api.${LIBEXT} (64bit) (./configure --with-sdrplay3-64)"
      if test "$HELP" = "1" -o "$HELP" = "1025"; then
        setterm -term linux -fore red
        echo "This package is needed for the SDRPlay devices (API v3.X)"
        echo "Download from www.sdrplay.com"
        echo "Go back to the linrad directory and run ./configure again."
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ----------------- Airspy --------------------------
    if test "$LIBAIRSPY64_OK" = "0"; then
      echo "Not present: libairspy.${LIBEXT} (64bit)  (./configure --with-airspy-64)"
      if test "$HELP" = "1" -o "$HELP" = "1013"; then
        setterm -term linux -fore red
        install_libairspy
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ----------------- Airspy HF+ --------------------------
    if test "$LIBAIRSPYHF64_OK" = "0"; then
      echo "Not present: libairspyhf.${LIBEXT} (64bit) (./configure --with-airspyhf-64)"
      if test "$HELP" = "1" -o "$HELP" = "1017"; then
        setterm -term linux -fore red
        install_libairspyhf
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ----------------- ELAD FDM-S1 --------------------------
    if test "$CPU" = "CPU_INTEL" ; then
      if test "$LIBFDMS1_64_OK" = "0"; then
        echo "Not present: libfdms1.${LIBEXT}.1.0 (64 bit) (./configure --with-FDM-S1-64)"
        if test "$HELP" = "1" -o "$HELP" = "1012"; then
          setterm -term linux -fore red
          echo "This package is needed for the ELAD FDM-S1 receiver"      
          if test "$HAVE_WGET" = ""; then
            install_wget
          else  
            echo Install the libfdms1 library like this:
            echo "(copy and paste into the command line)"
            echo "  cd /usr/src"
            echo "  mkdir libfdms1"
            echo "  cd libfdms1"
            echo "  wget http://sm5bsz.com/linuxdsp/archive/libfdms1-64bit-1.0.1.tgz"
            echo "  tar xvfz libfdms1-64bit-1.0.1.tgz"
            echo "  mkdir /usr/lib/x86_64-linux-gnu"
            echo "  cp usr/lib/x86_64-linux-gnu/* /usr/lib/x86_64-linux-gnu"
            echo "Go back to the linrad directory and run ./configure again."
            echo " "
          fi
          setterm -term linux -fore black
        fi
      fi
    fi
# 64 bit -------------------- Perseus  -------------------------------
    if test "$PERSEUS64_OK" = "0"; then
      echo "Not present: libperseus-sdr.${LIBEXT} (64 bit) (./configure --with-perseus-64)"
      if test "$HELP" = "1" -o "$HELP" = "1007"; then
        setterm -term linux -fore red
        if test "$LIBUSB1_64_OK" = "0"; then
          echo "First install libusb-1.0 like this:"
          install_libusb1_64
        else
          echo "Only for the Perseus HF receiver. Install from source!"
          echo "Here are the instructions (run as root or with sudo)"
          if test "$HAVE_GIT" = ""; then
            install_git
          else 
            if test "$HAVE_AUTOMAKE" = ""; then
              install_automake
            else
              if test "$HAVE_LIBTOOL" = ""; then
                install_libtool
              else
                if test "$HAVE_XXD" = ""; then
                  install_xxd
                else 
                  echo "   cd /usr/src"
                  echo "   rm -r libperseus-sdr"
                  echo "   git clone https://github.com/amontefusco/libperseus-sdr"
                  echo "   cd libperseus-sdr"
                  echo "   libtoolize"
                  echo "   autoconf"
                  echo "   autoreconf -i"
                  echo "   ./configure"
                  echo "   make"
                  echo "   make install"
                  echo "Go back to the linrad directory and run ./configure again."
                  echo "Note that you need root privileges."
                fi
              fi
            fi
          fi
        fi
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ---------------- libusb-1.0 ------------------------
    if test "$LIBUSB1_64_OK" = "0"; then
      echo "Not present: libusb-1.0.${LIBEXT} (64bit)(./configure --with-libusb1-64)"
      if test "$HELP" = "1" -o "$HELP" = "1006"; then
        setterm -term linux -fore red
        install_libusb1_64
        setterm -term linux -fore black
      fi
    fi
# 64 bit -------------------- libusb ---------------------------------
    if test "$LIBUSB64_OK" = "0"; then
      echo "Not present: libusb.${LIBEXT} (64bit) (./configure --with-libusb0-64)"
      if test "$HELP" = "1" -o "$HELP" = "1018"; then
        setterm -term linux -fore red
        echo "This package is needed only for installing libftdi for soft66."
        echo "For the Si570 (Softrock, IQ+, Fifi SDR and others) and USB2LPT"
        echo "it is obsolete and libusb-1.0 is a replacement."
        install_libusb64
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit --------------------- MIT SHM -------------------------------
    if test "$X11_64_ALL_OK" = "1"; then
      if test "$LIBXEXT64_OK" = "0"; then
        echo "Not present: libXext.${LIBEXT} (64bit) or headers (./configure --with-xext-64)"
        if test "$HELP" = "1" -o "$HELP" = "1003"; then
          setterm -term linux -fore red
          echo "The MIT-SHM package allows faster graphics with X11"
          echo "If you want to compile for terminal mode with svgalib there is"
          echo "no need for it."
          if test "$LIBXEXT64_H" = "0"; then
            echo "Header file(s) missing: sys/ipc.h, X11/extensions/XShm.h or X11/Xfuncproto.h"
          fi
          echo "Debian, Ubuntu: apt-get install libxext-dev"
          echo "Old Fedora: yum install libXext-devel"
          echo "Fedora 22 and later: dnf install libXext-devel"
          echo "Mandriva: urpmi lib64xext6-devel"
          echo "Mageia: urpmi lib64xext-devel"
          echo "PCLinuxOS: apt-get install lib64xext-devel"
          echo "old SuSE: zypper install xorg-x11-libXext-devel"
          echo "new SuSE: zypper install libXext-devel"
          echo "Some development packages may be incomplete."    
          echo "A symbolic link may help:"
          echo "ln -s ..../libXext.${LIBEXT}.6.4.0 ..../libXext.${LIBEXT}"
          echo "Then run ./configure again."
          setterm -term linux -fore black
          echo " "
        fi
      fi
    fi
# 64 bit -------------------svgalib---------------------------
    if test "$SVGALIB64_OK" = "0"; then
      echo "Not present: libvga.${LIBEXT} (64bit)  or headers (./configure --with-svgalib-64)"
      if test "$HELP" = "1" -o "$HELP" = "1004"; then
        setterm -term linux -fore red
        echo "svgalib is only needed for running Linrad in terminal mode."
        echo "If you want to use X11, there is no need to install it."
        if test "$CPU" = "CPU_ARM"; then
          echo "To use svgalib, install the distribution package."
          echo "Raspberry Pi does not work with a svgalib screen and FBDEV does"
          echo "not work with the distribution package. To use the front buffer device"
          echo "you must install svgalib-1.9.25-3bsz from source as described below"
          echo "Note that svgalib-1.9.25-3bsz contains inline assembly for x86"
          echo "use it with FBDEV only. Attempts to use a svgalib screen will crash."
        else
          if test "$SVGALIB64_H" = "0"; then
            echo "Header file vga.h vgagl.h or vgamouse.h missing or misplaced"
            echo "or libx86.so.1 missing"
          fi
          echo "The versions supplied by the distributions may support very old"
          echo "videocards only."
          echo "Obsolete svgalib versions can be used in linrad to provide a mouse"
          echo "to use with the framebuffer device."
          echo "Debian, Ubuntu: apt-get install libsvga1-dev"
          echo "Old Fedora: yum install svgalib-devel"
          echo "Fedora 22 and later: dnf install svgalib-devel"
          echo "   symlinks may be needed:"
          echo "   ln -s  /usr/lib64/libvga.so.1.9.25 /usr/lib64/libvga.so"
          echo "   ln -s  /usr/lib64/libvgagl.so.1.9.25 /usr/lib64/libvgagl.so"
          echo "Mageia: urpmi lib64svgalib-devel"
          echo "PCLinuxOS: apt-get install lib64svgalib1-devel"
          echo "Mandriva: urpmi lib64svgalib1-devel"
          echo "Then run ./configure again."
          echo " "
        fi
        install_svgalib_source
        setterm -term linux -fore black
      fi   
    fi
# 64 bit --------------------------X11-----------------------------
    if test "$X11_64_ALL_OK" = "0"; then
      echo "Not present: libX11.${LIBEXT} (64bit) or headers  (./configure --with-x11-64)"
      if test "$HELP" = "1" -o "$HELP" = "1002"; then
        setterm -term linux -fore red
        echo "The X11 package is only needed for running Linrad with X11"
        echo "If you want to compile for terminal mode with svgalib there is"
        echo "no need for it."
        if test "$X11_64_OK" = "1"; then
          echo "Header file X11/Xlib.h missing (or mis-placed)."
        fi
        echo "Debian, Ubuntu: apt-get install libx11-dev"
        echo "Mandriva: urpmi lib64x11_6-devel"
        echo "Mageia: urpmi lib64x11-devel"
        echo "SuSE: zypper install xorg-x11-libX11-devel"
        echo "Old Fedora: yum install libX11-devel"
        echo "Fedora 22 and later: dnf install libX11-devel"
        echo "PCLinuxOS: apt-get install lib64x11-devel"
        echo "Mac OSX: Install XQuartz http://xquartz.macosforge.org/landing"
        echo "Then run ./configure again."
        setterm -term linux -fore black
        echo " "
      fi
    fi
# 64 bit ------------------ ALSA ---------------------------------
    if test ${OS} != "Darwin"; then
      if test "$ALSA64_OK" = "0"; then
        echo "Not present: libasound.${LIBEXT} (64bit)  (./configure --with-ALSA-64)"
        if test "$HELP" = "1" -o "$HELP" = "1005"; then
          setterm -term linux -fore red
          echo "This is the library for the ALSA sound system."
          echo "Install the ALSA package like this:" 
          echo "Debian, Ubuntu: apt-get install libasound2-dev"
          echo "Old Fedora: yum install alsa-lib-devel"
          echo "Fedora 22 and later: dnf install alsa-lib-devel"
          echo "Mandriva, Mageia: urpmi lib64alsa-oss-devel"
          echo "SuSE: zypper install alsa-devel"
          echo "PCLinuxOS: apt-get install lib64alsa-oss-devel"
          echo "Then run ./configure again."
          setterm -term linux -fore black
          echo " "
        fi
      fi
    fi
  fi 
  ${CC} -m64 verify_cdefs.c -o cdefs -lm 2>x.x
  Y=`cat x.x`
  if test "$Y" != ""; then
    echo "64 bit sys/cdefs not found"
    if test "$HELP" = "1"; then
      setterm -term linux -fore red
      echo "Install gcc-multilib"
      echo "Debian, Ubuntu: apt-get install gcc-multilib"
      setterm -term linux -fore black
    fi
  fi
fi
setterm -term linux -default
#------------------------------------------------------------------
echo " "
if test ${OS} = "Darwin"; then
  echo "Use MacPorts to install Linux packages under Mac OS X."
fi
if test ${OS} == "Linux"; then
  echo "Normal End. You can now run make"
  echo "(Run make without argument for info.)"
elif test ${OS} == "Darwin"; then
  echo "Normal End. You can now run make"
  echo "(Run make without argument for info.)"
elif test ${OS} == "FreeBSD"; then
  echo "Normal End. (FreeBSD) You can now run gmake"
  echo "(Run gmake without argument for info.)"
fi
if test "$MAKE_SERVER" = "1"; then
  echo "Open the Linrad server with your browser at http://localhost:8888/"
else
  echo "To generate servers (daemons) run ./configure --with-server"
fi
rm -f verify_oss
rm -f verify_svgalib
rm -f verify_x11
rm -f verify_xext
rm -f verify_xkblib
rm -f verify_cuda32
rm -f verify_cuda64
rm -f a.out
rm -f *~
rm -f cdefs
rm -f xlinrad
rm -f linrad
rm -f flinrad
rm -f linrad.exe
rm -f xlinrad64
rm -f linrad64
rm -f flinrad64
rm -f linrad64.exe
rm -f *.o
rm -f *.ol
rm -f *.ow
rm -f *.om
rm -f *.oy
