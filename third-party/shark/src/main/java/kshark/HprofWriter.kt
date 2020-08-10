package kshark

import kshark.GcRoot.*
import kshark.Hprof.HprofVersion
import kshark.HprofRecord.HeapDumpEndRecord
import kshark.HprofRecord.HeapDumpRecord.GcRootRecord
import kshark.HprofRecord.HeapDumpRecord.HeapDumpInfoRecord
import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.ClassDumpRecord
import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.InstanceDumpRecord
import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.ObjectArrayDumpRecord
import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.PrimitiveArrayDumpRecord
import kshark.HprofRecord.HeapDumpRecord.ObjectRecord.PrimitiveArrayDumpRecord.*
import kshark.HprofRecord.LoadClassRecord
import kshark.HprofRecord.StackTraceRecord
import kshark.HprofRecord.StringRecord
import kshark.HprofWriter.Companion.open
import kshark.PrimitiveType.*
import kshark.ValueHolder.*
import okio.Buffer
import okio.BufferedSink
import okio.Okio
import java.io.Closeable
import java.io.File

/**
 * Generates Hprof files.
 *
 * Call [open] to create an instance, [write] to add instances and [close] when you're done.
 */
@Suppress("TooManyFunctions")
class HprofWriter private constructor(
    private val sink: BufferedSink,
    val identifierByteSize: Int,
    val hprofVersion: HprofVersion
) : Closeable {

  private val workBuffer = Buffer()

  /**
   * Appends a [HprofRecord] to the heap dump. If [record] is a [HprofRecord.HeapDumpRecord] then
   * it will not be written to an in memory buffer and written to file only when the next a record
   * that is not a [HprofRecord.HeapDumpRecord] is written or when [close] is called.
   */
  fun write(record: HprofRecord) {
    sink.write(record)
  }

  /**
   * Helper method for creating a [ByteArray] for [InstanceDumpRecord.fieldValues] from a
   * list of [ValueHolder].
   */
  fun valuesToBytes(values: List<ValueHolder>): ByteArray {
    val valuesBuffer = Buffer()
    values.forEach { value ->
      valuesBuffer.writeValue(value)
    }
    return valuesBuffer.readByteArray()
  }

  /**
   * Flushes to disk all [HprofRecord.HeapDumpRecord] that are currently written to the in memory
   * buffer, then closes the file.
   */
  override fun close() {
    sink.flushHeapBuffer()
    sink.close()
  }

  private fun BufferedSink.writeValue(wrapper: ValueHolder) {
    when (wrapper) {
      is ReferenceHolder -> writeId(wrapper.value)
      is BooleanHolder -> writeBoolean(wrapper.value)
      is CharHolder -> write(charArrayOf(wrapper.value))
      is FloatHolder -> writeFloat(wrapper.value)
      is DoubleHolder -> writeDouble(wrapper.value)
      is ByteHolder -> writeByte(wrapper.value.toInt())
      is ShortHolder -> writeShort(wrapper.value.toInt())
      is IntHolder -> writeInt(wrapper.value)
      is LongHolder -> writeLong(wrapper.value)
    }
  }

  @Suppress("LongMethod")
  private fun BufferedSink.write(record: HprofRecord) {
    when (record) {
      is StringRecord -> {
        writeNonHeapRecord(HprofReader.STRING_IN_UTF8) {
          writeId(record.id)
          writeUtf8(record.string)
        }
      }
      is LoadClassRecord -> {
        writeNonHeapRecord(HprofReader.LOAD_CLASS) {
          writeInt(record.classSerialNumber)
          writeId(record.id)
          writeInt(record.stackTraceSerialNumber)
          writeId(record.classNameStringId)
        }
      }
      is StackTraceRecord -> {
        writeNonHeapRecord(HprofReader.STACK_TRACE) {
          writeInt(record.stackTraceSerialNumber)
          writeInt(record.threadSerialNumber)
          writeInt(record.stackFrameIds.size)
          writeIdArray(record.stackFrameIds)
        }
      }
      is GcRootRecord -> {
        with(workBuffer) {
          when (val gcRoot = record.gcRoot) {
            is Unknown -> {
              writeByte(HprofReader.ROOT_UNKNOWN)
              writeId(gcRoot.id)
            }
            is JniGlobal -> {
              writeByte(
                  HprofReader.ROOT_JNI_GLOBAL
              )
              writeId(gcRoot.id)
              writeId(gcRoot.jniGlobalRefId)
            }
            is JniLocal -> {
              writeByte(HprofReader.ROOT_JNI_LOCAL)
              writeId(gcRoot.id)
              writeInt(gcRoot.threadSerialNumber)
              writeInt(gcRoot.frameNumber)
            }
            is JavaFrame -> {
              writeByte(HprofReader.ROOT_JAVA_FRAME)
              writeId(gcRoot.id)
              writeInt(gcRoot.threadSerialNumber)
              writeInt(gcRoot.frameNumber)
            }
            is NativeStack -> {
              writeByte(HprofReader.ROOT_NATIVE_STACK)
              writeId(gcRoot.id)
              writeInt(gcRoot.threadSerialNumber)
            }
            is StickyClass -> {
              writeByte(HprofReader.ROOT_STICKY_CLASS)
              writeId(gcRoot.id)
            }
            is ThreadBlock -> {
              writeByte(HprofReader.ROOT_THREAD_BLOCK)
              writeId(gcRoot.id)
              writeInt(gcRoot.threadSerialNumber)
            }
            is MonitorUsed -> {
              writeByte(HprofReader.ROOT_MONITOR_USED)
              writeId(gcRoot.id)
            }
            is ThreadObject -> {
              writeByte(HprofReader.ROOT_THREAD_OBJECT)
              writeId(gcRoot.id)
              writeInt(gcRoot.threadSerialNumber)
              writeInt(gcRoot.stackTraceSerialNumber)
            }
            is ReferenceCleanup -> {
              writeByte(HprofReader.ROOT_REFERENCE_CLEANUP)
              writeId(gcRoot.id)
            }
            is VmInternal -> {
              writeByte(HprofReader.ROOT_VM_INTERNAL)
              writeId(gcRoot.id)
            }
            is JniMonitor -> {
              writeByte(HprofReader.ROOT_JNI_MONITOR)
              writeId(gcRoot.id)
              writeInt(gcRoot.stackTraceSerialNumber)
              writeInt(gcRoot.stackDepth)
            }
            is InternedString -> {
              writeByte(HprofReader.ROOT_INTERNED_STRING)
              writeId(gcRoot.id)
            }
            is Finalizing -> {
              writeByte(HprofReader.ROOT_FINALIZING)
              writeId(gcRoot.id)
            }
            is Debugger -> {
              writeByte(HprofReader.ROOT_DEBUGGER)
              writeId(gcRoot.id)
            }
            is Unreachable -> {
              writeByte(HprofReader.ROOT_UNREACHABLE)
              writeId(gcRoot.id)
            }
          }
        }
      }
      is ClassDumpRecord -> {
        with(workBuffer) {
          writeByte(HprofReader.CLASS_DUMP)
          writeId(record.id)
          writeInt(record.stackTraceSerialNumber)
          writeId(record.superclassId)
          writeId(record.classLoaderId)
          writeId(record.signersId)
          writeId(record.protectionDomainId)
          // reserved
          writeId(0)
          // reserved
          writeId(0)
          writeInt(record.instanceSize)
          // Not writing anything in the constant pool
          val constantPoolCount = 0
          writeShort(constantPoolCount)
          writeShort(record.staticFields.size)
          record.staticFields.forEach { field ->
            writeId(field.nameStringId)
            writeByte(field.type)
            writeValue(field.value)
          }
          writeShort(record.fields.size)
          record.fields.forEach { field ->
            writeId(field.nameStringId)
            writeByte(field.type)
          }
        }
      }
      is InstanceDumpRecord -> {
        with(workBuffer) {
          writeByte(HprofReader.INSTANCE_DUMP)
          writeId(record.id)
          writeInt(record.stackTraceSerialNumber)
          writeId(record.classId)
          writeInt(record.fieldValues.size)
          write(record.fieldValues)
        }
      }
      is ObjectArrayDumpRecord -> {
        with(workBuffer) {
          writeByte(HprofReader.OBJECT_ARRAY_DUMP)
          writeId(record.id)
          writeInt(record.stackTraceSerialNumber)
          writeInt(record.elementIds.size)
          writeId(record.arrayClassId)
          writeIdArray(record.elementIds)
        }
      }
      is PrimitiveArrayDumpRecord -> {
        with(workBuffer) {
          writeByte(HprofReader.PRIMITIVE_ARRAY_DUMP)
          writeId(record.id)
          writeInt(record.stackTraceSerialNumber)

          when (record) {
            is BooleanArrayDump -> {
              writeInt(record.array.size)
              writeByte(BOOLEAN.hprofType)
              write(record.array)
            }
            is CharArrayDump -> {
              writeInt(record.array.size)
              writeByte(CHAR.hprofType)
              write(record.array)
            }
            is FloatArrayDump -> {
              writeInt(record.array.size)
              writeByte(FLOAT.hprofType)
              write(record.array)
            }
            is DoubleArrayDump -> {
              writeInt(record.array.size)
              writeByte(DOUBLE.hprofType)
              write(record.array)
            }
            is ByteArrayDump -> {
              writeInt(record.array.size)
              writeByte(BYTE.hprofType)
              write(record.array)
            }
            is ShortArrayDump -> {
              writeInt(record.array.size)
              writeByte(SHORT.hprofType)
              write(record.array)
            }
            is IntArrayDump -> {
              writeInt(record.array.size)
              writeByte(INT.hprofType)
              write(record.array)
            }
            is LongArrayDump -> {
              writeInt(record.array.size)
              writeByte(LONG.hprofType)
              write(record.array)
            }
          }
        }
      }
      is HeapDumpInfoRecord -> {
        with(workBuffer) {
          writeByte(HprofReader.HEAP_DUMP_INFO)
          writeInt(record.heapId)
          writeId(record.heapNameStringId)
        }
      }
      is HeapDumpEndRecord -> {
        throw IllegalArgumentException("HprofWriter automatically emits HeapDumpEndRecord")
      }
    }
  }

  private fun BufferedSink.writeDouble(value: Double) {
    writeLong(value.toBits())
  }

  private fun BufferedSink.writeFloat(value: Float) {
    writeInt(value.toBits())
  }

  private fun BufferedSink.writeBoolean(value: Boolean) {
    writeByte(if (value) 1 else 0)
  }

  private fun BufferedSink.writeIdArray(array: LongArray) {
    array.forEach { writeId(it) }
  }

  private fun BufferedSink.write(array: BooleanArray) {
    array.forEach { writeByte(if (it) 1 else 0) }
  }

  private fun BufferedSink.write(array: CharArray) {
    writeString(String(array), Charsets.UTF_16BE)
  }

  private fun BufferedSink.write(array: FloatArray) {
    array.forEach { writeFloat(it) }
  }

  private fun BufferedSink.write(array: DoubleArray) {
    array.forEach { writeDouble(it) }
  }

  private fun BufferedSink.write(array: ShortArray) {
    array.forEach { writeShort(it.toInt()) }
  }

  private fun BufferedSink.write(array: IntArray) {
    array.forEach { writeInt(it) }
  }

  private fun BufferedSink.write(array: LongArray) {
    array.forEach { writeLong(it) }
  }

  private fun BufferedSink.writeNonHeapRecord(
      tag: Int,
      block: BufferedSink.() -> Unit
  ) {
    flushHeapBuffer()
    workBuffer.block()
    writeTagHeader(tag, workBuffer.size())
    writeAll(workBuffer)
  }

  private fun BufferedSink.flushHeapBuffer() {
    if (workBuffer.size() > 0) {
      writeTagHeader(HprofReader.HEAP_DUMP, workBuffer.size())
      writeAll(workBuffer)
      writeTagHeader(HprofReader.HEAP_DUMP_END, 0)
    }
  }

  private fun BufferedSink.writeTagHeader(
      tag: Int,
      length: Long
  ) {
    writeByte(tag)
    // number of microseconds since the time stamp in the header
    writeInt(0)
    writeInt(length.toInt())
  }

  private fun BufferedSink.writeId(id: Long) {
    when (identifierByteSize) {
      1 -> writeByte(id.toInt())
      2 -> writeShort(id.toInt())
      4 -> writeInt(id.toInt())
      8 -> writeLong(id)
      else -> throw IllegalArgumentException("ID Length must be 1, 2, 4, or 8")
    }
  }

  companion object {
    /**
     * Opens a new file for writing hprof records. Don't forget to call [close] once done.
     */
    fun open(
        hprofFile: File,
        /**
         * Size of Hprof identifiers. Identifiers are used to represent UTF8 strings, objects,
         * stack traces, etc. They can have the same size as host pointers or sizeof(void*), but are
         * not required to be.
         */
        identifierByteSize: Int = 4,
        /** Version of the opened hprof, which is tied to the runtime where the heap was dumped. */
        hprofVersion: HprofVersion = HprofVersion.ANDROID
    ): HprofWriter {
      val sink = Okio.buffer(Okio.sink(hprofFile.outputStream()))
      sink.writeUtf8(hprofVersion.versionString)
      sink.writeByte(0)
      sink.writeInt(identifierByteSize)
      val heapDumpTimestamp = System.currentTimeMillis()
      sink.writeLong(heapDumpTimestamp)
      return HprofWriter(sink, identifierByteSize, hprofVersion)
    }
  }
}