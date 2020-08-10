# keep fork dump related jni function,   must needed
-keep class com.kwai.koom.javaoom.dump.ForkJvmHeapDumper {*;}

# keep heap report json format,          must needed
-keep class com.kwai.koom.javaoom.report.HeapReport { *; }
-keep class com.kwai.koom.javaoom.report.HeapReport$* { *; }
-keep class com.kwai.koom.javaoom.report.HeapReport$*$* { *; }
