# FM and wideband-FM

FM demodulation runs on the complex baseband. Narrowband FM produces audio
directly from the instantaneous frequency; wideband FM (broadcast) decodes the
full stereo composite, including the 19 kHz pilot, the 38 kHz stereo subcarrier
and 57 kHz RDS.

![FM / WFM decoding](/img/fm.png)

## FM detector

The detector computes the instantaneous frequency as the time derivative of the
baseband phase. The phase is obtained either directly with `atan2`, or, for better
weak-signal behaviour, from a short windowed FFT that picks the dominant bin and
interpolates (`fmfix`). With two channels the second channel is detected in
parallel. If the requested audio bandwidth already equals the baseband bandwidth,
the detector output goes straight to `baseb_out`.

<details><summary>In the source</summary>

`detect_fm()` — [fm.c:93]($source$/fm.c#L93) (called from
[mix2.c:1838]($source$/mix2.c#L1838)); phase/frequency estimator `fmfix()` —
[fm.c:45]($source$/fm.c#L45) (`atan2` when `fm_n == 0`, otherwise the
windowed-FFT peak). The audio-bandwidth decision (`use_audio_filter`) is at
[fm.c:108]($source$/fm.c#L108). BG parameters `fm_mode`, `fm_subtract`,
`fm_audio_bw` are in [globdef.h:1032]($source$/globdef.h#L1032).

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
