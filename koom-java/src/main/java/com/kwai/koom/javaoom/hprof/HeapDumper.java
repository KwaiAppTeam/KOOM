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
 * Hprof dumper
 *
 * @author Rui Li <lirui05@kuaishou.com>
 */

package com.kwai.koom.javaoom.hprof;

public abstract class HeapDumper {

  protected final boolean soLoaded;

  public HeapDumper() {
    soLoaded = NativeHandler.load();
  }

  /**
   * dump may cost several seconds, make sure called in a separated thread.
   *
   * @param path dump file
   * @return dump result success or not
   */
  public abstract boolean dump(String path);
}
