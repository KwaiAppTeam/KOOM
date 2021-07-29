package com.kwai.koom.base

abstract class MonitorConfig<M> {
  interface Builder<C: MonitorConfig<*>> {
    fun build(): C
  }
}