//===--- Minix.cpp - Minix ToolChain Implementations ------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Minix.h"
#include "Arch/ARM.h"
#include "Arch/Mips.h"
#include "Arch/Sparc.h"
#include "CommonArgs.h"
#include "InputInfo.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/VirtualFileSystem.h"

using namespace clang::driver;
using namespace clang::driver::tools;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

void tools::minix::Assembler::ConstructJob(Compilation &C, const JobAction &JA,
                                           const InputInfo &Output,
                                           const InputInfoList &Inputs,
                                           const ArgList &Args,
                                           const char *LinkingOutput) const {
  claimNoWarnArgs(Args);
  ArgStringList CmdArgs;

  // GNU as needs different flags for creating the correct output format
  // on architectures with different ABIs or optional feature sets.
  switch (getToolChain().getArch()) {
  case llvm::Triple::x86:
    CmdArgs.push_back("--32");
    break;
  case llvm::Triple::arm:
  case llvm::Triple::armeb:
  case llvm::Triple::thumb:
  case llvm::Triple::thumbeb: {
    StringRef MArch, MCPU;
    arm::getARMArchCPUFromArgs(Args, MArch, MCPU, /*FromAs*/ true);
    std::string Arch =
        arm::getARMTargetCPU(MCPU, MArch, getToolChain().getTriple());
    CmdArgs.push_back(Args.MakeArgString("-mcpu=" + Arch));
    break;
  }

#if 0 /* LSC: Not needed for MINIX but kept to ease comparison with NetBSD. */
  case llvm::Triple::mips:
  case llvm::Triple::mipsel:
  case llvm::Triple::mips64:
  case llvm::Triple::mips64el: {
    StringRef CPUName;
    StringRef ABIName;
    mips::getMipsCPUAndABI(Args, getToolChain().getTriple(), CPUName, ABIName);

    CmdArgs.push_back("-march");
    CmdArgs.push_back(CPUName.data());

    CmdArgs.push_back("-mabi");
    CmdArgs.push_back(mips::getGnuCompatibleMipsABIName(ABIName).data());

    if (getToolChain().getTriple().isLittleEndian())
      CmdArgs.push_back("-EL");
    else
      CmdArgs.push_back("-EB");

    AddAssemblerKPIC(getToolChain(), Args, CmdArgs);
    break;
  }

  case llvm::Triple::sparc:
  case llvm::Triple::sparcel: {
    CmdArgs.push_back("-32");
    std::string CPU = getCPUName(Args, getToolChain().getTriple());
    CmdArgs.push_back(sparc::getSparcAsmModeForCPU(CPU, getToolChain().getTriple()));
    AddAssemblerKPIC(getToolChain(), Args, CmdArgs);
    break;
  }

  case llvm::Triple::sparcv9: {
    CmdArgs.push_back("-64");
    std::string CPU = getCPUName(Args, getToolChain().getTriple());
    CmdArgs.push_back(sparc::getSparcAsmModeForCPU(CPU, getToolChain().getTriple()));
    AddAssemblerKPIC(getToolChain(), Args, CmdArgs);
    break;
  }
#endif /* 0 */

  default:
    break;
  }

  Args.AddAllArgValues(CmdArgs, options::OPT_Wa_COMMA, options::OPT_Xassembler);

  CmdArgs.push_back("-o");
  CmdArgs.push_back(Output.getFilename());

  for (const auto &II : Inputs)
    CmdArgs.push_back(II.getFilename());

  const char *Exec = Args.MakeArgString((getToolChain().GetProgramPath("as")));
  C.addCommand(std::make_unique<Command>(JA, *this, Exec, CmdArgs, Inputs));
}

