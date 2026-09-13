# Noise blankers

Linrad removes impulse noise with two blankers, both operating on the
*weak-signal* part of the `timf2` time function, where the strong signals have
already been split off (see
[First FFT and the strong/weak channel split](/channel-split/)):

- a **clever blanker** that detects each impulse, measures its shape and — in a
  two-channel system — its polarization, fits a stored reference pulse, and
  **subtracts** it, leaving the weak signal underneath intact; and
- a **stupid blanker** that **zeroes** every sample above a threshold plus a guard
  region. It is destructive to any weak signal under the pulse and is used as a
  fallback, and as the only blanker when the clever one is disabled.

The two blankers use the code's own names, `clever_bln` and `stupid_bln`.

<details><summary>In the source</summary>

Everything here is in [`blank1.c`]($source$/blank1.c), inside
`first_noise_blanker()` — [blank1.c:684]($source$/blank1.c#L684) — which the
timf2 thread calls once per `timf2` update ([wcw.c:435]($source$/wcw.c#L435)).
Control parameters are in the `HG_PARMS hg` struct
([globdef.h:972]($source$/globdef.h#L972)); related state is in
[`blnkdef.h`]($source$/blnkdef.h). Every routine has parallel `float` and 16-bit
`short int` paths (selected by `swfloat`) and separate one-channel / two-channel
paths.

</details>

## What runs each cycle

Once per `timf2` update the blanker processes the samples between where it left
off (`timf2p_fit`) and a point a safety margin behind the newest data (so a pulse
straddling the boundary is fitted next time). It runs the clever blanker first,
then the stupid blanker over the same region, then updates the noise-floor
statistics that set both thresholds.

<details><summary>In the source</summary>

`first_noise_blanker()` sets the working region `blnk_pbeg … blnk_pend`
([blank1.c:704]($source$/blank1.c#L704)) and returns early unless at least
`min_delay_time*ui.rx_ad_speed` new points are available (so it runs at most
~50×/s). Order within one call: clever fit loop
([blank1.c:763]($source$/blank1.c#L763)) → stupid clear loop
([blank1.c:1004]($source$/blank1.c#L1004)) → oscilloscope max
([blank1.c:1338]($source$/blank1.c#L1338)) → advance `timf2p_fit` and update
noise-floor statistics ([blank1.c:1458]($source$/blank1.c#L1458)).

</details>

## Clever blanker: detect, fit, subtract

The clever blanker removes each impulse by matched subtraction rather than
gating. The flow for each detected pulse:

![Clever noise blanker flow](/img/blanker-clever.png)

It scans the per-sample power for a peak above `clever_bln_limit`, checks the peak
is an isolated single pulse of a plausible width, and, for a genuine pulse,
determines how to reproduce it: the phase is de-rotated to a reference, the
sub-sample peak position is found by a parabolic fit, and that fractional position
selects one of 256 stored reference-pulse shapes (the shape of a band-limited
impulse depends on where its peak falls between samples). The scaled reference is
subtracted from `timf2` and the per-sample power rebuilt.

If the residual power in the subtracted region exceeds half the original, the
model did not fit: the original data is **restored** and the pulse is flagged so
it is not retried. A good fit is flagged as handled, and the scan continues.

### Pulse polarization (two channels)

With two RF channels the pulse is first collapsed onto a single polarization.
Assuming one common source with equal, uncorrelated noise in both channels, the
normalized 2×2 covariance of the two channels over the pulse gives the
polarization angle and the combining coefficients `C1, C2, C3`; the reference
pulse is subtracted from both channels using those coefficients. This is the same
computation used to receive the wanted signal on its optimum polarization (see
[Diversity and adaptive polarization](/diversity-polarization/)), applied here to
cancel an interference source.

<details><summary>In the source</summary>

- Detection / qualification: the `find_pulse:` loop
  ([blank1.c:780]($source$/blank1.c#L780)); rejection of unresolved pulses against
  the `bln[]` width table ([blank1.c:926]($source$/blank1.c#L926)),
  `BLANKER_CONTROL_INFO` ([blnkdef.h:3]($source$/blnkdef.h#L3)).
- Reference-pulse subtraction, one channel:
  `subtract_onechan_pulse()` — [blank1.c:36]($source$/blank1.c#L36); two channels:
  `subtract_twochan_pulse()` — [blank1.c:232]($source$/blank1.c#L232). The scale
  includes `liminfo_amplitude_factor` ([blank1.c:148]($source$/blank1.c#L148)),
  correcting for the energy the strong/weak split removed. The residual test
  (`retval > 0.5` → restore) is at [blank1.c:191]($source$/blank1.c#L191).
- Sub-sample position via parabolic fit selects one of `MAX_REFPULSES` = 256
  stored shapes ([blnkdef.h:13]($source$/blnkdef.h#L13),
  [blank1.c:122]($source$/blank1.c#L122)); the shapes are built in `buf.c`.
- Pulse polarization: `get_pulse_pol()` — [blank1.c:433]($source$/blank1.c#L433)
  (the covariance derivation is in the comments there);
  `transform_timf2_pol()` — [blank1.c:565]($source$/blank1.c#L565). A pulse whose
  estimated noise fraction exceeds 0.15 is not treated as a single source and is
  skipped ([blank1.c:522]($source$/blank1.c#L522)).
- Region protection after a pulse: `set_flag()` —
  [blank1.c:611]($source$/blank1.c#L611) (extends the flag while power keeps
  decreasing).

</details>

## Stupid blanker: threshold clear

The stupid blanker zeroes any run of samples whose power exceeds
`stupid_bln_limit`. Around each cleared run it also clears a guard band whose width
grows with pulse strength, capped at the equivalent of about 40 dB.

<details><summary>In the source</summary>

Threshold-clear loop: [blank1.c:1004]($source$/blank1.c#L1004). Guard-band widths
are `stupid_clr1 = (pulsewidth+1)/2` before the run and
`stupid_clr2 = pulsewidth+1` after, scaled by `√(pulmax/totnoise)/100` and capped
at `t1<=10000` ≈ 40 dB ([blank1.c:1049]($source$/blank1.c#L1049)). Separate
one-channel ([blank1.c:1019]($source$/blank1.c#L1019)) and two-channel
([blank1.c:1156]($source$/blank1.c#L1156)) paths, each with float and 16-bit
variants.

</details>

## Noise-floor tracking and adaptive thresholds

Both thresholds derive from a continuously updated noise-floor estimate, measured
only from the samples the blanker did not touch. In automatic mode the limits are
the noise floor times a user factor. If the stupid blanker is clearing more than
~20 % of samples (a busy band rather than impulse noise), the noise-floor estimate
is raised to back the blanker off. The blanker rates are reported on screen.

<details><summary>In the source</summary>

Noise-floor / rate update: [blank1.c:1458]($source$/blank1.c#L1458). The despiked
power is normalized by `fft1_lowlevel_fraction` (the fraction of the band in the
weak slot); `timf2_noise_floor` is formed at
[blank1.c:1570]($source$/blank1.c#L1570); the busy-band back-off
(`stupid_blanker_rate > 20`) at [blank1.c:1574]($source$/blank1.c#L1574); the
adaptive limits `stupid_bln_limit`/`clever_bln_limit` at
[blank1.c:1586]($source$/blank1.c#L1586). If almost the whole band is strong
(`fft1_lowlevel_fraction < 0.1`) it shows "SELLIM ERROR" and skips the update
([blank1.c:1553]($source$/blank1.c#L1553)).

</details>

## Parameters

From the `HG_PARMS hg` struct ([globdef.h:972]($source$/globdef.h#L972)):

| Field | Meaning |
|-------|---------|
| `clever_bln_mode` | clever blanker: 0 off, 1 auto-threshold, other = manual limit |
| `stupid_bln_mode` | stupid blanker: 0 off, 1 auto-threshold, other = manual limit |
| `clever_bln_factor` / `clever_bln_limit` | clever threshold = noise floor × factor |
| `stupid_bln_factor` / `stupid_bln_limit` | stupid threshold = noise floor × factor |
| `blanker_ston_fft1` / `blanker_ston_fft2` | S/N thresholds for the level split |
| `sellim_par1..8` | selective level-limiter tuning (see the split page) |
| `timf2_oscilloscope*` | the blanker-waveform oscilloscope display |

---

*Related:* [strong/weak channel split](/channel-split/) ·
[diversity and adaptive polarization](/diversity-polarization/) ·
[signal-chain overview](/signal-chain/)
