# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

**Please do NOT report security vulnerabilities through public GitHub issues.**

If you discover a security vulnerability in Il2CPP Dumper, please report it responsibly through one of the following channels:

### Preferred: Telegram (Private)

Send a direct message to [@springmusk](https://t.me/springmusk) with:
- A description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

### Alternative: Create a Private Security Advisory

Use [GitHub's private vulnerability reporting](https://github.com/springmusk026/Android-Native-Il2cppDumper/security/advisories/new) to report the issue privately.

## What to Include

When reporting a vulnerability, please include:

1. **Type of vulnerability** (e.g., buffer overflow, path traversal, code injection)
2. **Location** — File path and line number(s) if possible
3. **Reproduction steps** — Clear steps to trigger the vulnerability
4. **Impact** — What an attacker could achieve
5. **Affected components** — Native C++ core, Kotlin UI, JNI bridge, etc.
6. **Device/OS info** — Android version, device model if relevant

## Response Timeline

- **Acknowledgment**: Within 48 hours of report
- **Initial assessment**: Within 1 week
- **Fix timeline**: Depends on severity
  - Critical: As soon as possible (1-3 days)
  - High: Within 1 week
  - Medium: Within 2 weeks
  - Low: Next release cycle

## Scope

### In Scope

- **Native C++ core** (`app/src/main/cpp/`) — Buffer overflows, integer overflows, use-after-free, out-of-bounds access
- **JNI bridge** (`jni_bridge.cpp`, `NativeDumper.kt`) — Type confusion, memory safety issues
- **File handling** — Path traversal, symlink attacks, zip bombs
- **Room database** — SQL injection, data leakage
- **FileProvider** — URI permission issues
- **Export/Share** — File permission issues, unintended data exposure
- **ProGuard/R8** — Overly aggressive obfuscation breaking security properties

### Out of Scope

- Social engineering attacks
- Physical device access attacks
- Vulnerabilities in third-party dependencies (report to the dependency maintainer)
- Issues requiring root access to the device
- Denial of service through large file inputs (the app processes user-selected files)

## Disclosure Policy

- We will work with you to understand and fix the issue
- We will credit you in the security advisory (unless you prefer to remain anonymous)
- We request a reasonable disclosure timeline — typically 90 days after a fix is released
- We will not take legal action against security researchers acting in good faith

## Security Best Practices for Users

- Only download Il2CPP Dumper from the [official GitHub repository](https://github.com/springmusk026/Android-Native-Il2cppDumper)
- Verify the APK signature if building from source
- Do not use the app on untrusted or malicious files without understanding the risks
- Keep your Android device updated with the latest security patches

## Credits

We thank all security researchers who help keep this project safe. Responsible disclosure helps protect all users.
