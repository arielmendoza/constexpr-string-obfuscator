# constexpr-string-obfuscator

<p align="center">
  <strong>Header-only C++20 compile-time string obfuscation</strong>
</p>

<p align="center">
  Hide string literals from basic static extraction tools such as <code>strings</code>, hex editors, and simple binary scanners.
</p>

<p align="center">
  <img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-blue.svg">
  <img alt="Header Only" src="https://img.shields.io/badge/header--only-yes-brightgreen.svg">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg">
  <img alt="License" src="https://img.shields.io/badge/license-MIT-green.svg">
</p>

---

## Overview

`constexpr-string-obfuscator` is a small **header-only C++20 library** that transforms string literals at compile time and stores only obfuscated data in the final binary.

At runtime, the original string is reconstructed only when requested.

This helps prevent readable strings such as internal URLs, API paths, user agents, feature names, log messages, protocol fragments, or internal identifiers from appearing directly inside the compiled executable.

---

## Important Security Notice

> [!IMPORTANT]
> This library provides **obfuscation**, not secure secret storage.

If your program can recover a string at runtime, then a determined reverse engineer can recover it too.

This project is designed to protect against **casual static inspection**, such as running:

```bash
strings app.exe
```

or searching the binary with a hex editor.

It does **not** protect against:

- Debuggers
- Memory dumps
- API hooking
- Runtime instrumentation
- Reverse engineering tools
- Function call interception
- Attackers who can execute and inspect the program

Do **not** use this library to protect:

- Passwords
- Private keys
- Production credentials
- Long-lived API tokens
- Encryption keys
- License bypass logic
- DRM secrets

---

## What Problem Does This Solve?

When you write code like this:

```cpp
std::wstring endpoint = L"https://example.com/private/api";
std::string tokenName = "access_token";
```

those strings may appear directly in the compiled binary.

A basic static inspection tool may reveal them immediately:

```bash
strings app.exe
```

This library allows you to write:

```cpp
std::wstring endpoint = OBF(L"https://example.com/private/api");
std::string tokenName = OBF("access_token");
```

The string literal is transformed at compile time, so the plaintext value should not appear directly in the final executable.

---

## Features

- Header-only
- C++20
- No external dependencies
- Compile-time string transformation
- Runtime reconstruction only when needed
- Supports narrow and wide strings
- Supports `std::string`
- Supports `std::wstring`
- Supports `.c_str()`
- Supports `.str()`
- Works with Windows wide APIs
- Per-use seed variation using `__COUNTER__`
- Optional external build key with `OBF_BUILD_KEY`
- Best-effort runtime buffer wiping
- MSVC-friendly implementation

---

## Supported String Types

| Type | Example | Supported |
|---|---|---|
| `char` | `OBF("hello")` | Yes |
| `wchar_t` | `OBF(L"hello")` | Yes |
| `char16_t` | `OBF(u"hello")` | Yes |
| `char32_t` | `OBF(U"hello")` | Yes |
| `std::string` | `std::string s = OBF("hello");` | Yes |
| `std::wstring` | `std::wstring s = OBF(L"hello");` | Yes |

---

## Installation

This is a header-only library.

Copy the header into your project:

```text
include/obf_string.hpp
```

Then include it:

```cpp
#include "obf_string.hpp"
```

Compile with **C++20** or later.

---

## Basic Usage

```cpp
#include <iostream>
#include <string>
#include "obf_string.hpp"

int main()
{
    std::string narrow = OBF("This narrow string is obfuscated");
    std::wstring wide = OBF(L"This wide string is obfuscated");

    std::cout << narrow << "\n";
    std::wcout << wide << L"\n";

    return 0;
}
```

---

## Using With `std::string`

```cpp
std::string value = OBF("hidden text");
```

You can also call `.str()` explicitly:

```cpp
std::string value = OBF("hidden text").str();
```

---

## Using With `std::wstring`

```cpp
std::wstring value = OBF(L"hidden wide text");
```

Or explicitly:

```cpp
std::wstring value = OBF(L"hidden wide text").str();
```

---

## Using With Windows APIs

For Windows APIs that expect `const wchar_t*`, use `.c_str()`:

```cpp
MessageBoxW(
    nullptr,
    OBF(L"Hidden message").c_str(),
    OBF(L"Hidden title").c_str(),
    MB_OK
);
```

This is safe because temporaries live until the end of the full expression.

However, do **not** store a pointer returned from a temporary:

```cpp
const wchar_t* ptr = OBF(L"Hidden text").c_str(); // Wrong
```

