#ifndef APM_THREAD_H
#define APM_THREAD_H

#include "common/item.h"

namespace koom {

class ThreadItem : public ResItemData {
 public:
  std::string collect_mode{};

  bool thread_detached{};

  long long startTime{};

  long long exitTime{};

  bool thread_reported{};

  pthread_t thread_internal_id{};

  std::string name{};

  ThreadItem();

  ThreadItem(const ThreadItem &threadItem);

  void Clear();
};

#endif //APM_THREAD_H
}