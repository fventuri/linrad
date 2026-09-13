# Linrad Notes

Unofficial notes on the internal DSP of Linrad, the software-defined-radio
receiver by Leif Åsbrink, SM5BSZ, written from a reading of the source code. They
cover the noise blankers, the two-channel adaptive-polarization (diversity)
reception, and the FFT-based filter chain.

Linrad itself (source, binaries, and Leif's documentation) is at
[sm5bsz.com]($linrad_home$) and on [SourceForge]($linrad_sourceforge$); this fork,
with SDRplay RSP (API 3.x) support, is the [fventuri/linrad]($source$) repository
these notes accompany. The code is © Leif Åsbrink, MIT-licensed.

![Linrad DSP signal chain](/img/signal-chain.png)

Each page pairs a description and a block diagram with a collapsible **"In the
source"** panel pointing at the functions and lines it refers to.

## Contents

1. [Signal-chain overview](/signal-chain/) — the path from antenna to loudspeaker
1. [Architecture: threads and buffers](/architecture/) — how the pipeline is wired
1. [First FFT and the strong/weak channel split](/channel-split/) — the split
   that feeds the noise blanker
1. [Noise blankers](/noise-blankers/) — the clever (pulse-fitting) and stupid
   (threshold) blankers
1. [Diversity and adaptive polarization](/diversity-polarization/) — combining two
   RF channels for optimum signal-to-noise
1. [Mixers, filters, baseband and output](/mixers-filters-baseband/) — tuning, the
   baseband filter, AGC, demodulation and resampling
1. [Coherent CW and Morse decoding](/coherent-cw/) — coherent carrier extraction
   and Morse-to-ASCII
1. [Spur removal](/spur-removal/) — cancelling stable carriers from the spectrum
1. [FM and wideband-FM](/fm/) — FM detection, stereo composite and RDS
1. [Calibration](/calibration/) — hardware filter correction and I/Q balance
1. [The network interface (and MAP65)](/network-map65/) — multicasting pipeline
   stages between machines and to decoders
1. [Source file map](/file-map/) — which source file does what
1. [Links and references](/links/)

## Summary

Linrad is a fully oversampled, FFT-based receiver. The raw A/D stream is
transformed by the **first FFT** into a wideband spectrum (the main waterfall).
That spectrum is **split by frequency bin** into strong signals and weak signals +
noise, and both parts are **back-transformed** to a time function in which impulse
noise is exposed, where the **noise blanker** operates. A **second FFT** provides
fine resolution for **AFC** and spur removal; a **first mixer** tunes the wanted
signal to zero frequency and decimates it; a **third FFT plus second mixer** apply
the baseband filter and produce the complex **baseband**, which is then
demodulated, AGC'd, resampled and sent to the D/A. With **two RF channels** the
whole chain carries both and combines them adaptively for optimum
signal-to-noise (**adaptive polarization**); the same computation is used to
characterize and cancel interference pulses.
