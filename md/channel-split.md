# First FFT and the strong/weak channel split

The split separates strong signals from weak signals + noise before the noise
blanker runs, so the blanker sees impulse noise without strong signals swamping
it. From [`z_SETTINGS.txt`]($source$/z_SETTINGS.txt):

> If the second FFT is enabled, the transforms of the first FFT are split into two
> sets of transforms. Both sets are back transformed to produce two functions of
> time, one of which contains all strong signals … while the other contains
> impulse noise and weak signals.

![Strong/weak split and back-transform](/img/channel-split.png)

## First FFT

The first FFT ([`fft1.c`]($source$/fft1.c), driven from
[`wcw.c`]($source$/wcw.c)) transforms `timf1` into the wideband complex spectrum
`fft1` and the spectrum/waterfall. Its analysis window `fft1_window` (a `sin^p`
power set by `genparm[FIRST_FFT_SINPOW]`) doubles as the anti-alias filter for the
later decimation, so power ≥ 2 is needed for good spur suppression. Transforms
overlap in time by `fft1_interleave_points` (= `fft1_size/2` for a `sin^2`
window). The FFT can be spread over up to 6 worker threads or run on a GPU.

## Classifying each bin: `liminfo[]`

The routing decision for each first-FFT bin is stored in the float array
`liminfo[]` ([sellim.c:79]($source$/sellim.c#L79)):

```
liminfo[i]  < 0  →  strong slot, amplitude factor = 1
liminfo[i] == 0  →  weak slot   (blanker input)
liminfo[i]  > 0  →  strong slot, amplitude factor = liminfo[i]   (< 1, limited)
```

Two producers set it: `fft2_update_liminfo()` — the automatic level limiter, using
the finer `fft2` resolution to mark and scale strong bins — and
`selfreq_liminfo()`, which forces the selected passband into a chosen class.

Because some bins are moved to the strong slot, the weak-signal time function is
built from a reduced bandwidth, so a pulse there is weaker and longer than the
stored reference assumes. `liminfo_amplitude_factor` compensates the blanker's
amplitude estimate; if ≥ 50 % of bins are strong, the clever blanker is skipped.

<details><summary>In the source</summary>

`liminfo` meaning: [sellim.c:79]($source$/sellim.c#L79). Automatic limiter:
`fft2_update_liminfo()` — [sellim.c:159]($source$/sellim.c#L159), tuned by
`hg.sellim_par1..8` and `genparm[SELLIM_MAXLEVEL]`. Passband forcing:
`selfreq_liminfo()` — [sellim.c:38]($source$/sellim.c#L38).
`liminfo_amplitude_factor`: [sellim.c:117]($source$/sellim.c#L117) (the
half-bandwidth amplitude note is at [sellim.c:124]($source$/sellim.c#L124)).

</details>

## Split and back-transform: `make_timf2()`

`make_timf2()` ([timf2.c:31]($source$/timf2.c#L31)) writes each bin into one of two
interleaved slots. For one channel the split array is 4 floats per bin —
`(weak_re, weak_im, strong_re, strong_im)`; for two channels it is 8. Bins outside
the band are zeroed in both slots.

`fft1back_one()` / `fft1back_two()` inverse-transform **both slots in one pass**
(the butterflies operate on the interleaved words). `fft1back_fp_finish()`
overlap-adds the result into the `timf2` circular buffer:

- `sin^2` window: overlapping halves are simply **added** (`sin²+cos²=1`) — exact
  reconstruction with no window division;
- arbitrary window: multiply by the inverted window and keep the central portion.

While writing each sample it also stores the per-sample power into `timf2_pwr[]`,
which is what the noise blanker scans.

`timf2` layout per time sample: one channel `timf2[4k+0..1]` = weak (I,Q),
`timf2[4k+2..3]` = strong; two channels `timf2[8k+0..3]` = weak (ch1, ch2),
`timf2[8k+4..7]` = strong.

The two parts are added back into one time function for the second FFT, so the
split is transparent to everything downstream of the blanker.

<details><summary>In the source</summary>

Split (float, one channel): [timf2.c:39]($source$/timf2.c#L39); two channels:
[timf2.c:77]($source$/timf2.c#L77); the 16-bit path uses `split_one()` /
`split_two()` ([`split.s`]($source$/split.s)). Back-transform:
`fft1back_one()` — [timf2.c:689]($source$/timf2.c#L689);
`fft1back_two()` — [timf2.c:210]($source$/timf2.c#L210). Overlap-add and
per-sample power: `fft1back_fp_finish()` — [timf2.c:970]($source$/timf2.c#L970)
(`sin^2` add at [timf2.c:1003]($source$/timf2.c#L1003)); scale factor
`ampfac = 1/(1<<genparm[FIRST_BCKFFT_ATT_N])` at
[timf2.c:976]($source$/timf2.c#L976). `fft1_lowlevel_fraction` (weak-slot fraction)
at [timf2.c:205]($source$/timf2.c#L205). Recombination for the second FFT is in
`make_fft2()` (driven from [wcw.c:250]($source$/wcw.c#L250)).

</details>
