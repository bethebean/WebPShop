// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "PIFormat.h"
#include "SPPlugs.h"
#include "WebPShop.h"
#include "WebPShopSelector.h"

static bool IsAnimation(FormatRecordPtr format_record) {
  const ReadLayerDesc* layer_desc =
      format_record->documentInfo->layersDescriptor;
  int num_layers = 0;
  while (layer_desc != nullptr && num_layers < MAX_NUM_BROWSED_LAYERS) {
    int frame_duration;
    if (!TryExtractDuration(layer_desc->unicodeName, &frame_duration)) {
      return false;
    }
    layer_desc = layer_desc->next;
    ++num_layers;
  }
  return (num_layers > 1 && num_layers < MAX_NUM_BROWSED_LAYERS);
}

//------------------------------------------------------------------------------

void DoOptionsPrepare(FormatRecordPtr format_record, Data* const data,
                      int16* const result) {
  format_record->maxData = 0;
}

//------------------------------------------------------------------------------

void DoOptionsStart(FormatRecordPtr format_record, Data* const data,
                    int16* const result, const SPPluginRef& plugin_ref) {
  LoadWriteConfig(format_record, &data->write_config, result);
  data->write_config.animation = IsAnimation(format_record);

  WebPDataClear(&data->encoded_data);

  // Don't ask encoding parameters to the user unless Photoshop requests it.
  // This is usually plugInDialogSilent during Batch.
  const PIDescriptorParameters* const desc_params =
      format_record->descriptorParameters;
  const bool display_encoding_parameters =
      desc_params == nullptr || desc_params->playInfo == plugInDialogDisplay;

  if (display_encoding_parameters) {
    std::vector<FrameMemoryDesc> frames;
    if (*result == noErr) {
      if (data->write_config.animation) {
        CopyAllLayers(format_record, data, result, &frames);
      } else {
        ResizeFrameVector(&frames, 1);
        CopyWholeCanvas(format_record, data, result, &frames[0].image);
      }
    }

    if (*result == noErr) {
      if (!DoUI(&data->write_config, plugin_ref, frames, &data->encoded_data,
                format_record->displayPixels)) {
        *result = userCanceledErr;
      }
    }

    if (*result != noErr) WebPDataClear(&data->encoded_data);
    ClearFrameVector(&frames);
  }
  format_record->data = nullptr;
}

//------------------------------------------------------------------------------

void DoOptionsContinue(FormatRecordPtr format_record, Data* const data,
                       int16* const result) {}

//------------------------------------------------------------------------------

void DoOptionsFinish(FormatRecordPtr format_record, Data* const data,
                     int16* const result) {}
