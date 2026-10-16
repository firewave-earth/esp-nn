// Copyright 2022 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

/**
 * @brief structure to club data dims
 * this structure can be used for input, output and filter
 */
typedef struct data_dims {
    int32_t width;
    int32_t height;
    int32_t channels;

    int32_t extra; // can be used as batch or any other param
} data_dims_t;

/**
 * @brief 2d data structure (width, height)
 *
 */
typedef struct data_2d {
    int32_t width;
    int32_t height;
} data_2d_t;

/**
 * @brief min/max activation
 */
typedef struct act_params {
    int32_t min;
    int32_t max;
} act_params_t;

/**
 * @brief per channel quant data
 *
 * @note number of shift and mult elements are equal to output channels
 */
typedef struct quant_data {
    int32_t *shift;
    int32_t *mult;
} quant_data_t;

/**
 * @brief params specific to convolution 2d
 *
 */
typedef struct conv_params {
    int32_t in_offset;
    int32_t out_offset;
    data_2d_t stride;
    data_2d_t padding;
    data_2d_t dilation;
    act_params_t activation;
} conv_params_t;

/**
 * @brief per channel constants for esp_nn_conv_s8_window16
 *
 * @note filled by esp_nn_conv_s8_window16_quant_init. The S3 assembly reads the
 *       fields at these offsets, so keep the order.
 */
typedef struct conv_window16_quant {
    int32_t bias4;      // 4 * (bias + in_offset * sum(filter))
    int32_t mult;       // output multiplier
    int32_t round;      // 1 + 2^rs + out_offset * 2^(rs + 1), rs = -out_shift
    int32_t shift;      // rs + 1
    int32_t act_min;
    int32_t act_max;
} conv_window16_quant_t;

/**
 * @brief params specific to depthwise convolution 2d
 *
 */
typedef struct dw_conv_params {
    int32_t in_offset;
    int32_t out_offset;
    int32_t ch_mult; // channel multiplier. (in_ch * ch_mult = out_ch)
    data_2d_t stride;
    data_2d_t padding;
    data_2d_t dilation;
    act_params_t activation;
} dw_conv_params_t;
