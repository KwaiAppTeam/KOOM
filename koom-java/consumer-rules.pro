# keep fork dump related jni function,   must needed
-keep class com.kwai.koom.javaoom.hprof.* {*;}

# keep heap report json format,          must needed
-keep class com.kwai.koom.javaoom.monitor.analysis.HeapReport { *; }
-keep class com.kwai.koom.javaoom.monitor.analysis.HeapReport$* { *; }
-keep class com.kwai.koom.javaoom.monitor.analysis.HeapReport$*$* { *; }
