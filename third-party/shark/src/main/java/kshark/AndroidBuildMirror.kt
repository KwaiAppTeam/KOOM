package kshark

/**
 * Caches values from the android.os.Build class in the heap dump.
 * Retrieve a cached instances via [fromHeapGraph].
 */
class AndroidBuildMirror(
    /**
     * Value of android.os.Build.MANUFACTURER
     */
    //Added by Kwai, Inc.
    //Add default value, have compatibility with stripped hprof.
    val manufacturer: String = "Crop",
    /**
     * Value of android.os.Build.VERSION.SDK_INT
     */
    //Added by Kwai, Inc.
    //Add default value, have compatibility with stripped hprof.
    val sdkInt: Int = 21
) {
  companion object {
    /**
     * @see AndroidBuildMirror
     */
    fun fromHeapGraph(graph: HeapGraph): AndroidBuildMirror {
      return graph.context.getOrPut(AndroidBuildMirror::class.java.name) {
        val buildClass = graph.findClassByName("android.os.Build")!!
        val versionClass = graph.findClassByName("android.os.Build\$VERSION")!!
        val manufacturer = buildClass["MANUFACTURER"]!!.value
        if (manufacturer.isNonNullReference
            || manufacturer.readAsJavaString().isNullOrEmpty())
          AndroidBuildMirror()
        else {
          val sdkInt = versionClass["SDK_INT"]!!.value.asInt!!
          AndroidBuildMirror(manufacturer.readAsJavaString()!!, sdkInt)
        }
      }
    }
  }
}
