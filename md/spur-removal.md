# Spur removal

Spur removal cancels stable, unmodulated carriers — birdies, reference leakage,
switching-supply lines — from the fine `fft2` spectrum before AFC and the audio
path use it. Each spur is modeled as a drifting complex sinusoid tracked by a PLL
and (with two channels) on its own polarization, then subtracted.

![Spur removal](/img/spur-removal.png)

## Detecting spurs

When AFC is in auto mode (`genparm[AFC_ENABLE] == 2`, `wg.spur_inhibit == 0`),
Linrad searches the averaged spectrum for narrow peaks that stand above a noise
threshold and are frequency-stable, adding them as spurs up to
`genparm[MAX_NO_OF_SPURS]`. The search runs incrementally so it does not stall the
DSP thread.

<details><summary>In the source</summary>

Driver `spur_removal()` — [wcw.c:204]($source$/wcw.c#L204) (called from
`second_fft()`). Candidate search and setup: `init_spur_elimination()` and
`spursearch_spectrum_cleanup()` in [`spursub.c`]($source$/spursub.c#L177); the
threshold is `noise × 10^(1.5/√(3·spur_speknum))`
([spursub.c:93]($source$/spursub.c#L93)). Spur size constants:
`SPUR_N = 3`, `SPUR_SIZE = 8`, `SPUR_WIDTH = 7`
([globdef.h:173]($source$/globdef.h#L173), [seldef.h:6]($source$/seldef.h#L6));
`genparm[MAX_NO_OF_SPURS]` and `genparm[SPUR_TIMECONSTANT]`
([globdef.h:311]($source$/globdef.h#L311)).

</details>

## Modeling and subtracting each spur

For every spur, each new second-FFT frame updates:

- a **PLL** describing the spur's phase as a polynomial — phase, phase slope and
  phase curvature (`φ, φ′, φ″`) — i.e. frequency plus drift, refined against the
  in-phase / orthogonal split of the measured signal;
- (two channels) the spur's **polarization** `c1, c2, c3`, from the same 2×2
  covariance used for the wanted signal, so the spur is placed in one channel and
  noise in the other.

The modeled spur is subtracted from the spectrum. If the model drifts off the
signal, `spur_relock()` re-acquires it.

<details><summary>In the source</summary>

Main loop `eliminate_spurs()` — [spur.c:36]($source$/spur.c#L36); PLL update
`refine_pll_parameters()` — [spur.c:634]($source$/spur.c#L634) (uses
`spur_d0pha`/`spur_d1pha`/`spur_d2pha` for `φ, φ′, φ″`); polarization update
`update_spur_pol()` — [spur.c:496]($source$/spur.c#L496) (same covariance form as
`fft3_mix2()`, see [diversity](/diversity-polarization/)); subtraction
`remove_spur()` — [spur.c:596]($source$/spur.c#L596); re-lock `spur_relock()` —
[spur.c:682]($source$/spur.c#L682). Initial per-spur setup and polarization:
`initial_remove_spur()` / `make_spur_pol()` in
[`spursub.c`]($source$/spursub.c#L348).

</details>

## Related

Spur removal operates on the [second FFT](/mixers-filters-baseband/) output and
feeds [AFC](/mixers-filters-baseband/); its polarization tracking is the same
computation as [adaptive polarization](/diversity-polarization/) and the
[pulse blanker](/noise-blankers/).
