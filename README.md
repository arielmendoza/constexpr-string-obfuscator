# constexpr-string-obfuscator

A small header-only C++20 library for compile-time string obfuscation.

It transforms string literals at compile time and stores only obfuscated bytes in the final binary. At runtime, the original string is reconstructed only when requested.

The goal is to make basic static string extraction harder.

This is useful when you do not want readable strings such as internal URLs, API paths, log messages, feature names, user agents, or protocol fragments to appear directly in the compiled executable.

## Important Security Notice

This library provides obfuscation, not secure secret storage.

If your program can recover a string at runtime, a determined reverse engineer can recover it too.

This project is designed to protect against casual static inspection, such as running `strings` on a binary or searching it with a hex editor. It does not protect against debugging, memory inspection, API hooking, runtime instrumentation, or advanced reverse engineering.

Do not use this library to protect passwords, private keys, long-lived API tokens, production credentials, encryption keys, or any value that must remain secret against an attacker who can run or inspect the program.

## Features

- Header-only.
- C++20.
- Compile-time string transformation.
- Supports narrow and wide strings:
  - `char`
  - `wchar_t`
  - `char16_t`
  - `char32_t`
- Supports:
  - `std::string`
  - `std::wstring`
  - `.c_str()`
  - `.str()`
- Per-use seed variation with `__COUNTER__`.
- Optional external build key through `OBF_BUILD_KEY`.
- Runtime buffer wiping on destruction.
- MSVC-friendly implementation.
- No external dependencies.

## What It Protects Against

This library helps prevent plaintext strings from appearing directly in the compiled binary.

It can make the following harder:

- Running `strings app.exe`.
- Searching the executable with a hex editor.
- Basic static scanning for known text.
- Casual extraction of URLs, routes, log messages, user agents, or internal names.

Example:

```cpp
std::wstring appId = OBF(L"TEST_APP_ID");