The temporary object is destroyed at the end of the line, leaving `ptr` dangling.

Use this instead:

```cpp
auto hidden = OBF(L"Hidden text");
const wchar_t* ptr = hidden.c_str();
```

Or simply:

```cpp
std::wstring value = OBF(L"Hidden text");
```

---

## Build Key

By default, the library uses a built-in build key:

```cpp
#define OBF_BUILD_KEY 0xA7C3E29B4F13579Dull
```

For real projects, you should provide your own build key at compile time.

### MSVC

```text
/DOBF_BUILD_KEY=0x91E34A78BCDD120F
```

### GCC / Clang

```bash
g++ -std=c++20 -O2 -DOBF_BUILD_KEY=0x91E34A78BCDD120F example.cpp -o example
```

### CMake

```cmake
target_compile_definitions(your_target PRIVATE OBF_BUILD_KEY=0x91E34A78BCDD120F)
```

> [!NOTE]
> Do not hardcode a sensitive production build key in a public repository.
>
> The build key is not a true cryptographic secret once the binary is distributed, but using a project-specific value makes static reuse and pattern matching less convenient.

---

## Verifying The Result

After compiling, inspect the binary.

### Linux / macOS

```bash
strings ./app | grep "hidden text"
```

### Windows PowerShell with Sysinternals Strings

```powershell
strings.exe .\app.exe | findstr /i "hidden text"
```

### Windows PowerShell Basic Search

```powershell
Select-String -Path .\app.exe -Pattern "hidden text" -SimpleMatch
```

For wide strings, use a Unicode-aware extraction tool or search for UTF-16 patterns.

A successfully obfuscated string should not appear directly as plaintext in the binary.

---

## How It Works

Conceptually, this:

```cpp
std::string text = OBF("secret text");
```

becomes something closer to this:

```text
source code:
"secret text"

compiled binary:
transformed bytes

runtime:
"secret text"
```

The library performs a compile-time transformation of the string literal and stores the transformed representation in the binary.

At runtime, a lightweight object reconstructs the original string when needed.

Each use of `OBF(...)` receives seed variation through:

- `OBF_BUILD_KEY`
- `__FILE__`
- `__DATE__`
- `__TIME__`
- `__COUNTER__`

The runtime object wipes its internal buffer when destroyed.

---

## What It Protects Against

This library helps against **basic static analysis**.

It can make these actions less useful:

- Running `strings` on the executable
- Opening the executable in a hex editor
- Searching for known URLs
- Searching for API paths
- Searching for user-agent strings
- Searching for log messages
- Searching for internal feature names

It raises the effort required to extract strings from the binary.

---

## What It Does Not Protect Against

This library does not provide strong security.

It does not protect against:

| Attack | Protected? |
|---|---|
| `strings` / basic static extraction | Partially |
| Hex editor search | Partially |
| Basic plaintext scanning | Partially |
| Debugger inspection | No |
| Runtime memory dump | No |
| API hooking | No |
| Reverse engineering | No |
| Dynamic instrumentation | No |
| Malware analysis sandbox | No |
| A determined reverse engineer | No |

The executable still contains everything needed to reconstruct the string:

- The transformed bytes
- The transformation code
- The seed or seed derivation logic

If the application can decrypt the string, so can an attacker with enough time and access.

---

## Plaintext Lifetime

Be careful with this:

```cpp
std::wstring appId = OBF(L"TEST_APP_ID");
```

This creates a normal plaintext `std::wstring`.

That plaintext copy is owned by `std::wstring`, not by this library.

It will remain in memory for as long as `appId` exists.

For shorter plaintext lifetime, prefer using the temporary directly:

```cpp
SomeWindowsApiW(OBF(L"TEST_APP_ID").c_str());
```

Or keep the value scoped tightly:

```cpp
{
    std::wstring appId = OBF(L"TEST_APP_ID");
    UseValue(appId);
}
```

---

## Recommended Use Cases

Good use cases:

```text
Internal endpoints
API paths
User-agent strings
Log messages
Feature flags
Protocol fragments
Internal command names
Low-sensitivity identifiers
```

Bad use cases:

```text
Passwords
Private keys
Production secrets
Long-lived API tokens
Encryption keys
DRM secrets
Authentication bypass logic
Anything that must remain secret against a determined attacker
```

---

## Threat Model

### Intended Threat Model

This library is useful when:

```text
An attacker has the compiled binary and performs basic static analysis.
```

Examples:

