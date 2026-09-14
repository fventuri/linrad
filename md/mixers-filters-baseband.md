# Mixers, filters, baseband and output

The narrowband DSP: from the wideband `timf2` to demodulated audio.

## Second FFT: AFC and spur removal

`second_fft()` adds the weak + strong parts of `timf2` into one time function,
produces the high-resolution `fft2` spectrum, and runs spur removal. `fft2` feeds
AFC and spur removal only; the audio path does not pass through it.

**AFC** ([`afcsub.c`]($source$/afcsub.c)) tracks weak/unstable signals so the first
mixer can center them at 0 Hz. `collect_initial_spectrum()` acquires a signal
(averaged transforms, peak by parabolic interpolation, initial polarization);
`make_afc_signoi()` forms the S/N used to validate the track; `make_ag_point()`
builds the AFC-graph points with noise estimated from filters outside the signal.
Longer averaging finds weaker signals at the cost of delay. AFC runs across all
receive modes and closes the loop through the first mixer; the tracking loop and
its `AFC_ENABLE` / `AFC_LOCK_RANGE` / `AFC_MAX_DRIFT` settings have their own
[AFC](/afc/) page.

<details><summary>In the source</summary>

`second_fft()` — [wcw.c:250]($source$/wcw.c#L250); `make_fft2()` in
[`fft2.c`]($source$/fft2.c); `spur_removal()` in [`spur.c`]($source$/spur.c) /
[`spursub.c`]($source$/spursub.c) ([wcw.c:286]($source$/wcw.c#L286)). AFC:
`collect_initial_spectrum()` [afcsub.c:34]($source$/afcsub.c#L34),
`make_afc_signoi()` [afcsub.c:693]($source$/afcsub.c#L693),
`make_ag_point()` [afcsub.c:792]($source$/afcsub.c#L792),
`make_afct_window()` [afcsub.c:992]($source$/afcsub.c#L992);
`AG_PARMS ag` [globdef.h:884]($source$/globdef.h#L884).

</details>

## First mixer: tune + decimate

`do_mix1()` implements the first mixer as a limited back FFT rather than an NCO +
FIR ([`z_SETTINGS.txt`]($source$/z_SETTINGS.txt)):

> Rather than actually mixing … with a digitally controlled oscillator … and then
> filtering …, limited back FFTs are used. … The filter in use during this process
> is the window function of the FFT.

A slice of `mix1.size` bins centered on the selected frequency is multiplied by the
frequency-domain window `mix1_fqwin[]` (the anti-alias/decimation filter, so its
`sin^p` power sets spur suppression), given a residual fine-tuning phase rotation
from the AFC error, and inverse-transformed to `timf3` at the reduced rate.
`set_mix1_phases()` maintains phase continuity across overlapping transforms.

<details><summary>In the source</summary>

`do_mix1()` — [mix1.c:55]($source$/mix1.c#L55); `do_mix1_afc()` —
[mix1.c:648]($source$/mix1.c#L648); `mix1_fqwin` window at
[mix1.c:118]($source$/mix1.c#L118); phase rotation at
[mix1.c:104]($source$/mix1.c#L104); `set_mix1_phases()` —
[mix1.c:781]($source$/mix1.c#L781); second-FFT-driven variants
`fft2_mix1_afc()` / `fft2_mix1_fixed()` —
[mix1.c:863]($source$/mix1.c#L863). `mix1` / `mix2` are `MIXER_VARIABLES`
([uidef.h:37]($source$/uidef.h#L37)).

</details>

## Third FFT + second mixer: the baseband filter

`do_mix2()` → `fft3_mix2()` applies the baseband filter and produces the complex
baseband. The third FFT ([`fft3.c`]($source$/fft3.c)) transforms `timf3`; `mix2`
selects `mix2.size` bins around 0 Hz.

![Second mixer and baseband filter](/img/mixer-baseband.png)

Two methods, by `bg.mixer_mode`:

- **frequency domain** (`mixer_mode == 1`): multiply the selected bins by
  `bg_filterfunc[]`, inverse-transform, and overlap-add into `baseb_raw`. This is
  the default; `bg_filterfunc` is a flat-topped shape set by
  `bg.filter_flat` / `bg.filter_curv`.
- **time-domain FIR** (`mixer_mode == 2`): convolve `timf3` with the symmetric FIR
  `basebraw_fir[]` and resample.

Alongside the audio filter, `mix2` also extracts a narrow **carrier filter**
`bg_carrfilter[]` (`bg.coh_factor` times narrower) used for coherent CW, AM
synchronous detection, the S-meter and the phase/AFC displays. In the two-channel
branch the polarization combination is applied per bin together with the filter,
and the adaptive-coefficient update runs from the same loop
([Diversity and adaptive polarization](/diversity-polarization/)); the main output
goes to `baseb_raw`, the orthogonal polarization to `baseb_raw_orthog`.

<details><summary>In the source</summary>

`do_mix2()` — [mix2.c:41]($source$/mix2.c#L41); `fft3_mix2()` —
[mix2.c:83]($source$/mix2.c#L83). Frequency-domain filter/decimate:
[mix2.c:146]($source$/mix2.c#L146) (1 ch), [mix2.c:600]($source$/mix2.c#L600)
(2 ch); time-domain FIR [mix2.c:217]($source$/mix2.c#L217); carrier filter
[mix2.c:246]($source$/mix2.c#L246). Filters are (re)built in
[`baseb_graph.c`]($source$/baseb_graph.c) (`make_bg_filter()`,
`make_baseband_graph()`).

</details>

## Baseband processing

`make_baseband_graph()` turns `baseb_raw` into `baseb_out` and drives the baseband
spectrum/waterfall. It computes the AGC envelope `baseb_agc_level[]` (per sample,
per channel when `bg.agc_flag == 2`) with attack/release/hang from
`bg.agc_attack`/`bg.agc_release`/`bg.agc_hang`; tracks the AM DC/carrier level for
envelope and synchronous AM; demodulates FM (`detect_fm()`, with WFM pilot / RDS /
de-emphasis); extracts the coherent-CW phase from the narrow carrier filter; and
applies the baseband notch filters. Parameters are in `BG_PARMS bg`.

<details><summary>In the source</summary>

`make_baseband_graph()` — [baseb_graph.c:3249]($source$/baseb_graph.c#L3249);
`make_bfo()` — [baseb_graph.c:355]($source$/baseb_graph.c#L355); AM DC level
[baseb_graph.c:935]($source$/baseb_graph.c#L935); FM in
[`fm.c`]($source$/fm.c) and the `fm*` arrays ([sigdef.h:226]($source$/sigdef.h#L226));
coherent CW in [`coherent.c`]($source$/coherent.c) / [`cohsub.c`]($source$/cohsub.c);
`BG_PARMS bg` [globdef.h:1012]($source$/globdef.h#L1012).

</details>

## Output and demodulation

`make_audio_signal()` produces the D/A stream: squelch gating; **fractional
resampling** to the exact D/A rate via a cubic Lagrange fit to 4 neighbouring
baseband samples, with an optional order-5 IIR anti-image filter; AGC gain
(pulled down when `daout_gain·baseb_agc_level > bg_agc_amplimit`, common or
per-channel); and the BFO / demodulation selected by `rx_mode`
(`MODE_WCW/NCW/HSMS/SSB/FM/AM/QRSS`). The baseband can also be streamed over the
network.

![Output resampling and demodulation](/img/output.png)

<details><summary>In the source</summary>

`make_audio_signal()` — [rxout.c:970]($source$/rxout.c#L970); squelch
[rxout.c:995]($source$/rxout.c#L995); cubic-Lagrange resampling
[rxout.c:1068]($source$/rxout.c#L1068); order-5 IIR
[rxout.c:1115]($source$/rxout.c#L1115); AGC gain
[rxout.c:1087]($source$/rxout.c#L1087); `rx_mode` values
[globdef.h:125]($source$/globdef.h#L125). Blocking soundcard path
`blocking_rxout()` [rxout.c:80]($source$/rxout.c#L80); non-blocking `rx_output()`
[rxout.c:266]($source$/rxout.c#L266).

</details>

## Filters in the chain

| Filter | Where | Purpose |
|--------|-------|---------|
| `fft1_window` (`sin^p`) | first FFT | analysis + anti-alias for `mix1` decimation |
| `liminfo` selective limiter | `sellim.c` | route strong signals away from the blanker |
| reference-pulse subtraction | `blank1.c` | impulse-noise removal, matched to pulse shape |
| `mix1_fqwin` | first mixer | anti-alias / decimation (frequency-domain window) |
| spur removal | `spur.c` | remove mixing spurs |
| `bg_filterfunc` | second mixer | main passband filter |
| `bg_carrfilter` | second mixer | narrow carrier filter (coherent CW / sync AM / S-meter) |
| `basebraw_fir` | second mixer (mode 2) | time-domain FIR alternative |
| baseband notches | `baseb_graph.c` | manual notch(es) in the passband |
| polarization combine | `mix2.c` | spatial/polarization combination (2 ch) |
| cubic Lagrange + IIR5 | `rxout.c` | fractional resampling / anti-image |
| FM de-emphasis / pilot / RDS FIRs | `fm.c` | WFM broadcast |

## Related

Baseband subsystems covered separately:
[Coherent CW and Morse decoding](/coherent-cw/) ·
[Spur removal](/spur-removal/) ·
[FM and wideband-FM](/fm/).
