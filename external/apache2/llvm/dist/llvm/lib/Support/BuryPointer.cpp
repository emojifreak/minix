//===- BuryPointer.cpp - Memory Manipulation/Leak ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/BuryPointer.h"
#include "llvm/Support/Compiler.h"
#if !defined(_LIBCPP_HAS_NO_THREADS) && defined(__minix)
#include <atomic>
#endif // !defined(_LIBCPP_HAS_NO_THREADS) && defined(__minix)

namespace llvm {

void BuryPointer(const void *Ptr) {
  // This function may be called only a small fixed amount of times per each
  // invocation, otherwise we do actually have a leak which we want to report.
  // If this function is called more than kGraveYardMaxSize times, the pointers
  // will not be properly buried and a leak detector will report a leak, which
  // is what we want in such case.
  static const size_t kGraveYardMaxSize = 16;
  LLVM_ATTRIBUTE_UNUSED static const void *GraveYard[kGraveYardMaxSize];
#if !defined(_LIBCPP_HAS_NO_THREADS) && defined(__minix)
  static std::atomic<unsigned> GraveYardSize;
#else
  static unsigned GraveYardSize;
#endif // !defined(_LIBCPP_HAS_NO_THREADS) && defined(__minix)
  unsigned Idx = GraveYardSize++;
  if (Idx >= kGraveYardMaxSize)
    return;
  GraveYard[Idx] = Ptr;
}

}