void tools::minix::Linker::ConstructJob(Compilation &C, const JobAction &JA,
                                 const InputInfo &Output,
                                 const InputInfoList &Inputs,
                                 const ArgList &Args,
                                 const char *LinkingOutput) const {
  const toolchains::Minix &ToolChain =
    static_cast<const toolchains::Minix &>(getToolChain());
  const Driver &D = getToolChain().getDriver();
  ArgStringList CmdArgs;

  if (!D.SysRoot.empty())
    CmdArgs.push_back(Args.MakeArgString("--sysroot=" + D.SysRoot));

  CmdArgs.push_back("--eh-frame-hdr");
  if (Args.hasArg(options::OPT_static)) {
    CmdArgs.push_back("-Bstatic");
  } else {
    if (Args.hasArg(options::OPT_rdynamic))
      CmdArgs.push_back("-export-dynamic");
    if (Args.hasArg(options::OPT_shared)) {
      CmdArgs.push_back("-Bshareable");
    } else {
      CmdArgs.push_back("-dynamic-linker");
      // LSC: Small deviation from the NetBSD version.
      //      Use the same linker path as gcc.
      CmdArgs.push_back("/usr/libexec/ld.elf_so");
    }
  }

  // Many NetBSD architectures support more than one ABI.
  // Determine the correct emulation for ld.
  switch (ToolChain.getArch()) {
  case llvm::Triple::x86:
    CmdArgs.push_back("-m");
    CmdArgs.push_back("elf_i386_minix");
#if 0 /* LSC: Not needed for MINIX but kept to ease comparison with NetBSD. */
    break;
  case llvm::Triple::arm:
  case llvm::Triple::thumb:
    CmdArgs.push_back("-m");
    switch (ToolChain.getTriple().getEnvironment()) {
    case llvm::Triple::EABI:
    case llvm::Triple::GNUEABI:
      CmdArgs.push_back("armelf_nbsd_eabi");
      break;
    case llvm::Triple::EABIHF:
    case llvm::Triple::GNUEABIHF:
      CmdArgs.push_back("armelf_nbsd_eabihf");
      break;
    default:
      CmdArgs.push_back("armelf_nbsd");
      break;
    }
    break;
  case llvm::Triple::armeb:
  case llvm::Triple::thumbeb:
    arm::appendBE8LinkFlag(Args, CmdArgs, ToolChain.getEffectiveTriple());
    CmdArgs.push_back("-m");
    switch (ToolChain.getTriple().getEnvironment()) {
    case llvm::Triple::EABI:
    case llvm::Triple::GNUEABI:
      CmdArgs.push_back("armelfb_nbsd_eabi");
      break;
    case llvm::Triple::EABIHF:
    case llvm::Triple::GNUEABIHF:
      CmdArgs.push_back("armelfb_nbsd_eabihf");
      break;
    default:
      CmdArgs.push_back("armelfb_nbsd");
      break;
    }
    break;
  case llvm::Triple::mips64:
  case llvm::Triple::mips64el:
    if (mips::hasMipsAbiArg(Args, "32")) {
      CmdArgs.push_back("-m");
      if (ToolChain.getArch() == llvm::Triple::mips64)
        CmdArgs.push_back("elf32btsmip");
      else
        CmdArgs.push_back("elf32ltsmip");
    } else if (mips::hasMipsAbiArg(Args, "64")) {
      CmdArgs.push_back("-m");
      if (ToolChain.getArch() == llvm::Triple::mips64)
        CmdArgs.push_back("elf64btsmip");
      else
        CmdArgs.push_back("elf64ltsmip");
    }
    break;
  case llvm::Triple::ppc:
    CmdArgs.push_back("-m");
    CmdArgs.push_back("elf32ppc_nbsd");
    break;

  case llvm::Triple::ppc64:
  case llvm::Triple::ppc64le:
    CmdArgs.push_back("-m");
    CmdArgs.push_back("elf64ppc");
    break;

  case llvm::Triple::sparc:
    CmdArgs.push_back("-m");
    CmdArgs.push_back("elf32_sparc");
    break;

  case llvm::Triple::sparcv9:
    CmdArgs.push_back("-m");
    CmdArgs.push_back("elf64_sparc");
    break;
#endif /* 0 */

  default:
    break;
  }

  if (Output.isFilename()) {
    CmdArgs.push_back("-o");
    CmdArgs.push_back(Output.getFilename());
  } else {
    assert(Output.isNothing() && "Invalid output.");
  }

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
    if (!Args.hasArg(options::OPT_shared)) {
      CmdArgs.push_back(
          Args.MakeArgString(ToolChain.GetFilePath("crt0.o")));
    }
    CmdArgs.push_back(
        Args.MakeArgString(ToolChain.GetFilePath("crti.o")));
    if (Args.hasArg(options::OPT_shared)){
      CmdArgs.push_back(
          Args.MakeArgString(ToolChain.GetFilePath("crtbeginS.o")));
    } else {
      CmdArgs.push_back(
          Args.MakeArgString(ToolChain.GetFilePath("crtbegin.o")));
    }
  }


  Args.AddAllArgs(CmdArgs, options::OPT_L);
  Args.AddAllArgs(CmdArgs, options::OPT_T_Group);
  Args.AddAllArgs(CmdArgs, options::OPT_e);
  Args.AddAllArgs(CmdArgs, options::OPT_s);
  Args.AddAllArgs(CmdArgs, options::OPT_t);
  Args.AddAllArgs(CmdArgs, options::OPT_Z_Flag);
  Args.AddAllArgs(CmdArgs, options::OPT_r);

  AddLinkerInputs(getToolChain(), Inputs, Args, CmdArgs, JA);

