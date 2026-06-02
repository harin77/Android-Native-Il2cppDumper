#include "il2cpp_executor.h"
#include "section_helper.h"
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

const std::unordered_map<int, std::string> Il2CppExecutor::typeStringMap = {
    {1, "void"}, {2, "bool"}, {3, "char"}, {4, "sbyte"}, {5, "byte"},
    {6, "short"}, {7, "ushort"}, {8, "int"}, {9, "uint"},
    {10, "long"}, {11, "ulong"}, {12, "float"}, {13, "double"},
    {14, "string"}, {22, "TypedReference"}, {24, "IntPtr"}, {25, "UIntPtr"}, {28, "object"}
};

Il2CppExecutor::Il2CppExecutor(Metadata& metadata, Il2CppEngine& il2Cpp)
    : metadata(metadata), il2Cpp(il2Cpp) {
    if (il2Cpp.version >= 27 && il2Cpp.version < 29) {
        int total = 0;
        for (auto& img : metadata.imageDefs) total += img.customAttributeCount;
        customAttributeGenerators.resize(total, 0);
        for (auto& imageDef : metadata.imageDefs) {
            auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
            auto it = il2Cpp.codeGenModules.find(imageName);
            if (it != il2Cpp.codeGenModules.end() && imageDef.customAttributeCount > 0) {
                auto pointers = il2Cpp.readPrimitiveArray<uint64_t>(
                    il2Cpp.mapVATR(it->second.customAttributeCacheGenerator),
                    imageDef.customAttributeCount);
                std::copy(pointers.begin(), pointers.end(),
                    customAttributeGenerators.begin() + imageDef.customAttributeStart);
            }
        }
    } else if (il2Cpp.version < 27) {
        customAttributeGenerators = il2Cpp.customAttributeGenerators;
    }
}

