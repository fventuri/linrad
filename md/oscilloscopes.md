# Oscilloscope displays

Alongside the spectra and waterfalls, Linrad has three time-domain oscilloscopes,
each tapping the signal chain at a different point. All three are expert-mode
displays, toggled from the graph they belong to. They show the actual waveform —
useful for judging impulse noise, AGC behaviour and coherent detection that the
averaged spectra hide.

| Scope | Where | Shows | Toggle / state |
|-------|-------|-------|----------------|
| Blanker (timf2) | high-resolution graph | the wideband `timf2` time function and the noise blanker acting on it | `hg.timf2_oscilloscope` (7 display states) |
| Baseband | baseband graph | the baseband time function after the audio filter | `bg.oscill_on`, gain `bg.oscill_gain` |
| Coherent | coherent graph | the coherent time traces (I/Q and recovered carrier) | `cg.oscill_on` |

The **blanker oscilloscope** is described on the
[noise blankers](/noise-blankers/) page — it is where the pulse-fitting blanker's
work is visible. The **baseband oscilloscope** shows what reaches the demodulator,
with its own gain control. The **coherent oscilloscope** shows the traces built for
[coherent detection](/coherent-output/), so the synchronous/binaural output can be
seen as well as heard.

<details><summary>In the source</summary>

Blanker/timf2 scope: `hg.timf2_oscilloscope` toggled by `HG_TIMF2_STATUS`
([hires_graph.c:501]($source$/hires_graph.c#L501), cycling 0–6), fed by the
`timf2_oscilloscope_*` state set up in [`buf.c`]($source$/buf.c) and the
oscilloscope-max pick in [blank1.c:1004]($source$/blank1.c#L1004). Baseband scope:
`BG_OSCILLOSCOPE` toggles `bg.oscill_on` [baseb_graph.c:2541]($source$/baseb_graph.c#L2541),
`BG_OSC_INCREASE` / `BG_OSC_DECREASE` scale `bg.oscill_gain`. Coherent scope:
`CG_OSCILLOSCOPE` toggles `cg.oscill_on` [coherent.c:548]($source$/coherent.c#L548);
the traces are drawn in [`coh_osc.c`]($source$/coh_osc.c) (`update_cg_traces()`
[coh_osc.c:45]($source$/coh_osc.c#L45), `make_cg_trace()`
[coh_osc.c:88]($source$/coh_osc.c#L88)). All three require
`OPERATOR_SKIL_EXPERT`.

</details>

## Related

The blanker scope belongs to the [noise blankers](/noise-blankers/); the baseband
scope to the baseband graph in the [mixers/baseband chain](/mixers-filters-baseband/);
the coherent scope to the [coherent output modes](/coherent-output/) and
[coherent CW](/coherent-cw/).
