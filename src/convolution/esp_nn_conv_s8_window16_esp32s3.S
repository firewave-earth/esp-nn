// SPDX-License-Identifier: Apache-2.0
//
// esp_nn_conv_s8_window16_esp32s3: one output channel of an int8 convolution
// whose taps fit in one 16-byte window per pixel. Same values as
// esp_nn_conv_s8_window16_ansi; esp_nn_ansi_headers.h has the arithmetic.
//
// Per pixel: one EE.VMULAS.S8.ACCX of window and filter, which also loads the
// next window, then the requantisation in scalar registers. Pixels go in pairs,
// A then B, so the instructions of one fill the ACCX and multiplier latencies of
// the other. Each pair's B is clamped and stored at the start of the next pair.
//
// a2: windows, 16-byte aligned; reads count + 1 windows when count is even
// a3: filter, 16 bytes, aligned
// a4: out
// a5: count
// a6: out_stride
// a7: quant (conv_window16_quant_t)
//
// a8, a7: A and its sign. a3, a10: B and its sign. a9 bias4, a11 mult,
// a12 round, a13 act_min, a14 act_max, SAR shift.

    .text
    .align  4
    .global esp_nn_conv_s8_window16_esp32s3
    .type   esp_nn_conv_s8_window16_esp32s3, @function

esp_nn_conv_s8_window16_esp32s3:
    entry       a1, 32
    blti        a5, 1, .Lw16_done

    ee.vld.128.ip   q7, a3, 0                       // filter
    ee.vld.128.ip   q0, a2, 16                      // window 0
    l32i        a9, a7, 0                           // bias4
    l32i        a11, a7, 4                          // mult
    l32i        a12, a7, 8                          // round
    l32i        a10, a7, 12                         // shift
    l32i        a13, a7, 16                         // act_min
    l32i        a14, a7, 20                         // act_max
    ssr         a10
    srai        a15, a5, 1                          // pairs
    beqz        a15, .Lw16_last

    // First pair: nothing to finish from before.
    ee.zero.accx
    ee.vmulas.s8.accx.ld.ip q1, a2, 16, q0, q7      // A; q1 = B's window
    nop
    nop
    rur.accx_0  a8
    ee.zero.accx
    ee.vmulas.s8.accx.ld.ip q0, a2, 16, q1, q7      // B; q0 = next window
    addx4       a8, a8, a9                          // A: 4x
    srai        a7, a8, 31
    rur.accx_0  a3
    mulsh       a8, a8, a11                         // A: y
    addx4       a3, a3, a9                          // B: 4x
    srai        a10, a3, 31
    addx2       a8, a7, a8                          // A: y - 2 [x < 0]
    mulsh       a3, a3, a11                         // B: y
    add         a8, a8, a12
    sra         a8, a8
    addx2       a3, a10, a3
    max         a8, a8, a13
    add         a3, a3, a12
    min         a8, a8, a14
    sra         a3, a3
    s8i         a8, a4, 0                           // store A
    add         a4, a4, a6                          // a4: B's slot

    addi        a15, a15, -1
    loopgtz     a15, .Lw16_pairs_end
    ee.zero.accx
    max         a3, a3, a13                         // previous B
    ee.vmulas.s8.accx.ld.ip q1, a2, 16, q0, q7      // A
    min         a3, a3, a14
    s8i         a3, a4, 0                           // store previous B
    rur.accx_0  a8
    add         a4, a4, a6                          // a4: A's slot
    ee.zero.accx
    ee.vmulas.s8.accx.ld.ip q0, a2, 16, q1, q7      // B
    addx4       a8, a8, a9
    srai        a7, a8, 31
    rur.accx_0  a3
    mulsh       a8, a8, a11
    addx4       a3, a3, a9
    srai        a10, a3, 31
    addx2       a8, a7, a8
    mulsh       a3, a3, a11
    add         a8, a8, a12
    sra         a8, a8
    addx2       a3, a10, a3
    max         a8, a8, a13
    add         a3, a3, a12
    min         a8, a8, a14
    sra         a3, a3
    s8i         a8, a4, 0                           // store A
    add         a4, a4, a6                          // a4: B's slot
.Lw16_pairs_end:

    max         a3, a3, a13                         // last pair's B
    min         a3, a3, a14
    s8i         a3, a4, 0
    add         a4, a4, a6

.Lw16_last:
    bbci        a5, 0, .Lw16_done
    // Odd count: the last window is in q0.
    ee.zero.accx
    ee.vmulas.s8.accx q0, q7
    nop
    nop
    rur.accx_0  a8
    addx4       a8, a8, a9
    srai        a7, a8, 31
    mulsh       a8, a8, a11
    addx2       a8, a7, a8
    add         a8, a8, a12
    sra         a8, a8
    max         a8, a8, a13
    min         a8, a8, a14
    s8i         a8, a4, 0

.Lw16_done:
    retw.n

    .size   esp_nn_conv_s8_window16_esp32s3, . - esp_nn_conv_s8_window16_esp32s3
