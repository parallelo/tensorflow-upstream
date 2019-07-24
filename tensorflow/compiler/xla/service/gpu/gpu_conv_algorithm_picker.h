/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_COMPILER_XLA_SERVICE_GPU_CONV_ALGORITHM_PICKER_H_
#define TENSORFLOW_COMPILER_XLA_SERVICE_GPU_CONV_ALGORITHM_PICKER_H_

#include "absl/time/time.h"
#include "absl/types/optional.h"
#include "tensorflow/compiler/xla/service/compiler.h"
#include "tensorflow/compiler/xla/service/gpu/cudnn_conv_runner.h"
#include "tensorflow/compiler/xla/service/hlo_instructions.h"
#include "tensorflow/compiler/xla/service/hlo_module.h"
#include "tensorflow/compiler/xla/service/hlo_pass_interface.h"
#include "tensorflow/core/platform/stream_executor_no_cuda.h"
#include "tensorflow/core/protobuf/autotuning.pb.h"
#include "tensorflow/stream_executor/device_memory_allocator.h"

namespace xla {
namespace gpu {

// Modifies CustomCalls to cudnn convolutions, choosing the best algorithm for
// each and adding explicit scratch space to the CustomCalls.
class GpuConvAlgorithmPicker : public HloModulePass {
 public:
  // If the `allocator` parameter is not null, we will use it to allocate temp
  // memory while timing the various convolution algorithms.  If it's null,
  // we'll use the default allocator on the StreamExecutor.
  GpuConvAlgorithmPicker(se::StreamExecutor* stream_exec,
                         se::DeviceMemoryAllocator* allocator)
      : stream_exec_(stream_exec), allocator_(allocator) {}

  virtual absl::string_view name() const = 0;

  StatusOr<bool> Run(HloModule* module) override;

 protected:
  string AlgorithmToString(const se::dnn::AlgorithmDesc& algo) {
    if (algo.tensor_ops_enabled()) {
      return absl::StrCat(algo.algo_id(), "+TC");
    }
    return absl::StrCat(algo.algo_id());
  }

  StatusOr<bool> RunOnComputation(HloComputation* computation);
  StatusOr<bool> RunOnInstruction(HloInstruction* instr);
  StatusOr<tensorflow::AutotuneResult> PickBestAlgorithm(
      const HloCustomCallInstruction* instr);

  virtual StatusOr<tensorflow::AutotuneResult> PickBestAlgorithmNoCache(
      const HloCustomCallInstruction& instr,
      se::DeviceMemoryAllocator* allocator, se::Stream* stream) = 0;

  se::StreamExecutor* stream_exec_;       // never null
  se::DeviceMemoryAllocator* allocator_;  // may be null
};

}  // namespace gpu
}  // namespace xla
#endif  // TENSORFLOW_COMPILER_XLA_SERVICE_GPU_CONV_ALGORITHM_PICKER_H_