std::string Il2CppExecutor::getTypeName(const Il2CppType& il2CppType, bool addNamespace, bool isNested) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY: {
            auto arrayType = il2Cpp.readClass<Il2CppArrayType>(il2Cpp.mapVATR(il2CppType.array()));
            auto& elementType = il2Cpp.types[il2Cpp.getIl2CppType(arrayType.etype) ?
                il2Cpp.getTypeIndex(arrayType.etype) : 0];
            std::string suffix = "[";
            for (int i = 1; i < arrayType.rank; i++) suffix += ",";
            suffix += "]";
            return getTypeName(elementType, addNamespace, false) + suffix;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY: {
            auto idx = il2Cpp.getTypeIndex(il2CppType.typeRef());
            if (idx != SIZE_MAX) {
                return getTypeName(il2Cpp.types[idx], addNamespace, false) + "[]";
            }
            return "object[]";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_PTR: {
            auto idx = il2Cpp.getTypeIndex(il2CppType.typeRef());
            if (idx != SIZE_MAX) {
                return getTypeName(il2Cpp.types[idx], addNamespace, false) + "*";
            }
            return "void*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_VAR:
        case Il2CppTypeEnum::IL2CPP_TYPE_MVAR: {
            auto param = getGenericParameterFromIl2CppType(il2CppType);
            return metadata.getStringFromIndex(param.nameIndex);
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            std::string str;
            Il2CppTypeDefinition typeDef{};
            Il2CppGenericClass genericClass{};
            bool hasGenericClass = false;

            if (il2CppType.type == Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST) {
                genericClass = il2Cpp.readClass<Il2CppGenericClass>(il2Cpp.mapVATR(il2CppType.generic_class()));
                hasGenericClass = true;
                if (il2Cpp.version >= 27) {
                    auto typeIdx = il2Cpp.getTypeIndex(genericClass.type);
                    if (typeIdx != SIZE_MAX && typeIdx < il2Cpp.types.size()) {
                        auto& t = il2Cpp.types[typeIdx];
                        typeDef = getTypeDefinitionFromIl2CppType(t);
                    }
                } else {
                    if (genericClass.typeDefinitionIndex >= 0 &&
                        static_cast<size_t>(genericClass.typeDefinitionIndex) < metadata.typeDefs.size())
                        typeDef = metadata.typeDefs[genericClass.typeDefinitionIndex];
                }
            } else {
                typeDef = getTypeDefinitionFromIl2CppType(il2CppType);
            }

            if (typeDef.declaringTypeIndex != -1) {
                str += getTypeName(il2Cpp.types[typeDef.declaringTypeIndex], addNamespace, true) + ".";
            } else if (addNamespace) {
                auto ns = metadata.getStringFromIndex(typeDef.namespaceIndex);
                if (!ns.empty()) str += ns + ".";
            }

            auto typeName = metadata.getStringFromIndex(typeDef.nameIndex);
            if (typeName.empty() && typeDef.nameIndex != 0) {
                static int emptyNameCount = 0;
                if (emptyNameCount < 3) {
                    LOGE("getTypeName: empty name for typeDef, nameIndex=%u, namespaceIndex=%u",
                         typeDef.nameIndex, typeDef.namespaceIndex);
                    emptyNameCount++;
                }
            }
            auto tickPos = typeName.find('`');
            if (tickPos != std::string::npos)
                str += typeName.substr(0, tickPos);
            else
                str += typeName;

            if (isNested) return str;

            if (hasGenericClass && genericClass.context_class_inst != 0) {
                auto genericInst = il2Cpp.readClass<Il2CppGenericInst>(il2Cpp.mapVATR(genericClass.context_class_inst));
                str += getGenericInstParams(genericInst);
            } else if (typeDef.genericContainerIndex >= 0) {
                auto& container = metadata.genericContainers[typeDef.genericContainerIndex];
                str += getGenericContainerParams(container);
            }

            return str;
        }
        default: {
            auto it = typeStringMap.find(static_cast<int>(il2CppType.type));
            if (it != typeStringMap.end()) return it->second;
            return "unknown";
        }
    }
}

std::string Il2CppExecutor::getTypeDefName(const Il2CppTypeDefinition& typeDef, bool addNamespace, bool genericParameter) {
    std::string prefix;
    if (typeDef.declaringTypeIndex != -1) {
        prefix = getTypeName(il2Cpp.types[typeDef.declaringTypeIndex], addNamespace, true) + ".";
    } else if (addNamespace) {
        auto ns = metadata.getStringFromIndex(typeDef.namespaceIndex);
        if (!ns.empty()) prefix = ns + ".";
    }
    auto typeName = metadata.getStringFromIndex(typeDef.nameIndex);
    if (typeDef.genericContainerIndex >= 0) {
        auto tickPos = typeName.find('`');
        if (tickPos != std::string::npos) typeName = typeName.substr(0, tickPos);
        if (genericParameter) {
            auto& container = metadata.genericContainers[typeDef.genericContainerIndex];
            typeName += getGenericContainerParams(container);
        }
    }
    return prefix + typeName;
}

std::string Il2CppExecutor::getGenericInstParams(const Il2CppGenericInst& genericInst) {
    std::vector<std::string> names;
    auto pointers = il2Cpp.readPrimitiveArray<uint64_t>(
        il2Cpp.mapVATR(genericInst.type_argv), genericInst.type_argc);
    for (auto ptr : pointers) {
        auto idx = il2Cpp.getTypeIndex(ptr);
        if (idx != SIZE_MAX) {
            names.push_back(getTypeName(il2Cpp.types[idx], false, false));
        }
    }
    std::string result = "<";
    for (size_t i = 0; i < names.size(); i++) {
        if (i > 0) result += ", ";
        result += names[i];
    }
    result += ">";
    return result;
}

std::string Il2CppExecutor::getGenericContainerParams(const Il2CppGenericContainer& container) {
    std::vector<std::string> names;
    for (int i = 0; i < container.type_argc; i++) {
        auto idx = container.genericParameterStart + i;
        if (idx >= 0 && static_cast<size_t>(idx) < metadata.genericParameters.size()) {
            names.push_back(metadata.getStringFromIndex(metadata.genericParameters[idx].nameIndex));
        }
    }
    std::string result = "<";
    for (size_t i = 0; i < names.size(); i++) {
        if (i > 0) result += ", ";
        result += names[i];
    }
    result += ">";
    return result;
}

std::pair<std::string, std::string> Il2CppExecutor::getMethodSpecName(const Il2CppMethodSpec& methodSpec, bool addNamespace) {
    auto& methodDef = metadata.methodDefs[methodSpec.methodDefinitionIndex];
    auto& typeDef = metadata.typeDefs[methodDef.declaringType];
    auto typeName = getTypeDefName(typeDef, addNamespace, false);
    if (methodSpec.classIndexIndex != -1 &&
        static_cast<size_t>(methodSpec.classIndexIndex) < il2Cpp.genericInsts.size()) {
        typeName += getGenericInstParams(il2Cpp.genericInsts[methodSpec.classIndexIndex]);
    }
    auto methodName = metadata.getStringFromIndex(methodDef.nameIndex);
    if (methodSpec.methodIndexIndex != -1 &&
        static_cast<size_t>(methodSpec.methodIndexIndex) < il2Cpp.genericInsts.size()) {
        methodName += getGenericInstParams(il2Cpp.genericInsts[methodSpec.methodIndexIndex]);
    }
    return {typeName, methodName};
}

Il2CppTypeDefinition Il2CppExecutor::getTypeDefinitionFromIl2CppType(const Il2CppType& il2CppType) {
    if (il2Cpp.version >= 27 && il2Cpp.isDumped) {
        auto offset = il2CppType.typeHandle() - metadata.imageBase - metadata.header.typeDefinitionsOffset;
        auto idx = offset / metadata.sizeOfStruct<Il2CppTypeDefinition>();
        if (idx >= 0 && static_cast<size_t>(idx) < metadata.typeDefs.size())
            return metadata.typeDefs[idx];
        else {
            static int errCount = 0;
            if (errCount < 3) {
                LOGE("getTypeDefFromType: offset=0x%llx, idx=%lld, typeDefs.size=%zu, imageBase=0x%llx",
                     (unsigned long long)offset, (long long)idx, metadata.typeDefs.size(),
                     (unsigned long long)metadata.imageBase);
                errCount++;
            }
        }
    } else {
        auto idx = il2CppType.klassIndex();
        if (idx >= 0 && static_cast<size_t>(idx) < metadata.typeDefs.size())
            return metadata.typeDefs[idx];
    }
    return {};
}

Il2CppGenericParameter Il2CppExecutor::getGenericParameterFromIl2CppType(const Il2CppType& il2CppType) {
    if (il2Cpp.version >= 27 && il2Cpp.isDumped) {
        auto offset = il2CppType.genericParameterHandle() - metadata.imageBase - metadata.header.genericParametersOffset;
        auto idx = offset / metadata.sizeOfStruct<Il2CppGenericParameter>();
        if (idx >= 0 && static_cast<size_t>(idx) < metadata.genericParameters.size())
            return metadata.genericParameters[idx];
    } else {
        auto idx = il2CppType.genericParameterIndex();
        if (idx >= 0 && static_cast<size_t>(idx) < metadata.genericParameters.size())
            return metadata.genericParameters[idx];
    }
    return {};
}

SectionHelper* Il2CppExecutor::getSectionHelper() {
    int methodCount = 0;
    for (auto& md : metadata.methodDefs) {
        if (md.methodIndex >= 0) methodCount++;
    }
    return il2Cpp.getSectionHelper(methodCount, metadata.typeDefs.size(), metadata.imageDefs.size());
}

bool Il2CppExecutor::tryGetDefaultValue(int typeIndex, int dataIndex, BlobValue& value) {
    auto pointer = metadata.getDefaultValueFromIndex(1);
    auto& defaultValueType = il2Cpp.types[typeIndex];
    // Simplified - would need full blob reading
    value.il2cppTypeEnum = defaultValueType.type;
    return false;
}

bool Il2CppExecutor::getConstantValueFromBlob(Il2CppTypeEnum type, BinaryStream& reader, BlobValue& value) {
    value.il2cppTypeEnum = type;
    switch (type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
            value.value = reader.readBoolean();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U1:
            value.value = reader.readByte();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I1:
            value.value = reader.readSByte();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U2:
            value.value = reader.readUInt16();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I2:
            value.value = reader.readInt16();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U4:
            if (il2Cpp.version >= 29) value.value = reader.readCompressedUInt32();
            else value.value = reader.readUInt32();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I4:
            if (il2Cpp.version >= 29) value.value = reader.readCompressedInt32();
            else value.value = reader.readInt32();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_U8:
            value.value = reader.readUInt64();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_I8:
            value.value = reader.readInt64();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_R4:
            value.value = reader.readFloat();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_R8:
            value.value = reader.readDouble();
            return true;
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING: {
            int length;
            if (il2Cpp.version >= 29) {
                length = reader.readCompressedInt32();
                if (length == -1) { value.value = nullptr; return true; }
            } else {
                length = reader.readInt32();
            }
            value.value = reader.readString(length);
            return true;
        }
        default:
            return false;
    }
}

} // namespace il2cpp_dumper
