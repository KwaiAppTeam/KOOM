package kshark

import kshark.LeakTraceReference.ReferenceType.ARRAY_ENTRY
import kshark.LeakTraceReference.ReferenceType.INSTANCE_FIELD
import kshark.LeakTraceReference.ReferenceType.LOCAL
import kshark.LeakTraceReference.ReferenceType.STATIC_FIELD
import java.io.Serializable

/**
 * A [LeakTraceReference] represents and origin [LeakTraceObject] and either a reference from that
 * object to the [LeakTraceObject] in the next [LeakTraceReference] in [LeakTrace.referencePath],
 * or to [LeakTrace.leakingObject] if this is the last [LeakTraceReference] in
 * [LeakTrace.referencePath].
 */
data class LeakTraceReference(
    val originObject: LeakTraceObject,

    val referenceType: ReferenceType,

    val referenceName: String,

    //Added by Kwai, Inc
    //field's declared class name, which can be ancestor class
    // when field are inherited from ancestor's class.
    val declaredClassName: String
) : Serializable {

  enum class ReferenceType {
    INSTANCE_FIELD,
    STATIC_FIELD,
    LOCAL,
    ARRAY_ENTRY
  }

  val referenceDisplayName: String
    get() {
      return when (referenceType) {
        ARRAY_ENTRY -> "[$referenceName]"
        STATIC_FIELD, INSTANCE_FIELD -> referenceName
        LOCAL -> "<Java Local>"
      }
    }

  val referenceGenericName: String
    get() {
      return when (referenceType) {
        // The specific array index in a leak rarely matters, this improves grouping.
        ARRAY_ENTRY -> "[x]"
        STATIC_FIELD, INSTANCE_FIELD -> referenceName
        LOCAL -> "<Java Local>"
      }
    }

  companion object {
    private const val serialVersionUID = 1L
  }

}