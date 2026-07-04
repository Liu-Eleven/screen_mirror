/*
 * Copyright (C) 2024 Allwinnertech
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AUDIO_API_H
#define AUDIO_API_H

#include "audio_hw.h"

int audio_get_volume(int *vol);
int audio_set_volume(int vol);
int audio_get_mute(bool *status);
int audio_set_mute(bool status);
int audio_set_eq_treble_gain(int8_t gain);
int audio_set_eq_bass_gain(int8_t gain);
int audio_set_eq_mode(int mode);
int audio_set_output_device(int mode);
#endif
