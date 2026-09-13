# Diversity and adaptive polarization

With two coherent RF receivers sharing a common local oscillator, Linrad combines
the two channels sample-by-sample into the linear combination that maximizes the
wanted signal's S/N. The same covariance computation is applied to the wanted
signal (in the second mixer), to each interference pulse (in the noise blanker),
and to derive the initial polarization at acquisition (in AFC).

![Adaptive polarization](/img/diversity-polarization.png)

## Two channels

`ui.rx_rf_channels` is 1 or 2. The two-RF-channel radio-interface modes are
described in [`z_SETTINGS.txt`]($source$/z_SETTINGS.txt): two RF + two audio
channels (normal audio) and two RF + four audio channels (direct conversion). Both
require common oscillators so the relative phase is stable. Typical uses: the two
feeds of a crossed-Yagi (removes Faraday-rotation loss on EME; receives any
linear/elliptical/circular polarization optimally), or two antennas on HF
(adaptive direction finding / interference nulling). If the driver can only open
two independent stereo pairs rather than one 4-channel device, the inter-pair
phase is unknown, so absolute ellipticity/sense cannot be shown, though the
sensitivity and rejection benefit remains.

## The combining coefficients

The wanted-signal polarization is held in `PG_PARMS pg` as three real coefficients
`c1, c2, c3` (with `c1²+c2²+c3²=1`). They combine channels X and Y into A (all
signal) and B (orthogonal, noise only):

```
re_A = c1·re_X + c2·re_Y + c3·im_Y
im_A = c1·im_X + c2·im_Y − c3·re_Y
re_B = c1·re_Y − c2·re_X + c3·im_X
im_B = c1·im_Y − c2·im_X − c3·re_X
```

`c1` is `cos a` (the polarization-plane angle); `c2/c3` carry `sin a` split by the
real/imaginary parts of the inter-channel correlation (ellipticity / voltage
phase). A becomes the demodulated audio; B is available for display.

<details><summary>In the source</summary>

`PG_PARMS pg`: [globdef.h:1060]($source$/globdef.h#L1060). The A/B combination is
applied per bin, together with the baseband filter, at
[mix2.c:336]($source$/mix2.c#L336); the algebra is written out in the comments at
[mix2.c:487]($source$/mix2.c#L487).

</details>

## Adapting the coefficients

Auto-adaptation (when `pg.adapt == 0`) accumulates a spectrum-power-weighted 2×2
complex covariance of the combined signals A, B over the passband:

```
a2 += r1·|A|²        b2 += r1·|B|²
re += r1·Re(A·B̄)     im += r1·Im(A·B̄)      r1 = P4SCALE·(spectrum − noise_floor)
```

If the polarization is already optimal, `a2` (S+N) is large and `b2`, `re`, `im`
(noise) are small. To track weak signals, results are integrated over a ring of 32
frames (`POLEVAL_SIZE`) before any change is considered. Two guards must pass: the
two-dimensional phase must be stable across frames (`r2 ≤ 0.2`) and the estimated
noise fraction `noi2 = a2·b2 − |xy|²` must be `≤ 0.12`. New coefficients are then
computed (same closed form as the pulse case), composed with the current ones, and
blended in with time constant `pg.avg` and renormalized, so the polarization
tracks smoothly.

<details><summary>In the source</summary>

All in `fft3_mix2()`, two-channel branch, [`mix2.c`]($source$/mix2.c):
covariance accumulation [mix2.c:344]($source$/mix2.c#L344); frame ring
`poleval_data[]` [mix2.c:398]($source$/mix2.c#L398) (`POLEVAL_SIZE = 32`,
[sigdef.h:85]($source$/sigdef.h#L85)); stability guards
[mix2.c:419]($source$/mix2.c#L419) (`r2 ≤ 0.2`, `noi2 ≤ 0.12`); new coefficients
[mix2.c:442]($source$/mix2.c#L442); composition with current + slew-limit /
renormalize [mix2.c:497]($source$/mix2.c#L497). Correlation modes bypass the update
([mix2.c:271]($source$/mix2.c#L271)).

</details>

## Manual polarization and initial acquisition

The polarization graph ([`pol_graph.c`]($source$/pol_graph.c)) draws the
polarization ellipse/azimuth and lets the user set the polarization by hand when
**AUTO** (`pg.adapt`) is off. `pg.ch2_amp` / `pg.ch2_phase` correct hardware
imbalance between the two front-ends. At acquisition,
`collect_initial_spectrum()` seeds `pg.c1..c3` from the acquisition spectrum using
the same covariance quantities.

<details><summary>In the source</summary>

`make_pol_graph()` — [pol_graph.c:391]($source$/pol_graph.c#L391);
`ampl_phase_to_c()` and `az_cal()` — [pol_graph.c:159]($source$/pol_graph.c#L159).
Initial polarization: `collect_initial_spectrum()` —
[afcsub.c:34]($source$/afcsub.c#L34).

</details>

## Relationship to the noise blanker

The wanted-signal combiner and the [pulse blanker](/noise-blankers/) use the same
covariance → optimum-combination math with opposite goals: the combiner puts all
*signal* into A; the blanker puts all *pulse* into one component so it can be
subtracted from both channels. The combiner integrates over 32 frames with a
noise-fraction guard of 0.12; the blanker works per pulse with a guard of 0.15.
