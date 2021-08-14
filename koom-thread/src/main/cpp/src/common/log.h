#ifndef APM_LOG_H
#define APM_LOG_H

#include <stdio.h>
#include <android/log.h>

namespace koom {

class Log {
 public:

  static const int max_log_line = 512;
  static bool logEnable;

  enum Type {
    Info, Error
  };

  static void info(const char *tag, const char *format, ...) {
    if (!logEnable) return;
    char log_buffer[max_log_line];
    va_list args;
    va_start(args, format);
    vsnprintf(const_cast<char *>(log_buffer), max_log_line, format, args);
    va_end(args);
    log(Info, tag, log_buffer);
  }

  static void error(const char *tag, const char *format, ...) {
    if (!logEnable) return;
    char log_buffer[max_log_line];
    va_list args;
    va_start(args, format);
    vsnprintf(const_cast<char *>(log_buffer), max_log_line, format, args);
    va_end(args);
    log(Error, tag, log_buffer);
  }

 private:
  static void log(Type type, const char *tag, char *log_buffer) {
    if (!logEnable) return;
    __android_log_print(type == Info ? ANDROID_LOG_INFO : ANDROID_LOG_ERROR,
                        tag, "%s", log_buffer);
  }
};
}

#endif //APM_LOG_H
