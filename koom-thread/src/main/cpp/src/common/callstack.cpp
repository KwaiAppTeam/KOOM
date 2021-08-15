#include "callstack.h"
#include "bionic/tls_defines.h"
#include "bionic/tls.h"

namespace koom {

const char *callstack_tag = "koom-callstack";

//静态变量初始化
pthread_key_t CallStack::pthread_key_self;
dump_java_stack_above_o_ptr CallStack::dump_java_stack_above_o;
dump_java_stack_ptr CallStack::dump_java_stack;

std::atomic<bool> CallStack::dumpingJava;
std::atomic<bool> CallStack::dumpingNative;
std::atomic<bool> CallStack::disableJava;
std::atomic<bool> CallStack::disableNative;

std::atomic<bool> CallStack::inSymbolize;

timespec CallStack::lastJavaTime;
timespec CallStack::lastNativeTime;

int CallStack::java_gap_count;
int CallStack::native_gap_count;

int CallStack::java_single_gap;
int CallStack::native_single_gap;
int CallStack::max_java_loop_count;
int CallStack::max_native_loop_count;

unwindstack::UnwinderFromPid *CallStack::unwinder;

void CallStack::SetJava(int gap, int loop) {
  java_single_gap = gap;
  max_java_loop_count = loop;
}

void CallStack::SetNative(int gap, int loop) {
  native_single_gap = gap;
  max_native_loop_count = loop;
}

void CallStack::Init() {
  java_single_gap = koom::Constant::java_callstack_single_gap;
  native_single_gap = koom::Constant::native_callstack_single_gap;
  //用作获取余数，必须非0
  max_java_loop_count = koom::Constant::java_callstack_loop_count;
  max_native_loop_count = koom::Constant::native_callstack_loop_count;

  if (koom::Util::AndroidApi() < __ANDROID_API_L__) {
    koom::Log::error(callstack_tag, "android api < __ANDROID_API_L__");
    return;
  }

  // 云os用的是libaoc.so，量比较少，不做兼容了
  void *handle = kwai::linker::DlFcn::dlopen("libart.so", RTLD_LAZY | RTLD_LOCAL);

  if (koom::Util::AndroidApi() >= __ANDROID_API_O__) {
    dump_java_stack_above_o = reinterpret_cast<dump_java_stack_above_o_ptr>(
        kwai::linker::DlFcn::dlsym(handle,
                                   "_ZNK3art6Thread13DumpJavaStackERNSt3__113basic_ostreamIcNS1_11char_"
                                   "traitsIcEEEEbb"));
    if (dump_java_stack_above_o == nullptr) {
      koom::Log::error(callstack_tag, "dump_java_stack_above_o is null");
    }
  } else if (koom::Util::AndroidApi() >= __ANDROID_API_L__) {
    dump_java_stack = reinterpret_cast<dump_java_stack_ptr>(
        kwai::linker::DlFcn::dlsym(handle,
                                   "_ZNK3art6Thread13DumpJavaStackERNSt3__113basic_ostreamIcNS1_11char_"
                                   "traitsIcEEEE"));
    if (dump_java_stack == nullptr) {
      koom::Log::error(callstack_tag, "dump_java_stack is null");
    }
  }

  if (koom::Util::AndroidApi() < __ANDROID_API_N__) {
    auto *pthread_key_self_art =
        (pthread_key_t *) kwai::linker::DlFcn::dlsym(handle,
                                                     "_ZN3art6Thread17pthread_key_self_E");
    if (pthread_key_self_art != nullptr) {
      pthread_key_self = reinterpret_cast<pthread_key_t>(*pthread_key_self_art);
    } else {
      koom::Log::error(callstack_tag, "pthread_key_self_art is null");
    }
  }

  kwai::linker::DlFcn::dlclose(handle);
}

void *CallStack::GetCurrentThread() {
  if (koom::Util::AndroidApi() >= __ANDROID_API_N__) {
    //koom::Log::info(callstack_tag, "GetCurrentThread __ANDROID_API_N__");
    return __get_tls()[TLS_SLOT_ART_THREAD_SELF];
  }
  if (koom::Util::AndroidApi() >= __ANDROID_API_L__) {
    //koom::Log::info(callstack_tag, "GetCurrentThread __ANDROID_API_L__");
    return pthread_getspecific(pthread_key_self);
  }
  koom::Log::info(callstack_tag, "GetCurrentThread return");
  return nullptr;
}

void CallStack::JavaStackTrace(void *thread, std::ostream &os) {
  if (dumpingJava.load() || disableJava.load()
      || !MeetFrequencyLimit(java)) {
    os << "no java stack when dumping";
    return;
  }
  dumpingJava = true;
  if (koom::Util::AndroidApi() >= __ANDROID_API_O__) {
    //不dump locks，有稳定性问题
    dump_java_stack_above_o(thread, os, true, false);
  } else if (koom::Util::AndroidApi() >= __ANDROID_API_L__) {
    dump_java_stack(thread, os);
  }
  lastJavaTime = Util::CurrentClockTime();

  dumpingJava = false;
}

uintptr_t get_thread_stack_top() {
  pthread_attr_t attr;
  pthread_getattr_np(pthread_self(), &attr);
  return (uintptr_t) (attr.stack_size + static_cast<char *>(attr.stack_base));
}

size_t CallStack::FastUnwind(uintptr_t *buf, size_t num_entries) {
  if (dumpingNative.load() || disableNative.load()
      || !MeetFrequencyLimit(native))
    return 0;
  dumpingNative = true;
  struct frame_record {
    uintptr_t next_frame, return_addr;
  };

  auto begin = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
  auto end = get_thread_stack_top();
  // 这里是做什么的，没有概念
  stack_t ss;
  if (sigaltstack(nullptr, &ss) == 0 && (ss.ss_flags & SS_ONSTACK)) {
    end = reinterpret_cast<uintptr_t>(ss.ss_sp) + ss.ss_size;
  }
  size_t num_frames = 0;
  while (true) {
    auto *frame = reinterpret_cast<frame_record *>(begin);
    if (num_frames < num_entries) {
      buf[num_frames] = frame->return_addr;
    }
    ++num_frames;
    if (frame->next_frame < begin + sizeof(frame_record) || frame->next_frame >= end ||
        frame->next_frame % sizeof(void *) != 0) {
      break;
    }
    begin = frame->next_frame;
    if (num_frames > 16) {
      break;
    }
  }

  lastNativeTime = Util::CurrentClockTime();

  dumpingNative = false;
  return num_frames;
}

std::string CallStack::SymbolizePc(uintptr_t pc, int index) {
  if (inSymbolize.load()) return "";
  inSymbolize = true;

  if (unwinder == nullptr) {
    unwinder = new unwindstack::UnwinderFromPid(koom::Constant::max_call_stack_depth,
                                                getpid(), unwindstack::Regs::CurrentArch());
    unwinder->Init();
    unwinder->SetDisplayBuildID(true);
    unwinder->SetRegs(unwindstack::Regs::CreateFromLocal());
  }
  std::string format;
  unwindstack::FrameData data = unwinder->BuildFrameFromPcOnly(pc);
  if (data.map_name.find("libkoom-thread") != std::string::npos) {
    inSymbolize = false;
    return "";
  }
  data.num = index;
  format = unwinder->FormatFrame(data);

  inSymbolize = false;
  return format;
}

int64_t CallStack::GetCurrentTimeLimit(CallStack::Type type) {
  int max_loop_count = type == java ? max_java_loop_count : max_native_loop_count;
  int gap = type == java ? java_single_gap : native_single_gap;
  int64_t t = ((type == java ? ++java_gap_count : ++native_gap_count) % max_loop_count) * gap;
  return (t == 0 ? gap : t) * koom::Constant::ns_per_ms;
}

bool CallStack::MeetFrequencyLimit(CallStack::Type type) {
  return true;
//  struct timespec now_time{};
//  clock_gettime(CLOCK_MONOTONIC, &now_time);
//  return (now_time.tv_sec * koom::Constant::ns_per_second + now_time.tv_nsec) -
//      ((type == java ? lastJavaTime.tv_sec : lastNativeTime.tv_sec) * koom::Constant::ns_per_second
//          + (type == java ? lastJavaTime.tv_nsec : lastNativeTime.tv_nsec))
//      > GetCurrentTimeLimit(type);
}

void CallStack::DisableJava() {
  disableJava = true;
}

void CallStack::DisableNative() {
  disableNative = true;
}

}
