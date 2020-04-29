// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_crash_reporter.h"

#include <map>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/no_destructor.h"
#include "components/crash/core/app/crashpad.h"
#include "components/crash/core/common/crash_key.h"
#include "content/public/common/content_switches.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "services/service_manager/embedder/switches.h"
#include "shell/common/crash_keys.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_helper/dictionary.h"

#if defined(OS_POSIX) && !defined(OS_MACOSX)
#include "components/crash/core/app/breakpad_linux.h"
#include "v8/include/v8-wasm-trap-handler-posix.h"
#include "v8/include/v8.h"
#endif

#include "shell/common/node_includes.h"

namespace gin {

/*
template <>
struct Converter<CrashReporter::UploadReportResult> {
static v8::Local<v8::Value> ToV8(
    v8::Isolate* isolate,
    const CrashReporter::UploadReportResult& reports) {
  return gin::DataObjectBuilder(isolate)
      .Set("date",
           v8::Date::New(isolate->GetCurrentContext(), reports.first * 1000.0)
               .ToLocalChecked())
      .Set("id", reports.second)
      .Build();
}
};
*/

}  // namespace gin

namespace {

std::map<std::string, std::string>& GetGlobalCrashKeysMutable() {
  static base::NoDestructor<std::map<std::string, std::string>>
      global_crash_keys;
  return *global_crash_keys;
}

bool g_crash_reporter_initialized = false;

}  // namespace

namespace electron {

namespace api {

namespace crash_reporter {

bool IsCrashReporterEnabled() {
  return g_crash_reporter_initialized;
}

#if defined(OS_LINUX)
const std::map<std::string, std::string>& GetGlobalCrashKeys() {
  return GetGlobalCrashKeysMutable();
}
#endif

}  // namespace crash_reporter

}  // namespace api

}  // namespace electron

namespace {

void SetCrashKeysFromMap(const std::map<std::string, std::string>& extra) {
  for (const auto& pair : extra) {
    electron::crash_keys::SetCrashKey(pair.first, pair.second);
  }
}

void Start(const std::string& submit_url,
           const std::string& crashes_directory,
           bool upload_to_server,
           bool ignore_system_crash_handler,
           bool rate_limit,
           bool compress,
           const std::map<std::string, std::string>& extra_global,
           const std::map<std::string, std::string>& extra) {
  if (g_crash_reporter_initialized)
    return;
  g_crash_reporter_initialized = true;
  auto* command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(::switches::kProcessType);
  if (crash_reporter::IsCrashpadEnabled()) {
    crash_reporter::InitializeCrashpad(process_type.empty(), process_type);
    // crash_reporter::SetFirstChanceExceptionHandler(v8::TryHandleWebAssemblyTrapPosix);
  } else {
    breakpad::SetUploadURL(submit_url);
    auto& global_crash_keys = GetGlobalCrashKeysMutable();
    for (const auto& pair : extra_global) {
      global_crash_keys[pair.first] = pair.second;
    }
    SetCrashKeysFromMap(extra);
    SetCrashKeysFromMap(extra_global);
    breakpad::InitCrashReporter(process_type);
  }
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  // auto reporter = base::Unretained(CrashReporter::GetInstance());
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("start", base::BindRepeating(&Start));
  /*
  dict.SetMethod(
      "addExtraParameter",
      base::BindRepeating(&CrashReporter::AddExtraParameter, reporter));
  dict.SetMethod(
      "removeExtraParameter",
      base::BindRepeating(&CrashReporter::RemoveExtraParameter, reporter));
  dict.SetMethod("getParameters",
                 base::BindRepeating(&CrashReporter::GetParameters, reporter));
  dict.SetMethod(
      "getUploadedReports",
      base::BindRepeating(&CrashReporter::GetUploadedReports, reporter));
  dict.SetMethod(
      "setUploadToServer",
      base::BindRepeating(&CrashReporter::SetUploadToServer, reporter));
  dict.SetMethod(
      "getUploadToServer",
      base::BindRepeating(&CrashReporter::GetUploadToServer, reporter));
      */
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_crash_reporter, Initialize)
