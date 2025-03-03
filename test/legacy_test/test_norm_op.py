#   Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest

import numpy as np
from eager_op_test import OpTest, convert_float_to_uint16, skip_check_grad_ci

import paddle
from paddle import base
from paddle.base import core


def l2_norm(x, axis, epsilon):
    x2 = x**2
    s = np.sum(x2, axis=axis, keepdims=True)
    r = np.sqrt(s + epsilon)
    y = x / np.broadcast_to(r, x.shape)
    return y, r


def norm_wrapper(x, axis=1, epsilon=1e-12, is_test=False):
    return paddle.nn.functional.normalize(x, axis=axis, epsilon=epsilon)


class TestNormOp(OpTest):
    def setUp(self):
        self.op_type = "norm"
        self.python_api = norm_wrapper
        self.init_test_case()
        self.init_dtype()
        x = np.random.random(self.shape).astype(self.dtype)
        y, norm = l2_norm(x, self.axis, self.epsilon)
        self.inputs = {'X': x}
        self.attrs = {'epsilon': self.epsilon, 'axis': self.axis}
        self.outputs = {'Out': y, 'Norm': norm}
        self.python_out_sig = ['Out']

    def test_check_output(self):
        self.check_output(check_cinn=True)

    def test_check_grad(self):
        self.check_grad(['X'], 'Out', check_cinn=True)

    def init_test_case(self):
        self.shape = [2, 3, 4, 5]
        self.axis = 1
        self.epsilon = 1e-8

    def init_dtype(self):
        self.dtype = "float64"


class TestNormOp2(TestNormOp):
    def init_test_case(self):
        self.shape = [5, 3, 9, 7]
        self.axis = 0
        self.epsilon = 1e-8


class TestNormOp3(TestNormOp):
    def init_test_case(self):
        self.shape = [5, 3, 2, 7]
        self.axis = -1
        self.epsilon = 1e-8


@skip_check_grad_ci(
    reason="'check_grad' on large inputs is too slow, "
    + "however it is desirable to cover the forward pass"
)
class TestNormOp4(TestNormOp):
    def init_test_case(self):
        self.shape = [128, 1024, 14, 14]
        self.axis = 2
        self.epsilon = 1e-8

    def test_check_grad(self):
        pass


@skip_check_grad_ci(
    reason="'check_grad' on large inputs is too slow, "
    + "however it is desirable to cover the forward pass"
)
class TestNormOp5(TestNormOp):
    def init_test_case(self):
        self.shape = [2048, 2048]
        self.axis = 1
        self.epsilon = 1e-8

    def test_check_grad(self):
        pass


class TestNormOp6(TestNormOp):
    def init_dtype(self):
        self.dtype = "float32"

    def test_check_grad(self):
        self.check_grad(['X'], 'Out', max_relative_error=0.008, check_cinn=True)


@unittest.skipIf(
    not base.core.is_compiled_with_cuda(), "core is not compiled with CUDA"
)
class TestNormOp7(TestNormOp):
    def init_dtype(self):
        self.dtype = "float16"

    def test_check_output(self):
        self.check_output_with_place(
            base.core.CUDAPlace(0), atol=5e-2, check_cinn=True
        )

    def test_check_grad(self):
        self.check_grad_with_place(
            base.core.CUDAPlace(0),
            ['X'],
            'Out',
            max_relative_error=0.05,
            check_cinn=True,
        )


@skip_check_grad_ci(reason="skip check grad for test mode.")
class TestNormTestOp(OpTest):
    def setUp(self):
        self.op_type = "norm"
        self.python_api = norm_wrapper
        self.init_test_case()
        x = np.random.random(self.shape).astype("float64")
        y, norm = l2_norm(x, self.axis, self.epsilon)
        self.inputs = {'X': x}
        self.attrs = {
            'epsilon': self.epsilon,
            'axis': int(self.axis),
            'is_test': True,
        }
        self.outputs = {'Out': y}
        self.python_out_sig = ["out"]

    def test_check_output(self):
        # dynamic graph just supports float tensor
        self.check_output(check_dygraph=True, check_cinn=True)

    def test_check_grad(self):
        pass

    def init_test_case(self):
        self.shape = [2, 3, 4, 5]
        self.axis = 1
        self.epsilon = 1e-8


@unittest.skipIf(
    not core.is_compiled_with_cuda(),
    "core is not compiled with CUDA and not support the bfloat16",
)
class TestNormBF16Op(OpTest):
    def setUp(self):
        self.op_type = "norm"
        self.python_api = norm_wrapper
        self.init_test_case()
        self.dtype = "float32"
        x = np.random.random(self.shape).astype(self.dtype)
        y, norm = l2_norm(x, self.axis, self.epsilon)
        self.inputs = {'X': convert_float_to_uint16(x)}
        self.attrs = {'epsilon': self.epsilon, 'axis': self.axis}
        self.outputs = {'Out': convert_float_to_uint16(y), 'Norm': norm}
        self.python_out_sig = ['Out']

    def test_check_output(self):
        self.check_output_with_place(
            core.CUDAPlace(0), atol=1e-1, check_cinn=True
        )

    def test_check_grad(self):
        self.check_grad_with_place(
            core.CUDAPlace(0),
            ['X'],
            'Out',
            max_relative_error=1e-2,
            check_cinn=True,
        )

    def init_test_case(self):
        self.shape = [2, 3, 4, 5]
        self.axis = 1
        self.epsilon = 1e-8


class API_NormTest(unittest.TestCase):
    def test_errors(self):
        with base.program_guard(base.Program()):

            def test_norm_x_type():
                data = paddle.static.data(name="x", shape=[3, 3], dtype="int64")
                out = paddle.nn.functional.normalize(data)

            self.assertRaises(TypeError, test_norm_x_type)


if __name__ == '__main__':
    paddle.enable_static()
    unittest.main()
