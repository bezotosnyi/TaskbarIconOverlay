#include "logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/spdlog.h>

namespace
{
    constexpr wchar_t kLogFileName[] = L"TaskbarIconOverlay.log";
    constexpr char kLogPattern[] = "%Y-%m-%d %H:%M:%S.%e [%t] [%n] [%l] %v";

    // This static lib gets linked separately into every DLL that uses it
    // (Engine.dll, each mod DLL via WindhawkWrapper) - each therefore gets its
    // own independent spdlog::logger instance, same as with
    // DiaSymbolResolver's cache. All of them are configured to point at the
    // SAME file path (same directory, same file name, since every binary
    // lives in the same dist folder) so a single log file still shows events
    // from every component in one place, correlated by timestamp. %n in the
    // pattern (the logger's own name, set to the calling module's base file
    // name below) is what lets a reader tell them apart.
    //
    // Known caveat, not solved here: this means multiple independent file
    // handles (one per DLL-local spdlog instance) can be open on the same
    // path at once. spdlog's file sink appends safely within a single
    // process/handle, but two separate handles writing concurrently under
    // heavy load could theoretically interleave at the OS buffering level.
    // For our logging volume (occasional diagnostic lines, not a hot path)
    // this is a real but low-risk simplification - revisit with a proper
    // single-writer/IPC-forwarded logger if it ever becomes a problem.
    std::shared_ptr<spdlog::logger> g_logger;

    std::wstring ModuleDirectory(HMODULE module)
    {
        wchar_t path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(module, path, ARRAYSIZE(path));
        if (!length || length == ARRAYSIZE(path))
            return {};

        std::wstring result(path, length);
        const size_t slash = result.find_last_of(L"\\/");
        return slash == std::wstring::npos ? std::wstring{} : result.substr(0, slash + 1);
    }

    // Extracts just the base name without extension - e.g.
    // "TaskbarIconOverlay.Engine.dll" -> "TaskbarIconOverlay.Engine",
    // "taskbar-grouping.dll" -> "taskbar-grouping". Used as the spdlog logger
    // name, shown via %n in the pattern, so a shared log file still shows
    // which binary wrote each line.
    std::string ModuleBaseName(HMODULE module)
    {
        wchar_t path[MAX_PATH]{};
        const DWORD length = GetModuleFileNameW(module, path, ARRAYSIZE(path));
        if (!length || length == ARRAYSIZE(path))
            return "unknown-module";

        std::wstring fileName(path, length);
        const size_t slash = fileName.find_last_of(L"\\/");
        if (slash != std::wstring::npos) fileName = fileName.substr(slash + 1);

        const size_t dot = fileName.find_last_of(L'.');
        if (dot != std::wstring::npos) fileName = fileName.substr(0, dot);

        return std::string(fileName.begin(), fileName.end());
    }
} // namespace

void Logger::Init(HMODULE ownModule)
{
    if (g_logger) return; // already initialized for this DLL

    const std::wstring directory = ModuleDirectory(ownModule);
    const std::wstring logPath = directory.empty()
                                     ? kLogFileName
                                     : directory + kLogFileName;

    try
    {
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            std::string(logPath.begin(), logPath.end()), /*truncate=*/false);
        auto msvcSink = std::make_shared<spdlog::sinks::msvc_sink_mt>();

        g_logger = std::make_shared<spdlog::logger>(
            ModuleBaseName(ownModule), spdlog::sinks_init_list{fileSink, msvcSink});
        g_logger->set_pattern(kLogPattern);
        g_logger->set_level(spdlog::level::info);
        g_logger->flush_on(spdlog::level::trace); // flush after every message -
        // our volume is low (diagnostic
        // logging, not a hot path), so
        // immediate visibility while
        // debugging matters more than
        // write-batching performance
    }
    catch (const spdlog::spdlog_ex&)
    {
        // Logging failed to initialize - deliberately not throwing further,
        // a broken logger must never be the reason explorer.exe crashes.
        // Info/Warn/Error below no-op safely if g_logger stays null.
    }
}

namespace
{
    // Narrow conversion for spdlog (its API takes std::string/fmt-style
    // arguments) - our call sites use std::wstring throughout the rest of the
    // codebase, so we convert once here rather than at every call site.
    std::string ToUtf8(const std::wstring& text)
    {
        if (text.empty()) return {};
        const int sizeNeeded = WideCharToMultiByte(
            CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        std::string result(sizeNeeded, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                            result.data(), sizeNeeded, nullptr, nullptr);
        return result;
    }
} // namespace

void Logger::Info(const std::wstring& text)
{
    if (g_logger) g_logger->info(ToUtf8(text));
}

void Logger::Warn(const std::wstring& text)
{
    if (g_logger) g_logger->warn(ToUtf8(text));
}

void Logger::Error(const std::wstring& text)
{
    if (g_logger) g_logger->error(ToUtf8(text));
}
