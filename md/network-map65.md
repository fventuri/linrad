# The network interface (and MAP65)

Linrad can multicast data from several points in its pipeline, and can take data from
the network as input. This lets several operators share one receiver, distributes the
processing across machines, and feeds external decoders such as **MAP65**. From
[`z_NETWORK.txt`]($source$/z_NETWORK.txt).

![Network interface and MAP65](/img/network.png)

## Formats and tap points

Any of these formats can be sent simultaneously; each goes to the base port plus a
format-dependent offset:

| Format | Offset | Description |
|--------|:------:|-------------|
| RAW16 / RAW18 / RAW24 | 0 / 1 / 2 | raw A/D data (16/18/24-bit) |
| FFT1 | 3 | first-FFT transforms (float) — the wideband spectrum |
| TIMF2 | 4 | noise-blanker output (16/32-bit int/float) |
| FFT2 | 5 | second-FFT transforms (16/32-bit int/float) |
| BASEBAND | 6 | baseband I/Q (16-bit) |

Multicasting uses groups `239.255.0.0`–`239.255.0.15`, so several receivers can send
in different groups at once; the port is set between 50000 and 65000 in steps of 10.
Each packet carries a `NET_RX_STRUCT` header (passband center/direction, timestamp,
block number, pointer) ahead of the payload.

<details><summary>In the source</summary>

Send functions per format: `lir_send_raw16/18/24`, `lir_send_fft1`,
`lir_send_timf2`, `lir_send_fft2`, `lir_send_baseb`, `lir_send_basebraw` —
[network.c:362]($source$/network.c#L362); driver `do_network_send()` /
`network_send()` — [rxin.c:669]($source$/rxin.c#L669) (thread
`THREAD_NETWORK_SEND`). Output-format flags `NET_RXOUT_*` and input flags
`NET_RXIN_*` in `ui.network_flag` — [globdef.h:237]($source$/globdef.h#L237). Packet
header `NET_RX_STRUCT` and the per-format `net_rxdata_*` buffers —
[globdef.h:1284]($source$/globdef.h#L1284); payload size `NET_MULTICAST_PAYLOAD`
([globdef.h:1283]($source$/globdef.h#L1283)).

</details>

## Network as input

RAW or FFT1 transforms received from the network can be used as Linrad's input, so
several operators listen to the same receiver/antenna, or one machine does the early
processing and sends a later stage on (same base port, different group) for another
machine to continue.

<details><summary>In the source</summary>

Network-input threads `THREAD_RX_RAW_NETINPUT` and `THREAD_RX_FFT1_NETINPUT`
([thrdef.h:48]($source$/thrdef.h#L48)); receive setup and the `par_netrec_ip`
mechanism in [`network.c`]($source$/network.c) (`par_netrec_filename`,
[network.c:121]($source$/network.c#L121)). Input format is selected by the
`NET_RXIN_*` bits of `ui.network_flag`.

</details>

## MAP65 and arbitrary destinations

For a decoder such as MAP65 (wideband EME) Linrad can multicast to any IP address
rather than only the `239.255.0.x` groups: put the destination in a file
`par_netsend_ip` and set −1 for the last number of the send address in the network
setup (then press W in the main menu). The receive address can likewise be read from
`par_netrec_ip`. MAP65 receives a Linrad wideband stream over this multicast.

<details><summary>In the source</summary>

`par_netsend_ip` / `par_netrec_ip` handling — [network.c:120]($source$/network.c#L120);
the −1 last-octet convention and W-to-apply are described in
[`z_NETWORK.txt`]($source$/z_NETWORK.txt). Multicast group base is
`239.255.0.0`; the send socket and `chk_send()` are in
[network.c:165]($source$/network.c#L165). A slave/master frequency-sharing protocol
(`SLAVE_MESSAGE`, `net_send_slaves_freq()`) coordinates linked instances.

</details>

## Related

The tapped stages are the [first FFT](/channel-split/),
[noise blanker / TIMF2](/noise-blankers/),
[second FFT](/mixers-filters-baseband/) and
[baseband](/mixers-filters-baseband/) of the main [signal chain](/signal-chain/).
