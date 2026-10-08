cmake_minimum_required(VERSION 3.20)
project(MiniRVTarget LANGUAGES C CXX)
find_package(LLVM REQUIRED CONFIG)
if(NOT LLVM_VERSION_MAJOR EQUAL 21)
  message(FATAL_ERROR "MiniRV requires LLVM 21")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(${LLVM_DEFINITIONS})

find_program(LLVM_TBLGEN llvm-tblgen HINTS ${LLVM_TOOLS_BINARY_DIR} REQUIRED)
set(TARGET_TD ${CMAKE_CURRENT_SOURCE_DIR}/MiniRV.td)
set(GENERATED)
function(generate_table MODE FILE)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    COMMAND ${LLVM_TBLGEN} -gen-${MODE}
            -I ${LLVM_INCLUDE_DIRS}
            -I ${CMAKE_CURRENT_SOURCE_DIR}
            ${TARGET_TD}
            -o ${CMAKE_CURRENT_BINARY_DIR}/${FILE}
    DEPENDS ${TARGET_TD}
            ${CMAKE_CURRENT_SOURCE_DIR}/MiniRVRegisterInfo.td
            ${CMAKE_CURRENT_SOURCE_DIR}/MiniRVInstrInfo.td
    VERBATIM)
  set(GENERATED ${GENERATED} ${CMAKE_CURRENT_BINARY_DIR}/${FILE} PARENT_SCOPE)
endfunction()

generate_table(register-info MiniRVGenRegisterInfo.inc)
generate_table(instr-info MiniRVGenInstrInfo.inc)
generate_table(subtarget MiniRVGenSubtargetInfo.inc)
generate_table(dag-isel MiniRVGenDAGISel.inc)
generate_table(asm-writer MiniRVGenAsmWriter.inc)

add_executable(minirv-llc
  MiniRVCodeGen.cpp
  MiniRVISel.cpp
  MiniRVMC.cpp
  MiniRVAsmPrinter.cpp
  minirv-llc.cpp
  ${GENERATED})
target_include_directories(minirv-llc PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${LLVM_INCLUDE_DIRS})
target_compile_options(minirv-llc PRIVATE -fno-exceptions)
find_library(LLVM_SHARED_LIBRARY NAMES LLVM-21 LLVM
             PATHS ${LLVM_LIBRARY_DIRS} REQUIRED NO_DEFAULT_PATH)
target_link_libraries(minirv-llc PRIVATE ${LLVM_SHARED_LIBRARY})
