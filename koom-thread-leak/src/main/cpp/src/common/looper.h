//
// Created by shenguanchu on 1/12/21.
//

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