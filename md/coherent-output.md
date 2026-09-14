# Coherent output modes (Coh1–Coh4)

These are *listening* modes, not a decoder. Using the carrier phase recovered from
a narrow filter, Linrad can route the in-phase and quadrature parts of a signal to
the two ears in several ways — synchronous AM, or a binaural presentation that
separates amplitude from phase. They share the coherent machinery of
[coherent CW](/coherent-cw/) (the narrow carrier filter and phase extraction) but
produce audio for the operator instead of decoded Morse.

The `Coh` button on the baseband graph cycles `bg_coherent` from 0 (Off) through 4;
the neighbouring `Rat` value (`bg.coh_factor`) sets how much narrower the carrier
filter is than the audio passband. The operator chooses which carrier to lock onto
by placing the receive passband on it.

## The modes

| Button | What you hear | Output |
|--------|---------------|--------|
| **Off** (`Coh0`) | normal demodulation, no coherent processing | mono/stereo per mode |
| **Coh1** | the signal in one ear, the recovered carrier in the other | stereo |
| **Coh2** | binaural I/Q: amplitude (AM) in one ear, phase (FM) in the other — the "3D" soundscape | stereo |
| **Coh3** | synchronous: the in-phase component (I) to both ears | mono |
| **Coh4** | synchronous: the quadrature component (Q) to both ears | mono |

In **Coh3** and **Coh4** the recovered carrier phase rotates the signal so the
wanted modulation lands on one axis. With a BFO in use (CW/SSB) only the positive
half is passed; for AM/FM the DC (carrier) level is removed instead — synchronous
AM detection, which suppresses the noise and fading on the quadrature axis. Coh3
and Coh4 are mono (the same audio to both ears) and force single-channel baseband
processing; Coh1 and Coh2 are two-channel and cannot be combined with the
two-polarization or synthetic-stereo-by-delay outputs.

<details><summary>In the source</summary>

The output is built by the `switch (bg_coherent)` in `fft3_mix2()` —
[mix2.c:1774]($source$/mix2.c#L1774): signal + carrier
[mix2.c:1843]($source$/mix2.c#L1843) (Coh1); I/Q binaural
[mix2.c:1884]($source$/mix2.c#L1884) (Coh2); I to both ears
[mix2.c:1918]($source$/mix2.c#L1918) (Coh3, positive-only when `use_bfo`, else DC
removed); Q to both ears [mix2.c:1954]($source$/mix2.c#L1954) (Coh4). The
coherently detected I/Q it reads (`baseb[]`) is produced upstream by the coherent
function. Toggling: `BG_TOGGLE_COHERENT` increments `bg_coherent`
([baseb_graph.c:2577]($source$/baseb_graph.c#L2577)), wrapping past 4 to 0 and
forcing single-channel processing for modes ≥ 3
([baseb_graph.c:3086]($source$/baseb_graph.c#L3086),
[baseb_graph.c:3150]($source$/baseb_graph.c#L3150)); the button text `Coh%d` is
drawn at [screen.c:2971]($source$/screen.c#L2971) and the `Rat` (`coh_factor`)
button at [screen.c:2830]($source$/screen.c#L2830). The narrow carrier filter is
extracted in the second mixer [mix2.c:246]($source$/mix2.c#L246).

</details>

## Related

The carrier filter and phase extraction come from the
[second mixer](/mixers-filters-baseband/) and the same coherent code as
[coherent CW and Morse decoding](/coherent-cw/); locking onto a drifting carrier
relies on [AFC](/afc/). The synchronous output can be inspected on the coherent
graph's [oscilloscope](/oscilloscopes/).
