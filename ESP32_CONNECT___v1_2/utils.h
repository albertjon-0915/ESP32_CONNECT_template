#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <type_traits>

// Disable Serial.print/println
// #define DISABLE_DEBUG --> usage(declare on .ino file)
#ifndef DISABLE_DEBUG
// Normal Serial (Arduino built-in)
#else
struct DebugClass : public Stream {
  void begin(long) {}
  void end() {}
  void flush() {}
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  size_t write(uint8_t) override { return 1; }

  template <typename T>
  void print(const T&) {}
  template <typename T>
  void println(const T&) {}
  void println() {}
  void printf(const char*, ...) {}
} debugSerial;
#define Serial debugSerial
#endif

// Timer struct
template<typename Func>
struct Timer {
  unsigned long lastRun = 0;
  unsigned long interval;
  Func callback;
};

// runOnce Struct --> void-returning functions
template<typename Func>
struct RunOnce {
  bool executed = false;
  Func callback;

  auto run() {
    if (!executed) {
      executed = true;
      if constexpr (std::is_same_v<decltype(callback()), void>) {
        callback();
      } else {
        return callback();
      }
    }
    if constexpr (!std::is_same_v<decltype(callback()), void>) {
      return decltype(callback()){};  // default
    }
  }
};

// Helper macro
#define CREATE_ASYNC_FN(name, interval, func) Timer<decltype(func)*> name{ 0, interval, func };
#define CREATE_ASYNC_OBJ(name, interval, func) Timer<decltype(func)> name{ 0, interval, func };
#define CREATE_RUNONCE_FN(name, func) RunOnce<decltype(func)*> name{ false, func };
#define CREATE_RUNONCE_OBJ(name, func) RunOnce<decltype(func)> name{ false, func };
#define CREATE_RUNONCE_LAMBDA(name, lambda) RunOnce<decltype(lambda)> name{false, lambda};

// void version
template<typename Func>
inline void asyncDelay(Timer<Func>& t, std::enable_if_t<std::is_same_v<decltype(t.callback()), void>, int> = 0) {
  unsigned long now = millis();
  if (now - t.lastRun >= t.interval) {
    t.lastRun = now;
    t.callback();
  }
}

// non-void version
template<typename Func>
inline auto asyncDelay(Timer<Func>& t, std::enable_if_t<!std::is_same_v<decltype(t.callback()), void>, int> = 0) {
  unsigned long now = millis();
  if (now - t.lastRun >= t.interval) {
    t.lastRun = now;
    return t.callback();  // directly return the value
  }
  return decltype(t.callback()){};  // default value if not executed
}

// Generic wrapper to store function + parameters
template<typename Ret, typename... Args>
struct FuncWrapper {
  Ret (*func)(Args...);
  std::tuple<Args...> args;

  Ret operator()() {
    return std::apply(func, args);
  }
};

// Helper to create FuncWrapper easily
template<typename Ret, typename... Args>
auto wrapper(Ret (*f)(Args...), Args... args) {
  return FuncWrapper<Ret, Args...>{ f, std::make_tuple(args...) };
}

#endif  // UTILS_H
