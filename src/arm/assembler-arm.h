// Copyright (c) 1994-2006 Sun Microsystems Inc.
// All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// - Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// - Redistribution in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution.
//
// - Neither the name of Sun Microsystems or the names of contributors may
// be used to endorse or promote products derived from this software without
// specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

// The original source code covered by the above license above has been
// modified significantly by Google Inc.
// Copyright 2012 the V8 project authors. All rights reserved.

// A light-weight ARM Assembler
// Generates user mode instructions for the ARM architecture up to version 5

#ifndef V8_ARM_ASSEMBLER_ARM_H_
#define V8_ARM_ASSEMBLER_ARM_H_

#include <stdio.h>
#include <vector>

#include "src/arm/constants-arm.h"
#include "src/assembler.h"
#include "src/boxed-float.h"
#include "src/double.h"

namespace v8 {
namespace internal {

// clang-format off
#define GENERAL_REGISTERS(V)                              \
  V(r0)  V(r1)  V(r2)  V(r3)  V(r4)  V(r5)  V(r6)  V(r7)  \
  V(r8)  V(r9)  V(r10) V(fp)  V(ip)  V(sp)  V(lr)  V(pc)

#define ALLOCATABLE_GENERAL_REGISTERS(V)                  \
  V(r0)  V(r1)  V(r2)  V(r3)  V(r4)  V(r5)  V(r6)  V(r7)  \
  V(r8)  V(r9)

#define FLOAT_REGISTERS(V)                                \
  V(s0)  V(s1)  V(s2)  V(s3)  V(s4)  V(s5)  V(s6)  V(s7)  \
  V(s8)  V(s9)  V(s10) V(s11) V(s12) V(s13) V(s14) V(s15) \
  V(s16) V(s17) V(s18) V(s19) V(s20) V(s21) V(s22) V(s23) \
  V(s24) V(s25) V(s26) V(s27) V(s28) V(s29) V(s30) V(s31)

#define LOW_DOUBLE_REGISTERS(V)                           \
  V(d0)  V(d1)  V(d2)  V(d3)  V(d4)  V(d5)  V(d6)  V(d7)  \
  V(d8)  V(d9)  V(d10) V(d11) V(d12) V(d13) V(d14) V(d15)

#define NON_LOW_DOUBLE_REGISTERS(V)                       \
  V(d16) V(d17) V(d18) V(d19) V(d20) V(d21) V(d22) V(d23) \
  V(d24) V(d25) V(d26) V(d27) V(d28) V(d29) V(d30) V(d31)

#define DOUBLE_REGISTERS(V) \
  LOW_DOUBLE_REGISTERS(V) NON_LOW_DOUBLE_REGISTERS(V)

#define SIMD128_REGISTERS(V)                              \
  V(q0)  V(q1)  V(q2)  V(q3)  V(q4)  V(q5)  V(q6)  V(q7)  \
  V(q8)  V(q9)  V(q10) V(q11) V(q12) V(q13) V(q14) V(q15)

#define ALLOCATABLE_DOUBLE_REGISTERS(V)                   \
  V(d0)  V(d1)  V(d2)  V(d3)  V(d4)  V(d5)  V(d6)  V(d7)  \
  V(d8)  V(d9)  V(d10) V(d11) V(d12)                      \
  V(d16) V(d17) V(d18) V(d19) V(d20) V(d21) V(d22) V(d23) \
  V(d24) V(d25) V(d26) V(d27) V(d28) V(d29) V(d30) V(d31)

#define ALLOCATABLE_NO_VFP32_DOUBLE_REGISTERS(V)          \
  V(d0)  V(d1)  V(d2)  V(d3)  V(d4)  V(d5)  V(d6)  V(d7)  \
  V(d8)  V(d9)  V(d10) V(d11) V(d12) V(d15)

#define C_REGISTERS(V)                                            \
  V(cr0)  V(cr1)  V(cr2)  V(cr3)  V(cr4)  V(cr5)  V(cr6)  V(cr7)  \
  V(cr8)  V(cr9)  V(cr10) V(cr11) V(cr12) V(cr15)
// clang-format on

// The ARM ABI does not specify the usage of register r9, which may be reserved
// as the static base or thread register on some platforms, in which case we
// leave it alone. Adjust the value of kR9Available accordingly:
const int kR9Available = 1;  // 1 if available to us, 0 if reserved

// Register list in load/store instructions
// Note that the bit values must match those used in actual instruction encoding
const int kNumRegs = 16;

// Caller-saved/arguments registers
const RegList kJSCallerSaved =
  1 << 0 |  // r0 a1
  1 << 1 |  // r1 a2
  1 << 2 |  // r2 a3
  1 << 3;   // r3 a4

const int kNumJSCallerSaved = 4;

// Callee-saved registers preserved when switching from C to JavaScript
const RegList kCalleeSaved =
  1 <<  4 |  //  r4 v1
  1 <<  5 |  //  r5 v2
  1 <<  6 |  //  r6 v3
  1 <<  7 |  //  r7 v4 (cp in JavaScript code)
  1 <<  8 |  //  r8 v5 (pp in JavaScript code)
  kR9Available <<  9 |  //  r9 v6
  1 << 10 |  // r10 v7
  1 << 11;   // r11 v8 (fp in JavaScript code)

// When calling into C++ (only for C++ calls that can't cause a GC).
// The call code will take care of lr, fp, etc.
const RegList kCallerSaved =
  1 <<  0 |  // r0
  1 <<  1 |  // r1
  1 <<  2 |  // r2
  1 <<  3 |  // r3
  1 <<  9;   // r9

const int kNumCalleeSaved = 7 + kR9Available;

// Double registers d8 to d15 are callee-saved.
const int kNumDoubleCalleeSaved = 8;

// Number of registers for which space is reserved in safepoints. Must be a
// multiple of 8.
// TODO(regis): Only 8 registers may actually be sufficient. Revisit.
const int kNumSafepointRegisters = 16;

// Define the list of registers actually saved at safepoints.
// Note that the number of saved registers may be smaller than the reserved
// space, i.e. kNumSafepointSavedRegisters <= kNumSafepointRegisters.
const RegList kSafepointSavedRegisters = kJSCallerSaved | kCalleeSaved;
const int kNumSafepointSavedRegisters = kNumJSCallerSaved + kNumCalleeSaved;

enum RegisterCode {
#define REGISTER_CODE(R) kRegCode_##R,
  GENERAL_REGISTERS(REGISTER_CODE)
#undef REGISTER_CODE
      kRegAfterLast
};

class Register : public RegisterBase<Register, kRegAfterLast> {
  friend class RegisterBase;
  explicit constexpr Register(int code) : RegisterBase(code) {}
};

static_assert(IS_TRIVIALLY_COPYABLE(Register) &&
                  sizeof(Register) == sizeof(int),
              "Register can efficiently be passed by value");

// r7: context register
// r9: lithium scratch
#define DECLARE_REGISTER(R) \
  constexpr Register R = Register::from_code<kRegCode_##R>();
GENERAL_REGISTERS(DECLARE_REGISTER)
#undef DECLARE_REGISTER
constexpr Register no_reg = Register::no_reg();

constexpr bool kPadArguments = false;
constexpr bool kSimpleFPAliasing = false;
constexpr bool kSimdMaskRegisters = false;

enum SwVfpRegisterCode {
#define REGISTER_CODE(R) kSwVfpCode_##R,
  FLOAT_REGISTERS(REGISTER_CODE)
#undef REGISTER_CODE
      kSwVfpAfterLast
};

// Representation of a list of non-overlapping VFP registers. This list
// represents the data layout of VFP registers as a bitfield:
//   S registers cover 1 bit
//   D registers cover 2 bits
//   Q registers cover 4 bits
//
// This way, we make sure no registers in the list ever overlap. However, a list
// may represent multiple different sets of registers,
// e.g. [d0 s2 s3] <=> [s0 s1 d1].
typedef uint64_t VfpRegList;

// Single word VFP register.
class SwVfpRegister : public RegisterBase<SwVfpRegister, kSwVfpAfterLast> {
 public:
  static constexpr int kSizeInBytes = 4;

  static void split_code(int reg_code, int* vm, int* m) {
    DCHECK(from_code(reg_code).is_valid());
    *m = reg_code & 0x1;
    *vm = reg_code >> 1;
  }
  void split_code(int* vm, int* m) const { split_code(code(), vm, m); }
  VfpRegList ToVfpRegList() const {
    DCHECK(is_valid());
    // Each bit in the list corresponds to a S register.
    return uint64_t{0x1} << code();
  }

