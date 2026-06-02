#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "il2cpp_executor.h"
#include "config.h"

namespace il2cpp_dumper {

class Il2CppDecompiler {
public:
    Il2CppDecompiler(Il2CppExecutor& executor);

    void decompile(const Config& config, const std::string& outputDir);

private:
    Il2CppExecutor& executor;
    Metadata& metadata;
    Il2CppEngine& il2Cpp;
    std::unordered_map<int, std::string> methodModifiers;

    std::string getCustomAttribute(const Il2CppImageDefinition& imageDef,
                                    int customAttributeIndex, uint32_t token,
                                    const std::string& padding = "");
    std::string getModifiers(const Il2CppMethodDefinition& methodDef);
};

} // namespace il2cpp_dumper
