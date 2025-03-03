// Copyright (c) 2023 PaddlePaddle Authors. All Rights Reserved.
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

#include "paddle/pir/core/value.h"

namespace pir {
static const uint32_t OUTLINE_OP_RESULT_INDEX = 6;

class Operation;

namespace detail {
///
/// \brief OpOperandImpl
///
class OpOperandImpl {
 public:
  pir::Operation *owner() const;

  pir::detail::OpOperandImpl *next_use();

  pir::Value source() const;

  void set_source(Value value);

  /// Remove this op_operand from the current use list.
  void RemoveFromUdChain();

  ~OpOperandImpl();

  friend pir::Operation;

 private:
  OpOperandImpl(pir::Value source, pir::Operation *owner);

  // Insert self to the UD chain holded by source_;
  // It is not safe. So set private.
  void InsertToUdChain();

  pir::detail::OpOperandImpl *next_use_ = nullptr;

  pir::detail::OpOperandImpl **prev_use_addr_ = nullptr;

  pir::Value source_;

  pir::Operation *const owner_ = nullptr;
};

///
/// \brief ValueImpl is the base class of all derived Value classes such as
/// OpResultImpl. This class defines all the information and usage interface in
/// the IR Value. Each Value include three attributes:
/// (1) type: pir::Type; (2) UD-chain of value: OpOperandImpl*, first op_operand
/// address with offset of this value; (3) index: the position where the output
/// list of the parent operator.
///
class alignas(8) ValueImpl {
 public:
  ///
  /// \brief Interface functions of "type_" attribute.
  ///
  pir::Type type() const { return type_; }

  void set_type(pir::Type type) { type_ = type; }

  ///
  /// \brief Interface functions of "first_use_offseted_by_index_" attribute.
  ///
  uint32_t index() const;

  OpOperandImpl *first_use() const {
    return reinterpret_cast<OpOperandImpl *>(
        reinterpret_cast<uintptr_t>(first_use_offseted_by_index_) & (~0x07));
  }

  void set_first_use(OpOperandImpl *first_use) {
    uint32_t offset = index();
    first_use_offseted_by_index_ = reinterpret_cast<OpOperandImpl *>(
        reinterpret_cast<uintptr_t>(first_use) + offset);
    VLOG(4) << "The index of this value is " << offset
            << ". Offset and set first use: " << first_use << " -> "
            << first_use_offseted_by_index_ << ".";
  }

  OpOperandImpl **first_use_addr() { return &first_use_offseted_by_index_; }

  bool use_empty() const { return first_use() == nullptr; }

  bool HasOneUse() const {
    return (first_use() != nullptr) && (first_use()->next_use() == nullptr);
  }

  std::string PrintUdChain();

 protected:
  ///
  /// \brief Only can be constructed by derived classes such as OpResultImpl.
  ///
  explicit ValueImpl(pir::Type type, uint32_t index) {
    if (index > OUTLINE_OP_RESULT_INDEX) {
      throw("The value of index must not exceed 6");
    }
    type_ = type;
    first_use_offseted_by_index_ = reinterpret_cast<OpOperandImpl *>(
        reinterpret_cast<uintptr_t>(nullptr) + index);
    VLOG(4) << "Construct a ValueImpl whose's index is " << index
            << ". The offset first_use address is: "
            << first_use_offseted_by_index_;
  }

  ///
  /// \brief Attribute1: Type of value.
  ///
  pir::Type type_;

  ///
  /// \brief Attribute2/3: Record the UD-chain of value and index.
  /// NOTE: The members of the OpOperandImpl include four pointers, so this
  /// class is 8-byte aligned, and the lower 3 bits of its address are 0, so the
  /// index can be stored in these 3 bits, stipulate:
  /// (1) index = 0~5: represent positions 0 to 5 inline
  /// output(OpInlineResultImpl); (2) index = 6: represent the position >=6
  /// outline output(OpOutlineResultImpl); (3) index = 7 is reserved.
  ///
  OpOperandImpl *first_use_offseted_by_index_ = nullptr;
};

///
/// \brief OpResultImpl is the implementation of an operation result.
///
class alignas(8) OpResultImpl : public ValueImpl {
 public:
  using ValueImpl::ValueImpl;

  static bool classof(const ValueImpl &value) { return true; }

  ///
  /// \brief Get the parent operation of this result.(op_ptr = value_ptr +
  /// index)
  ///
  pir::Operation *owner() const;

  ///
  /// \brief Get the result index of the operation result.
  ///
  uint32_t GetResultIndex() const;

  ///
  /// \brief Get the maximum number of results that can be stored inline.
  ///
  static uint32_t GetMaxInlineResultIndex() {
    return OUTLINE_OP_RESULT_INDEX - 1;
  }

  ~OpResultImpl();
};

///
/// \brief OpInlineResultImpl is the implementation of an operation result whose
/// index <= 5.
///
class OpInlineResultImpl : public OpResultImpl {
 public:
  OpInlineResultImpl(pir::Type type, uint32_t result_index)
      : OpResultImpl(type, result_index) {
    if (result_index > GetMaxInlineResultIndex()) {
      throw("Inline result index should not exceed MaxInlineResultIndex(5)");
    }
  }

  static bool classof(const OpResultImpl &value) {
    return value.index() < OUTLINE_OP_RESULT_INDEX;
  }

  uint32_t GetResultIndex() const { return index(); }
};

///
/// \brief OpOutlineResultImpl is the implementation of an operation result
/// whose index > 5.
///
class OpOutlineResultImpl : public OpResultImpl {
 public:
  OpOutlineResultImpl(pir::Type type, uint32_t outline_index)
      : OpResultImpl(type, OUTLINE_OP_RESULT_INDEX),
        outline_index_(outline_index) {}

  static bool classof(const OpResultImpl &value) {
    return value.index() >= OUTLINE_OP_RESULT_INDEX;
  }

  uint32_t GetResultIndex() const { return outline_index_; }

  uint32_t outline_index_;
};

}  // namespace detail
}  // namespace pir