 private:
  friend class RegisterBase;
  explicit constexpr SwVfpRegister(int code) : RegisterBase(code) {}
};

static_assert(IS_TRIVIALLY_COPYABLE(SwVfpRegister) &&
                  sizeof(SwVfpRegister) == sizeof(int),
              "SwVfpRegister can efficiently be passed by value");

typedef SwVfpRegister FloatRegister;

enum DoubleRegisterCode {
#define REGISTER_CODE(R) kDoubleCode_##R,
  DOUBLE_REGISTERS(REGISTER_CODE)
#undef REGISTER_CODE
      kDoubleAfterLast
};

// Double word VFP register.
class DwVfpRegister : public RegisterBase<DwVfpRegister, kDoubleAfterLast> {
 public:
  static constexpr int kSizeInBytes = 8;

  inline static int NumRegisters();

  static void split_code(int reg_code, int* vm, int* m) {
    DCHECK(from_code(reg_code).is_valid());
    *m = (reg_code & 0x10) >> 4;
    *vm = reg_code & 0x0F;
  }
  void split_code(int* vm, int* m) const { split_code(code(), vm, m); }
  VfpRegList ToVfpRegList() const {
    DCHECK(is_valid());
    // A D register overlaps two S registers.
    return uint64_t{0x3} << (code() * 2);
  }

 private:
  friend class RegisterBase;
  friend class LowDwVfpRegister;
  explicit constexpr DwVfpRegister(int code) : RegisterBase(code) {}
};

static_assert(IS_TRIVIALLY_COPYABLE(DwVfpRegister) &&
                  sizeof(DwVfpRegister) == sizeof(int),
              "DwVfpRegister can efficiently be passed by value");

typedef DwVfpRegister DoubleRegister;


// Double word VFP register d0-15.
class LowDwVfpRegister
    : public RegisterBase<LowDwVfpRegister, kDoubleCode_d16> {
 public:
  constexpr operator DwVfpRegister() const { return DwVfpRegister(reg_code_); }

  SwVfpRegister low() const { return SwVfpRegister::from_code(code() * 2); }
  SwVfpRegister high() const {
    return SwVfpRegister::from_code(code() * 2 + 1);
  }
  VfpRegList ToVfpRegList() const {
    DCHECK(is_valid());
    // A D register overlaps two S registers.
    return uint64_t{0x3} << (code() * 2);
  }

 private:
  friend class RegisterBase;
  explicit constexpr LowDwVfpRegister(int code) : RegisterBase(code) {}
};

enum Simd128RegisterCode {
#define REGISTER_CODE(R) kSimd128Code_##R,
  SIMD128_REGISTERS(REGISTER_CODE)
#undef REGISTER_CODE
      kSimd128AfterLast
};

// Quad word NEON register.
class QwNeonRegister : public RegisterBase<QwNeonRegister, kSimd128AfterLast> {
 public:
  static void split_code(int reg_code, int* vm, int* m) {
    DCHECK(from_code(reg_code).is_valid());
    int encoded_code = reg_code << 1;
    *m = (encoded_code & 0x10) >> 4;
    *vm = encoded_code & 0x0F;
  }
  void split_code(int* vm, int* m) const { split_code(code(), vm, m); }
  DwVfpRegister low() const { return DwVfpRegister::from_code(code() * 2); }
  DwVfpRegister high() const {
    return DwVfpRegister::from_code(code() * 2 + 1);
  }
  VfpRegList ToVfpRegList() const {
    DCHECK(is_valid());
    // A Q register overlaps four S registers.
    return uint64_t{0xf} << (code() * 4);
  }

 private:
  friend class RegisterBase;
  explicit constexpr QwNeonRegister(int code) : RegisterBase(code) {}
};


typedef QwNeonRegister QuadRegister;

typedef QwNeonRegister Simd128Register;

enum CRegisterCode {
#define REGISTER_CODE(R) kCCode_##R,
  C_REGISTERS(REGISTER_CODE)
#undef REGISTER_CODE
      kCAfterLast
};

// Coprocessor register
class CRegister : public RegisterBase<CRegister, kCAfterLast> {
  friend class RegisterBase;
  explicit constexpr CRegister(int code) : RegisterBase(code) {}
};

// Support for the VFP registers s0 to s31 (d0 to d15).
// Note that "s(N):s(N+1)" is the same as "d(N/2)".
#define DECLARE_FLOAT_REGISTER(R) \
  constexpr SwVfpRegister R = SwVfpRegister::from_code<kSwVfpCode_##R>();
FLOAT_REGISTERS(DECLARE_FLOAT_REGISTER)
#undef DECLARE_FLOAT_REGISTER

#define DECLARE_LOW_DOUBLE_REGISTER(R) \
  constexpr LowDwVfpRegister R = LowDwVfpRegister::from_code<kDoubleCode_##R>();
LOW_DOUBLE_REGISTERS(DECLARE_LOW_DOUBLE_REGISTER)
#undef DECLARE_LOW_DOUBLE_REGISTER

#define DECLARE_DOUBLE_REGISTER(R) \
  constexpr DwVfpRegister R = DwVfpRegister::from_code<kDoubleCode_##R>();
NON_LOW_DOUBLE_REGISTERS(DECLARE_DOUBLE_REGISTER)
#undef DECLARE_DOUBLE_REGISTER

constexpr DwVfpRegister no_dreg = DwVfpRegister::no_reg();

#define DECLARE_SIMD128_REGISTER(R) \
  constexpr Simd128Register R = Simd128Register::from_code<kSimd128Code_##R>();
SIMD128_REGISTERS(DECLARE_SIMD128_REGISTER)
#undef DECLARE_SIMD128_REGISTER

// Aliases for double registers.
constexpr LowDwVfpRegister kFirstCalleeSavedDoubleReg = d8;
constexpr LowDwVfpRegister kLastCalleeSavedDoubleReg = d15;
constexpr LowDwVfpRegister kDoubleRegZero  = d13;

constexpr CRegister no_creg = CRegister::no_reg();

#define DECLARE_C_REGISTER(R) \
  constexpr CRegister R = CRegister::from_code<kCCode_##R>();
C_REGISTERS(DECLARE_C_REGISTER)
#undef DECLARE_C_REGISTER

// Coprocessor number
enum Coprocessor {
  p0  = 0,
  p1  = 1,
  p2  = 2,
  p3  = 3,
  p4  = 4,
  p5  = 5,
  p6  = 6,
  p7  = 7,
  p8  = 8,
  p9  = 9,
  p10 = 10,
  p11 = 11,
  p12 = 12,
  p13 = 13,
  p14 = 14,
  p15 = 15
};

// -----------------------------------------------------------------------------
// Machine instruction Operands

// Class Operand represents a shifter operand in data processing instructions
class Operand BASE_EMBEDDED {
 public:
  // immediate
  INLINE(explicit Operand(int32_t immediate,
                          RelocInfo::Mode rmode = RelocInfo::NONE));
  INLINE(static Operand Zero());
  INLINE(explicit Operand(const ExternalReference& f));
  explicit Operand(Handle<HeapObject> handle);
  INLINE(explicit Operand(Smi* value));

  // rm
  INLINE(explicit Operand(Register rm));

  // rm <shift_op> shift_imm
  explicit Operand(Register rm, ShiftOp shift_op, int shift_imm);
  INLINE(static Operand SmiUntag(Register rm)) {
    return Operand(rm, ASR, kSmiTagSize);
  }
  INLINE(static Operand PointerOffsetFromSmiKey(Register key)) {
    STATIC_ASSERT(kSmiTag == 0 && kSmiTagSize < kPointerSizeLog2);
    return Operand(key, LSL, kPointerSizeLog2 - kSmiTagSize);
  }
  INLINE(static Operand DoubleOffsetFromSmiKey(Register key)) {
    STATIC_ASSERT(kSmiTag == 0 && kSmiTagSize < kDoubleSizeLog2);
    return Operand(key, LSL, kDoubleSizeLog2 - kSmiTagSize);
  }

  // rm <shift_op> rs
  explicit Operand(Register rm, ShiftOp shift_op, Register rs);

  static Operand EmbeddedNumber(double number);  // Smi or HeapNumber.
  static Operand EmbeddedCode(CodeStub* stub);

  // Return true if this is a register operand.
  bool IsRegister() const {
    return rm_.is_valid() && rs_ == no_reg && shift_op_ == LSL &&
           shift_imm_ == 0;
  }
  // Return true if this is a register operand shifted with an immediate.
  bool IsImmediateShiftedRegister() const {
    return rm_.is_valid() && !rs_.is_valid();
  }
  // Return true if this is a register operand shifted with a register.
  bool IsRegisterShiftedRegister() const {
    return rm_.is_valid() && rs_.is_valid();
  }

