# LeakMonitor Introduction
Use Native memory leak problem for monitoring application, its core principle
- hook malloc/free and other memory allocator methods, used to record Native memory allocation metadata "size, stack, address, etc."
- Periodically use mark-and-sweep to analyze the Native Heap of the entire process and obtain the "address, size" of the unreachable memory block information
- Use the address, size, etc. of the unreachable memory block to obtain its allocation stack from the metadata we recorded, and produce leaked data "unreachable memory block address, size, allocation stack, etc."
# LeakMonitor Scope
- Above Android N (API level >= 24)
- Only support arm64-v8a

# LeakMonitor Getting Started
## Setup dependencies
## Code usage
- Initialize MonitorManager
- Initialize LeakMonitor
```java
......
LeakMonitorConfig config = new LeakMonitorConfig.Builder()
    .setLoopInterval(50000) // Set polling interval
    .setLeakItemThreshold(200) // The limit of collecting leaked native objects
    .setMonitorThreshold(16) // Set the threshold of the monitored memory block
    .setNativeHeapAllocatedThreshold(0) // Set the threshold of how much memory allocated by the native heap reaches to start monitoring
    .setSelectedSoList(new String[0]) // Set the monitor specific libraries, such as monitoring libcore.so, just write 'libcore'
    .setIgnoredSoList(new String[0]) // Set the libraries that you need to ignore monitoring
    .setLeakListener(leaks -> { }) // Set leak listener for receive leak records
    .build();
MonitorManager.addMonitorConfig(config);
......
```
- Start LeakMonitor for periodic monitoring
```java
......
LeakMonitor.INSTANCE.start()
......
```
- Stop LeakMonitor, Usually, don't need do this
```java
LeakMonitor.INSTANCE.stop()
```
- Take the initiative to obtain leaks, and receive leaked information in `LeakListener`; usually there is no need to actively check
```java
LeakMonitor.INSTANCE.checkLeaks()
```

# FAQ
- Why are devices below Android N not supported?
    - AOSP added the libmemunreachable module after Android N, "Of course, you can also extract it by yourself and test it in the APP."
    - Considering the commonality of memory leaks, monitoring only devices with Android N or higher should solve most problems
- Why not support armeabi-v7a?
    - When obtaining the memory allocation stack, we used FP Unwind (frame pointer unwind) for performance considerations; while armeabi-v7a uses ARM/Thumb mixed instructions, while FP Unwind is unreliable on the ARM/Thumb instruction mixed method stack
    - The ABI of AArch32 (Arm 32-bit Architecture) also does not specify the behavior of FP
- What is the performance overhead of LeakMonitor?
    - Due to the need to obtain and save metadata information such as the stack of the memory block, there is a certain overhead in performance. According to our test, the overall performance loss is <5%, which basically does not affect the user experience
    - For online use, it is strongly recommended to turn on "Sampling on devices with better performance"
- Are leaks detected 100% leaks?
    - According to the detection principle and experience, the accuracy of leak detection can basically reach 90%+, and there may be individual bad cases.
        - Inaccurate mark-and-sweep algorithm is used for detection, and reachability analysis is always performed according to 8 bytes during analysis, which is not 100% accurate
        - Due to the dirty memory of the memory allocator, some leaked memory may be reachable and affect the result