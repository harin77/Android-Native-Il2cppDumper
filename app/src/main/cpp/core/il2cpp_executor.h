#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include "metadata.h"
#include "il2cpp.h"
#include "blob_value.h"

namespace il2cpp_dumper {

class Il2CppExecutor {
public:
    Metadata& metadata;
    Il2CppEngine& il2Cpp;

    Il2CppExecutor(Metadata& metadata, Il2CppEngine& il2Cpp);

    std::vector<uint64_t> customAttributeGenerators;

    std::string getTypeName(const Il2CppType& il2CppType, bool addNamespace, bool isNested);
    std::string getTypeDefName(const Il2CppTypeDefinition& typeDef, bool addNamespace, bool genericParameter);
    std::string getGenericInstParams(const Il2CppGenericInst& genericInst);
    std::string getGenericContainerParams(const Il2CppGenericContainer& genericContainer);
    std::pair<std::string, std::string> getMethodSpecName(const Il2CppMethodSpec& methodSpec, bool addNamespace = false);

    Il2CppGenericClass getGenericClassTypeDefinition(const Il2CppGenericClass& genericClass);
    Il2CppTypeDefinition getTypeDefinitionFromIl2CppType(const Il2CppType& il2CppType);
    Il2CppGenericParameter getGenericParameterFromIl2CppType(const Il2CppType& il2CppType);

    SectionHelper* getSectionHelper();

    bool tryGetDefaultValue(int typeIndex, int dataIndex, BlobValue& value);
    bool getConstantValueFromBlob(Il2CppTypeEnum type, BinaryStream& reader, BlobValue& value);

private:
    static const std::unordered_map<int, std::string> typeStringMap;
};

} // namespace il2cpp_dumper
