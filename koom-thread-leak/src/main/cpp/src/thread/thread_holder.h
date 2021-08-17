//
// Created by lirui on 2020/10/27.
//

#ifndef APM_RESOURCEDATA_H
#define APM_RESOURCEDATA_H

#include "common/log.h"
#include "common/util.h"
#include "common/callstack.h"
#include "thread_item.h"
#include <map>
#include "rapidjson/writer.h"

namespace koom {

class ThreadHolder {
 public:
  void AddThread(int tid, pthread_t pthread, bool isThreadDetached,
                 int64_t allocateTime, long long startTime, const std::string &callstack,
                 uintptr_t (&pc)[koom::Constant::max_call_stack_depth]);
  void JoinThread(pthread_t threadId);
  void ExitThread(pthread_t threadId, std::string& threadName, long long int i);
  void DetachThread(pthread_t threadId);
  void ReportThreadLeak(long long time);

 private:
  std::map<pthread_t, ThreadItem> leakThreadMap;
  std::map<pthread_t, ThreadItem> threadMap;
  void WriteThreadJson(rapidjson::Writer<rapidjson::StringBuffer> &writer, ThreadItem &thread_item);
  void Clear() {
    leakThreadMap.clear();
    threadMap.clear();
  }
};
}
#endif //APM_RESOURCEDATA_H
