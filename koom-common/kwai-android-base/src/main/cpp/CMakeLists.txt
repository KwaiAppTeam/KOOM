# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

set(TARGET kwai-android-base)
project(${TARGET})

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

add_compile_options(-Wall -Wextra -Wexit-time-destructors -D_FILE_OFFSET_BITS=64 -D_7ZIP_ST)
#liblog
add_compile_options(-DLIBLOG_LOG_TAG=1006 -DSNET_EVENT_LOG_TAG=1397638484 -Oz)

include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/liblog/include
        ${CMAKE_SOURCE_DIR}/libcutils/include
        ${CMAKE_SOURCE_DIR}/libutils/include
        ${CMAKE_SOURCE_DIR}/libsystem/include
        ${CMAKE_SOURCE_DIR}/lzma
)

add_subdirectory(fmtlib)

set(BASE_SOURCES abi_compatibility.cpp chrono_utils.cpp cmsg.cpp file.cpp liblog_symbols.cpp
        logging.cpp mapped_file.cpp parsebool.cpp parsenetaddress.cpp process.cpp properties.cpp
        stringprintf.cpp strings.cpp threads.cpp test_utils.cpp)

file(GLOB PROC_INFO_SOURCES procinfo/*.cpp)

file(GLOB ASYNC_SAFE_SOURCES async_safe/*.cpp)

set(LZMA_SOURCES lzma/7zAlloc.c lzma/7zArcIn.c lzma/7zBuf2.c lzma/7zBuf.c lzma/7zCrc.c lzma/7zCrcOpt.c
        lzma/7zDec.c lzma/7zFile.c lzma/7zStream.c lzma/Aes.c lzma/AesOpt.c lzma/Alloc.c lzma/Bcj2.c
        lzma/Bra86.c lzma/Bra.c lzma/BraIA64.c lzma/CpuArch.c lzma/Delta.c lzma/LzFind.c
        lzma/Lzma2Dec.c lzma/Lzma2Enc.c lzma/Lzma86Dec.c lzma/Lzma86Enc.c lzma/LzmaDec.c lzma/LzmaEnc.c
        lzma/LzmaLib.c lzma/Ppmd7.c lzma/Ppmd7Dec.c lzma/Ppmd7Enc.c lzma/Sha256.c lzma/Sort.c
        lzma/Xz.c lzma/XzCrc64.c lzma/XzCrc64Opt.c lzma/XzDec.c lzma/XzEnc.c lzma/XzIn.c)

set(BIONIC_SOURCES bionic/ndk_port.cpp bionic/system_property/prop_info.cpp)

set(LOG_SOURCES liblog/event_tag_map.cpp liblog/log_event_list.cpp liblog/log_event_write.cpp
        liblog/log_time.cpp liblog/logd_reader.cpp liblog/logd_writer.cpp liblog/logger_name.cpp
        liblog/logger_read.cpp liblog/logger_write.cpp liblog/logprint.cpp liblog/pmsg_reader.cpp
        liblog/pmsg_writer.cpp liblog/properties.cpp)

set(KWAI_LINKER_SOURCES kwai_linker/kwai_dlfcn.cpp kwai_linker/elf_reader.cpp)

set(KWAI_UTIL_SOURCES kwai_util/ktime.cpp)

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
        ${PROC_INFO_SOURCES}
        ${ASYNC_SAFE_SOURCES}
        ${LZMA_SOURCES}
        ${BIONIC_SOURCES}
        ${LOG_SOURCES}
        ${KWAI_LINKER_SOURCES}
        ${KWAI_UTIL_SOURCES}
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

        fmt

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})
