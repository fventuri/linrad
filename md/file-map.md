# Source file map

Which source file does what. DSP-path files are grouped first. Paths link to the
[fventuri/linrad]($source$) source.

## Core DSP pipeline

| File | Role |
|------|------|
| [`rxin.c`]($source$/rxin.c) | RX input; fills `timf1`; internal test generator; raw file/net save |
| [`fft1.c`]($source$/fft1.c) | first (wideband) FFT; waterfall; back-transform helpers |
| [`sellim.c`]($source$/sellim.c) | `liminfo[]` strong/weak classification (selective limiter) |
| [`timf2.c`]($source$/timf2.c) | split + simultaneous back-FFT → `timf2` |
| [`blank1.c`]($source$/blank1.c) | noise blankers (clever + stupid) and pulse polarization |
| [`fft2.c`]($source$/fft2.c) | second (fine) FFT for AFC / spur removal |
| [`spur.c`]($source$/spur.c), [`spursub.c`]($source$/spursub.c) | spur / birdie removal |
| [`afcsub.c`]($source$/afcsub.c), [`afc_graph.c`]($source$/afc_graph.c) | AFC + initial polarization; AFC graph |
| [`mix1.c`]($source$/mix1.c) | first mixer (tune + decimate as a limited back-FFT) → `timf3` |
| [`fft3.c`]($source$/fft3.c) | third FFT feeding the second mixer |
| [`mix2.c`]($source$/mix2.c) | second mixer: baseband filter + adaptive polarization → `baseb_raw` |
| [`baseb_graph.c`]($source$/baseb_graph.c) | baseband: filters, AGC detection, AM/FM, baseband spectrum |
| [`rxout.c`]($source$/rxout.c) | output: resampling, AGC gain, BFO/demod, squelch, D/A |
| [`wcw.c`]($source$/wcw.c) | wideband-DSP thread glue; coherent-CW weak-signal path |
| [`fm.c`]($source$/fm.c) | FM demodulation, WFM stereo pilot / RDS / de-emphasis |

## FFT kernels / SIMD

[`fft0.c`]($source$/fft0.c) and the `fft2mmx*.s`, `fftasm*.s`, `split*.s`,
`simdasm*.s` assembly kernels; GPU path [`oclprogs.c`]($source$/oclprogs.c),
[`cuda.c`]($source$/cuda.c).

## Coherent CW / Morse / measurement

[`coherent.c`]($source$/coherent.c), [`cohsub.c`]($source$/cohsub.c),
[`coh_osc.c`]($source$/coh_osc.c) (coherent CW, waveform fitting);
[`cwdetect.c`]($source$/cwdetect.c), [`cwspeed.c`]($source$/cwspeed.c),
[`morse.c`]($source$/morse.c) (auto Morse decode);
[`siganal_graph.c`]($source$/siganal_graph.c),
[`allan_graph.c`]($source$/allan_graph.c) (correlation / Allan-variance displays);
[`pol_graph.c`]($source$/pol_graph.c) (polarization graph);
[`hires_graph.c`]($source$/hires_graph.c) (high-resolution blanker/level graph);
[`radar.c`]($source$/radar.c).

## Hardware / device drivers

`sdrplay2.c`, `sdrplay3.c`, `mirics.c`, `perseus.c`, `sdrip.c`, `sdr14.c`,
`excalibur.c`, `airspy.c`, `airspyhf.c`, `bladerf.c`, `rtl2832.c`, `cloudiq.c`,
`afedri.c`, `elad.c`, `elektor.c`, `soft66.c`, `fcdpp.c`, `openhpsdr.c`,
`pcie9842.c`, `si570.c`, `wse.c`, `extio.c`, `hwaredriver.c`, `hid*.c`,
`loadusb.*`, `loadalsa.*`, `soundcard.c`, `pa.c` (PortAudio), `sim2*.c`
(simulators). Networking: `network.c`, `httpd.c`, `html_server.c`.

## Calibration

`calibrate.c`, `calsub.c`, `calsub2.c`, `caliq.c`, `calvar.c`, `caldef.h` —
amplitude / phase / IQ-balance calibration.

## UI / display / platform

[`menu.c`]($source$/menu.c), [`ui.c`]($source$/ui.c) (menus, UI, thread startup);
`screen.c`, `screensub.c`, `fonts.c`, `palette.c` (rendering); `wide_graph.c`,
`baseb_graph.c`, `meter_graph.c`, `tx_graph.c` (graphs); `mouse.c`, `help.c`;
[`xmain.c`]($source$/xmain.c) / [`lxsys.c`]($source$/lxsys.c) (X11 / Linux),
[`wmain.c`]($source$/wmain.c) / [`wsys.c`]($source$/wsys.c) (Windows).

Central headers: [`globdef.h`]($source$/globdef.h) (parameter structs `hg`, `bg`,
`pg`, `ag`, `ui`, `mix1`, `mix2`, `genparm[]`, mode defines);
[`thrdef.h`]($source$/thrdef.h) (threads/events/mutexes);
[`fft1def.h`]($source$/fft1def.h), [`sigdef.h`]($source$/sigdef.h),
[`blnkdef.h`]($source$/blnkdef.h), [`sdrdef.h`]($source$/sdrdef.h).

## Transmit

`tx.c`, `txssb.c`, `tx_graph.c`, `txtest.c`, `txdef.h` (not covered in these DSP
notes).

## Author's documentation shipped with the source

[`z_SETTINGS.txt`]($source$/z_SETTINGS.txt) (DSP overview),
[`z_BUFFERS.txt`]($source$/z_BUFFERS.txt) (buffer conventions),
[`z_TIMING.txt`]($source$/z_TIMING.txt) (the T display / delay chain),
[`z_NETWORK.txt`]($source$/z_NETWORK.txt), [`z_CALIBRATE.txt`]($source$/z_CALIBRATE.txt),
[`z_MORSE_DECODING.txt`]($source$/z_MORSE_DECODING.txt) and other `z_*.txt` files.
