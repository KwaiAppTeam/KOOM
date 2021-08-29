/*
 * Copyright (c) 2021. Kwai, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Created by shenvsv on 2021.
 *
 */

#include <pthread.h>
#include <semaphore.h>
struct LooperMessage;
class looper {
 public:
  looper();
  ~looper();
  virtual void post(int what, void *data, bool flush = false);
  void quit();
  virtual void handle(int what, void *data);

 private:
  void addMsg(LooperMessage *msg, bool flush);
  static void *trampoline(void *p);
  void loop();
  LooperMessage *head = nullptr;
  LooperMessage *tail = nullptr;
  pthread_t worker;
  sem_t headWriteProtect;
  sem_t headDataAvailable;
  bool running;
};