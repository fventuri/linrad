# Linrad - SDR receiver

Linrad is an SDR receiver with advanced features developed by Leif Åsbrink, SM5BSZ.


Linrad main page is at [http://sm5bsz.com/linuxdsp/linrad.htm](http://sm5bsz.com/linuxdsp/linrad.htm).<br/>
Linrad source code is at [https://sourceforge.net/projects/linrad/](https://sourceforge.net/projects/linrad/).<br/>
The [References](#References) section below has more information about Linrad.


The version of Linrad in this repository includes support for the SDRplay RSP devices using SDRplay API version 3.X. The SDRplay models supported are:
- RSP1
- RSP1A
- RSP2 (and RSP2pro)
- RSPduo (including Dual Tuner, Master, and Slave modes)
- RSPdx


## How to build

### Linux

```
git clone https://github.com/fventuri/linrad.git
cd linrad
./configure
make xlinrad64
```

### Windows MinGW

Install MinGW as per the instructions here: http://sm5bsz.com/linuxdsp/install/compile/wincompile.htm

```
git clone https://github.com/fventuri/linrad.git
cd linrad
cp Makefile.MinGW Makefile
make linrad.exe    (32 bit version)
make linrad64.exe    (64 bit version)
```

### Windows MSYS2

Install MSYS2 as per the instructions here: https://www.msys2.org/

```
git clone https://github.com/fventuri/linrad.git
cd linrad
cp Makefile.msys2.nodebug Makefile
make linrad.exe                     (32 bit version)
make linrad64.exe                   (64 bit version)
```

To build an exacutable containing debugging symbols (for use with 'gdb' for instance), replace the last two commands above with:

```
cp Makefile.msys2.debug Makefile
make linrad.exe                     (32 bit version)
make linrad64.exe                   (64 bit version)
```


## How to run

### Linux

- Install the latest SDRplay API/driver for Linux (version 3.07) from here: https://www.sdrplay.com/downloads/

### Windows

- Install the latest SDRplay API/HW driver for Windows (version 3.09) from here: https://www.sdrplay.com/downloads/
- Install the Windows DLL files for Linrad (see here: http://sm5bsz.com/dll.htm)
- Add the SDRplay API DLL files to the folder 'C:\Linrad\dll':
```
copy /B "C:\Program Files\SDRplay\API\x86\sdrplay_api.dll" C:\linrad\dll\sdrplay_api.dll
copy /B "C:\Program Files\SDRplay\API\x64\sdrplay_api.dll" C:\linrad\dll\x64\sdrplay_api.dll
```
- If using the pre-compiled linrad.exe and linrad64.exe binaries, download from this repository the two files `errors.lir` and `help.lir`, and copy them to the same folder where you saved the executables.


## References
- [Linrad: New Possibilties for the Communications Experimenter, Part 1](https://www.arrl.org/files/file/Technology/tis/info/pdf/021112qex037.pdf)
- [Linrad: New Possibilties for the Communications Experimenter, Part 2](https://www.arrl.org/files/file/Technology/tis/info/pdf/030102qex041.pdf)
- [Linrad: New Possibilties for the Communications Experimenter, Part 3](https://www.arrl.org/files/file/Technology/tis/info/pdf/030506qex036.pdf)
- [Linrad: New Possibilties for the Communications Experimenter, Part 4](https://www.arrl.org/files/file/Technology/tis/info/pdf/030910qex029.pdf)
- [Linrad with High-Performance Hardware](https://www.arrl.org/files/file/Technology/tis/info/pdf/040102qex020.pdf)
- [Linrad users group](https://groups.google.com/g/linrad)


## Credits

- Leif Åsbrink, SM5BSZ, for creating linrad
- Davide Gerhard, IV3CVE, for SDRplay version 2 driver


## Copyright

(C) 2002-2022 Leif Åsbrink - MIT license<br/>
(C) 2019 Davide Gerhard (SDRplay API 2.X driver) - MIT license<br/>
(C) 2022 Franco Venturi (SDRplay API 3.X driver) - MIT license<br/>