#if 0 /* LSC: Not needed for MINIX, but kept to ease comparison with NetBSD. */
  unsigned Major, Minor, Micro;
  ToolChain.getTriple().getOSVersion(Major, Minor, Micro);
  bool useLibgcc = true;
  if (Major >= 7 || Major == 0) {
    switch (ToolChain.getArch()) {
    case llvm::Triple::aarch64:
    case llvm::Triple::aarch64_be:
    case llvm::Triple::arm:
    case llvm::Triple::armeb:
    case llvm::Triple::thumb:
    case llvm::Triple::thumbeb:
    case llvm::Triple::ppc:
    case llvm::Triple::ppc64:
    case llvm::Triple::ppc64le:
    case llvm::Triple::sparc:
    case llvm::Triple::sparcv9:
    case llvm::Triple::x86:
    case llvm::Triple::x86_64:
      useLibgcc = false;
      break;
    default:
      break;
    }
  }
#endif /* 0 */

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nodefaultlibs)) {
    if (D.CCCIsCXX()) {
      getToolChain().AddCXXStdlibLibArgs(Args, CmdArgs);
      CmdArgs.push_back("-lm");

      /* LSC: Hack as lc++ is linked against mthread. */
      CmdArgs.push_back("-lmthread");
    }
    if (Args.hasArg(options::OPT_pthread))
       CmdArgs.push_back("-lpthread");
    CmdArgs.push_back("-lc");

#if 0 /* LSC: Minix always use CompilerRT-Generic. */
    if (useLibgcc) {
      if (Args.hasArg(options::OPT_static)) {
        // libgcc_eh depends on libc, so resolve as much as possible,
        // pull in any new requirements from libc and then get the rest
        // of libgcc.
        CmdArgs.push_back("-lgcc_eh");
        CmdArgs.push_back("-lc");
        CmdArgs.push_back("-lgcc");
      } else {
        CmdArgs.push_back("-lgcc");
        CmdArgs.push_back("--as-needed");
        CmdArgs.push_back("-lgcc_s");
        CmdArgs.push_back("--no-as-needed");
      }
    }
#endif /* 0 */
  }

  if (!Args.hasArg(options::OPT_nostdlib, options::OPT_nostartfiles)) {
    if (Args.hasArg(options::OPT_shared))
      CmdArgs.push_back(
          Args.MakeArgString(ToolChain.GetFilePath("crtendS.o")));
    else
      CmdArgs.push_back(
          Args.MakeArgString(ToolChain.GetFilePath("crtend.o")));
    CmdArgs.push_back(Args.MakeArgString(ToolChain.GetFilePath("crtn.o")));
  }

  ToolChain.addProfileRTLibs(Args, CmdArgs);

  const char *Exec = Args.MakeArgString(ToolChain.GetLinkerPath());
  C.addCommand(std::make_unique<Command>(JA, *this, Exec, CmdArgs, Inputs));
}

/// Minix - Minix tool chain which can call as(1) and ld(1) directly.

