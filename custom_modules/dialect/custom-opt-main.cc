// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Main entry function for custom-opt.
// Forked from IREE's iree-opt main entry function (iree-opt-main.cc).
//
// We need this entry function because we want to register the custom
// dialect, which is missing in IREE's opt main entry function.
//
// An alternative approach is to link the dialect into the excutable as
// whole archive.

#include "init_dialect.h"

#include "iree/tools/init_dialects.h"
#include "iree/tools/init_passes.h"
#include "iree/tools/init_targets.h"
#include "llvm/Support/InitLLVM.h"
#include "mlir/IR/Dialect.h"
#include "mlir/Support/MlirOptMain.h"

int main(int argc, char **argv) {
  llvm::InitLLVM y(argc, argv);

  mlir::DialectRegistry registry;
  mlir::iree_compiler::registerAllDialects(registry);
  mlir::iree_compiler::registerCustomDialect(registry);

  mlir::iree_compiler::registerAllPasses();
  mlir::iree_compiler::registerHALTargetBackends();

  if (failed(MlirOptMain(argc, argv, "IREE-TF modular optimizer driver\n",
                         registry,
                         /*preloadDialectsInContext=*/false))) {
    return 1;
  }
  return 0;
}
