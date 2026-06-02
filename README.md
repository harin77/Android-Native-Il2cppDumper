# Android Native Il2CppDumper

<div align="center">

[![License](https://img.shields.io/badge/License-Springmusk%20Non--Commercial-blue.svg)](LICENSE)
[![Android](https://img.shields.io/badge/Android-8.0%2B-green.svg)](https://developer.android.com/about/versions/oreo)
[![API](https://img.shields.io/badge/API-26%2B-brightgreen.svg)](https://developer.android.com/studio/releases/platforms)
[![Architecture](https://img.shields.io/badge/Architecture-arm64--v8a-orange.svg)](https://developer.android.com/ndk/guides/abis)
[![Kotlin](https://img.shields.io/badge/Kotlin-2.1.20-purple.svg)](https://kotlinlang.org)
[![Compose](https://img.shields.io/badge/Jetpack%20Compose-Material%203-blue.svg)](https://developer.android.com/jetpack/compose)

An Android-native IL2CPP metadata dumper built with Kotlin/Jetpack Compose and a C++20 native core. Runs entirely on-device — no PC or root required.

Extracts type definitions, method signatures, field offsets, and struct layouts from Unity IL2CPP binaries (`libil2cpp.so` + `global-metadata.dat`).

</div>

## Features

### Core Dumping
- **On-device dumping** — select files via Android file picker, dump directly on your phone
- **ELF parsing** — full 32-bit and 64-bit ELF support for `libil2cpp.so`
- **Multiple search strategies** — PlusSearch (section-based), pattern search, and symbol search for locating registration structures
- **Dump file detection** — auto-detects previously dumped binaries and accepts manual image base addresses
- **Configurable output** — toggle methods, fields, properties, attributes, offsets, and struct generation
- **Force Il2Cpp version** — manually specify the Il2Cpp version for problematic binaries
- **Struct generator** — produces Il2Cpp script JSON with struct definitions, vtables, and RGCTX info

### Job Management
- **Job history** — all dump jobs are persisted in a local Room database with full metadata
- **Job details** — view input file info, hashes, configuration, output files, and logs for each job
- **Export options** — save individual files or export all as ZIP to Downloads folder
- **Share output** — share output files via Android's share sheet (Telegram, email, etc.)
- **Debug info** — copy device/app/file diagnostics for error reporting

### User Interface
- **Material 3 design** — dynamic color support on Android 12+, fallback theme on older devices
- **Jetpack Navigation** — animated transitions between 5 screens
- **Dark mode** — full dark theme support
- **Bottom navigation** — quick access to Dump, History, and About screens

## Screenshots

<div align="center">

| Dump | History | Job Detail | About |
|:----:|:-------:|:----------:|:-----:|
| ![Dump](screenshots/dump.png) | ![History](screenshots/history.png) | ![Detail](screenshots/detail.png) | ![About](screenshots/about.png) |

</div>

> Screenshots coming soon. Run the app to see it in action!

## Requirements

- Android 8.0+ (API 26)
- ARM64 device (arm64-v8a)
- ~50MB free storage for output files

## Download

### Build from Source

```bash
git clone https://github.com/springmusk026/Android-Native-Il2cppDumper.git
cd Android-Native-Il2cppDumper
./gradlew assembleDebug
```

The APK will be at `app/build/outputs/apk/debug/app-debug.apk`.

For release build (R8 optimized):

```bash
./gradlew assembleRelease
```

## Usage

1. Launch the app
2. Tap **Il2Cpp Binary** and select your `libil2cpp.so` file
3. Tap **Global Metadata** and select your `global-metadata.dat` file
4. (Optional) Enter a dump address in hex if working with a previously dumped binary
5. Configure dump options in the expandable config panel
6. Tap **Start Dump**
7. View results in **Job History** or tap **View Job Details** after completion
8. Export or share output files from the job detail screen

## Architecture

```
app/src/main/
├── cpp/                                # Native C++20 core
│   ├── jni_bridge.cpp                  # JNI interface
│   └── core/
│       ├── metadata.{cpp,h}            # global-metadata.dat parser
│       ├── il2cpp.{cpp,h}              # Il2Cpp engine (base + ELF)
│       ├── elf.{cpp,h}                 # ELF binary parser
│       ├── section_helper.{cpp,h}      # Section-based structure search
│       ├── il2cpp_executor.{cpp,h}     # Type/method resolution
│       ├── il2cpp_decompiler.{cpp,h}   # C#-style decompilation output
│       ├── struct_generator.{cpp,h}    # Il2Cpp script JSON generation
│       ├── binary_stream.{cpp,h}       # Memory-mapped binary reader
│       └── config.{cpp,h}              # Dump configuration
│
├── java/com/il2cpp/dumper/
│   ├── MainActivity.kt                 # Entry point + navigation scaffold
│   ├── NativeDumper.kt                 # JNI bridge (loads libdumper.so)
│   │
│   ├── navigation/
│   │   └── Screen.kt                   # Route definitions (5 screens)
│   │
│   ├── data/
│   │   ├── db/                         # Room database
│   │   │   ├── AppDatabase.kt          # Database definition
│   │   │   ├── DumpJobDao.kt           # Data access object
│   │   │   ├── DumpJobEntity.kt        # Job entity with full metadata
│   │   │   └── Converters.kt           # Type converters (List<String>, etc.)
│   │   └── repository/
│   │       └── DumpJobRepository.kt    # Repository + file hashing
│   │
│   ├── viewmodel/
│   │   ├── DumperViewModel.kt          # Main dump orchestration
│   │   ├── JobHistoryViewModel.kt      # History list management
│   │   └── JobDetailViewModel.kt       # Single job detail + export
│   │
│   ├── ui/
│   │   ├── screens/
│   │   │   ├── HomeScreen.kt           # File selection + dump interface
│   │   │   ├── JobHistoryScreen.kt     # Past jobs list
│   │   │   ├── JobDetailScreen.kt      # Job details + export/share
│   │   │   ├── AboutScreen.kt          # App info + credits
│   │   │   └── LicensesScreen.kt       # Open source licenses
│   │   ├── components/
│   │   │   ├── ConfigPanel.kt          # Dump options (10 toggles)
│   │   │   ├── FileSelector.kt         # File picker card
│   │   │   └── LogViewer.kt            # Scrollable log output
│   │   └── theme/
│   │       ├── Color.kt                # Semantic + fallback colors
│   │       ├── Theme.kt                # Material 3 dynamic theme
│   │       └── Type.kt                 # Typography
│   │
│   └── util/
│       ├── DebugInfoCollector.kt       # Device/app diagnostics
│       └── ExportUtil.kt               # Save to Downloads, ZIP, share
```

## Tech Stack

| Layer | Technology |
|-------|-----------|
| **UI** | Kotlin, Jetpack Compose, Material 3, Navigation Compose |
| **Architecture** | MVVM, StateFlow, ViewModel, Room Database |
| **Native** | C++20, CMake, JNI, ELF parsing |
| **Serialization** | kotlinx.serialization |
| **Build** | Gradle 9.x, AGP 9.1, KSP, R8 |
| **Libraries** | AboutLibraries, Coroutines |

## Screens

| Screen | Bottom Nav | Description |
|--------|:----------:|-------------|
| **Home** | Dump | File selection, config panel, dump execution |
| **History** | History | List of all past dump jobs with status |
| **Job Detail** | — | Full job info, output files, export/share |
| **About** | About | App info, developer, credits, error reporting |
| **Licenses** | — | Open source license viewer |

## Credits

This project is an Android port inspired by [Il2CppDumper](https://github.com/perfare/Il2CppDumper) by **Perfare**. The original project is a desktop tool for dumping IL2CPP metadata — this version reimplements the core dumping logic as a native Android application.

| Role | Name | Link |
|------|------|------|
| **Android App Developer** | springmusk | [GitHub](https://github.com/springmusk026) · [Telegram](https://t.me/springmusk) |
| **Original Il2CppDumper** | Perfare | [GitHub](https://github.com/perfare/Il2CppDumper) |
| **Telegram Channel** | @Layout_musk | [t.me/Layout_musk](https://t.me/Layout_musk) |

## Contributing

Contributions are welcome! Please read the [Contributing Guidelines](CONTRIBUTING.md) before submitting a pull request.

- [Contributing Guidelines](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security Policy](SECURITY.md)

## License

This project is licensed under the **Springmusk Non-Commercial License** — see the [LICENSE](LICENSE) file for details.

**Key points:**
- Free to use, modify, and redistribute
- Attribution to **springmusk** and **Perfare** must be retained
- No commercial use, no ads, no subscriptions, no paywalls
- Must be distributed free of charge

## Disclaimer

This tool is intended for **educational and research purposes only**. Users are responsible for ensuring compliance with applicable laws and terms of service. The developers are not responsible for any misuse of this software.
