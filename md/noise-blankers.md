# Noise blankers

Impulse noise — ignition noise, power-line arcing, switching supplies, radar —
is one of the things Linrad handles better than almost any other receiver. It does
so with **two cooperating blankers** that work on a wideband time function *after*
the strong signals have been taken out of the way:

- a **"smart" (clever) blanker** that detects each impulse, measures its shape and
  (in a two-channel system) its polarization, fits a stored reference pulse, and
  **subtracts** it — removing the interference while leaving the weak signal
  underneath essentially intact; and
- a **"dumb" (stupid) blanker** that simply **zeroes** every sample above a
  threshold, plus a guard region — crude, but a reliable safety net for whatever
  the smart blanker did not catch.

Both run on the *weak-signal* part of the `timf2` time function. That split — done
one stage earlier — is what makes the blanker so effective, because the strong
signals that would otherwise swamp an impulse detector have already been routed
elsewhere. If you have not read it yet, see
[First FFT and the strong/weak channel split](/channel-split/) for how `timf2`
and its per-sample power array `timf2_pwr[]` are produced.

<details><summary>In the source</summary>

Everything here is in [`blank1.c`]($source$/blank1.c), inside
`first_noise_blanker()` — [blank1.c:684]($source$/blank1.c#L684) — which the
timf2 thread calls once per `timf2` update ([wcw.c:435]($source$/wcw.c#L435)).
Control parameters live in the `HG_PARMS hg` struct
([globdef.h:972]($source$/globdef.h#L972)); related state is declared in
[`blnkdef.h`]($source$/blnkdef.h). Every routine has parallel `float` and 16-bit
`short int` code paths (selected by `swfloat`) and separate one-channel /
two-channel paths.

</details>

## What runs each cycle

Once per `timf2` update the blanker processes the newly available samples between
where it left off (`timf2p_fit`) and a point a safety margin behind the newest
data (so a pulse straddling the boundary can be fitted next time). It runs the
smart blanker first, then the dumb blanker over the same region, then updates the
noise-floor statistics that set both thresholds.

<details><summary>In the source</summary>

`first_noise_blanker()` establishes the working region `blnk_pbeg … blnk_pend`
([blank1.c:704]($source$/blank1.c#L704)) and returns early unless at least
`min_delay_time*ui.rx_ad_speed` new points are available (so it runs at most
~50×/s). Order within one call: smart fit loop
([blank1.c:763]($source$/blank1.c#L763)) → dumb clear loop
([blank1.c:1004]($source$/blank1.c#L1004)) → oscilloscope max
([blank1.c:1338]($source$/blank1.c#L1338)) → advance `timf2p_fit` and update
noise-floor statistics ([blank1.c:1458]($source$/blank1.c#L1458)).

</details>

## The smart blanker: detect, fit, subtract

The smart blanker treats each impulse as a *known waveform* to be removed by
matched subtraction rather than a spike to be gated. The flow for each detected
pulse:

![Smart noise blanker flow](/img/blanker-clever.png)

It scans the per-sample power for a peak above `clever_bln_limit`, checks the peak
really is an isolated single pulse of a plausible width, and — for a genuine
pulse — determines exactly how to reproduce it: the phase is de-rotated to a
reference, the sub-sample peak position is found by a parabolic fit, and that
fractional position selects one of **256 stored reference-pulse shapes**. (A
band-limited impulse's shape depends on where its true peak falls *between*
samples, which is why a family of pre-computed shapes is kept.) The scaled
reference is then subtracted from `timf2` and the per-sample power rebuilt.

Finally there is a **sanity check**: if the residual power in the subtracted
region is more than half the original, the model did not fit, so the original
data is **restored** and the pulse is flagged so it is not retried. A good fit is
flagged as handled, and the scan continues to the next peak.

### Pulse polarization (two channels)

In a two-RF-channel system the pulse is first collapsed onto a single
polarization. Assuming the impulse comes from one common source with equal,
uncorrelated noise in both channels, the normalized 2×2 covariance of the two
channels over the pulse yields the polarization angle and the combining
coefficients `C1, C2, C3`; the reference pulse is then subtracted from *both*
channels using those coefficients. This is the same mathematics Linrad uses to
receive the wanted signal on its optimum polarization — see
[Diversity and adaptive polarization](/diversity-polarization/) — applied here to
*cancel* an interference source instead of *keep* a signal.

<details><summary>In the source</summary>

- Detection / qualification: the `find_pulse:` loop
  ([blank1.c:780]($source$/blank1.c#L780)); rejection of unresolved/ugly pulses
  against the `bln[]` width table ([blank1.c:926]($source$/blank1.c#L926)),
  `BLANKER_CONTROL_INFO` ([blnkdef.h:3]($source$/blnkdef.h#L3)).
- Reference-pulse subtraction, one channel:
  `subtract_onechan_pulse()` — [blank1.c:36]($source$/blank1.c#L36); two channels:
  `subtract_twochan_pulse()` — [blank1.c:232]($source$/blank1.c#L232). The scale
  includes `liminfo_amplitude_factor` ([blank1.c:148]($source$/blank1.c#L148)) to
  correct for the energy the strong/weak split removed. The residual test
  (`retval > 0.5` → restore) is at [blank1.c:191]($source$/blank1.c#L191).
- Sub-sample position via parabolic fit selects one of `MAX_REFPULSES` = 256
  stored shapes ([blnkdef.h:13]($source$/blnkdef.h#L13),
  [blank1.c:122]($source$/blank1.c#L122)); the shapes are built in `buf.c`.
- Pulse polarization: `get_pulse_pol()` — [blank1.c:433]($source$/blank1.c#L433)
  (the covariance derivation is written out in the comments there);
  `transform_timf2_pol()` — [blank1.c:565]($source$/blank1.c#L565). A pulse whose
  estimated noise fraction exceeds 0.15 is not treated as a single source and is
  skipped ([blank1.c:522]($source$/blank1.c#L522)).
- Region protection after handling a pulse: `set_flag()` —
  [blank1.c:611]($source$/blank1.c#L611) (extends the flag outward while power
  keeps decreasing).

</details>

## The dumb blanker: threshold clear

The dumb blanker simply zeroes any run of samples whose power exceeds
`stupid_bln_limit`. Around each cleared run it also clears a **guard band** whose
width grows with the pulse strength (a stronger pulse smears further, so more
neighbouring samples are removed), capped at the equivalent of about 40 dB. It is
destructive to any weak signal under the pulse, which is why the smart blanker is
preferred; the dumb blanker is the backstop, and the only blanker when the smart
one is disabled.

<details><summary>In the source</summary>

Threshold-clear loop: [blank1.c:1004]($source$/blank1.c#L1004). The guard-band
widths are `stupid_clr1 = (pulsewidth+1)/2` before the run and
`stupid_clr2 = pulsewidth+1` after, scaled by `√(pulmax/totnoise)/100` and capped
at `t1<=10000` ≈ 40 dB ([blank1.c:1049]($source$/blank1.c#L1049)). Separate
one-channel ([blank1.c:1019]($source$/blank1.c#L1019)) and two-channel
([blank1.c:1156]($source$/blank1.c#L1156)) paths, each with float and 16-bit
variants.

</details>

## Noise-floor tracking and adaptive thresholds

Both thresholds are derived from a continuously updated estimate of the noise
floor, measured only from the samples the blanker did *not* touch. In automatic
mode the limits are set to the noise floor times a user factor. There is a neat
feedback detail: if the dumb blanker is clearing more than ~20 % of samples (the
band is genuinely busy, not impulsive), Linrad *raises* its noise-floor estimate
to back the blanker off, so it does not start eating the signal. The measured
"blanker rates" are reported on screen.

<details><summary>In the source</summary>

Noise-floor / rate update: [blank1.c:1458]($source$/blank1.c#L1458). The despiked
power is normalized by `fft1_lowlevel_fraction` (how much of the band was actually
in the weak slot); `timf2_noise_floor` is formed at
[blank1.c:1570]($source$/blank1.c#L1570); the busy-band back-off (`stupid_blanker_rate > 20`)
at [blank1.c:1574]($source$/blank1.c#L1574); the adaptive limits
`stupid_bln_limit`/`clever_bln_limit` at
[blank1.c:1586]($source$/blank1.c#L1586). If almost the whole band is "strong"
(`fft1_lowlevel_fraction < 0.1`) it shows **"SELLIM ERROR"** and skips the update
([blank1.c:1553]($source$/blank1.c#L1553)).

</details>

## Parameters

The blanker is controlled from the `HG_PARMS hg` struct
([globdef.h:972]($source$/globdef.h#L972)):

| Field | Meaning |
|-------|---------|
| `clever_bln_mode` | smart blanker: 0 off, 1 auto-threshold, other = manual limit |
| `stupid_bln_mode` | dumb blanker: 0 off, 1 auto-threshold, other = manual limit |
| `clever_bln_factor` / `clever_bln_limit` | smart threshold = noise floor × factor |
| `stupid_bln_factor` / `stupid_bln_limit` | dumb threshold = noise floor × factor |
| `blanker_ston_fft1` / `blanker_ston_fft2` | S/N thresholds for the level split |
| `sellim_par1..8` | selective level-limiter tuning (see the split page) |
| `timf2_oscilloscope*` | the blanker-waveform oscilloscope display |

---

*Related:* [strong/weak channel split](/channel-split/) ·
[diversity and adaptive polarization](/diversity-polarization/) ·
[signal-chain overview](/signal-chain/)