toolchains::Minix::Minix(const Driver &D, const llvm::Triple &Triple,
                         const ArgList &Args)
    : Generic_ELF(D, Triple, Args) {
  if (!Args.hasArg(options::OPT_nostdlib)) {
    // When targeting a 32-bit platform, try the special directory used on
    // 64-bit hosts, and only fall back to the main library directory if that
    // doesn't work.
    // FIXME: It'd be nicer to test if this directory exists, but I'm not sure
    // what all logic is needed to emulate the '=' prefix here.
    switch (Triple.getArch()) {
    case llvm::Triple::x86:
      getFilePaths().push_back("=/usr/lib/i386");
      break;
#if 0 // LSC: Not yet useful for MINIX, but kept to simplify comparison with NetBSD
    case llvm::Triple::arm:
    case llvm::Triple::armeb:
    case llvm::Triple::thumb:
    case llvm::Triple::thumbeb:
      switch (Triple.getEnvironment()) {
      case llvm::Triple::EABI:
      case llvm::Triple::GNUEABI:
        getFilePaths().push_back("=/usr/lib/eabi");
        break;
      case llvm::Triple::EABIHF:
      case llvm::Triple::GNUEABIHF:
        getFilePaths().push_back("=/usr/lib/eabihf");
        break;
      default:
        getFilePaths().push_back("=/usr/lib/oabi");
        break;
      }
      break;
    case llvm::Triple::mips64:
    case llvm::Triple::mips64el:
      if (tools::mips::hasMipsAbiArg(Args, "o32"))
        getFilePaths().push_back("=/usr/lib/o32");
      else if (tools::mips::hasMipsAbiArg(Args, "64"))
        getFilePaths().push_back("=/usr/lib/64");
      break;
    case llvm::Triple::ppc:
      getFilePaths().push_back("=/usr/lib/powerpc");
      break;
    case llvm::Triple::sparc:
      getFilePaths().push_back("=/usr/lib/sparc");
      break;
#endif /* 0 */
    default:
      break;
    }

    getFilePaths().push_back("=/usr/lib");
  }
}

Tool *toolchains::Minix::buildAssembler() const {
  return new tools::minix::Assembler(*this);
}

Tool *toolchains::Minix::buildLinker() const {
  return new tools::minix::Linker(*this);
}

ToolChain::CXXStdlibType
Minix::GetCXXStdlibType(const ArgList &Args) const {
  if (Arg *A = Args.getLastArg(options::OPT_stdlib_EQ)) {
    StringRef Value = A->getValue();
    if (Value == "libstdc++")
      return ToolChain::CST_Libstdcxx;
    if (Value == "libc++")
      return ToolChain::CST_Libcxx;

    getDriver().Diag(diag::err_drv_invalid_stdlib_name)
      << A->getAsString(Args);
  }

#if 0 /* LSC: We only us libcxx by default */
  unsigned Major, Minor, Micro;
  getTriple().getOSVersion(Major, Minor, Micro);
  if (Major >= 7 || (Major == 6 && Minor == 99 && Micro >= 49) || Major == 0) {
    switch (getArch()) {
    case llvm::Triple::aarch64:
    case llvm::Triple::arm:
    case llvm::Triple::armeb:
    case llvm::Triple::thumb:
    case llvm::Triple::thumbeb:
    case llvm::Triple::ppc:
    case llvm::Triple::ppc64:
    case llvm::Triple::ppc64le:
    case llvm::Triple::x86:
    case llvm::Triple::x86_64:
      return ToolChain::CST_Libcxx;
    default:
      break;
    }
  }
  return ToolChain::CST_Libstdcxx;
#else
  return ToolChain::CST_Libcxx;
#endif /* 0 */
}

void Minix::AddClangCXXStdlibIncludeArgs(const ArgList &DriverArgs,
                                          ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdlibinc) ||
      DriverArgs.hasArg(options::OPT_nostdincxx))
    return;

  switch (GetCXXStdlibType(DriverArgs)) {
  case ToolChain::CST_Libcxx:
    addSystemInclude(DriverArgs, CC1Args,
                     getDriver().SysRoot + "/usr/include/c++/");
    break;
  case ToolChain::CST_Libstdcxx:
    addSystemInclude(DriverArgs, CC1Args,
                     getDriver().SysRoot + "/usr/include/g++");
    addSystemInclude(DriverArgs, CC1Args,
                     getDriver().SysRoot + "/usr/include/g++/backward");
    break;
  }
}