  // Return the number of actual instructions required to implement the given
  // instruction for this particular operand. This can be a single instruction,
  // if no load into a scratch register is necessary, or anything between 2 and
  // 4 instructions when we need to load from the constant pool (depending upon
  // whether the constant pool entry is in the small or extended section). If
  // the instruction this operand is used for is a MOV or MVN instruction the
  // actual instruction to use is required for this calculation. For other
  // instructions instr is ignored.
  //
  // The value returned is only valid as long as no entries are added to the
  // constant pool between this call and the actual instruction being emitted.
  int InstructionsRequired(const Assembler* assembler, Instr instr = 0) const;
  bool MustOutputRelocInfo(const Assembler* assembler) const;

  inline int32_t immediate() const {
    DCHECK(IsImmediate());
    DCHECK(!IsHeapObjectRequest());
    return value_.immediate;
  }
  bool IsImmediate() const {
    return !rm_.is_valid();
  }

  HeapObjectRequest heap_object_request() const {
    DCHECK(IsHeapObjectRequest());
    return value_.heap_object_request;
  }
  bool IsHeapObjectRequest() const {
    DCHECK_IMPLIES(is_heap_object_request_, IsImmediate());
    DCHECK_IMPLIES(is_heap_object_request_,
        rmode_ == RelocInfo::EMBEDDED_OBJECT ||
        rmode_ == RelocInfo::CODE_TARGET);
    return is_heap_object_request_;
  }

  Register rm() const { return rm_; }
  Register rs() const { return rs_; }
  ShiftOp shift_op() const { return shift_op_; }


 private:
  Register rm_ = no_reg;
  Register rs_ = no_reg;
  ShiftOp shift_op_;
  int shift_imm_;                // valid if rm_ != no_reg && rs_ == no_reg
  union Value {
    Value() {}
    HeapObjectRequest heap_object_request;  // if is_heap_object_request_
    int32_t immediate;                      // otherwise
  } value_;                                 // valid if rm_ == no_reg
  bool is_heap_object_request_ = false;
  RelocInfo::Mode rmode_;

  friend class Assembler;
};


// Class MemOperand represents a memory operand in load and store instructions
class MemOperand BASE_EMBEDDED {
 public:
  // [rn +/- offset]      Offset/NegOffset
  // [rn +/- offset]!     PreIndex/NegPreIndex
  // [rn], +/- offset     PostIndex/NegPostIndex
  // offset is any signed 32-bit value; offset is first loaded to a scratch
  // register if it does not fit the addressing mode (12-bit unsigned and sign
  // bit)
  explicit MemOperand(Register rn, int32_t offset = 0, AddrMode am = Offset);

  // [rn +/- rm]          Offset/NegOffset
  // [rn +/- rm]!         PreIndex/NegPreIndex
  // [rn], +/- rm         PostIndex/NegPostIndex
  explicit MemOperand(Register rn, Register rm, AddrMode am = Offset);

  // [rn +/- rm <shift_op> shift_imm]      Offset/NegOffset
  // [rn +/- rm <shift_op> shift_imm]!     PreIndex/NegPreIndex
  // [rn], +/- rm <shift_op> shift_imm     PostIndex/NegPostIndex
  explicit MemOperand(Register rn, Register rm,
                      ShiftOp shift_op, int shift_imm, AddrMode am = Offset);
  INLINE(static MemOperand PointerAddressFromSmiKey(Register array,
                                                    Register key,
                                                    AddrMode am = Offset)) {
    STATIC_ASSERT(kSmiTag == 0 && kSmiTagSize < kPointerSizeLog2);
    return MemOperand(array, key, LSL, kPointerSizeLog2 - kSmiTagSize, am);
  }

  void set_offset(int32_t offset) {
    DCHECK(rm_ == no_reg);
    offset_ = offset;
  }

  uint32_t offset() const {
    DCHECK(rm_ == no_reg);
    return offset_;
  }

  Register rn() const { return rn_; }
  Register rm() const { return rm_; }
  AddrMode am() const { return am_; }

  bool OffsetIsUint12Encodable() const {
    return offset_ >= 0 ? is_uint12(offset_) : is_uint12(-offset_);
  }

 private:
  Register rn_;  // base
  Register rm_;  // register offset
  int32_t offset_;  // valid if rm_ == no_reg
  ShiftOp shift_op_;
  int shift_imm_;  // valid if rm_ != no_reg && rs_ == no_reg
  AddrMode am_;  // bits P, U, and W

  friend class Assembler;
};


// Class NeonMemOperand represents a memory operand in load and
// store NEON instructions
class NeonMemOperand BASE_EMBEDDED {
 public:
  // [rn {:align}]       Offset
  // [rn {:align}]!      PostIndex
  explicit NeonMemOperand(Register rn, AddrMode am = Offset, int align = 0);

  // [rn {:align}], rm   PostIndex
  explicit NeonMemOperand(Register rn, Register rm, int align = 0);

  Register rn() const { return rn_; }
  Register rm() const { return rm_; }
  int align() const { return align_; }

 private:
  void SetAlignment(int align);

  Register rn_;  // base
  Register rm_;  // register increment
  int align_;
};


// Class NeonListOperand represents a list of NEON registers
class NeonListOperand BASE_EMBEDDED {
 public:
  explicit NeonListOperand(DoubleRegister base, int register_count = 1)
    : base_(base), register_count_(register_count) {}
  explicit NeonListOperand(QwNeonRegister q_reg)
    : base_(q_reg.low()), register_count_(2) {}
  DoubleRegister base() const { return base_; }
  int register_count() { return register_count_; }
  int length() const { return register_count_ - 1; }
  NeonListType type() const {
    switch (register_count_) {
      default: UNREACHABLE();
      // Fall through.
      case 1: return nlt_1;
      case 2: return nlt_2;
      case 3: return nlt_3;
      case 4: return nlt_4;
    }
  }
 private:
  DoubleRegister base_;
  int register_count_;
};


struct VmovIndex {
  unsigned char index;
};
constexpr VmovIndex VmovIndexLo = { 0 };
constexpr VmovIndex VmovIndexHi = { 1 };

class Assembler : public AssemblerBase {
 public:
  // Create an assembler. Instructions and relocation information are emitted
  // into a buffer, with the instructions starting from the beginning and the
  // relocation information starting from the end of the buffer. See CodeDesc
  // for a detailed comment on the layout (globals.h).
  //
  // If the provided buffer is nullptr, the assembler allocates and grows its
  // own buffer, and buffer_size determines the initial buffer size. The buffer
  // is owned by the assembler and deallocated upon destruction of the
  // assembler.
  //
  // If the provided buffer is not nullptr, the assembler uses the provided
  // buffer for code generation and assumes its size to be buffer_size. If the
  // buffer is too small, a fatal error occurs. No deallocation of the buffer is
  // done upon destruction of the assembler.
  Assembler(Isolate* isolate, void* buffer, int buffer_size)
      : Assembler(IsolateData(isolate), buffer, buffer_size) {}
  Assembler(IsolateData isolate_data, void* buffer, int buffer_size);
  virtual ~Assembler();

  // GetCode emits any pending (non-emitted) code and fills the descriptor
  // desc. GetCode() is idempotent; it returns the same result if no other
  // Assembler functions are invoked in between GetCode() calls.
  void GetCode(Isolate* isolate, CodeDesc* desc);

  // Label operations & relative jumps (PPUM Appendix D)
  //
  // Takes a branch opcode (cc) and a label (L) and generates
  // either a backward branch or a forward branch and links it
  // to the label fixup chain. Usage:
  //
  // Label L;    // unbound label
  // j(cc, &L);  // forward branch to unbound label
  // bind(&L);   // bind label to the current pc
  // j(cc, &L);  // backward branch to bound label
  // bind(&L);   // illegal: a label may be bound only once
  //
  // Note: The same Label can be used for forward and backward branches
  // but it may be bound only once.

  void bind(Label* L);  // binds an unbound label L to the current code position

  // Returns the branch offset to the given label from the current code position
  // Links the label to the current position if it is still unbound
  // Manages the jump elimination optimization if the second parameter is true.
  int branch_offset(Label* L);

  // Returns true if the given pc address is the start of a constant pool load
  // instruction sequence.
  INLINE(static bool is_constant_pool_load(Address pc));

  // Return the address in the constant pool of the code target address used by
  // the branch/call instruction at pc, or the object in a mov.
  INLINE(static Address constant_pool_entry_address(Address pc,
                                                    Address constant_pool));

  // Read/Modify the code target address in the branch/call instruction at pc.
  // The isolate argument is unused (and may be nullptr) when skipping flushing.
  INLINE(static Address target_address_at(Address pc, Address constant_pool));
  INLINE(static void set_target_address_at(
      Isolate* isolate, Address pc, Address constant_pool, Address target,
      ICacheFlushMode icache_flush_mode = FLUSH_ICACHE_IF_NEEDED));

  // Return the code target address at a call site from the return address
  // of that call in the instruction stream.
  INLINE(static Address target_address_from_return_address(Address pc));

