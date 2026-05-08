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

This helps prevent readable strings such as:

- Internal URLs
- API paths
- User agents
- Feature names
- Log messages
- Protocol fragments
- Internal identifiers

from appearing directly inside the compiled executable.

---

## Important Security Notice

> [!IMPORTANT]
> This library provides **obfuscation**, not secure secret storage.

If your program can recover a string at runtime, then a determined reverse engineer can recover it too.

This project is designed to protect against **casual static inspection**, such as:

```bash
strings app.exe
