add_definitions( -DKSC_BUILD_LIB )
add_definitions( "/wd4800 /wd4146 /wd4996 /wd4244 " )

set( LLVM_SDK_PATH $ENV{LLVM_INSTALL_PATH} )

include_directories( "${LLVM_SDK_PATH}/include" )


file( GLOB_RECURSE KSC_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp *.c *.h )
add_library( ${KSC_MODULE_NAME} SHARED ${KSC_SRC} )

install( TARGETS ${KSC_MODULE_NAME} RUNTIME DESTINATION bin CONFIGURATIONS)

# Specify the dependencies of library
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMJIT.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMInterpreter.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86CodeGen.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86AsmParser.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86Disassembler.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMRuntimeDyld.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMExecutionEngine.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMAsmPrinter.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMSelectionDAG.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86Desc.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMMCParser.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMCodeGen.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86AsmPrinter.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86Info.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMScalarOpts.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMX86Utils.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMInstCombine.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMTransformUtils.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMipa.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMAnalysis.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMTarget.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMCore.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMMC.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMObject.lib" )
target_link_libraries( ${KSC_MODULE_NAME} "${LLVM_SDK_PATH}/lib/LLVMSupport.lib" )


