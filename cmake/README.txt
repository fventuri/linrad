CMake is a build automation system
  http://en.wikipedia.org/wiki/Cmake

We try to use it as a replacement for the established GNU build system.
This attempt is currently only experimental. If you wonder why anyone
should do this, read

  Why the KDE project switched to CMake -- and how 
  http://lwn.net/Articles/188693/
  Escape from GNU Autohell!
  http://www.shlomifish.org/open-source/anti/autohell

- How can I get Linrad compiled with CMake as fast as possible ?
  svn checkout https://svn.code.sf.net/p/linrad/code/trunk linrad
  cd linrad
  mkdir build
  cd build
  cmake ..
  make
  ./clinrad
Notice that this svn-checkout allows you to read the source code
and get updates. You will not be able to commit anything.

- How can I use svn to contribute source code ?
You need an account at SourceForge. Read this to understand the first steps:
  http://svnbook.red-bean.com/en/1.7/svn.tour.initial.html
  http://sourceforge.net/apps/trac/sourceforge/wiki/Subversion

- Where can I find a tutorial on CMake basics ?
Use the "official tutorial":
  http://www.cmake.org/cmake/help/cmake_tutorial.html

- Where is the reference of all commands and variables ?
Depending on the CMake version you use, select one of these:
  http://www.cmake.org/cmake/help/v2.8.10/cmake.html

- How can I cross-compile ?
Proceed in the same way as explained above for native compilation,
but use a different build directory. When using CMake, do this:
  cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain_mingw32.cmake ..
Write a new Toolchain file for your cross-compiler and use it.

- How can I build an installable file ?
By default, installable files will not be generated.
But if you instruct CMake about the kind of installable file you want,
then some kinds of files can be generated.
The exact kind of installable file depends on your operating system.
Possible kinds are TGZ (.tar.gz file), RPM (.rpm file), and DEB (.deb file).
   cmake -DCPACK_GENERATOR=DEB ..
   make package

- Can I build an executable that runs on any Win32 platform ?
Yes, there are two ways of doing this.
In both cases you need a MinGW compiler and the NSIS package builder
installed on the host that shall do the build.
  http://sourceforge.net/projects/mingw
  http://sourceforge.net/projects/nsis
When installed properly, the NSIS tool can even build an installer file
(a single .exe file that unpacks, registers and installs the clinrad executable).
1. way: native build on a Win32 platform
   http://www.cmake.org/cmake/help/runningcmake.html
   After clicking "Configure" select the MinGW option with the default native compiler
   In the build directory, the command "mingw32-make" will build the clinrad.exe
   The command "mingw32-make package" will build the installer file
2. way: build with cross-compiler on a Linux platform like Ubuntu 12.04 LTS
   Proceed as describe above for cross-compilers.
   The command "make ; make package" will build clinrad.exe and the installer file

- How can I create a SVN tag in the repository for the current source code ?
  svn copy . https://svn.code.sf.net/p/linrad/code/tags/2014-07-13_04-01 -m "This is a copy of the trunk for Linrad version 4.01."
  svn copy -r204  https://svn.code.sf.net/p/linrad/code/tags/2014-07-13_04-01 -m "This is a copy of the trunk for Linrad version 4.01."
