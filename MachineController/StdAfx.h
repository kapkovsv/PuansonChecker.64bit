#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <intrin.h>

#include <memory>
#include <vector>
#include <algorithm>

#include <newdev.h>

extern "C" {
#include <usbioctl.h>
#include <winusb.h>
}

template<class T, int N> constexpr unsigned dimof(T(&)[N]) {
	return N;
}
template<class T, int N> T* endof(T(&ar)[N]) {
	return ar + N;
}

inline int DbgPrint(const char* Format, ...) {
	va_list ArgPtr;
	va_start(ArgPtr, Format);
	char Buf[0x200];
	int Len = _vsnprintf_s(Buf, sizeof Buf, Format, ArgPtr);
	OutputDebugStringA(Buf);
	vprintf(Format, ArgPtr);
	va_end(ArgPtr);
	return Len;
}
