# Architecture: threads and buffers

Each stage of the DSP chain, plus I/O and the UI, runs in its own thread; stages
exchange data through power-of-two circular buffers.

![Threads and circular buffers](/img/architecture.png)

## Threads

Thread identities and states are in [`thrdef.h`]($source$/thrdef.h). The DSP-path
threads:

| Thread | Purpose |
|--------|---------|
| `THREAD_RX_ADINPUT` / `THREAD_*_INPUT` | device / network / file input → fills `timf1` |
| `THREAD_DO_FFT1C`, `THREAD_FFT1B1..6` | first FFT, spread over up to 6 workers |
| `THREAD_TIMF2` | `do_fft1_c()` → `make_timf2()` → `first_noise_blanker()` |
| `THREAD_SECOND_FFT` | recombine the two `timf2` parts → `fft2`; AFC / spur removal |
| `THREAD_NARROWBAND_DSP` | first mixer / AFC, timing |
| `THREAD_MIX2` | second mixer + baseband (`do_mix2` → `fft3_mix2`) |
| `THREAD_FFT3` | third FFT feeding the second mixer |
| `THREAD_RX_OUTPUT`, `THREAD_BLOCKING_RXOUT` | resample + demodulate → D/A |
| `THREAD_SCREEN`, `THREAD_USER_COMMAND` | display and UI |

Synchronization uses atomic status/command flags, an event set (`lir_await_event`
/ `lir_set_event`), and a small set of mutexes.

<details><summary>In the source</summary>

Thread IDs from [thrdef.h:47]($source$/thrdef.h#L47); thread states
(`THRFLAG_*`) [thrdef.h:6]($source$/thrdef.h#L6); atomic flag helpers
(`lir_interlocked_*`) [thrdef.h:184]($source$/thrdef.h#L184); events (`EVENT_*`)
[thrdef.h:125]($source$/thrdef.h#L125); mutexes [thrdef.h:162]($source$/thrdef.h#L162).
The narrow-band trio is created together at [menu.c:702]($source$/menu.c#L702).
OS-specific thread bodies are in [`lxsys.c`]($source$/lxsys.c) (Linux) and
[`wsys.c`]($source$/wsys.c) (Windows); they call the portable DSP entry points
(e.g. [lxsys.c:1397]($source$/lxsys.c#L1397) `do_mix2()`). Per-thread CPU/wall
time (`thread_tottim*`, `thread_cputim*`) is shown when **T** is pressed
([`z_TIMING.txt`]($source$/z_TIMING.txt)).

</details>

## Circular buffers

The conventions ([`z_BUFFERS.txt`]($source$/z_BUFFERS.txt)):

- A buffer `xxx` may be aliased by type — `xxx_char`, `xxx_short_int`/`xxx_shi`,
  `xxx_int`, `xxx_float` — all pointing at the same memory; the active type
  depends on `swfloat` and the stage.
- Sizes: `xxx_size` (elements), `xxx_bytes`, `xxx_mask = size-1` (for `&`-wrap),
  `xxx_block` (one update).
- **One creator** owns `xxx_pa` (next write position); **one consumer** owns
  `xxx_px` (next read position). Valid data occupies `px … pa`; `px == pa` is
  empty. Wrapping is `p = (p + n) & mask`.
- Extra users/modifiers keep their own named pointers (e.g. `timf2p_fit`).

The main DSP buffers, in order:

```
timf1 → fft1 → (split) → timf2 → fft2 → timf3/fft3 → baseb → daout
```

The **T** display lists the same buffers as delay times: `Raw`(timf1) → `fft1` →
`timf2` → `fft2` → `timf3` → `fft3` → `baseb` → `buf`(daout) → `D/A`.

<details><summary>In the source</summary>

Buffer naming/conventions: [`z_BUFFERS.txt`]($source$/z_BUFFERS.txt). Delay-time
readout: [`z_TIMING.txt`]($source$/z_TIMING.txt). The `timf2` buffer and its
pointers (`timf2_pa`, `timf2_px`, `timf2p_fit`, `timf2_mask`, …) are declared in
[fft1def.h:273]($source$/fft1def.h#L273).

</details>
