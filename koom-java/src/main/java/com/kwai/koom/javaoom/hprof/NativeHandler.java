/*
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
 * <p>
 * A jvm hprof dumper which use io hook to strip, primitive
 * array and Zygote/Image heap space will be stripped.
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.hprof;

import static com.kwai.koom.base.Monitor_SoKt.loadSoQuietly;

public class NativeHandler {
  private static boolean sSoLoaded;

  public static native boolean isARM64();

  public static boolean load() {
    if (!sSoLoaded) {
      return sSoLoaded = loadSoQuietly("koom-java");
    }

    return true;
  }

  static {
    load();
  }
}
