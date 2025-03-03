add_subdirectory(engine)
proto_library(paddle_jit_property_proto SRCS property.proto)

cc_library(
  jit_property
  SRCS property.cc
  DEPS paddle_jit_property_proto tensor)

cc_library(
  jit_serializer
  SRCS serializer.cc
  DEPS lod_tensor device_context jit_property)

cc_library(
  jit_function_utils
  SRCS function_utils.cc
  DEPS lod_tensor scope proto_desc)

cc_library(
  jit_serializer_utils
  SRCS serializer_utils.cc
  DEPS proto_desc)

cc_library(
  jit_compilation_unit
  SRCS compilation_unit.cc
  DEPS proto_desc executor parallel_executor)

cc_library(
  jit_function_schema
  SRCS function_schema.cc
  DEPS jit_function_utils)

cc_library(
  jit_function
  SRCS function.cc
  DEPS jit_function_utils jit_interpreter_engine jit_predictor_engine)

cc_library(
  jit_layer
  SRCS layer.cc
  DEPS jit_serializer
       jit_function_utils
       jit_serializer_utils
       jit_compilation_unit
       jit_function_schema
       jit_interpreter_engine
       jit_predictor_engine
       jit_function
       executor
       recurrent_op_helper
       recurrent_op)
