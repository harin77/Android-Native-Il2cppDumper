# Contributing to Il2CPP Dumper

Thank you for your interest in contributing to Il2CPP Dumper! This document provides guidelines and instructions for contributing.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Contribute](#how-to-contribute)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)
- [Pull Request Process](#pull-request-process)
- [Code Style](#code-style)
- [Commit Messages](#commit-messages)
- [Branching Model](#branching-model)
- [Attribution](#attribution)

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior via Telegram: [@springmusk](https://t.me/springmusk).

## How to Contribute

### Areas Where Help is Needed

- **Bug fixes** — Check the [issue tracker](https://github.com/springmusk026/Android-Native-Il2cppDumper/issues) for open bugs
- **Native C++ improvements** — ELF parsing, search strategies, decompilation output
- **UI/UX improvements** — Jetpack Compose screens, Material 3 theming
- **Documentation** — README improvements, code comments, translations
- **Testing** — Unit tests, integration tests, device testing
- **New features** — See the feature request template

### Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/<your-username>/Android-Native-Il2cppDumper.git`
3. Create a branch: `git checkout -b feature/your-feature-name`
4. Make your changes
5. Test thoroughly on a physical device (ARM64)
6. Push and create a Pull Request

## Reporting Bugs

Use the [Bug Report template](https://github.com/springmusk026/Android-Native-Il2cppDumper/issues/new?template=bug_report.md) when filing a bug report. Include:

- Device model and Android version
- App version
- Steps to reproduce
- Expected vs actual behavior
- Screenshots or screen recordings if applicable
- Log output from the app's log viewer

**Security vulnerabilities** should be reported privately — see [SECURITY.md](SECURITY.md).

## Suggesting Features

Use the [Feature Request template](https://github.com/springmusk026/Android-Native-Il2cppDumper/issues/new?template=feature_request.md). Describe:

- The problem you're trying to solve
- Your proposed solution
- Alternatives you've considered
- Any mockups or examples

## Pull Request Process

1. **Ensure your PR addresses an open issue** — Create one if none exists
2. **Fill out the PR template** completely
3. **Keep PRs focused** — One feature or fix per PR
4. **Write tests** if applicable
5. **Update documentation** if your change affects user-facing behavior
6. **Ensure the build passes** — `./gradlew assembleDebug` must succeed
7. **Test on a physical device** — Emulators may not accurately test native code

### PR Review Checklist

- [ ] Code compiles without warnings
- [ ] No new lint errors
- [ ] Tested on physical ARM64 device
- [ ] Documentation updated if needed
- [ ] Commit messages follow the format below
- [ ] No commercial use or monetization code introduced

## Code Style

### Kotlin

- Follow the [Kotlin Coding Conventions](https://kotlinlang.org/docs/coding-conventions.html)
- Use `camelCase` for functions and variables, `PascalCase` for classes
- Use meaningful names — avoid single-letter variables except in lambdas
- Prefer `val` over `var`
- Use coroutines for async work, not raw threads
- Use `StateFlow` and `SharedFlow` for reactive state

### C++

- Follow the existing code style in `app/src/main/cpp/core/`
- Use `snake_case` for functions and variables
- Use `PascalCase` for types and structs
- Always use braces for control flow blocks
- Use RAII and smart pointers where possible
- Log errors with the `LOGE` macro

### Compose

- Use Material 3 components
- Follow the existing screen pattern: `Screen` composable with `ViewModel`
- Use `collectAsState()` for ViewModel state observation
- Use `Modifier` chaining for styling

## Commit Messages

Follow this format:

```
<type>(<scope>): <short description>

<optional body>

<optional footer>
```

### Types

| Type | Description |
|------|-------------|
| `feat` | New feature |
| `fix` | Bug fix |
| `docs` | Documentation changes |
| `style` | Code style changes (formatting, no logic change) |
| `refactor` | Code refactoring (no feature or fix) |
| `perf` | Performance improvement |
| `test` | Adding or updating tests |
| `chore` | Build, CI, or tooling changes |

### Examples

```
feat(navigation): add job detail screen with export functionality
fix(executor): prevent underflow in getTypeDefinitionFromIl2CppType
docs(readme): update architecture diagram for Room database
chore(deps): update Kotlin serialization plugin to 2.1.20
```

## Branching Model

- `main` — Stable release branch
- `develop` — Development branch (if used)
- `feature/*` — Feature branches
- `fix/*` — Bug fix branches
- `docs/*` — Documentation branches

Always branch from `main` and create PRs back to `main`.

## Attribution

By contributing, you agree that your contributions will be licensed under the same [Springmusk Non-Commercial License](LICENSE) that covers this project. You retain copyright to your contributions but grant the project maintainer the right to include and distribute them under this license.

**Important**: This project has specific attribution requirements. All contributions must maintain the existing attribution to:
- **springmusk** — Project creator and maintainer
- **Perfare** — Original Il2CppDumper project

## Questions?

If you have questions about contributing, reach out via:
- **Telegram**: [@springmusk](https://t.me/springmusk)
- **Telegram Channel**: [@Layout_musk](https://t.me/Layout_musk)
- **GitHub Issues**: For public questions about the project
