package kshark.internal

import kshark.PrimitiveType

internal sealed class IndexedObject {
  abstract val position: Long

  class IndexedClass(
      override val position: Long,
      val superclassId: Long,
      val instanceSize: Int
  ) : IndexedObject()

  class IndexedInstance(
      override val position: Long,
      val classId: Long
  ) : IndexedObject()

  class IndexedObjectArray(
      override val position: Long,
      val arrayClassId: Long,
      val size: Int
  ) : IndexedObject()

  class IndexedPrimitiveArray(
      override val position: Long,
      primitiveType: PrimitiveType,
      val size: Int
  ) : IndexedObject() {
    private val primitiveTypeOrdinal: Byte = primitiveType.ordinal.toByte()
    val primitiveType: PrimitiveType
      get() = PrimitiveType.values()[primitiveTypeOrdinal.toInt()]
  }

}