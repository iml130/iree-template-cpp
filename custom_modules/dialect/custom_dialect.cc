// Licensed under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Forked from IREE with modified includes

#include "custom_dialect.h"
#include "conversion_patterns.h"
#include "custom.imports.h"

#include "iree/compiler/Dialect/HAL/Conversion/ConversionDialectInterface.h"
#include "iree/compiler/Dialect/VM/Conversion/ConversionDialectInterface.h"
#include "llvm/ADT/StringSwitch.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Parser.h"
#include "mlir/Transforms/DialectConversion.h"

namespace mlir {
namespace iree_compiler {
namespace IREE {
namespace Custom {

namespace {

// Exposes conversion patterns that transition tensors to buffers during the
// Flow->HAL dialect lowering. This is only required if the dialect has ops that
// use tensor types.
class CustomToHALConversionInterface : public HALConversionDialectInterface {
 public:
  using HALConversionDialectInterface::HALConversionDialectInterface;

  void setupConversionTarget(ConversionTarget &target,
                             OwningRewritePatternList &patterns,
                             TypeConverter &typeConverter) const override {
    populateCustomToHALPatterns(getDialect()->getContext(), patterns,
                                typeConverter);
  }
};

// Exposes the import module and conversion patterns used to convert custom
// ops to their vm.import counterparts.
class CustomToVMConversionInterface : public VMConversionDialectInterface {
 public:
  using VMConversionDialectInterface::VMConversionDialectInterface;

  OwningModuleRef parseVMImportModule() const override {
    return mlir::parseSourceString(
        StringRef(custom_imports_create()->data, custom_imports_create()->size),
        getDialect()->getContext());
  }

  void populateVMConversionPatterns(
      SymbolTable &importSymbols, OwningRewritePatternList &patterns,
      TypeConverter &typeConverter) const override {
    populateCustomToVMPatterns(getDialect()->getContext(), importSymbols,
                               patterns, typeConverter);
  }
};

}  // namespace

CustomDialect::CustomDialect(MLIRContext *context)
    : Dialect(getDialectNamespace(), context, TypeID::get<CustomDialect>()) {
  addInterfaces<CustomToHALConversionInterface,
                CustomToVMConversionInterface>();

  addTypes<MessageType>();

#define GET_OP_LIST
  addOperations<
#include "custom_ops.cc.inc"
      >();
}

Type CustomDialect::parseType(DialectAsmParser &parser) const {
  StringRef typeName;
  if (failed(parser.parseKeyword(&typeName))) return {};
  auto type = llvm::StringSwitch<Type>(typeName)
                  .Case("message", MessageType::get(getContext()))
                  .Default(nullptr);
  if (!type) {
    parser.emitError(parser.getCurrentLocation())
        << "unknown type: " << typeName;
  }
  return type;
}

void CustomDialect::printType(Type type, DialectAsmPrinter &p) const {
  if (type.isa<MessageType>()) {
    p << "message";
  } else {
    llvm_unreachable("unknown type");
  }
}

}  // namespace Custom
}  // namespace IREE
}  // namespace iree_compiler
}  // namespace mlir

#define GET_OP_CLASSES
#include "custom_ops.cc.inc"
