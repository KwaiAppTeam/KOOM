# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.6)

set(TARGET koom-native)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../koom-common/third-party)
set(KWAI_ANDROID_BASE_DIR ${CMAKE_SOURCE_DIR}/../../../../koom-common/kwai-android-base)

project(${TARGET})

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${KWAI_ANDROID_BASE_DIR}/src/main/cpp/include/
        ${KWAI_ANDROID_BASE_DIR}/src/main/cpp/liblog/include/
        ${THIRD_PARTY_DIR}/xhook/src/main/cpp/xhook/src/
        )

link_directories(
        ${KWAI_ANDROID_BASE_DIR}/src/main/libs/${ANDROID_ABI}/
        ${THIRD_PARTY_DIR}/xhook/src/main/libs/${ANDROID_ABI}/
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        ${TARGET}

        SHARED

        src/memory_map.cpp
        src/jni_leak_monitor.cpp
        src/leak_monitor.cpp
        src/memory_analyzer.cpp
        src/utils/hook_helper.cpp
        src/utils/stack_trace.cpp
        )

find_library( # Sets the name of the path variable.
        log-lib
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        ${TARGET}
        xhook_lib
        kwai-android-base
        ${log-lib})