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

#include "looper.h"

#include <android/log.h>
#include <fcntl.h>
#include <jni.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>

#include "log.h"

#define TAG "koom-looper"
#define LOGV(...) koom::Log::info(TAG, __VA_ARGS__);

struct LooperMessage {
  int what;
  void *obj;
  LooperMessage *next;
  bool quit;
};
void *looper::trampoline(void *p) {
  prctl(PR_SET_NAME, "koom-looper");
  ((looper *)p)->loop();
  return nullptr;
}
looper::looper() {
  sem_init(&headDataAvailable, 0, 0);
  sem_init(&headWriteProtect, 0, 1);
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&worker, &attr, trampoline, this);
  running = true;
}
looper::~looper() {
  if (running) {
    LOGV(
        "Looper deleted while still running. Some messages will not be "
        "processed");
    quit();
  }
}
void looper::post(int what, void *data, bool flush) {
  auto *msg = new LooperMessage();
  msg->what = what;
  msg->obj = data;
  msg->next = nullptr;
  msg->quit = false;
  addMsg(msg, flush);
}
void looper::addMsg(LooperMessage *msg, bool flush) {
  sem_wait(&headWriteProtect);
  LooperMessage *h = head;
  if (flush) {
    while (h) {
      LooperMessage *next = h->next;
      delete h;
      h = next;
    }
    h = nullptr;
  }
  if (h != nullptr) {
    tail->next = msg;
    tail = msg;
  } else {
    head = msg;
    tail = msg;
  }
  sem_post(&headWriteProtect);
  sem_post(&headDataAvailable);
}
void looper::loop() {
  while (true) {
    // wait for available message
    sem_wait(&headDataAvailable);
    // get next available message
    sem_wait(&headWriteProtect);
    LooperMessage *msg = head;
    if (msg == nullptr) {
      LOGV("no msg");
      sem_post(&headWriteProtect);
      continue;
    }
    head = msg->next;
    sem_post(&headWriteProtect);
    if (msg->quit) {
      LOGV("quitting");
      delete msg;
      return;
    }
    LOGV("processing msg %d", msg->what);
    handle(msg->what, msg->obj);
    delete msg;
  }
}
void looper::quit() {
  LOGV("quit");
  auto *msg = new LooperMessage();
  msg->what = 0;
  msg->obj = nullptr;
  msg->next = nullptr;
  msg->quit = true;
  addMsg(msg, false);
  void *val;
  pthread_join(worker, &val);
  sem_destroy(&headDataAvailable);
  sem_destroy(&headWriteProtect);
  running = false;
}
void looper::handle(int what, void *obj) {
  LOGV("dropping msg %d %p", what, obj);
}