```text
Running strings
Searching with a hex editor
Looking for obvious URLs or tokens
Scanning for plaintext indicators
```

### Not Intended Threat Model

This library is not sufficient when:

```text
An attacker can execute the binary, attach a debugger, inspect memory, hook APIs, or reverse engineer the program.
```

---

## Example: Hiding HTTP Strings

```cpp
std::wstring host = OBF(L"example.com");
std::wstring path = OBF(L"/api/v1/private");
std::wstring auth = OBF(L"Authorization: Bearer ");
std::string jsonKey = OBF("access_token");
```

These strings should no longer appear directly in the binary as plaintext literals.

However, if the program sends them over the network, passes them to an API, or stores them in a standard string, they will exist in plaintext at runtime.

---

## Example Project

```cpp
#include <iostream>
#include <string>
#include "obf_string.hpp"

int main()
{
    std::string narrow = OBF("hidden narrow string");
    std::wstring wide = OBF(L"hidden wide string");

    std::cout << narrow << "\n";
    std::wcout << wide << L"\n";

    auto temporary = OBF(L"temporary hidden string");
    std::wcout << temporary.c_str() << L"\n";

    return 0;
}
```

Compile with:

```bash
g++ -std=c++20 -O2 example.cpp -o example
```

Or with MSVC:

```text
cl /std:c++20 /O2 example.cpp
```

---

## MSVC Notes

This implementation avoids using `__LINE__` in the seed.

MSVC Debug builds with Edit and Continue enabled may transform `__LINE__` into a non-constant internal value, which can break `constexpr`.

If you use Visual Studio, make sure your project uses C++20:

```text
Project Properties
-> C/C++
-> Language
-> C++ Language Standard
-> ISO C++20 Standard (/std:c++20)
```

If you see this warning:

```text
warning C4067: unexpected tokens following preprocessor directive
```

check for trailing semicolons after preprocessor directives.

Correct:

```cpp
#include "obf_string.hpp"
#pragma once
```

Wrong:

```cpp
#include "obf_string.hpp";
#pragma once;
```

---

## Recommended Repository Layout

```text
constexpr-string-obfuscator/
│  obf_string.hpp
├─ README.md
```

---

## FAQ

### Is this encryption?

Not in the security sense.

It transforms string literals so they do not appear directly in the binary. It should be treated as obfuscation, not cryptographic protection.

### Can this protect API keys?

No.

Do not embed real API keys, passwords, private keys, or production credentials in a client-side binary.

Use a backend, environment-specific configuration, short-lived tokens, user authentication, or an operating-system secret store instead.

### Can someone recover the strings?

Yes.

A determined reverse engineer can recover them by analyzing the binary or observing the program at runtime.

### Why use it then?

Because it raises the effort required for casual extraction.

It prevents the simplest and most common form of string discovery: looking directly at the binary with `strings` or a hex editor.

### Does it work with wide strings?

Yes.

```cpp
std::wstring value = OBF(L"wide string");
```

### Does it work with Windows APIs?

Yes.

```cpp
MessageBoxW(nullptr, OBF(L"Message").c_str(), OBF(L"Title").c_str(), MB_OK);
```

### Is it safe to store `.c_str()`?

Not from a temporary.

Wrong:

```cpp
const wchar_t* p = OBF(L"text").c_str();
```

Correct:

```cpp
auto text = OBF(L"text");
const wchar_t* p = text.c_str();
```

### Does this remove all readable strings from my executable?

No.

Other strings may still appear, including:

- Compiler metadata
- Linker paths
- PDB paths
- Imported function names
- Runtime library strings
- Strings not wrapped with `OBF(...)`

For example, Windows import names such as `WinHttpOpen`, `CreateFileW`, or `MessageBoxW` may still appear because they are part of the import table.

---

## Best Practices

Use `OBF(...)` only around string literals:

```cpp
std::string value = OBF("hello");
```

Avoid storing pointers to temporary strings:

```cpp
const char* p = OBF("hello").c_str(); // Wrong
```

Prefer short scopes for plaintext copies:

```cpp
{
    std::string value = OBF("hello");
    Use(value);
}
```

Use a custom build key for real projects:

```bash
-DOBF_BUILD_KEY=0x91E34A78BCDD120F
```

Do not rely on this for real secrets.

---

## License

This project is licensed under the MIT License.

See [LICENSE](LICENSE) for details.

---

## Final Reminder

> [!WARNING]
> This library raises the cost of static string extraction.
>
> It does not make embedded secrets secure.
>
> If the application can recover the value at runtime, a determined attacker can recover it too.
