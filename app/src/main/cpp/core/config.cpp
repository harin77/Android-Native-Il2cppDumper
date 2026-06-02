#include "config.h"
#include <cstring>

namespace il2cpp_dumper {

// Minimal JSON parser for the simple config format
// The config JSON looks like:
// {"DumpMethod":true,"DumpField":true,...}
static bool parseBool(const char*& p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (strncmp(p, "true", 4) == 0) { p += 4; return true; }
    if (strncmp(p, "false", 5) == 0) { p += 5; return false; }
    return false;
}

static double parseDouble(const char*& p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    char* end;
    double val = strtod(p, &end);
    p = end;
    return val;
}

static std::string parseString(const char*& p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p != '"') return "";
    p++; // skip opening quote
    std::string result;
    while (*p && *p != '"') {
        if (*p == '\\') { p++; }
        result += *p;
        p++;
    }
    if (*p == '"') p++; // skip closing quote
    return result;
}

static void skipToColon(const char*& p) {
    while (*p && *p != ':') p++;
    if (*p == ':') p++;
}

static void skipValue(const char*& p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (*p == '"') {
        p++;
        while (*p && *p != '"') {
            if (*p == '\\') p++;
            p++;
        }
        if (*p == '"') p++;
    } else if (*p == '{') {
        int depth = 1;
        p++;
        while (*p && depth > 0) {
            if (*p == '{') depth++;
            else if (*p == '}') depth--;
            p++;
        }
    } else if (*p == '[') {
        int depth = 1;
        p++;
        while (*p && depth > 0) {
            if (*p == '[') depth++;
            else if (*p == ']') depth--;
            p++;
        }
    } else {
        while (*p && *p != ',' && *p != '}' && *p != ']' && *p != ' ')
            p++;
    }
}

Config Config::fromJson(const std::string& json) {
    Config config;
    const char* p = json.c_str();

    // Find opening brace
    while (*p && *p != '{') p++;
    if (!*p) return config;
    p++;

    while (*p && *p != '}') {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ',') p++;
        if (*p == '}') break;
        if (*p != '"') { skipValue(p); continue; }
        p++; // skip opening quote
        std::string key;
        while (*p && *p != '"') {
            key += *p;
            p++;
        }
        if (*p == '"') p++;
        skipToColon(p);

        if (key == "DumpMethod") config.dumpMethod = parseBool(p);
        else if (key == "DumpField") config.dumpField = parseBool(p);
        else if (key == "DumpProperty") config.dumpProperty = parseBool(p);
        else if (key == "DumpAttribute") config.dumpAttribute = parseBool(p);
        else if (key == "DumpFieldOffset") config.dumpFieldOffset = parseBool(p);
        else if (key == "DumpMethodOffset") config.dumpMethodOffset = parseBool(p);
        else if (key == "DumpTypeDefIndex") config.dumpTypeDefIndex = parseBool(p);
        else if (key == "GenerateDummyDll") config.generateDummyDll = false; // Not supported
        else if (key == "GenerateStruct") config.generateStruct = parseBool(p);
        else if (key == "DummyDllAddToken") config.dummyDllAddToken = parseBool(p);
        else if (key == "RequireAnyKey") { parseBool(p); } // Skip
        else if (key == "ForceIl2CppVersion") config.forceIl2CppVersion = parseBool(p);
        else if (key == "ForceVersion") config.forceVersion = parseDouble(p);
        else if (key == "ForceDump") config.forceDump = parseBool(p);
        else if (key == "NoRedirectedPointer") config.noRedirectedPointer = parseBool(p);
        else skipValue(p);
    }
    return config;
}

} // namespace il2cpp_dumper
