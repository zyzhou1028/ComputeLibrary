/*
 * Copyright (c) 2017-2019 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "arm_compute/runtime/CL/functions/CLL2NormalizeLayer.h"

#include "arm_compute/core/CL/ICLTensor.h"
#include "arm_compute/core/CL/kernels/CLL2NormalizeLayerKernel.h"
#include "arm_compute/core/Error.h"
#include "arm_compute/core/PixelValue.h"
#include "arm_compute/core/TensorInfo.h"
#include "arm_compute/core/Validate.h"
#include "arm_compute/runtime/CL/CLScheduler.h"
#include "support/ToolchainSupport.h"

namespace arm_compute
{
CLL2NormalizeLayer::CLL2NormalizeLayer(std::shared_ptr<IMemoryManager> memory_manager)
    : _memory_group(std::move(memory_manager)), _reduce_func(), _normalize_kernel(), _sumsq()
{
}

void CLL2NormalizeLayer::configure(ICLTensor *input, ICLTensor *output, unsigned int axis, float epsilon)
{
    // Manage intermediate buffers
    _memory_group.manage(&_sumsq);

    // Configure kernels
    _reduce_func.configure(input, &_sumsq, axis, ReductionOperation::SUM_SQUARE);
    _normalize_kernel.configure(input, &_sumsq, output, axis, epsilon);

    // Allocate intermediate tensor
    _sumsq.allocator()->allocate();
}

Status CLL2NormalizeLayer::validate(const ITensorInfo *input, const ITensorInfo *output, unsigned int axis, float epsilon)
{
    TensorShape shape(input->tensor_shape());

    // Create intermediate tensor info
    TensorInfo sum_sq;
    sum_sq.set_data_type(input->data_type());
    sum_sq.set_tensor_shape(shape);

    ARM_COMPUTE_RETURN_ON_ERROR(CLReductionOperation::validate(input, &sum_sq, axis, ReductionOperation::SUM_SQUARE));

    // Reduce shape on axis
    shape.set(axis, 1);
    sum_sq.set_tensor_shape(shape);

    ARM_COMPUTE_RETURN_ON_ERROR(CLL2NormalizeLayerKernel::validate(input, &sum_sq, output, axis, epsilon));

    return Status{};
}

void CLL2NormalizeLayer::run()
{
    MemoryGroupResourceScope scope_mg(_memory_group);

    _reduce_func.run();
    CLScheduler::get().enqueue(_normalize_kernel, true);
}
} // namespace arm_compute
