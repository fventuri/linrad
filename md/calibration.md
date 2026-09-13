# Calibration

Calibration builds a digital filter whose response is the inverse of the analog
hardware's, so the data entering the DSP has flat amplitude and linear phase. Because
absent frequencies cannot be recovered, a **desired response** is also applied so the
total filter does not have extreme gain or Q. For direct-conversion hardware
(two audio channels per HF channel) a separate step corrects I/Q amplitude and phase
imbalance. From [`z_CALIBRATE.txt`]($source$/z_CALIBRATE.txt):

> The calibration procedure adds a digital filter with a frequency response that is
> the inverse of your hardware's response … the desired frequency response must be
> chosen for the total digital filter to not have extreme gain or Q.

![Calibration](/img/calibration.png)

## Filter correction

A broadband calibration pulse is passed through the hardware and captured. Linrad
triggers on the pulse, averages many pulses to get the hardware's amplitude and phase
response, inverts it, multiplies by the desired passband shape, and stores the result
as `fft1_filtercorr[]`. The number of points is reduced (removing spur-induced
discontinuities and noise) to a size matched to the hardware's Q. The correction is
applied in the first FFT as a **per-bin complex multiply**, giving flat amplitude and
linear phase across the band.

<details><summary>In the source</summary>

Pulse capture and averaging: `cal_filtercorr()` —
[calibrate.c:376]($source$/calibrate.c#L376) (triggering via `trig_power`,
`START_PULSES` / `INIT_PULSENUM`, [caldef.h:4]($source$/caldef.h#L4)). Final
reduction/normalization: `final_filtercorr_init()` —
[calibrate.c:50]($source$/calibrate.c#L50); RAM update `cal_update_ram()` and
`make_cal_fft1_filtercorr()` in [`calsub2.c`]($source$/calsub2.c#L249). The per-bin
complex multiply by `fft1_filtercorr` is in [`fft1.c`]($source$/fft1.c#L4121) (one
and two channel). `fft1_desired` holds the target response. The `CALAMP` bit of
`fft1_calibrate_flag` is defined at [globdef.h:341]($source$/globdef.h#L341).

</details>

## I/Q balance (direct conversion)

When each HF channel is fed as two audio channels (I and Q) from two mixers, gain and
phase mismatches produce an image. Linrad measures the imbalance across the band and
stores a fold-correction (`iq_foldcorr`) that cancels the image; it is applied to the
raw I/Q. This step exists only in direct-conversion setups.

<details><summary>In the source</summary>

I/Q balance in [`caliq.c`]($source$/caliq.c): measurement `cal_iqbalance()` —
[caliq.c:403]($source$/caliq.c#L403); correction build `update_iq_foldcorr()` —
[caliq.c:222]($source$/caliq.c#L222); fold-correction expand/contract
`expand_foldcorr()` / `contract_foldcorr()` —
[caliq.c:40]($source$/caliq.c#L40); write `write_iq_foldcorr()` —
[caliq.c:152]($source$/caliq.c#L152). The `CALIQ` bit is at
[globdef.h:340]($source$/globdef.h#L340). Calibration modes are the `CAL_TYPE_*`
states in [caldef.h:10]($source$/caldef.h#L10).

</details>

## Per-mode files

Calibration is done per mode; the files can be shared by copying or symlinking. Each
mode has a filter-correction file `dsp_<mode>_corr` and, for direct conversion, an
I/Q file `dsp_<mode>_iqcorr` (modes: `wcw`, `cw`, `hsms`, `ssb`, `fm`, `am`, `qrss`,
`txtest`, `test`, `tune`). The full list is in
[`z_CALIBRATE.txt`]($source$/z_CALIBRATE.txt).

## Related

The two-RF-channel amplitude/phase balance between front-ends (`pg.ch2_amp` /
`pg.ch2_phase`) is a separate adjustment on the
[polarization graph](/diversity-polarization/); the corrected `fft1` spectrum feeds
the whole [signal chain](/signal-chain/).