  // Given the address of the beginning of a call, return the address
  // in the instruction stream that the call will return from.
  INLINE(static Address return_address_from_call_start(Address pc));

  // This sets the branch destination (which is in the constant pool on ARM).
  // This is for calls and branches within generated code.
  inline static void deserialization_set_special_target_at(
      Isolate* isolate, Address constant_pool_entry, Code* code,
      Address target);

  // This sets the internal reference at the pc.
  inline static void deserialization_set_target_internal_reference_at(
      Isolate* isolate, Address pc, Address target,
      RelocInfo::Mode mode = RelocInfo::INTERNAL_REFERENCE);

  // Here we are patching the address in the constant pool, not the actual call
  // instruction.  The address in the constant pool is the same size as a
  // pointer.
  static constexpr int kSpecialTargetSize = kPointerSize;

  // Size of an instruction.
  static constexpr int kInstrSize = sizeof(Instr);

  // Difference between address of current opcode and value read from pc
  // register.
  static constexpr int kPcLoadDelta = 8;
  RegList* GetScratchRegisterList() { return &scratch_register_list_; }
  VfpRegList* GetScratchVfpRegisterList() {
    return &scratch_vfp_register_list_;
  }

  // ---------------------------------------------------------------------------
  // Code generation

  // Insert the smallest number of nop instructions
  // possible to align the pc offset to a multiple
  // of m. m must be a power of 2 (>= 4).
  void Align(int m);
  // Insert the smallest number of zero bytes possible to align the pc offset
  // to a mulitple of m. m must be a power of 2 (>= 2).
  void DataAlign(int m);
  // Aligns code to something that's optimal for a jump target for the platform.
  void CodeTargetAlign();

  // Branch instructions
  void b(int branch_offset, Condition cond = al);
  void bl(int branch_offset, Condition cond = al);
  void blx(int branch_offset);  // v5 and above
  void blx(Register target, Condition cond = al);  // v5 and above
  void bx(Register target, Condition cond = al);  // v5 and above, plus v4t

  // Convenience branch instructions using labels
  void b(Label* L, Condition cond = al);
  void b(Condition cond, Label* L) { b(L, cond); }
  void bl(Label* L, Condition cond = al);
  void bl(Condition cond, Label* L) { bl(L, cond); }
  void blx(Label* L);  // v5 and above

  // Data-processing instructions

  void and_(Register dst, Register src1, const Operand& src2,
            SBit s = LeaveCC, Condition cond = al);

  void eor(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);

  void sub(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);
  void sub(Register dst, Register src1, Register src2,
           SBit s = LeaveCC, Condition cond = al);

  void rsb(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);

  void add(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);
  void add(Register dst, Register src1, Register src2,
           SBit s = LeaveCC, Condition cond = al);

  void adc(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);

  void sbc(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);

  void rsc(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);

  void tst(Register src1, const Operand& src2, Condition cond = al);
  void tst(Register src1, Register src2, Condition cond = al);

  void teq(Register src1, const Operand& src2, Condition cond = al);

  void cmp(Register src1, const Operand& src2, Condition cond = al);
  void cmp(Register src1, Register src2, Condition cond = al);

  void cmp_raw_immediate(Register src1, int raw_immediate, Condition cond = al);

  void cmn(Register src1, const Operand& src2, Condition cond = al);

  void orr(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);
  void orr(Register dst, Register src1, Register src2,
           SBit s = LeaveCC, Condition cond = al);

  void mov(Register dst, const Operand& src,
           SBit s = LeaveCC, Condition cond = al);
  void mov(Register dst, Register src, SBit s = LeaveCC, Condition cond = al);

  // Load the position of the label relative to the generated code object
  // pointer in a register.
  void mov_label_offset(Register dst, Label* label);

  // ARMv7 instructions for loading a 32 bit immediate in two instructions.
  // The constant for movw and movt should be in the range 0-0xffff.
  void movw(Register reg, uint32_t immediate, Condition cond = al);
  void movt(Register reg, uint32_t immediate, Condition cond = al);

  void bic(Register dst, Register src1, const Operand& src2,
           SBit s = LeaveCC, Condition cond = al);

  void mvn(Register dst, const Operand& src,
           SBit s = LeaveCC, Condition cond = al);

  // Shift instructions

  void asr(Register dst, Register src1, const Operand& src2, SBit s = LeaveCC,
           Condition cond = al);

  void lsl(Register dst, Register src1, const Operand& src2, SBit s = LeaveCC,
           Condition cond = al);

  void lsr(Register dst, Register src1, const Operand& src2, SBit s = LeaveCC,
           Condition cond = al);

  // Multiply instructions

  void mla(Register dst, Register src1, Register src2, Register srcA,
           SBit s = LeaveCC, Condition cond = al);

  void mls(Register dst, Register src1, Register src2, Register srcA,
           Condition cond = al);

  void sdiv(Register dst, Register src1, Register src2,
            Condition cond = al);

  void udiv(Register dst, Register src1, Register src2, Condition cond = al);

  void mul(Register dst, Register src1, Register src2,
           SBit s = LeaveCC, Condition cond = al);

  void smmla(Register dst, Register src1, Register src2, Register srcA,
             Condition cond = al);

  void smmul(Register dst, Register src1, Register src2, Condition cond = al);

  void smlal(Register dstL, Register dstH, Register src1, Register src2,
             SBit s = LeaveCC, Condition cond = al);

  void smull(Register dstL, Register dstH, Register src1, Register src2,
             SBit s = LeaveCC, Condition cond = al);

  void umlal(Register dstL, Register dstH, Register src1, Register src2,
             SBit s = LeaveCC, Condition cond = al);

  void umull(Register dstL, Register dstH, Register src1, Register src2,
             SBit s = LeaveCC, Condition cond = al);

  // Miscellaneous arithmetic instructions

  void clz(Register dst, Register src, Condition cond = al);  // v5 and above

  // Saturating instructions. v6 and above.

  // Unsigned saturate.
  //
  // Saturate an optionally shifted signed value to an unsigned range.
  //
  //   usat dst, #satpos, src
  //   usat dst, #satpos, src, lsl #sh
  //   usat dst, #satpos, src, asr #sh
  //
  // Register dst will contain:
  //
  //   0,                 if s < 0
  //   (1 << satpos) - 1, if s > ((1 << satpos) - 1)
  //   s,                 otherwise
  //
  // where s is the contents of src after shifting (if used.)
  void usat(Register dst, int satpos, const Operand& src, Condition cond = al);

  // Bitfield manipulation instructions. v7 and above.

  void ubfx(Register dst, Register src, int lsb, int width,
            Condition cond = al);

  void sbfx(Register dst, Register src, int lsb, int width,
            Condition cond = al);

  void bfc(Register dst, int lsb, int width, Condition cond = al);

  void bfi(Register dst, Register src, int lsb, int width,
           Condition cond = al);

  void pkhbt(Register dst, Register src1, const Operand& src2,
             Condition cond = al);

  void pkhtb(Register dst, Register src1, const Operand& src2,
             Condition cond = al);

  void sxtb(Register dst, Register src, int rotate = 0, Condition cond = al);
  void sxtab(Register dst, Register src1, Register src2, int rotate = 0,
             Condition cond = al);
  void sxth(Register dst, Register src, int rotate = 0, Condition cond = al);
  void sxtah(Register dst, Register src1, Register src2, int rotate = 0,
             Condition cond = al);

  void uxtb(Register dst, Register src, int rotate = 0, Condition cond = al);
  void uxtab(Register dst, Register src1, Register src2, int rotate = 0,
             Condition cond = al);
  void uxtb16(Register dst, Register src, int rotate = 0, Condition cond = al);
  void uxth(Register dst, Register src, int rotate = 0, Condition cond = al);
  void uxtah(Register dst, Register src1, Register src2, int rotate = 0,
             Condition cond = al);

  // Reverse the bits in a register.
  void rbit(Register dst, Register src, Condition cond = al);

  // Status register access instructions

  void mrs(Register dst, SRegister s, Condition cond = al);
  void msr(SRegisterFieldMask fields, const Operand& src, Condition cond = al);

  // Load/Store instructions
  void ldr(Register dst, const MemOperand& src, Condition cond = al);
  void str(Register src, const MemOperand& dst, Condition cond = al);
  void ldrb(Register dst, const MemOperand& src, Condition cond = al);
  void strb(Register src, const MemOperand& dst, Condition cond = al);
  void ldrh(Register dst, const MemOperand& src, Condition cond = al);
  void strh(Register src, const MemOperand& dst, Condition cond = al);
  void ldrsb(Register dst, const MemOperand& src, Condition cond = al);
  void ldrsh(Register dst, const MemOperand& src, Condition cond = al);
  void ldrd(Register dst1,
            Register dst2,
            const MemOperand& src, Condition cond = al);
  void strd(Register src1,
            Register src2,
            const MemOperand& dst, Condition cond = al);

