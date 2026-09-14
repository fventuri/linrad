# AFC (automatic frequency control)

AFC tracks a weak or drifting signal and steers the [first mixer](/mixers-filters-baseband/)
so the signal stays centered at 0 Hz. It is not tied to one mode: the same loop
runs under CW, SSB, AM and FM, which is why it is described here on its own rather
than as part of any single demodulator. Tracking a signal down to a narrow
filter is what makes weak-signal work and coherent detection possible.

![AFC tracking loop](/img/afc.png)

## The loop

The high-resolution spectrum is the input: when the second FFT is enabled AFC uses
`fft2` transforms, otherwise it falls back to the first-FFT spectrum. Averaging
more transforms finds weaker signals at the cost of delay. `collect_initial_spectrum()`
acquires the signal — averaged transforms, the peak located by parabolic
interpolation, and an initial polarization estimate on two-channel input — and the
loop then follows the peak from transform to transform. The measured frequency
error, bounded by `AFC_LOCK_RANGE` and rate-limited by `AFC_MAX_DRIFT`, becomes the
fine-tuning phase rotation applied inside the first mixer (`do_mix1_afc()` /
`fft2_mix1_afc()`), so the tuned signal re-enters the next transform closer to zero.

The signal-to-noise used to validate the track comes from `make_afc_signoi()` (noise
estimated from filters outside the signal); `make_ag_point()` turns each measurement
into a point on the **AFC graph**, the display that shows the tracked frequency over
time.

<details><summary>In the source</summary>

Acquisition/tracking in [`afcsub.c`]($source$/afcsub.c):
`collect_initial_spectrum()` [afcsub.c:34]($source$/afcsub.c#L34)
(`afcf_search_range = AFC_LOCK_RANGE · fftx_points_per_hz · ag.search_range`,
[afcsub.c:54]($source$/afcsub.c#L54)); `make_afc_signoi()`
[afcsub.c:693]($source$/afcsub.c#L693); `make_ag_point()`
[afcsub.c:792]($source$/afcsub.c#L792). Applied to the first mixer by
`do_mix1_afc()` [mix1.c:648]($source$/mix1.c#L648) and
`fft2_mix1_afc()` [mix1.c:863]($source$/mix1.c#L863). Which spectrum AFC uses is
noted in [z_SETTINGS.txt:163]($source$/z_SETTINGS.txt#L163). `AG_PARMS ag`
[globdef.h:884]($source$/globdef.h#L884); the AFC graph is drawn by
`fill_afc_graph()` [screen.c:1067]($source$/screen.c#L1067)
(graph type `AFC_GRAPH`, [globdef.h:549]($source$/globdef.h#L549)).

</details>

## Settings

Three general parameters control it ([globdef.h:307]($source$/globdef.h#L307)):

| Parameter | Effect |
|-----------|--------|
| `AFC_ENABLE` | 0 = off; 1 = on; 2 = on **and** automatic spur removal |
| `AFC_LOCK_RANGE` | how far (Hz) AFC will search / hold a signal |
| `AFC_MAX_DRIFT` | maximum tracked drift rate |

AFC is active only when `AFC_ENABLE != 0` **and** `AFC_LOCK_RANGE != 0`; setting
`AFC_ENABLE` to 0 forces `AFC_LOCK_RANGE` to 0. With `AFC_ENABLE == 2` the
automatic [spur search](/spur-removal/) also runs, so stable carriers are removed
from the spectrum before the peak is tracked.

<details><summary>In the source</summary>

Values `AFC_ENABLE` / `AFC_LOCK_RANGE` / `AFC_MAX_DRIFT` —
[globdef.h:307]($source$/globdef.h#L307). The
`AFC_ENABLE != 0 && AFC_LOCK_RANGE != 0` guard gates the loop throughout
([buf.c:1087]($source$/buf.c#L1087), [fft2.c:1833]($source$/fft2.c#L1833),
[wide_graph.c:183]($source$/wide_graph.c#L183)); `AFC_ENABLE == 0` clears the lock
range at [buf.c:836]($source$/buf.c#L836). The `AFC_ENABLE == 2` auto-spur branch is
in [spur.c:143]($source$/spur.c#L143) and [wide_graph.c:1506]($source$/wide_graph.c#L1506).

</details>

## Related

AFC works on the [second FFT](/mixers-filters-baseband/) spectrum and drives the
[first mixer](/mixers-filters-baseband/); its `AFC_ENABLE == 2` mode ties it to
[spur removal](/spur-removal/). A second, tighter AFC stage inside the narrowband
chain is what enables [coherent CW](/coherent-cw/) on very weak signals.
