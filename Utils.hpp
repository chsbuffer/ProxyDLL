#pragma once

#include <windows.h>
#include <psapi.h>
#include <span>
#include <algorithm>
#include <filesystem>
#include <vector>
#include <string>

// Logging

#ifdef LOGGING
#include <iostream>
#include <sstream>

#define LOG(...)                              \
  do {                                        \
    std::wstringstream ss;                    \
    ss << __VA_ARGS__;                        \
    ss << std::endl;                          \
    std::wstring debugString = ss.str();      \
    OutputDebugStringW(debugString.c_str());  \
  } while (0)
#else // !LOGGING
#define LOG(...) 
#endif // LOGGING

inline HMODULE LoadSystemLibrary(LPCSTR library_name)
{
  unsigned int sys_dir_size = GetSystemDirectoryA(nullptr, 0);
  std::vector<char> sys_dir(sys_dir_size);
  GetSystemDirectoryA(sys_dir.data(), sys_dir_size);
  std::filesystem::path full_path = sys_dir.data();
  full_path /= library_name;
  std::string full_path_str = full_path.string();
  if (GetFileAttributesA(full_path_str.data()) == INVALID_FILE_ATTRIBUTES)
  {
    return nullptr;
  }

  return LoadLibraryA(full_path_str.data());
}

inline std::span<char> GetModuleSpan() {
  auto mod = GetModuleHandle(nullptr);
  MODULEINFO modinfo;
  if (!GetModuleInformation(GetCurrentProcess(), mod, &modinfo, sizeof(MODULEINFO))) {
    LOG("GetModuleInformation failed: " << GetLastError());
    throw std::runtime_error("GetModuleInformation failed");
  }

  return { (char*)modinfo.lpBaseOfDll, modinfo.SizeOfImage };
}

std::uint32_t hash_str(const std::string& s) {
  std::uint32_t hash_value = 0;
  const std::uint32_t prime_number = 31;
  for (char c : s) {
    hash_value = hash_value * prime_number + static_cast<unsigned char>(c);
  }

  return hash_value;
}

template <class T>
bool patch(std::span<T> buf, std::span<const T> from, std::span<const T> to)
{
  if (buf.empty())
    return false;

  if (from.size() != to.size())
    return false;

  for (size_t i = 0; i < buf.size() - from.size(); i++)
  {
    auto target_span = buf.subspan(i, from.size());
    if (memcmp(target_span.data(), from.data(), from.size()) == 0)
    {
      LOG("Patch " << i);
      DWORD oldProtect;
      VirtualProtect(target_span.data(), from.size() * sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
      std::ranges::copy(to, target_span.begin());
      VirtualProtect(target_span.data(), from.size() * sizeof(T), oldProtect, &oldProtect);
      FlushInstructionCache(GetCurrentProcess(), 0, 0);
      return true;
    }
  }
  return false;
}

inline void hex2byte(std::span<char> hex, std::vector<char>& pattern, std::vector<char>& mask) {
  pattern.clear();
  mask.clear();

  if (hex.size() % 2 != 0) {
    throw std::invalid_argument("Hex pattern string length must be even.");
  }

  for (size_t i = 0; i < hex.size(); i += 2) {
    char high = hex[i];
    char low = hex[i + 1];

    if (high == '?' && low == '?') {
      pattern.push_back(0x00);
      mask.push_back(0x00);   // Mask byte is 0x00 for "don't care"
    }
    else {
      // Validate hex characters
      if (!std::isxdigit(high) || !std::isxdigit(low)) {
        throw std::invalid_argument("Invalid hex character in pattern.");
      }

      // Convert hex characters to integer values
      char h = std::isdigit(high) ? high - '0' : std::tolower(high) - 'a' + 10;
      char l = std::isdigit(low) ? low - '0' : std::tolower(low) - 'a' + 10;

      pattern.push_back(static_cast<char>((h << 4) | l));
      mask.push_back(0xFF); // Mask byte is 0xFF for "care"
    }
  }
}

inline std::vector<size_t> FindPattern(std::span<char> buf, std::span<char> pattern, std::span<char> mask)
{
  std::vector<size_t> ofs;
  for (size_t i = 0; i <= buf.size() - pattern.size(); i++)
  {
    bool bFound = true;
    for (size_t j = 0; j < pattern.size(); j++)
      bFound &= mask[j] == 0x00 || pattern[j] == buf[i + j];

    if (bFound)
      ofs.push_back(i);
  }
  return ofs;
}

inline void ApplyPattern(std::span<char> buf, std::span<char> pattern, std::span<char> mask)
{
  for (size_t i = 0; i < pattern.size(); i++)
  {
    buf[i] = mask[i] == 0x00 ? buf[i] : pattern[i];
  }
}

inline void PatternPatch(std::span<char> buf, std::string hexpattern1, std::string hexpattern2)
{
  std::erase_if(hexpattern1, [](char c) {
    return std::isspace(static_cast<unsigned char>(c));
    });
  std::erase_if(hexpattern2, [](char c) {
    return std::isspace(static_cast<unsigned char>(c));
    });

  std::vector<char> pattern(hexpattern1.size() / 2);
  std::vector<char> mask(hexpattern2.size() / 2);

  std::vector<char> pattern2(hexpattern1.size() / 2);
  std::vector<char> mask2(hexpattern2.size() / 2);

  hex2byte(hexpattern1, pattern, mask);
  hex2byte(hexpattern2, pattern2, mask2);
  auto matches = FindPattern(buf, pattern, mask);
  for (auto match : matches) {
    LOG("Patch " << match);
    auto target_span = buf.subspan(match, pattern.size());
    DWORD oldProtect;
    VirtualProtect(target_span.data(), target_span.size(), PAGE_EXECUTE_READWRITE, &oldProtect);
    ApplyPattern(target_span, pattern2, mask2);
    VirtualProtect(target_span.data(), target_span.size(), oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target_span.data(), target_span.size());
  }
}
