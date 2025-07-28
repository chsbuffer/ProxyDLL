#pragma once

#include <windows.h>

struct {
  FARPROC ptr[256];

  void DllInit(HMODULE hModule) {
    for (int i = 0; i < 256; i++) {
      ptr[i] = GetProcAddress(hModule, MAKEINTRESOURCEA(i));
    }
  }
} dll;

void _GetFileVersionInfoA() { dll.ptr[1](); }
void _GetFileVersionInfoByHandle() { dll.ptr[2](); }
void _GetFileVersionInfoExA() { dll.ptr[3](); }
void _GetFileVersionInfoExW() { dll.ptr[4](); }
void _GetFileVersionInfoSizeA() { dll.ptr[5](); }
void _GetFileVersionInfoSizeExA() { dll.ptr[6](); }
void _GetFileVersionInfoSizeExW() { dll.ptr[7](); }
void _GetFileVersionInfoSizeW() { dll.ptr[8](); }
void _GetFileVersionInfoW() { dll.ptr[9](); }
void _VerFindFileA() { dll.ptr[10](); }
void _VerFindFileW() { dll.ptr[11](); }
void _VerInstallFileA() { dll.ptr[12](); }
void _VerInstallFileW() { dll.ptr[13](); }
void _VerLanguageNameA() { dll.ptr[14](); }
void _VerLanguageNameW() { dll.ptr[15](); }
void _VerQueryValueA() { dll.ptr[16](); }
void _VerQueryValueW() { dll.ptr[17](); }