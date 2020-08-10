package kshark.internal

import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.ClassDumpRecord.FieldRecord
import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.InstanceDumpRecord
import kshark.PrimitiveType
import kshark.PrimitiveType.*
import kshark.ValueHolder
import kshark.ValueHolder.BooleanHolder
import kshark.ValueHolder.IntHolder
import kshark.ValueHolder.LongHolder
import kshark.ValueHolder.ReferenceHolder

internal class FieldValuesReader(
    private val record: InstanceDumpRecord,
    private val identifierByteSize: Int
) {

  private var position = 0

  //Added by Kwai, Inc.
  //Strip primitive type to improve performance.
  fun readValue(field: FieldRecord): ValueHolder? {
    return when (field.type) {
      PrimitiveType.REFERENCE_HPROF_TYPE -> ReferenceHolder(readId())
      BOOLEAN_TYPE -> {
        BooleanHolder(readBoolean())
      }
      CHAR_TYPE -> {
        readChar()
        null
      }
      FLOAT_TYPE -> {
        readFloat()
        null
      }
      DOUBLE_TYPE -> {
        readDouble()
        null
      }
      BYTE_TYPE -> {
        readByte()
        null
      }
      SHORT_TYPE -> {
        readShort()
        null
      }
      INT_TYPE -> {
        IntHolder(readInt())
      }
      LONG_TYPE -> {
        LongHolder(readLong())
      }
      else -> throw IllegalStateException("Unknown type ${field.type}")
    }
  }

  private fun readId(): Long {
    // As long as we don't interpret IDs, reading signed values here is fine.
    return when (identifierByteSize) {
      //1 -> readByte().toLong()
      //2 -> readShort().toLong()
      4 -> readInt().toLong()
      8 -> readLong()
      else -> throw IllegalArgumentException("ID Length must be 1, 2, 4, or 8")
    }
  }

  private fun readBoolean(): Boolean {
    val value = record.fieldValues[position]
    position++
    return value != 0.toByte()
  }

  private fun readByte() {
    //val value = record.fieldValues[position]
    position++
    //return value
  }

  private fun readInt(): Int {
    val value = record.fieldValues.readInt(position)
    position += 4
    return value
  }

  private fun readShort() {
    //val value = record.fieldValues.readShort(position)
    position += 2
    //return value
  }

  private fun readLong(): Long {
    val value = record.fieldValues.readLong(position)
    position += 8
    return value
  }

  private fun readFloat() {
    position += 4
    //return Float.fromBits(readInt())
  }

  private fun readDouble() {
    position += 8
    //return Double.fromBits(readLong())
  }

  private fun readChar() {
    //val string = String(record.fieldValues, position, 2, Charsets.UTF_16BE)
    position += 2
    //return string[0]
  }

  companion object {
    private val BOOLEAN_TYPE = BOOLEAN.hprofType
    private val CHAR_TYPE = CHAR.hprofType
    private val FLOAT_TYPE = FLOAT.hprofType
    private val DOUBLE_TYPE = DOUBLE.hprofType
    private val BYTE_TYPE = BYTE.hprofType
    private val SHORT_TYPE = SHORT.hprofType
    private val INT_TYPE = INT.hprofType
    private val LONG_TYPE = LONG.hprofType
  }

}