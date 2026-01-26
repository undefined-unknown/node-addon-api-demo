#include <napi.h>
#include <string>

// C++ 函数：返回一个字符串
Napi::String GetHello(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  std::string platform = "Unknown";

  #ifdef _WIN32
    platform = "Windows";
  #elif __APPLE__
    platform = "macOS";
  #else
    platform = "Linux";
  #endif

  std::string result = "Hello from C++ on " + platform;
  return Napi::String::New(env, result);
}

// 包含加法功能的例子 (可选)
Napi::Number Add(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsNumber()) {
    Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
    return Napi::Number::New(env, 0);
  }

  double arg0 = info[0].As<Napi::Number>().DoubleValue();
  double arg1 = info[1].As<Napi::Number>().DoubleValue();

  return Napi::Number::New(env, arg0 + arg1);
}

// 初始化插件并导出函数
Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "getHello"), Napi::Function::New(env, GetHello));
  exports.Set(Napi::String::New(env, "add"), Napi::Function::New(env, Add));
  return exports;
}

NODE_API_MODULE(addon, Init)
