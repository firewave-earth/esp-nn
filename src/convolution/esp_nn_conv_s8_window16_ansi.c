// SPDX-License-Identifier: Apache-2.0
//
// Portable version of esp_nn_conv_s8_window16 and the per channel constants it
// uses. esp_nn_ansi_headers.h shows why the requantisation is exact.

#include <stdint.h>

#include <common_functions.h>
#include <esp_nn_defs.h>

int esp_nn_conv_s8_window16_quant_init(const int8_t *filter,
                                       const int32_t bias,
                                       const int32_t in_offset,
                                       const int32_t out_offset,
                                       const int32_t out_mult,
                                       const int32_t out_shift,
                                       const int32_t activation_min,
                                       const int32_t activation_max,
                                       conv_window16_quant_t *quant)
{
    // A zero multiplier (a channel of zero weights) outputs out_offset at any
    // shift, which the formula gives with rs = 1.
    const int32_t rs = out_mult == 0 ? 1 : -out_shift;
    if (rs < 1 || rs > 21 || out_mult < 0 || out_offset < -128 || out_offset > 127) {
        return -1;
    }
    int32_t sum = 0, abs_sum = 0;
    for (int t = 0; t < 16; t++) {
        sum += filter[t];
        abs_sum += filter[t] < 0 ? -filter[t] : filter[t];
    }
    // Window bytes are int8, so |acc + bias| stays below this. Under 2^29, 4x
    // fits in int32, and so does y + round with rs up to 21.
    const int64_t bias_eff = (int64_t) bias + (int64_t) in_offset * sum;
    const int64_t bound = (bias_eff < 0 ? -bias_eff : bias_eff) + 128 * (int64_t) abs_sum;
    if (bound >= (1 << 29)) {
        return -1;
    }
    quant->bias4 = (int32_t) (bias_eff * 4);
    quant->mult = out_mult;
    quant->round = 1 + (1 << rs) + out_offset * (1 << (rs + 1));
    quant->shift = rs + 1;
    quant->act_min = activation_min;
    quant->act_max = activation_max;
    return 0;
}

void esp_nn_conv_s8_window16_ansi(const int8_t *windows,
                                  const int8_t *filter,
                                  int8_t *out,
                                  const int32_t count,
                                  const int32_t out_stride,
                                  const conv_window16_quant_t *quant)
{
    for (int32_t i = 0; i < count; i++) {
        const int8_t *window = windows + 16 * i;
        int32_t acc = 0;
        for (int t = 0; t < 16; t++) {
            acc += window[t] * filter[t];
        }
        const int32_t x4 = 4 * acc + quant->bias4;
        const int32_t y = (int32_t) (((int64_t) x4 * quant->mult) >> 32);
        int32_t result = (y + quant->round - (x4 < 0 ? 2 : 0)) >> quant->shift;
        result = max(result, quant->act_min);
        result = min(result, quant->act_max);
        out[i * out_stride] = (int8_t) result;
    }
}