  // Load literal from a pc relative address.
  void ldr_pcrel(Register dst, int imm12, Condition cond = al);

  // Load/Store exclusive instructions
  void ldrex(Register dst, Register src, Condition cond = al);
  void strex(Register src1, Register src2, Register dst, Condition cond = al);
  void ldrexb(Register dst, Register src, Condition cond = al);
  void strexb(Register src1, Register src2, Register dst, Condition cond = al);
  void ldrexh(Register dst, Register src, Condition cond = al);
  void strexh(Register src1, Register src2, Register dst, Condition cond = al);

  // Preload instructions
  void pld(const MemOperand& address);

  // Load/Store multiple instructions
  void ldm(BlockAddrMode am, Register base, RegList dst, Condition cond = al);
  void stm(BlockAddrMode am, Register base, RegList src, Condition cond = al);

  // Exception-generating instructions and debugging support
  void stop(const char* msg,
            Condition cond = al,
            int32_t code = kDefaultStopCode);

  void bkpt(uint32_t imm16);  // v5 and above
  void svc(uint32_t imm24, Condition cond = al);

  // Synchronization instructions.
  // On ARMv6, an equivalent CP15 operation will be used.
  void dmb(BarrierOption option);
  void dsb(BarrierOption option);
  void isb(BarrierOption option);

  // Coprocessor instructions

  void cdp(Coprocessor coproc, int opcode_1,
           CRegister crd, CRegister crn, CRegister crm,
           int opcode_2, Condition cond = al);

  void cdp2(Coprocessor coproc, int opcode_1,
            CRegister crd, CRegister crn, CRegister crm,
            int opcode_2);  // v5 and above

  void mcr(Coprocessor coproc, int opcode_1,
           Register rd, CRegister crn, CRegister crm,
           int opcode_2 = 0, Condition cond = al);

  void mcr2(Coprocessor coproc, int opcode_1,
            Register rd, CRegister crn, CRegister crm,
            int opcode_2 = 0);  // v5 and above

  void mrc(Coprocessor coproc, int opcode_1,
           Register rd, CRegister crn, CRegister crm,
           int opcode_2 = 0, Condition cond = al);

  void mrc2(Coprocessor coproc, int opcode_1,
            Register rd, CRegister crn, CRegister crm,
            int opcode_2 = 0);  // v5 and above

  void ldc(Coprocessor coproc, CRegister crd, const MemOperand& src,
           LFlag l = Short, Condition cond = al);
  void ldc(Coprocessor coproc, CRegister crd, Register base, int option,
           LFlag l = Short, Condition cond = al);

  void ldc2(Coprocessor coproc, CRegister crd, const MemOperand& src,
            LFlag l = Short);  // v5 and above
  void ldc2(Coprocessor coproc, CRegister crd, Register base, int option,
            LFlag l = Short);  // v5 and above

  // Support for VFP.
  // All these APIs support S0 to S31 and D0 to D31.

  void vldr(const DwVfpRegister dst,
            const Register base,
            int offset,
            const Condition cond = al);
  void vldr(const DwVfpRegister dst,
            const MemOperand& src,
            const Condition cond = al);

  void vldr(const SwVfpRegister dst,
            const Register base,
            int offset,
            const Condition cond = al);
  void vldr(const SwVfpRegister dst,
            const MemOperand& src,
            const Condition cond = al);

  void vstr(const DwVfpRegister src,
            const Register base,
            int offset,
            const Condition cond = al);
  void vstr(const DwVfpRegister src,
            const MemOperand& dst,
            const Condition cond = al);

  void vstr(const SwVfpRegister src,
            const Register base,
            int offset,
            const Condition cond = al);
  void vstr(const SwVfpRegister src,
            const MemOperand& dst,
            const Condition cond = al);

  void vldm(BlockAddrMode am,
            Register base,
            DwVfpRegister first,
            DwVfpRegister last,
            Condition cond = al);

  void vstm(BlockAddrMode am,
            Register base,
            DwVfpRegister first,
            DwVfpRegister last,
            Condition cond = al);

  void vldm(BlockAddrMode am,
            Register base,
            SwVfpRegister first,
            SwVfpRegister last,
            Condition cond = al);

  void vstm(BlockAddrMode am,
            Register base,
            SwVfpRegister first,
            SwVfpRegister last,
            Condition cond = al);

