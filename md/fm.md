# FM and wideband-FM

FM demodulation runs on the complex baseband. Narrowband FM produces audio
directly from the instantaneous frequency; wideband FM (broadcast) decodes the
full stereo composite, including the 19 kHz pilot, the 38 kHz stereo subcarrier
and 57 kHz RDS.

![FM / WFM decoding](/img/fm.png)

## FM detector

The detector computes the instantaneous frequency as the time derivative of the
baseband phase (`fmfix()`). How that phase is estimated is selected by the operator
with the **FM detection mode**. With two channels the second channel is detected in
parallel. If the requested audio bandwidth already equals the baseband bandwidth,
the detector output goes straight to `baseb_out`.

## Detection modes (FM0–FM3)

The `FM` button on the baseband graph cycles `bg.fm_mode` through four states. Mode
0 differentiates the raw `atan2` phase directly — lowest latency, best on strong
signals. Modes 1–3 instead estimate the phase from the dominant bin of a short
windowed FFT (finding the peak and interpolating across it), which rejects
off-frequency noise and improves weak-signal FM; a larger FFT gives finer frequency
resolution at the cost of time resolution and computation.

| Button | Phase estimate | FFT size |
|--------|----------------|----------|
| **FM0** | direct `atan2`, per sample | — |
| **FM1** | windowed FFT, dominant bin + interpolation | 8 |
| **FM2** | windowed FFT | 16 |
| **FM3** | windowed FFT | 32 |

Internally the button maps to the FFT order `fm_n = bg.fm_mode + 2`, which is then
forced to 0 (the `atan2` path) when it would be below 3 — so FM0 is `fm_n == 0`
(`fm_size == 1`) and FM1–FM3 are `fm_n` 3/4/5. Changing the mode rebuilds the
baseband stage.

<details><summary>In the source</summary>

`detect_fm()` — [fm.c:93]($source$/fm.c#L93) (called from
[mix2.c:1838]($source$/mix2.c#L1838)); phase/frequency estimator `fmfix()` —
[fm.c:45]($source$/fm.c#L45): `atan2` when `fm_n == 0` ([fm.c:51]($source$/fm.c#L51)),
otherwise the windowed-FFT peak with 3-bin interpolation
([fm.c:64]($source$/fm.c#L64)). The mode → FFT-size mapping
(`fm_n = bg.fm_mode + 2`, `if(fm_n < 3) fm_n = 0`, `fm_win` = `make_window(4,…)`)
is at [baseb_graph.c:4138]($source$/baseb_graph.c#L4138); the `FM` button
(`BG_TOGGLE_FM_MODE`) wraps at `MAX_FM_FFTN-2 == 3`
([baseb_graph.c:2798]($source$/baseb_graph.c#L2798), label `FM%d`
[screen.c:2839]($source$/screen.c#L2839); `MAX_FM_FFTN` / `MAX_FM_FFTSIZE`
[sigdef.h:2]($source$/sigdef.h#L2)). The audio-bandwidth decision
(`use_audio_filter`) is at [fm.c:108]($source$/fm.c#L108). BG parameters `fm_mode`,
`fm_subtract`, `fm_audio_bw` are in [globdef.h:1032]($source$/globdef.h#L1032).

</details>

## Wideband FM: composite, stereo and RDS

For broadcast WFM the demodulated composite is split with low-pass and subcarrier
filters into the mono sum (L+R), the stereo difference (L−R, recovered from the
38 kHz subcarrier regenerated from the 19 kHz pilot) and the 57 kHz RDS channel;
the sum and difference are combined into L/R audio with de-emphasis. This path is
taken only with one channel, a detected pilot, and `fm_subtract != 2`; otherwise
Linrad uses the narrowband path (`nbfm`). An optional **subtract** mode removes the
dominant carrier before further processing to cut intermodulation from adjacent
channels.

<details><summary>In the source</summary>

WFM steps are numbered in the comments of `detect_fm()`: composite low-pass
(`fmfil55`/`fmfil70`) [fm.c:326]($source$/fm.c#L326); 19 kHz pilot detection
[fm.c:372]($source$/fm.c#L372); 38 kHz stereo and 57 kHz RDS
[fm.c:387]($source$/fm.c#L387); audio filter `fm_audiofil`
[fm.c:417]($source$/fm.c#L417); RDS low-pass `fmfil_rds`
[fm.c:451]($source$/fm.c#L451). The `nbfm` branch is selected at
[fm.c:189]($source$/fm.c#L189). Filter and channel arrays (`fmfil*_fir`,
`baseb_fm_sumchan`/`diffchan`, `fm_pilot_tone`, `baseb_fm_pil2`/`pil3`) are
declared in [sigdef.h:226]($source$/sigdef.h#L226). Several `WFM_*` network modes
stream the raw or partially processed baseband for analysis.

</details>

## Related

FM runs after the [baseband stage](/mixers-filters-baseband/); the second channel
it uses (`baseb_raw_orthog` / `baseb_carrier`) comes from the
[second mixer](/mixers-filters-baseband/).
