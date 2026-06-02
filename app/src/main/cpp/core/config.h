#pragma once

#include <string>

namespace il2cpp_dumper {

struct Config {
    bool dumpMethod = true;
    bool dumpField = true;
    bool dumpProperty = false;
    bool dumpAttribute = false;
    bool dumpFieldOffset = true;
    bool dumpMethodOffset = true;
    bool dumpTypeDefIndex = true;
    bool generateDummyDll = false;  // Not supported in Android port
    bool generateStruct = true;
    bool dummyDllAddToken = true;
    bool requireAnyKey = false;     // Not applicable on Android
    bool forceIl2CppVersion = false;
    double forceVersion = 24.3;
    bool forceDump = false;
    bool noRedirectedPointer = false;

    // Parse from JSON string (minimal parser for the simple config format)
    static Config fromJson(const std::string& json);
};

} // namespace il2cpp_dumper
