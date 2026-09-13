# Linrad Notes

These are **unofficial, community notes on the internal DSP of Linrad**, the
software-defined-radio receiver written by **Leif Åsbrink, SM5BSZ**. They were
produced by reading the source code and are meant as a *reading guide* to how the
receiver works — with emphasis on the parts that make Linrad distinctive: its
**noise blankers**, its **two-channel adaptive-polarization (diversity)
reception**, and the **FFT-based filter chain** that ties them together.

Linrad itself (source, binaries, and Leif's own extensive documentation) lives at
[sm5bsz.com]($linrad_home$) and on [SourceForge]($linrad_sourceforge$); this
particular fork, with SDRplay RSP (API 3.x) support, is the
[fventuri/linrad]($source$) repository these notes accompany. The code is © Leif
Åsbrink, MIT-licensed.

![Linrad DSP signal chain](/img/signal-chain.png)

Each page pairs a plain-language explanation and a block diagram with a
collapsible **"In the source"** panel that points at the exact functions and
lines, so you can read at whichever level you need.

## Contents

1. [Signal-chain overview](/signal-chain/) — the end-to-end path from antenna to
   loudspeaker
1. [Architecture: threads and buffers](/architecture/) — how the pipeline is
   wired together
1. [First FFT and the strong/weak channel split](/channel-split/) — the trick
   that makes the noise blanker possible
1. [Noise blankers](/noise-blankers/) — the "smart" pulse-fitting blanker and the
   "dumb" threshold blanker
1. [Diversity and adaptive polarization](/diversity-polarization/) — combining two
   RF channels for optimum signal-to-noise
1. [Mixers, filters, baseband and output](/mixers-filters-baseband/) — tuning,
   the audio filter, AGC, demodulation and resampling
1. [Source file map](/file-map/) — which source file does what
1. [Links and references](/links/)

## The one-paragraph summary

Linrad is a **fully oversampled, FFT-based** receiver. The raw A/D stream is
Fourier-transformed by the **first FFT** into a wideband spectrum (the main
waterfall). That spectrum is **split by frequency bin** into *strong signals* and
*weak signals + noise*, and both parts are **back-transformed** to a time function
in which impulse noise stands out cleanly — so a very effective **noise blanker**
can operate. A **second FFT** provides fine resolution for **AFC** and spur
removal; a **first mixer** tunes the wanted signal to zero frequency and decimates
it; a **third FFT plus second mixer** apply the user's baseband filter and produce
the complex **baseband**, which is then demodulated, AGC'd, resampled and sent to
the D/A. With **two RF channels** the whole chain carries both and combines them
adaptively for optimum signal-to-noise — Linrad's celebrated **adaptive
polarization** — and the same mathematics is reused to characterize and cancel
interference pulses.
