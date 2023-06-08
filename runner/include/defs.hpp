#pragma once

typedef PVOID void_ptr;
typedef HANDLE handle;

#define FMT_FORMAT(X, ...) fmt::format(_XS(X), __VA_ARGS__)
