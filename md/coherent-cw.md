# Coherent CW and Morse decoding

Linrad's CW processing is designed for weak, non-perfect signals with QSB, chirp
and drift. It extracts the signal's carrier phase coherently and can decode Morse
to ASCII. The algorithm is a state machine driven by `cw_detect_flag`; the author
describes it in [`z_MORSE_DECODING.txt`]($source$/z_MORSE_DECODING.txt).

![Coherent CW / Morse decoding](/img/coherent-cw.png)

## Three bandwidths

The baseband time function is computed at three bandwidths, all complex and with
their phases rotated to match the carrier (so the signal is in I and any chirp/FM
appears in Q):

- `baseb` — the operator-selected bandwidth `bw`;
- `baseb_carrier` — `coh_factor` times narrower than `bw`; contains the on-off-keyed
  carrier (essentially 100 % AM);
- `baseb_wb` — at least 2× wider than `bw`.

<details><summary>In the source</summary>

The three bandwidths and the phase-alignment convention are described in
[`z_MORSE_DECODING.txt`]($source$/z_MORSE_DECODING.txt). `baseb_carrier` comes
from the narrow carrier filter in the second mixer
([mix2.c:246]($source$/mix2.c#L246)); `keying_spectrum` is also computed in
[`mix2.c`]($source$/mix2.c). CWDETECT states are defined in
[sigdef.h:5]($source$/sigdef.h#L5).

</details>

## Finding the keying speed

The power spectrum of the I component of `baseb_wb`, averaged over ~150 dots, has
peaks at the Morse "clock" frequency and its harmonics/sub-harmonics
(`keying_spectrum`). `evaluate_keying_spectrum()` picks the fundamental and derives
the dot length in samples, `cwbit_pts`. `make_ideal_waveform()` then builds the
ideal dash shape as seen through the selected baseband filter.

<details><summary>In the source</summary>

`evaluate_keying_spectrum()` — [coherent.c:77]($source$/coherent.c#L77);
`make_ideal_waveform()` — [coherent.c:212]($source$/coherent.c#L212);
`collect_ramp()` (power detector → `baseb_ramp`) —
[coherent.c:156]($source$/coherent.c#L156); `detect_cw_speed()` in
[`cwspeed.c`]($source$/cwspeed.c). Driven from `coherent_cw_detect()` states
`CWDETECT_CLEARED` / `CWDETECT_SEARCH_SPEED`
([coherent.c:283]($source$/coherent.c#L283)).

</details>

## Fitting dashes, then decoding

With a waveform template established (`CWDETECT_WAVEFORM_ESTABLISHED`), the decoder:

1. collects the average dash shape from regions where the power detector
   (`baseb_ramp`) shows the signal high for ≈ 3·`cwbit_pts`, refining `cwbit_pts`;
2. steps through the data fitting long regions to the dash shape, storing good
   fits in `cw[]`;
3. collects wideband dash/dot shapes and characterizes the intra-dash phase drift
   with two derivatives (frequency drift during a symbol), building noise-free
   reference functions;
4. interpolates over short gaps and guesses the symbol pattern where one option is
   clearly best (`short_region_guesses()`);
5. guesses characters across undecoded gaps by least-squares fitting the
   surrounding decoded signal, keeping only guesses valid in the Morse alphabet
   and choosing the best S/N (`character_guesses()`);
6. reconstructs a more accurate carrier from the guessed keying and re-detects
   from scratch.

The result is drawn on the coherent-CW graph and, for meteor scatter and similar,
output as ASCII.

<details><summary>In the source</summary>

State machine `coherent_cw_detect()` — [coherent.c:283]($source$/coherent.c#L283):
`first_find_parts()` / `second_find_parts()` (`CWDETECT_SOME_PARTS_FITTED`,
`CWDETECT_LIMITS_FOUND`), `init_cw_decode_region()` / `cw_decode_region()`
(`CWDETECT_REGION_*`), `init_cw_decode()` / `cw_decode()`
(`CWDETECT_SOME_ASCII_FITTED`); on failure it resets to `CWDETECT_CLEARED`.
Waveform fitting is in [`cohsub.c`]($source$/cohsub.c) (`fit_dash`,
`store_symmetry_adapted_dash`, `get_wb_average_dashes`); auto-decode and speed
estimation in [`cwdetect.c`]($source$/cwdetect.c), [`cwspeed.c`]($source$/cwspeed.c)
and [`morse.c`]($source$/morse.c). The 11 steps are enumerated in
[`z_MORSE_DECODING.txt`]($source$/z_MORSE_DECODING.txt).

</details>

## Related

The narrow carrier used here is produced by the
[second mixer](/mixers-filters-baseband/); the coherent phase extraction shares
the amplitude/phase measurement used for
[adaptive polarization](/diversity-polarization/). The same recovered carrier phase
drives the [coherent output modes (Coh1–Coh4)](/coherent-output/) for synchronous
and binaural listening.
