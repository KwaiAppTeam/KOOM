# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

set(TARGET koom-thread)
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../koom-common/third-party)
set(KWAI_ANDROID_BASE_DIR ${CMAKE_SOURCE_DIR}/../../../../koom-common/kwai-android-base)
set(KWAI_UNWIND_DIR ${CMAKE_SOURCE_DIR}/../../../../koom-common/kwai-unwind)

project(${TARGET})

add_compile_options(-Oz)

# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${KWAI_ANDROID_BASE_DIR}/src/main/cpp/include/
        ${KWAI_ANDROID_BASE_DIR}/src/main/cpp/liblog/include/
        ${THIRD_PARTY_DIR}/xhook/src/main/cpp/xhook/src/

        ${KWAI_UNWIND_DIR}/src/main/cpp/include
        ${KWAI_UNWIND_DIR}/src/main/cpp/libunwindstack/include
        ${KWAI_UNWIND_DIR}/src/main/cpp/libbacktrace/include

        ${CMAKE_SOURCE_DIR}/src
)

link_directories(
        ${KWAI_ANDROID_BASE_DIR}/src/main/libs/${ANDROID_ABI}/
        ${THIRD_PARTY_DIR}/xhook/src/main/libs/${ANDROID_ABI}/
        ${KWAI_UNWIND_DIR}/src/main/libs/${ANDROID_ABI}/
)


# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
add_library( # Sets the name of the library.
        ${TARGET}

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${CMAKE_SOURCE_DIR}/src/koom.cpp
        ${CMAKE_SOURCE_DIR}/src/jni_bridge.cpp
        ${CMAKE_SOURCE_DIR}/src/common/callstack.cpp
        ${CMAKE_SOURCE_DIR}/src/common/looper.cpp
        ${CMAKE_SOURCE_DIR}/src/thread/thread_item.cpp
        ${CMAKE_SOURCE_DIR}/src/thread/thread_holder.cpp
        ${CMAKE_SOURCE_DIR}/src/thread/thread_hook.cpp
        ${CMAKE_SOURCE_DIR}/src/thread/hook_looper.cpp
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

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
        kwai-unwind
        ${log-lib})