  void vmov(const SwVfpRegister dst, Float32 imm);
  void vmov(const DwVfpRegister dst,
            Double imm,
            const Register extra_scratch = no_reg);
  void vmov(const SwVfpRegister dst,
            const SwVfpRegister src,
            const Condition cond = al);
  void vmov(const DwVfpRegister dst,
            const DwVfpRegister src,
            const Condition cond = al);
  // TODO(bbudge) Replace uses of these with the more general core register to
  // scalar register vmov's.
  void vmov(const DwVfpRegister dst,
            const VmovIndex index,
            const Register src,
            const Condition cond = al);
  void vmov(const Register dst,
            const VmovIndex index,
            const DwVfpRegister src,
            const Condition cond = al);
  void vmov(const DwVfpRegister dst,
            const Register src1,
            const Register src2,
            const Condition cond = al);
  void vmov(const Register dst1,
            const Register dst2,
            const DwVfpRegister src,
            const Condition cond = al);
  void vmov(const SwVfpRegister dst,
            const Register src,
            const Condition cond = al);
  void vmov(const Register dst,
            const SwVfpRegister src,
            const Condition cond = al);
  void vcvt_f64_s32(const DwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_f32_s32(const SwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_f64_u32(const DwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_f32_u32(const SwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_s32_f32(const SwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_u32_f32(const SwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_s32_f64(const SwVfpRegister dst,
                    const DwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_u32_f64(const SwVfpRegister dst,
                    const DwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_f64_f32(const DwVfpRegister dst,
                    const SwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_f32_f64(const SwVfpRegister dst,
                    const DwVfpRegister src,
                    VFPConversionMode mode = kDefaultRoundToZero,
                    const Condition cond = al);
  void vcvt_f64_s32(const DwVfpRegister dst,
                    int fraction_bits,
                    const Condition cond = al);

  void vmrs(const Register dst, const Condition cond = al);
  void vmsr(const Register dst, const Condition cond = al);

  void vneg(const DwVfpRegister dst,
            const DwVfpRegister src,
            const Condition cond = al);
  void vneg(const SwVfpRegister dst, const SwVfpRegister src,
            const Condition cond = al);
  void vabs(const DwVfpRegister dst,
            const DwVfpRegister src,
            const Condition cond = al);
  void vabs(const SwVfpRegister dst, const SwVfpRegister src,
            const Condition cond = al);
  void vadd(const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vadd(const SwVfpRegister dst, const SwVfpRegister src1,
            const SwVfpRegister src2, const Condition cond = al);
  void vsub(const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vsub(const SwVfpRegister dst, const SwVfpRegister src1,
            const SwVfpRegister src2, const Condition cond = al);
  void vmul(const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vmul(const SwVfpRegister dst, const SwVfpRegister src1,
            const SwVfpRegister src2, const Condition cond = al);
  void vmla(const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vmla(const SwVfpRegister dst, const SwVfpRegister src1,
            const SwVfpRegister src2, const Condition cond = al);
  void vmls(const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vmls(const SwVfpRegister dst, const SwVfpRegister src1,
            const SwVfpRegister src2, const Condition cond = al);
  void vdiv(const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vdiv(const SwVfpRegister dst, const SwVfpRegister src1,
            const SwVfpRegister src2, const Condition cond = al);
  void vcmp(const DwVfpRegister src1,
            const DwVfpRegister src2,
            const Condition cond = al);
  void vcmp(const SwVfpRegister src1, const SwVfpRegister src2,
            const Condition cond = al);
  void vcmp(const DwVfpRegister src1,
            const double src2,
            const Condition cond = al);
  void vcmp(const SwVfpRegister src1, const float src2,
            const Condition cond = al);

  void vmaxnm(const DwVfpRegister dst,
              const DwVfpRegister src1,
              const DwVfpRegister src2);
  void vmaxnm(const SwVfpRegister dst,
              const SwVfpRegister src1,
              const SwVfpRegister src2);
  void vminnm(const DwVfpRegister dst,
              const DwVfpRegister src1,
              const DwVfpRegister src2);
  void vminnm(const SwVfpRegister dst,
              const SwVfpRegister src1,
              const SwVfpRegister src2);

  // VSEL supports cond in {eq, ne, ge, lt, gt, le, vs, vc}.
  void vsel(const Condition cond,
            const DwVfpRegister dst,
            const DwVfpRegister src1,
            const DwVfpRegister src2);
  void vsel(const Condition cond,
            const SwVfpRegister dst,
            const SwVfpRegister src1,
            const SwVfpRegister src2);

  void vsqrt(const DwVfpRegister dst,
             const DwVfpRegister src,
             const Condition cond = al);
  void vsqrt(const SwVfpRegister dst, const SwVfpRegister src,
             const Condition cond = al);

  // ARMv8 rounding instructions.
  void vrinta(const SwVfpRegister dst, const SwVfpRegister src);
  void vrinta(const DwVfpRegister dst, const DwVfpRegister src);
  void vrintn(const SwVfpRegister dst, const SwVfpRegister src);
  void vrintn(const DwVfpRegister dst, const DwVfpRegister src);
  void vrintm(const SwVfpRegister dst, const SwVfpRegister src);
  void vrintm(const DwVfpRegister dst, const DwVfpRegister src);
  void vrintp(const SwVfpRegister dst, const SwVfpRegister src);
  void vrintp(const DwVfpRegister dst, const DwVfpRegister src);
  void vrintz(const SwVfpRegister dst, const SwVfpRegister src,
              const Condition cond = al);
  void vrintz(const DwVfpRegister dst, const DwVfpRegister src,
              const Condition cond = al);

  // Support for NEON.

  // All these APIs support D0 to D31 and Q0 to Q15.
  void vld1(NeonSize size,
            const NeonListOperand& dst,
            const NeonMemOperand& src);
  void vst1(NeonSize size,
            const NeonListOperand& src,
            const NeonMemOperand& dst);
  // dt represents the narrower type
  void vmovl(NeonDataType dt, QwNeonRegister dst, DwVfpRegister src);
  // dt represents the narrower type.
  void vqmovn(NeonDataType dt, DwVfpRegister dst, QwNeonRegister src);

  // Only unconditional core <-> scalar moves are currently supported.
  void vmov(NeonDataType dt, DwVfpRegister dst, int index, Register src);
  void vmov(NeonDataType dt, Register dst, DwVfpRegister src, int index);

  void vmov(QwNeonRegister dst, QwNeonRegister src);
  void vdup(NeonSize size, QwNeonRegister dst, Register src);
  void vdup(NeonSize size, QwNeonRegister dst, DwVfpRegister src, int index);
  void vdup(NeonSize size, DwVfpRegister dst, DwVfpRegister src, int index);

  void vcvt_f32_s32(QwNeonRegister dst, QwNeonRegister src);
  void vcvt_f32_u32(QwNeonRegister dst, QwNeonRegister src);
  void vcvt_s32_f32(QwNeonRegister dst, QwNeonRegister src);
  void vcvt_u32_f32(QwNeonRegister dst, QwNeonRegister src);

  void vmvn(QwNeonRegister dst, QwNeonRegister src);
  void vswp(DwVfpRegister dst, DwVfpRegister src);
  void vswp(QwNeonRegister dst, QwNeonRegister src);
  void vabs(QwNeonRegister dst, QwNeonRegister src);
  void vabs(NeonSize size, QwNeonRegister dst, QwNeonRegister src);
  void vneg(QwNeonRegister dst, QwNeonRegister src);
  void vneg(NeonSize size, QwNeonRegister dst, QwNeonRegister src);

  void vand(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void veor(DwVfpRegister dst, DwVfpRegister src1, DwVfpRegister src2);
  void veor(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vbsl(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vorr(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vadd(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vadd(NeonSize size, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vqadd(NeonDataType dt, QwNeonRegister dst, QwNeonRegister src1,
             QwNeonRegister src2);
  void vsub(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vsub(NeonSize size, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vqsub(NeonDataType dt, QwNeonRegister dst, QwNeonRegister src1,
             QwNeonRegister src2);
  void vmul(QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vmul(NeonSize size, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vmin(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vmin(NeonDataType dt, QwNeonRegister dst,
            QwNeonRegister src1, QwNeonRegister src2);
  void vmax(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vmax(NeonDataType dt, QwNeonRegister dst,
            QwNeonRegister src1, QwNeonRegister src2);
  void vpadd(DwVfpRegister dst, DwVfpRegister src1, DwVfpRegister src2);
  void vpadd(NeonSize size, DwVfpRegister dst, DwVfpRegister src1,
             DwVfpRegister src2);
  void vpmin(NeonDataType dt, DwVfpRegister dst, DwVfpRegister src1,
             DwVfpRegister src2);
  void vpmax(NeonDataType dt, DwVfpRegister dst, DwVfpRegister src1,
             DwVfpRegister src2);
  void vshl(NeonDataType dt, QwNeonRegister dst, QwNeonRegister src, int shift);
  void vshr(NeonDataType dt, QwNeonRegister dst, QwNeonRegister src, int shift);
  void vsli(NeonSize size, DwVfpRegister dst, DwVfpRegister src, int shift);
  void vsri(NeonSize size, DwVfpRegister dst, DwVfpRegister src, int shift);
  // vrecpe and vrsqrte only support floating point lanes.
  void vrecpe(QwNeonRegister dst, QwNeonRegister src);
  void vrsqrte(QwNeonRegister dst, QwNeonRegister src);
  void vrecps(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vrsqrts(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vtst(NeonSize size, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vceq(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vceq(NeonSize size, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vcge(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vcge(NeonDataType dt, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vcgt(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2);
  void vcgt(NeonDataType dt, QwNeonRegister dst, QwNeonRegister src1,
            QwNeonRegister src2);
  void vext(QwNeonRegister dst, QwNeonRegister src1, QwNeonRegister src2,
            int bytes);
  void vzip(NeonSize size, DwVfpRegister src1, DwVfpRegister src2);
  void vzip(NeonSize size, QwNeonRegister src1, QwNeonRegister src2);
  void vuzp(NeonSize size, DwVfpRegister src1, DwVfpRegister src2);
  void vuzp(NeonSize size, QwNeonRegister src1, QwNeonRegister src2);
  void vrev16(NeonSize size, QwNeonRegister dst, QwNeonRegister src);
  void vrev32(NeonSize size, QwNeonRegister dst, QwNeonRegister src);
  void vrev64(NeonSize size, QwNeonRegister dst, QwNeonRegister src);
  void vtrn(NeonSize size, DwVfpRegister src1, DwVfpRegister src2);
  void vtrn(NeonSize size, QwNeonRegister src1, QwNeonRegister src2);
  void vtbl(DwVfpRegister dst, const NeonListOperand& list,
            DwVfpRegister index);
  void vtbx(DwVfpRegister dst, const NeonListOperand& list,
            DwVfpRegister index);

  // Pseudo instructions

  // Different nop operations are used by the code generator to detect certain
  // states of the generated code.
  enum NopMarkerTypes {
    NON_MARKING_NOP = 0,
    DEBUG_BREAK_NOP,
    // IC markers.
    PROPERTY_ACCESS_INLINED,
    PROPERTY_ACCESS_INLINED_CONTEXT,
    PROPERTY_ACCESS_INLINED_CONTEXT_DONT_DELETE,
    // Helper values.
    LAST_CODE_MARKER,
    FIRST_IC_MARKER = PROPERTY_ACCESS_INLINED
  };

  void nop(int type = 0);   // 0 is the default non-marking type.

  void push(Register src, Condition cond = al) {
    str(src, MemOperand(sp, 4, NegPreIndex), cond);
  }

  void pop(Register dst, Condition cond = al) {
    ldr(dst, MemOperand(sp, 4, PostIndex), cond);
  }

  void pop();

  void vpush(QwNeonRegister src, Condition cond = al) {
    vstm(db_w, sp, src.low(), src.high(), cond);
  }

  void vpush(DwVfpRegister src, Condition cond = al) {
    vstm(db_w, sp, src, src, cond);
  }

  void vpush(SwVfpRegister src, Condition cond = al) {
    vstm(db_w, sp, src, src, cond);
  }

  void vpop(DwVfpRegister dst, Condition cond = al) {
    vldm(ia_w, sp, dst, dst, cond);
  }

  // Jump unconditionally to given label.
  void jmp(Label* L) { b(L, al); }

  // Check the code size generated from label to here.
  int SizeOfCodeGeneratedSince(Label* label) {
    return pc_offset() - label->pos();
  }

  // Check the number of instructions generated from label to here.
  int InstructionsGeneratedSince(Label* label) {
    return SizeOfCodeGeneratedSince(label) / kInstrSize;
  }

  // Check whether an immediate fits an addressing mode 1 instruction.
  static bool ImmediateFitsAddrMode1Instruction(int32_t imm32);

  // Check whether an immediate fits an addressing mode 2 instruction.
  bool ImmediateFitsAddrMode2Instruction(int32_t imm32);

  // Class for scoping postponing the constant pool generation.
  class BlockConstPoolScope {
   public:
    explicit BlockConstPoolScope(Assembler* assem) : assem_(assem) {
      assem_->StartBlockConstPool();
    }
    ~BlockConstPoolScope() {
      assem_->EndBlockConstPool();
    }

   private:
    Assembler* assem_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(BlockConstPoolScope);
  };

  // Class for blocking sharing of code targets in constant pool.
  class BlockCodeTargetSharingScope {
   public:
    explicit BlockCodeTargetSharingScope(Assembler* assem) : assem_(nullptr) {
      Open(assem);
    }
    // This constructor does not initialize the scope. The user needs to
    // explicitly call Open() before using it.
    BlockCodeTargetSharingScope() : assem_(nullptr) {}
    ~BlockCodeTargetSharingScope() {
      Close();
    }
    void Open(Assembler* assem) {
      DCHECK_NULL(assem_);
      DCHECK_NOT_NULL(assem);
      assem_ = assem;
      assem_->StartBlockCodeTargetSharing();
    }

   private:
    void Close() {
      if (assem_ != nullptr) {
        assem_->EndBlockCodeTargetSharing();
      }
    }
    Assembler* assem_;

    DISALLOW_COPY_AND_ASSIGN(BlockCodeTargetSharingScope);
  };

  // Record a comment relocation entry that can be used by a disassembler.
  // Use --code-comments to enable.
  void RecordComment(const char* msg);

  // Record a deoptimization reason that can be used by a log or cpu profiler.
  // Use --trace-deopt to enable.
  void RecordDeoptReason(DeoptimizeReason reason, SourcePosition position,
                         int id);

  // Record the emission of a constant pool.
  //
  // The emission of constant pool depends on the size of the code generated and
  // the number of RelocInfo recorded.
  // The Debug mechanism needs to map code offsets between two versions of a
  // function, compiled with and without debugger support (see for example
  // Debug::PrepareForBreakPoints()).
  // Compiling functions with debugger support generates additional code
  // (DebugCodegen::GenerateSlot()). This may affect the emission of the
  // constant pools and cause the version of the code with debugger support to
  // have constant pools generated in different places.
  // Recording the position and size of emitted constant pools allows to
  // correctly compute the offset mappings between the different versions of a
  // function in all situations.
  //
  // The parameter indicates the size of the constant pool (in bytes), including
  // the marker and branch over the data.
  void RecordConstPool(int size);

  // Writes a single byte or word of data in the code stream.  Used
  // for inline tables, e.g., jump-tables. CheckConstantPool() should be
  // called before any use of db/dd/dq/dp to ensure that constant pools
  // are not emitted as part of the tables generated.
  void db(uint8_t data);
  void dd(uint32_t data);
  void dq(uint64_t data);
  void dp(uintptr_t data) { dd(data); }

  // Emits the address of the code stub's first instruction.
  void emit_code_stub_address(Code* stub);

  // Read/patch instructions
  Instr instr_at(int pos) { return *reinterpret_cast<Instr*>(buffer_ + pos); }
  void instr_at_put(int pos, Instr instr) {
    *reinterpret_cast<Instr*>(buffer_ + pos) = instr;
  }
  static Instr instr_at(byte* pc) { return *reinterpret_cast<Instr*>(pc); }
  static void instr_at_put(byte* pc, Instr instr) {
    *reinterpret_cast<Instr*>(pc) = instr;
  }
  static Condition GetCondition(Instr instr);
  static bool IsBranch(Instr instr);
  static int GetBranchOffset(Instr instr);
  static bool IsLdrRegisterImmediate(Instr instr);
  static bool IsVldrDRegisterImmediate(Instr instr);
  static int GetLdrRegisterImmediateOffset(Instr instr);
  static int GetVldrDRegisterImmediateOffset(Instr instr);
  static Instr SetLdrRegisterImmediateOffset(Instr instr, int offset);
  static Instr SetVldrDRegisterImmediateOffset(Instr instr, int offset);
  static bool IsStrRegisterImmediate(Instr instr);
  static Instr SetStrRegisterImmediateOffset(Instr instr, int offset);
  static bool IsAddRegisterImmediate(Instr instr);
  static Instr SetAddRegisterImmediateOffset(Instr instr, int offset);
  static Register GetRd(Instr instr);
  static Register GetRn(Instr instr);
  static Register GetRm(Instr instr);
  static bool IsPush(Instr instr);
  static bool IsPop(Instr instr);
  static bool IsStrRegFpOffset(Instr instr);
  static bool IsLdrRegFpOffset(Instr instr);
  static bool IsStrRegFpNegOffset(Instr instr);
  static bool IsLdrRegFpNegOffset(Instr instr);
  static bool IsLdrPcImmediateOffset(Instr instr);
  static bool IsVldrDPcImmediateOffset(Instr instr);
  static bool IsBlxReg(Instr instr);
  static bool IsBlxIp(Instr instr);
  static bool IsTstImmediate(Instr instr);
  static bool IsCmpRegister(Instr instr);
  static bool IsCmpImmediate(Instr instr);
  static Register GetCmpImmediateRegister(Instr instr);
  static int GetCmpImmediateRawImmediate(Instr instr);
  static bool IsNop(Instr instr, int type = NON_MARKING_NOP);
  static bool IsMovImmed(Instr instr);
  static bool IsOrrImmed(Instr instr);
  static bool IsMovT(Instr instr);
  static Instr GetMovTPattern();
  static bool IsMovW(Instr instr);
  static Instr GetMovWPattern();
  static Instr EncodeMovwImmediate(uint32_t immediate);
  static Instr PatchMovwImmediate(Instr instruction, uint32_t immediate);
  static int DecodeShiftImm(Instr instr);
  static Instr PatchShiftImm(Instr instr, int immed);

  // Constants in pools are accessed via pc relative addressing, which can
  // reach +/-4KB for integer PC-relative loads and +/-1KB for floating-point
  // PC-relative loads, thereby defining a maximum distance between the
  // instruction and the accessed constant.
  static constexpr int kMaxDistToIntPool = 4 * KB;
  static constexpr int kMaxDistToFPPool = 1 * KB;
  // All relocations could be integer, it therefore acts as the limit.
  static constexpr int kMinNumPendingConstants = 4;
  static constexpr int kMaxNumPending32Constants =
      kMaxDistToIntPool / kInstrSize;
  static constexpr int kMaxNumPending64Constants =
      kMaxDistToFPPool / kInstrSize;

  // Postpone the generation of the constant pool for the specified number of
  // instructions.
  void BlockConstPoolFor(int instructions);

  // Check if is time to emit a constant pool.
  void CheckConstPool(bool force_emit, bool require_jump);

  void MaybeCheckConstPool() {
    if (pc_offset() >= next_buffer_check_) {
      CheckConstPool(false, true);
    }
  }

  void PatchConstantPoolAccessInstruction(int pc_offset, int offset,
                                          ConstantPoolEntry::Access access,
                                          ConstantPoolEntry::Type type) {
    // No embedded constant pool support.
    UNREACHABLE();
  }

 protected:
  int buffer_space() const { return reloc_info_writer.pos() - pc_; }

  // Decode branch instruction at pos and return branch target pos
  int target_at(int pos);

  // Patch branch instruction at pos to branch to given branch target pos
  void target_at_put(int pos, int target_pos);

  // Prevent sharing of code target constant pool entries until
  // EndBlockCodeTargetSharing is called. Calls to this function can be nested
  // but must be followed by an equal number of call to
  // EndBlockCodeTargetSharing.
  void StartBlockCodeTargetSharing() {
    ++code_target_sharing_blocked_nesting_;
  }

  // Resume sharing of constant pool code target entries. Needs to be called
  // as many times as StartBlockCodeTargetSharing to have an effect.
  void EndBlockCodeTargetSharing() {
    --code_target_sharing_blocked_nesting_;
  }

  // Prevent contant pool emission until EndBlockConstPool is called.
  // Calls to this function can be nested but must be followed by an equal
  // number of call to EndBlockConstpool.
  void StartBlockConstPool() {
    if (const_pool_blocked_nesting_++ == 0) {
      // Prevent constant pool checks happening by setting the next check to
      // the biggest possible offset.
      next_buffer_check_ = kMaxInt;
    }
  }

  // Resume constant pool emission. Needs to be called as many times as
  // StartBlockConstPool to have an effect.
  void EndBlockConstPool() {
    if (--const_pool_blocked_nesting_ == 0) {
#ifdef DEBUG
      // Max pool start (if we need a jump and an alignment).
      int start = pc_offset() + kInstrSize + 2 * kPointerSize;
      // Check the constant pool hasn't been blocked for too long.
      DCHECK(pending_32_bit_constants_.empty() ||
             (start + pending_64_bit_constants_.size() * kDoubleSize <
              static_cast<size_t>(first_const_pool_32_use_ +
                                  kMaxDistToIntPool)));
      DCHECK(pending_64_bit_constants_.empty() ||
             (start < (first_const_pool_64_use_ + kMaxDistToFPPool)));
#endif
      // Two cases:
      //  * no_const_pool_before_ >= next_buffer_check_ and the emission is
      //    still blocked
      //  * no_const_pool_before_ < next_buffer_check_ and the next emit will
      //    trigger a check.
      next_buffer_check_ = no_const_pool_before_;
    }
  }

  bool is_const_pool_blocked() const {
    return (const_pool_blocked_nesting_ > 0) ||
           (pc_offset() < no_const_pool_before_);
  }

  bool VfpRegisterIsAvailable(DwVfpRegister reg) {
    DCHECK(reg.is_valid());
    return IsEnabled(VFP32DREGS) ||
           (reg.code() < LowDwVfpRegister::kNumRegisters);
  }

  bool VfpRegisterIsAvailable(QwNeonRegister reg) {
    DCHECK(reg.is_valid());
    return IsEnabled(VFP32DREGS) ||
           (reg.code() < LowDwVfpRegister::kNumRegisters / 2);
  }

  inline void emit(Instr x);

  // Code generation
  // The relocation writer's position is at least kGap bytes below the end of
  // the generated instructions. This is so that multi-instruction sequences do
  // not have to check for overflow. The same is true for writes of large
  // relocation info entries.
  static constexpr int kGap = 32;

  // Relocation info generation
  // Each relocation is encoded as a variable size value
  static constexpr int kMaxRelocSize = RelocInfoWriter::kMaxSize;
  RelocInfoWriter reloc_info_writer;

  // ConstantPoolEntry records are used during code generation as temporary
  // containers for constants and code target addresses until they are emitted
  // to the constant pool. These records are temporarily stored in a separate
  // buffer until a constant pool is emitted.
  // If every instruction in a long sequence is accessing the pool, we need one
  // pending relocation entry per instruction.

  // The buffers of pending constant pool entries.
  std::vector<ConstantPoolEntry> pending_32_bit_constants_;
  std::vector<ConstantPoolEntry> pending_64_bit_constants_;

  // Map of address of handle to index in pending_32_bit_constants_.
  std::map<Address, int> handle_to_index_map_;

  // Scratch registers available for use by the Assembler.
  RegList scratch_register_list_;
  VfpRegList scratch_vfp_register_list_;

 private:
  // Avoid overflows for displacements etc.
  static const int kMaximalBufferSize = 512 * MB;

  int next_buffer_check_;  // pc offset of next buffer check

  // Constant pool generation
  // Pools are emitted in the instruction stream, preferably after unconditional
  // jumps or after returns from functions (in dead code locations).
  // If a long code sequence does not contain unconditional jumps, it is
  // necessary to emit the constant pool before the pool gets too far from the
  // location it is accessed from. In this case, we emit a jump over the emitted
  // constant pool.
  // Constants in the pool may be addresses of functions that gets relocated;
  // if so, a relocation info entry is associated to the constant pool entry.

  // Repeated checking whether the constant pool should be emitted is rather
  // expensive. By default we only check again once a number of instructions
  // has been generated. That also means that the sizing of the buffers is not
  // an exact science, and that we rely on some slop to not overrun buffers.
  static constexpr int kCheckPoolIntervalInst = 32;
  static constexpr int kCheckPoolInterval = kCheckPoolIntervalInst * kInstrSize;

  // Sharing of code target entries may be blocked in some code sequences.
  int code_target_sharing_blocked_nesting_;
  bool IsCodeTargetSharingAllowed() const {
    return code_target_sharing_blocked_nesting_ == 0;
  }

  // Emission of the constant pool may be blocked in some code sequences.
  int const_pool_blocked_nesting_;  // Block emission if this is not zero.
  int no_const_pool_before_;  // Block emission before this pc offset.

  // Keep track of the first instruction requiring a constant pool entry
  // since the previous constant pool was emitted.
  int first_const_pool_32_use_;
  int first_const_pool_64_use_;

  // The bound position, before this we cannot do instruction elimination.
  int last_bound_pos_;

  inline void CheckBuffer();
  void GrowBuffer();

  // 32-bit immediate values
  void Move32BitImmediate(Register rd, const Operand& x, Condition cond = al);

  // Instruction generation
  void AddrMode1(Instr instr, Register rd, Register rn, const Operand& x);
  // Attempt to encode operand |x| for instruction |instr| and return true on
  // success. The result will be encoded in |instr| directly. This method may
  // change the opcode if deemed beneficial, for instance, MOV may be turned
  // into MVN, ADD into SUB, AND into BIC, ...etc.  The only reason this method
  // may fail is that the operand is an immediate that cannot be encoded.
  bool AddrMode1TryEncodeOperand(Instr* instr, const Operand& x);

  void AddrMode2(Instr instr, Register rd, const MemOperand& x);
  void AddrMode3(Instr instr, Register rd, const MemOperand& x);
  void AddrMode4(Instr instr, Register rn, RegList rl);
  void AddrMode5(Instr instr, CRegister crd, const MemOperand& x);

  // Labels
  void print(const Label* L);
  void bind_to(Label* L, int pos);
  void next(Label* L);

  // Record reloc info for current pc_
  void RecordRelocInfo(RelocInfo::Mode rmode, intptr_t data = 0);
  void ConstantPoolAddEntry(int position, RelocInfo::Mode rmode,
                            intptr_t value);
  void ConstantPoolAddEntry(int position, Double value);

  friend class RelocInfo;
  friend class BlockConstPoolScope;
  friend class BlockCodeTargetSharingScope;
  friend class EnsureSpace;
  friend class UseScratchRegisterScope;

  // The following functions help with avoiding allocations of embedded heap
  // objects during the code assembly phase. {RequestHeapObject} records the
  // need for a future heap number allocation or code stub generation. After
  // code assembly, {AllocateAndInstallRequestedHeapObjects} will allocate these
  // objects and place them where they are expected (determined by the pc offset
  // associated with each request). That is, for each request, it will patch the
  // dummy heap object handle that we emitted during code assembly with the
  // actual heap object handle.
  void RequestHeapObject(HeapObjectRequest request);
  void AllocateAndInstallRequestedHeapObjects(Isolate* isolate);

  std::forward_list<HeapObjectRequest> heap_object_requests_;
};

constexpr int kNoCodeAgeSequenceLength = 3 * Assembler::kInstrSize;

class EnsureSpace BASE_EMBEDDED {
 public:
  INLINE(explicit EnsureSpace(Assembler* assembler));
};

class PatchingAssembler : public Assembler {
 public:
  PatchingAssembler(IsolateData isolate_data, byte* address, int instructions);
  ~PatchingAssembler();

  void Emit(Address addr);
};

// This scope utility allows scratch registers to be managed safely. The
// Assembler's GetScratchRegisterList() is used as a pool of scratch
// registers. These registers can be allocated on demand, and will be returned
// at the end of the scope.
//
// When the scope ends, the Assembler's list will be restored to its original
// state, even if the list is modified by some other means. Note that this scope
// can be nested but the destructors need to run in the opposite order as the
// constructors. We do not have assertions for this.
class UseScratchRegisterScope {
 public:
  explicit UseScratchRegisterScope(Assembler* assembler);
  ~UseScratchRegisterScope();

  // Take a register from the list and return it.
  Register Acquire();
  SwVfpRegister AcquireS() { return AcquireVfp<SwVfpRegister>(); }
  LowDwVfpRegister AcquireLowD() { return AcquireVfp<LowDwVfpRegister>(); }
  DwVfpRegister AcquireD() {
    DwVfpRegister reg = AcquireVfp<DwVfpRegister>();
    DCHECK(assembler_->VfpRegisterIsAvailable(reg));
    return reg;
  }
  QwNeonRegister AcquireQ() {
    QwNeonRegister reg = AcquireVfp<QwNeonRegister>();
    DCHECK(assembler_->VfpRegisterIsAvailable(reg));
    return reg;
  }

 private:
  friend class Assembler;
  friend class TurboAssembler;

  // Check if we have registers available to acquire.
  // These methods are kept private intentionally to restrict their usage to the
  // assemblers. Choosing to emit a difference instruction sequence depending on
  // the availability of scratch registers is generally their job.
  bool CanAcquire() const { return *assembler_->GetScratchRegisterList() != 0; }
  template <typename T>
  bool CanAcquireVfp() const;

  template <typename T>
  T AcquireVfp();

  Assembler* assembler_;
  // Available scratch registers at the start of this scope.
  RegList old_available_;
  VfpRegList old_available_vfp_;
};

}  // namespace internal
}  // namespace v8

#endif  // V8_ARM_ASSEMBLER_ARM_H_
