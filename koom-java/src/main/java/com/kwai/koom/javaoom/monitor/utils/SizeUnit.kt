/**
 * Copyright 2020 Kwai, Inc. All rights reserved.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.monitor.utils

sealed class SizeUnit {
  abstract fun toByte(value: Long): Float
  abstract fun toKB(value: Long): Float
  abstract fun toMB(value: Long): Float

  abstract fun toByte(value: Int): Float
  abstract fun toKB(value: Int): Float
  abstract fun toMB(value: Int): Float

  object BYTE : SizeUnit() {
    override fun toByte(value: Long) = value.toFloat()

    override fun toKB(value: Long) = value / 1024.0f

    override fun toMB(value: Long) = value / 1024.0f / 1024.0f

    override fun toByte(value: Int) = value.toFloat()

    override fun toKB(value: Int) = value / 1024.0f

    override fun toMB(value: Int) = value / 1024.0f / 1024.0f
  }

  object KB : SizeUnit() {
    override fun toByte(value: Long) = value * 1024.0f

    override fun toKB(value: Long) = value.toFloat()

    override fun toMB(value: Long) = value / 1024.0f

    override fun toByte(value: Int) = value * 1024.0f

    override fun toKB(value: Int) = value.toFloat()

    override fun toMB(value: Int) = value / 1024.0f
  }

  object MB : SizeUnit() {
    override fun toByte(value: Long) = value * 1024.0f * 1024.0f

    override fun toKB(value: Long) = value * 1024.0f

    override fun toMB(value: Long) = value.toFloat()

    override fun toByte(value: Int) = value * 1024.0f * 1024.0f

    override fun toKB(value: Int) = value * 1024.0f

    override fun toMB(value: Int) = value.toFloat()
  }
}