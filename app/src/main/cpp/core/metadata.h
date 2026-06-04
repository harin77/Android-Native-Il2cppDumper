#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "binary_stream.h"
#include "metadata_class.h"

namespace il2cpp_dumper {

class Metadata : public BinaryStream {
public:
    Metadata(const uint8_t* data, size_t size);
    Metadata(std::vector<uint8_t>&& data);

    Il2CppGlobalMetadataHeader header{};
    std::vector<Il2CppImageDefinition> imageDefs;
    std::vector<Il2CppAssemblyDefinition> assemblyDefs;
    std::vector<Il2CppTypeDefinition> typeDefs;
    std::vector<Il2CppMethodDefinition> methodDefs;
    std::vector<Il2CppParameterDefinition> parameterDefs;
    std::vector<Il2CppFieldDefinition> fieldDefs;
    std::vector<Il2CppPropertyDefinition> propertyDefs;
    std::vector<Il2CppCustomAttributeTypeRange> attributeTypeRanges;
    std::vector<Il2CppCustomAttributeDataRange> attributeDataRanges;
    std::vector<Il2CppStringLiteral> stringLiterals;
    std::vector<int32_t> attributeTypes;
    std::vector<int32_t> interfaceIndices;
    std::vector<int32_t> nestedTypeIndices;
    std::vector<Il2CppEventDefinition> eventDefs;
    std::vector<Il2CppGenericContainer> genericContainers;
    std::vector<Il2CppFieldRef> fieldRefs;
    std::vector<Il2CppGenericParameter> genericParameters;
    std::vector<int32_t> constraintIndices;
    std::vector<uint32_t> vtableMethods;
    std::vector<Il2CppRGCTXDefinition> rgctxEntries;
    int64_t metadataUsagesCount = 0;
    uint64_t imageBase = 0;  // For dump files: calculated from type handles

    // Metadata usage dictionary: usage type -> (destination index -> decoded index)
    std::unordered_map<int, std::map<uint32_t, uint32_t>> metadataUsageDic;

    // Custom attribute type ranges dictionary: image -> (token -> index)
    std::unordered_map<int, std::unordered_map<uint32_t, int>> attributeTypeRangesDic;

    bool getFieldDefaultValueFromIndex(int index, Il2CppFieldDefaultValue& value) const;
    bool getParameterDefaultValueFromIndex(int index, Il2CppParameterDefaultValue& value) const;
    uint32_t getDefaultValueFromIndex(int index) const;
    std::string getStringFromIndex(uint32_t index);
    int getCustomAttributeIndex(const Il2CppImageDefinition& imageDef, int customAttributeIndex, uint32_t token) const;
    std::string getStringLiteralFromIndex(uint32_t index);

    static uint32_t getEncodedIndexType(uint32_t index);
    uint32_t getDecodedMethodIndex(uint32_t index) const;

    int sizeOf(const std::string& typeName) const;

    template<typename T>
    int sizeOfStruct() const;

    // Explicit declarations for template specializations (defined in metadata.cpp)
    template<> int sizeOfStruct<Il2CppImageDefinition>() const;
    template<> int sizeOfStruct<Il2CppAssemblyDefinition>() const;
    template<> int sizeOfStruct<Il2CppTypeDefinition>() const;
    template<> int sizeOfStruct<Il2CppMethodDefinition>() const;
    template<> int sizeOfStruct<Il2CppParameterDefinition>() const;
    template<> int sizeOfStruct<Il2CppFieldDefinition>() const;
    template<> int sizeOfStruct<Il2CppFieldDefaultValue>() const;
    template<> int sizeOfStruct<Il2CppPropertyDefinition>() const;
    template<> int sizeOfStruct<Il2CppCustomAttributeTypeRange>() const;
    template<> int sizeOfStruct<Il2CppMetadataUsageList>() const;
    template<> int sizeOfStruct<Il2CppMetadataUsagePair>() const;
    template<> int sizeOfStruct<Il2CppStringLiteral>() const;
    template<> int sizeOfStruct<Il2CppParameterDefaultValue>() const;
    template<> int sizeOfStruct<Il2CppEventDefinition>() const;
    template<> int sizeOfStruct<Il2CppGenericContainer>() const;
    template<> int sizeOfStruct<Il2CppFieldRef>() const;
    template<> int sizeOfStruct<Il2CppGenericParameter>() const;
    template<> int sizeOfStruct<Il2CppRGCTXDefinition>() const;
    template<> int sizeOfStruct<Il2CppCustomAttributeDataRange>() const;

private:
    std::unordered_map<int, Il2CppFieldDefaultValue> fieldDefaultValuesDic;
    std::unordered_map<int, Il2CppParameterDefaultValue> parameterDefaultValuesDic;
    std::unordered_map<uint32_t, std::string> stringCache;

    void initialize();

    template<typename T>
    std::vector<T> readMetadataClassArray(uint32_t addr, int32_t size);

    void processingMetadataUsage(const std::vector<Il2CppMetadataUsageList>& usageLists,
                                 const std::vector<Il2CppMetadataUsagePair>& usagePairs);
};

} // namespace il2cpp_dumper
