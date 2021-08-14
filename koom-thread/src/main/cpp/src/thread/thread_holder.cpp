#include "thread_holder.h"
#include "thread_hook.h"
#include "koom.h"
#include <filesystem>
#include <regex>

namespace koom {

const char *holder_tag = "koom-holder";

void ThreadHolder::AddThread(int tid, pthread_t threadId, bool isThreadDetached,
                             int64_t allocateTime, long long startTime, const std::string
                             &callstack,
                             uintptr_t (&pc)[koom::Constant::max_call_stack_depth]) {
  bool valid = threadMap.count(threadId) > 0;
  if (valid) return;

  koom::Log::info(holder_tag, "AddThread tid:%d pthread_t:%p",
                  tid, threadId);

  std::string stack;
  stack.assign("##\n");
  try {
    //拼接native调用栈
    int ignoreLines = 0;
    for (int index = 0; index < koom::Constant::max_call_stack_depth; ++index) {
      uintptr_t p = pc[index];
      if (p == 0) continue;
      //koom::Log::info(holder_tag, "unwind native callstack #%d pc%p", index, p);
      std::string line = koom::CallStack::SymbolizePc(p, index - ignoreLines);
      if (line.empty()) { ignoreLines++; }
      else {
        line.append("\n");
        stack.append(line);
      }
    }
    //拼接java调用栈
    std::vector<std::string> splits = koom::Util::Split(callstack, '\n');
    for (const auto &split : splits) {
      if (split.empty()) continue;
      std::string line;
      line.append("#");
      line.append(split);
      line.append("\n");
      stack.append(line);
    }
    //空白堆栈，去掉##
    if (stack.size() == 3)
      stack.assign("");
  } catch (const std::bad_alloc &) {
    stack.assign("error:bad_alloc");
  }

  auto &item = threadMap[threadId];
  item.thread_internal_id = threadId;
  item.thread_detached = isThreadDetached;
  item.startTime = startTime;
  item.Update(allocateTime, stack, tid, pc);

  koom::Log::info(holder_tag, "AddThread finish");
}

void ThreadHolder::JoinThread(pthread_t threadId) {
  bool valid = threadMap.count(threadId) > 0;
  koom::Log::info(holder_tag,
                  "JoinThread tid:%p",
                  threadId);
  if (valid) {
    threadMap[threadId].thread_detached = true;
  } else {
    leakThreadMap.erase(threadId);
  }
}

void ThreadHolder::ExitThread(pthread_t threadId, std::string& threadName, long long int time) {
  bool valid = threadMap.count(threadId) > 0;
  if (!valid) return;
  auto &item = threadMap[threadId];
  koom::Log::info(holder_tag,
                  "ExitThread tid:%p name:%s",
                  threadId,
                  item.name.c_str());

  item.exitTime = time;
  item.name.assign(threadName);
  if (!item.thread_detached) {
    // 泄露了
    koom::Log::error(holder_tag, "Exited thread Leak! Not joined or detached!\n tid:%p",
                     threadId);
    leakThreadMap[threadId] = item;
  }
  threadMap.erase(threadId);
  koom::Log::info(holder_tag, "ExitThread finish");
}

void ThreadHolder::DetachThread(pthread_t threadId) {
  bool valid = threadMap.count(threadId) > 0;
  koom::Log::info(holder_tag,
                  "DetachThread tid:%p",
                  threadId);
  if (valid) {
    threadMap[threadId].thread_detached = true;
  } else {
    leakThreadMap.erase(threadId);
  }
}

void ThreadHolder::WriteThreadJson(rapidjson::Writer<rapidjson::StringBuffer> &writer,
                                   ThreadItem &thread_item) {
  //写入单个thread数据
  writer.StartObject();

  writer.Key("tid");
  writer.Uint(thread_item.id);

  writer.Key("interal_id");
  writer.Uint(thread_item.thread_internal_id);

  writer.Key("allocateTime");
  writer.Int64(thread_item.allocate_time);

  writer.Key("startTime");
  writer.Int64(thread_item.startTime);

  writer.Key("endTime");
  writer.Int64(thread_item.exitTime);

  writer.Key("name");
  writer.String(thread_item.name.c_str());

  // 这里先注释掉，确认一下是不是这里的转换有问题，是的话，再处理
  writer.Key("allocateCallStack");
  auto stack = thread_item.java_call_stack.c_str();
  writer.String(stack);

  writer.EndObject();
}

void ThreadHolder::ReportThreadLeak(long long time) {
  int needReport{};
  const char *type = "detach_leak";
  auto delay = threadLeakDelay * 1000000LL; // ms -> ns
  rapidjson::StringBuffer jsonBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(jsonBuf);
  writer.StartObject();

  writer.Key("leakType");
  writer.String(type);

  writer.Key("threads");
  writer.StartArray();
  for (auto &item : leakThreadMap) {
    if (item.second.exitTime + delay < time && !item.second.thread_reported) {
      koom::Log::info(holder_tag, "ReportThreadLeak %ld, %ld, %ld", item.second.exitTime, time, delay);
      needReport++;
      item.second.thread_reported = true;
      WriteThreadJson(writer, item.second);
    }
  }
  writer.EndArray();
  writer.EndObject();
  koom::Log::info(holder_tag, "ReportThreadLeak %d", needReport);
  if (needReport) {
    JavaCallback(Constant::CALL_BACK_TYPE_REPORT, type, jsonBuf.GetString());
  }
}
}