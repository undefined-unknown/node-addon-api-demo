#ifndef ENCODING_UTILS_H
#define ENCODING_UTILS_H

#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>

// UTF-8 to Wide Char conversion helper - inline to avoid multiple definitions
inline std::wstring Utf8ToWide(const char* utf8Str) {
    if (!utf8Str) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (len <= 0) return L"";
    std::wstring wstr(len - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wstr[0], len);
    return wstr;
}

inline std::wstring Utf8ToWide(const std::string& utf8str) {
    if (utf8str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8str[0], (int)utf8str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8str[0], (int)utf8str.size(), &wstr[0], size_needed);
    return wstr;
}

// Wide Char to UTF-8 conversion helper
inline std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

// Create fs::path from UTF-8 string, properly handling encoding on Windows
inline std::filesystem::path CreatePathFromUtf8(const std::string& utf8path) {
    return std::filesystem::path(Utf8ToWide(utf8path));
}

// Convert fs::path to UTF-8 string for use with C APIs
inline std::string PathToUtf8String(const std::filesystem::path& p) {
    return WideToUtf8(p.wstring());
}

#else

// Non-Windows versions - just pass through
inline std::wstring Utf8ToWide(const char* utf8Str) {
    // Not used on non-Windows
    return L"";
}

inline std::wstring Utf8ToWide(const std::string& utf8str) {
    // Not used on non-Windows
    return std::wstring();
}

inline std::string WideToUtf8(const std::wstring& wstr) {
    // Not used on non-Windows
    return std::string();
}

inline std::filesystem::path CreatePathFromUtf8(const std::string& utf8path) {
    return std::filesystem::path(utf8path);
}

inline std::string PathToUtf8String(const std::filesystem::path& p) {
    return p.string();
}

#endif

#endif // ENCODING_UTILS_H
