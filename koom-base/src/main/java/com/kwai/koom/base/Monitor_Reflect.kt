@file:Suppress("UNCHECKED_CAST")

package com.kwai.koom.base

import java.lang.reflect.Field
import java.lang.reflect.Method

fun String.toClass(): Class<*>? {
  return runCatching { Class.forName(this) }.getOrNull()
}

fun <T> Class<*>.getStaticFiledValue(filedName: String): T? {
  return runCatching {
    return@runCatching getFiledQuietly(filedName)
        ?.get(null) as T?
  }.getOrNull()
}

fun <T> Any.getFiledValue(filedName: String): T? {
  return runCatching {
    return@runCatching this::class.java.getFiledQuietly(filedName)
        ?.get(this) as T?
  }.getOrNull()
}

fun Class<*>.setStaticFiledValue(filedName: String, filedValue: Any?) {
  runCatching {
    return@runCatching getFiledQuietly(filedName)
        ?.set(null, filedValue)
  }
}

fun Any.setFiledValue(filedName: String, filedValue: Any?) {
  runCatching {
    return@runCatching this::class.java.getFiledQuietly(filedName)
        ?.set(this, filedValue)
  }
}

fun Class<*>.getFiledQuietly(filedName: String): Field? {
  return runCatching {
    var targetClass: Class<*>? = this

    while (targetClass != Any::class.java) {
      val targetField = runCatching { targetClass?.getDeclaredField(filedName) }
          .getOrNull()

      if (targetField != null) {
        return@runCatching targetField.also { it.isAccessible = true }
      } else {
        targetClass = targetClass?.superclass
      }
    }

    return@runCatching null
  }.getOrNull()
}

fun <T> Any.callMethod(
    methodName: String,
    parameterTypes: Array<Class<*>>? = null,
    args: Array<Any>? = null
): T? {
  return runCatching {
    val method = this::class.java.getDeclaredMethodQuietly(methodName, parameterTypes)
        ?: return@runCatching null

    return@runCatching if (args == null) {
      method.invoke(this)
    } else {
      method.invoke(this, *args)
    } as T?
  }.getOrNull()
}

fun <T> Class<*>.callStaticMethod(
    methodName: String,
    parameterTypes: Array<Class<*>>? = null,
    args: Array<Any>? = null
): T? {
  return runCatching {
    val method = this.getDeclaredMethodQuietly(methodName, parameterTypes)
        ?: return@runCatching null

    return@runCatching if (args == null) {
      method.invoke(null)
    } else {
      method.invoke(null, *args)
    } as T?
  }.getOrNull()
}

fun Class<*>.getDeclaredMethodQuietly(
    filedName: String,
    parameterTypes: Array<Class<*>>? = null
): Method? {
  return runCatching {
    var targetClass: Class<*>? = this
    while (targetClass != Any::class.java) {
      val targetMethod = runCatching {
        if (parameterTypes == null) {
          targetClass?.getDeclaredMethod(filedName)
        } else {
          targetClass?.getDeclaredMethod(filedName, *parameterTypes)
        }
      }.getOrNull()

      if (targetMethod != null) {
        return@runCatching targetMethod.also { it.isAccessible = true }
      } else {
        targetClass = targetClass?.superclass
      }
    }

    return@runCatching null
  }.getOrNull()
}