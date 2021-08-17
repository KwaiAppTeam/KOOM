package kshark

import kshark.LeakTraceElement.Type.ARRAY_ENTRY
import kshark.LeakTraceElement.Type.INSTANCE_FIELD
import kshark.LeakTraceElement.Type.LOCAL
import kshark.LeakTraceElement.Type.STATIC_FIELD
import java.io.Serializable

/**
 * This class is kept to support backward compatible deserialization.
 */
internal class LeakReference : Serializable {

  private val type: LeakTraceElement.Type? = null
  private val name: String? = null

  fun fromV20(originObject: LeakTraceObject) = LeakTraceReference(
    originObject = originObject,
    referenceType = when (type!!) {
      INSTANCE_FIELD -> LeakTraceReference.ReferenceType.INSTANCE_FIELD
      STATIC_FIELD -> LeakTraceReference.ReferenceType.STATIC_FIELD
      LOCAL -> LeakTraceReference.ReferenceType.LOCAL
      ARRAY_ENTRY -> LeakTraceReference.ReferenceType.ARRAY_ENTRY
    },
    owningClassName = originObject.className,
    referenceName = name!!
  )

  companion object {
    private const val serialVersionUID: Long = 2028550902155599651
  }
}