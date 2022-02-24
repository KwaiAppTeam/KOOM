# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

set(TARGET kwai-unwind)
set(LINK_DIR ${CMAKE_SOURCE_DIR}/../../../../kwai-android-base)

project(${TARGET})

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

add_compile_options(-Wall -Wextra -Wexit-time-destructors -DDEXFILE_SUPPORT -Oz)

include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/libbacktrace/include
        ${CMAKE_SOURCE_DIR}/libunwindstack/include

        ${LINK_DIR}/src/main/cpp/include
        ${LINK_DIR}/src/main/cpp/liblog/include
        ${LINK_DIR}/src/main/cpp/lzma
)

link_directories(${LINK_DIR}/src/main/libs/${ANDROID_ABI}/)

# Define the base library, its includes and the needed defines.
set(BASE_SOURCES libunwindstack/ArmExidx.cpp libunwindstack/DexFiles.cpp libunwindstack/DwarfCfa.cpp libunwindstack/DwarfEhFrameWithHdr.cpp libunwindstack/DwarfMemory.cpp
        libunwindstack/DwarfOp.cpp libunwindstack/DwarfSection.cpp libunwindstack/Elf.cpp libunwindstack/ElfInterface.cpp libunwindstack/ElfInterfaceArm.cpp libunwindstack/Global.cpp
        libunwindstack/JitDebug.cpp libunwindstack/Log.cpp libunwindstack/MapInfo.cpp libunwindstack/Maps.cpp libunwindstack/Memory.cpp libunwindstack/MemoryMte.cpp libunwindstack/LocalUnwinder.cpp
        libunwindstack/Regs.cpp libunwindstack/RegsArm.cpp libunwindstack/RegsArm64.cpp libunwindstack/RegsX86.cpp libunwindstack/RegsX86_64.cpp libunwindstack/RegsMips.cpp libunwindstack/RegsMips64.cpp
        libunwindstack/Unwinder.cpp libunwindstack/Symbols.cpp libunwindstack/DexFile.cpp
        libunwindstack/ThreadEntry.cpp libunwindstack/ThreadUnwinder.cpp)

set(DEXFILE_SUPPORT_SOURCES dexfile_support/dex_file_supp.cpp)

set(BACKTRACE_SOURCES libbacktrace/Backtrace.cpp libbacktrace/BacktraceCurrent.cpp
        libbacktrace/BacktraceMap.cpp libbacktrace/BacktracePtrace.cpp libbacktrace/ThreadEntry.cpp
        libbacktrace/UnwindStack.cpp libbacktrace/UnwindStackMap.cpp)

set(FAST_UNWIND_SOURCES fast_unwind/fast_unwind.cpp fast_unwind/fast_unwind_test.cpp)

enable_language(ASM)

IF (${ANDROID_ABI} MATCHES "x86")
    set(ASM_SOURCES libunwindstack/AsmGetRegsX86.S)
ELSEIF (${ANDROID_ABI} MATCHES "x86_64")
    set(ASM_SOURCES libunwindstack/AsmGetRegsX86_64.S)
ENDIF ()

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        ${TARGET}

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${BASE_SOURCES}
        ${DEXFILE_SUPPORT_SOURCES}
        ${BACKTRACE_SOURCES}
        ${FAST_UNWIND_SOURCES}
        ${ASM_SOURCES}
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        ${TARGET}

        kwai-android-base

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})