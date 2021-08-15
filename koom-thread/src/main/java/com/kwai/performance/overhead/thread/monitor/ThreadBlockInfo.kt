package com.kwai.performance.overhead.thread.monitor

import com.google.gson.annotations.SerializedName

data class ThreadBlockInfo(val id: Long,
    val state: Thread.State,
    val name: String,
    val stack: Array<StackTraceElement>) {

  override fun equals(other: Any?): Boolean {
    if (this === other) return true
    if (javaClass != other?.javaClass) return false

    other as ThreadBlockInfo

    if (id != other.id) return false
    if (state != other.state) return false
    if (name != other.name) return false
    if (!stack.contentEquals(other.stack)) return false

    return true
  }

  override fun hashCode(): Int {
    var result = id.hashCode()
    result = 31 * result + state.hashCode()
    result = 31 * result + name.hashCode()
    result = 31 * result + stack.contentHashCode()
    return result
  }
}

data class BlockThreadData(@SerializedName("leakType") val leakType: String,
    @SerializedName("threads") val threads: List<ThreadBlockReport>)

data class ThreadBlockReport(@SerializedName("id") val id: Long,
    @SerializedName("state") val state: Thread.State,
    @SerializedName("name") val name: String,
    @SerializedName("stack") val stack: String,
    @SerializedName("count") val count: Long)