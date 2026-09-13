# Signal-chain overview

The path from antenna to loudspeaker, with the second FFT enabled (the general
case). Stages 2–5 are the **wideband DSP**; stages 6–9 the **narrowband DSP**.

![Linrad DSP signal chain](/img/signal-chain.png)

1. **Input** fills `timf1` with raw complex (I/Q) or real samples, one or two RF
   channels, from an A/D device, the network, or a file.

2. **First FFT** transforms `timf1` into the wideband complex spectrum `fft1` and
   the main spectrum/waterfall.

3. **Strong/weak split + back-transform** routes each FFT bin to a *strong-signal*
   slot or a *weak-signal + noise* slot (per the `liminfo[]` classification), and
   back-transforms both to the time function `timf2`. See
   [First FFT and the strong/weak channel split](/channel-split/).

4. **Noise blanker** operates on the weak-signal part of `timf2`, where impulse
   noise is exposed. See [Noise blankers](/noise-blankers/).

5. **Second FFT** recombines the two `timf2` parts into one time function and
   transforms it at high resolution for **AFC** and **spur removal**.

6. **First mixer + AFC** tunes the selected signal to 0 Hz and decimates it,
   implemented as a limited back FFT rather than an NCO + FIR. Output is `timf3`.

7. **Third FFT + second mixer** apply the baseband filter, optionally combine the
   two RF channels adaptively ([Diversity and adaptive
   polarization](/diversity-polarization/)), and produce the complex baseband.

8. **Baseband processing** does AGC detection, AM/FM demodulation, coherent-CW
   carrier extraction, and the baseband spectrum/waterfall.

9. **Output** applies the BFO, AGC gain and squelch, resamples to the exact D/A
   rate, and writes audio. See
   [Mixers, filters, baseband and output](/mixers-filters-baseband/).

When the second FFT is disabled, AFC uses first-FFT transforms directly and the
blanker is less effective.

<details><summary>In the source</summary>

Pipeline glue is in [`wcw.c`]($source$/wcw.c): `second_fft()`
([wcw.c:250]($source$/wcw.c#L250)) drives the second FFT; `timf2_routine()`
([wcw.c:401]($source$/wcw.c#L401)) runs `do_fft1_c()` → `make_timf2()` →
`first_noise_blanker()`. Stage entry points: first FFT
[`fft1.c`]($source$/fft1.c); split + back-FFT
[`timf2.c`]($source$/timf2.c) `make_timf2()`; blanker
[`blank1.c`]($source$/blank1.c); second FFT [`fft2.c`]($source$/fft2.c); AFC
[`afcsub.c`]($source$/afcsub.c); spurs [`spur.c`]($source$/spur.c); first mixer
[`mix1.c`]($source$/mix1.c); third FFT [`fft3.c`]($source$/fft3.c) and second
mixer [`mix2.c`]($source$/mix2.c); baseband
[`baseb_graph.c`]($source$/baseb_graph.c); output
[`rxout.c`]($source$/rxout.c). The DSP-overview text in
[`z_SETTINGS.txt`]($source$/z_SETTINGS.txt) is the author's own description of
this chain.

</details>
