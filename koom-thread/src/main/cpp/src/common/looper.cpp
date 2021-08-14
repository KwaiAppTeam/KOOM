//
// Created by shenguanchu on 1/12/21.
// 先这样写着，有时间再看下还有没有更高效的方案
//

#include "looper.h"
#include "log.h"
#include <cassert>
#include <jni.h>
#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <climits>
#include <semaphore.h>
#include <android/log.h>
#include <sys/prctl.h>

#define TAG "koom-looper"
#define LOGV(...) koom::Log::info(TAG, __VA_ARGS__);

struct LooperMessage {
  int what;
  void *obj;
  LooperMessage *next;
  bool quit;
};
void *looper::trampoline(void *p) {
  prctl(PR_SET_NAME,"koom-looper");
  ((looper *) p)->loop();
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
    LOGV("Looper deleted while still running. Some messages will not be processed");
    quit();
  }
}
void looper::post(int what, void *data, bool flush) {
  auto *msg = new LooperMessage();
  msg->what = what;
  msg->obj = data;
  msg->next = nullptr;
  msg->quit = false;
//  LOGV("post msg %d build msg finish", msg->what);
  addMsg(msg, flush);
}
void looper::addMsg(LooperMessage *msg, bool flush) {
  sem_wait(&headWriteProtect);
//  LOGV("post msg %d start", msg->what);
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
//  LOGV("post msg %d end", msg->what);
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
