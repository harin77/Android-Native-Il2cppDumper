#include "struct_generator.h"
#include "header_constants.h"
#include "il2cpp_constants.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <regex>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

const std::unordered_set<std::string> StructGenerator::keywords = {
    "klass", "monitor", "register", "_cs", "auto", "friend", "template", "flat", "default",
    "_ds", "interrupt", "unsigned", "signed", "asm", "if", "case", "break", "continue",
    "do", "new", "_", "short", "union", "class", "namespace"
};

const std::unordered_set<std::string> StructGenerator::specialKeywords = {
    "inline", "near", "far"
};

StructGenerator::StructGenerator(Il2CppExecutor& executor)
    : executor(executor), metadata(executor.metadata), il2Cpp(executor.il2Cpp) {}

std::string StructGenerator::fixName(const std::string& str) {
    if (keywords.count(str)) return "_" + str;
    if (specialKeywords.count(str)) return "_" + str + "_";
    if (!str.empty() && str[0] >= '0' && str[0] <= '9') return "_" + str;
    std::string result = str;
    for (auto& c : result) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            c = '_';
    }
    return result;
}

void StructGenerator::writeScript(const std::string& outputDir) {
    LOGI("Generating struct info...");
    ScriptJson json;
    json.scriptMethod.reserve(metadata.methodDefs.size());
    structInfoList.reserve(metadata.typeDefs.size());

    // Create struct name dictionary
    for (size_t imageIndex = 0; imageIndex < metadata.imageDefs.size(); imageIndex++) {
        auto& imageDef = metadata.imageDefs[imageIndex];
        auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
        auto typeEnd = imageDef.typeStart + imageDef.typeCount;
        for (int typeIndex = imageDef.typeStart; typeIndex < typeEnd; typeIndex++) {
            typeDefImageNames[typeIndex] = imageName;
            createStructNameDic(metadata.typeDefs[typeIndex], typeIndex);
        }
    }

    // Process generic instances
    for (size_t i = 0; i < il2Cpp.types.size(); i++) {
        auto& il2CppType = il2Cpp.types[i];
        if (il2CppType.type != Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST) continue;
        auto genericClass = il2Cpp.readClass<Il2CppGenericClass>(il2Cpp.mapVATR(il2CppType.generic_class()));
        // Process generic class struct names
        auto typeStructName = structNameDic.count(genericClass.typeDefinitionIndex) ?
            structNameDic[genericClass.typeDefinitionIndex] : "Unknown";
        genericClassStructNameDic[il2CppType.generic_class()] = typeStructName;
    }

    // Process functions
    for (auto& imageDef : metadata.imageDefs) {
        auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
        auto typeEnd = imageDef.typeStart + imageDef.typeCount;
        for (int typeIndex = imageDef.typeStart; typeIndex < typeEnd; typeIndex++) {
            auto& typeDef = metadata.typeDefs[typeIndex];
            addStruct(typeDef, typeIndex);
            auto typeName = executor.getTypeDefName(typeDef, true, true);
            auto methodEnd = typeDef.methodStart + typeDef.method_count;
            for (int i = typeDef.methodStart; i < methodEnd; i++) {
                auto& methodDef = metadata.methodDefs[i];
                auto methodPointer = il2Cpp.getMethodPointer(imageName, methodDef);
                if (methodPointer > 0) {
                    ScriptMethod sm;
                    sm.address = il2Cpp.getRVA(methodPointer);
                    auto methodName = metadata.getStringFromIndex(methodDef.nameIndex);
                    sm.name = typeName + "$$" + methodName;

                    auto& returnType = il2Cpp.types[methodDef.returnType];
                    auto retStr = parseType(returnType);
                    if (returnType.byref == 1) retStr += "*";

                    std::string signature = retStr + " " + fixName(sm.name) + " (";
                    std::vector<std::string> paramStrs;

                    // __this parameter
                    if (!(methodDef.flags & METHOD_ATTRIBUTE_STATIC)) {
                        auto thisType = parseType(il2Cpp.types[typeDef.byvalTypeIndex]);
                        paramStrs.push_back(thisType + " __this");
                    } else if (il2Cpp.version <= 24) {
                        paramStrs.push_back("Il2CppObject* __this");
                    }

                    // Method parameters
                    for (int j = 0; j < methodDef.parameterCount; j++) {
                        auto& paramDef = metadata.parameterDefs[methodDef.parameterStart + j];
                        auto paramName = fixName(metadata.getStringFromIndex(paramDef.nameIndex));
                        auto& paramType = il2Cpp.types[paramDef.typeIndex];
                        auto paramCType = parseType(paramType);
                        if (paramType.byref == 1) paramCType += "*";
                        paramStrs.push_back(paramCType + " " + paramName);
                    }

                    paramStrs.push_back("const MethodInfo* method");

                    for (size_t j = 0; j < paramStrs.size(); j++) {
                        if (j > 0) signature += ", ";
                        signature += paramStrs[j];
                    }
                    signature += ");";
                    sm.signature = signature;
                    json.scriptMethod.push_back(sm);
                }
            }
        }
    }

    // Write script.json
    {
        auto path = outputDir + "/script.json";
        std::ofstream f(path, std::ios::binary);
        char buf[65536];
        f.rdbuf()->pubsetbuf(buf, sizeof(buf));
        f << "{\n  \"ScriptMethod\": [\n";
        for (size_t i = 0; i < json.scriptMethod.size(); i++) {
            auto& m = json.scriptMethod[i];
            f << "    {\"Address\": " << m.address << ", \"Name\": \"" << m.name
              << "\", \"Signature\": \"" << m.signature << "\"}";
            if (i + 1 < json.scriptMethod.size()) f << ",";
            f << "\n";
        }
        f << "  ]\n}\n";
        f.close();
        LOGI("script.json written: %zu methods", json.scriptMethod.size());
    }

    // Write stringliteral.json
    {
        auto path = outputDir + "/stringliteral.json";
        std::ofstream f(path, std::ios::binary);
        char buf[65536];
        f.rdbuf()->pubsetbuf(buf, sizeof(buf));
        f << "[\n";

        // Collect string literals with addresses from metadata usages
        struct StringLiteralEntry { uint64_t address; std::string value; };
        std::vector<StringLiteralEntry> entries;

        if (il2Cpp.version > 16 && il2Cpp.version < 27) {
            // Use metadataUsageDic for versions 16-26
            auto it = metadata.metadataUsageDic.find(5); // kIl2CppMetadataUsageStringLiteral = 5
            if (it != metadata.metadataUsageDic.end()) {
                for (auto& [destIndex, decodedIndex] : it->second) {
                    if (destIndex < il2Cpp.metadataUsages.size() &&
                        decodedIndex < metadata.stringLiterals.size()) {
                        auto address = il2Cpp.metadataUsages[destIndex];
                        if (address > 0) {
                            StringLiteralEntry entry;
                            entry.address = il2Cpp.getRVA(address);
                            try {
                                entry.value = metadata.getStringLiteralFromIndex(decodedIndex);
                            } catch (...) { continue; }
                            entries.push_back(entry);
                        }
                    }
                }
            }
        }

        // Fallback: if no metadata usages found (v27+ or empty), dump all with index
        if (entries.empty()) {
            for (size_t i = 0; i < metadata.stringLiterals.size(); i++) {
                try {
                    StringLiteralEntry entry;
                    entry.address = 0;
                    entry.value = metadata.getStringLiteralFromIndex(static_cast<uint32_t>(i));
                    entries.push_back(entry);
                } catch (...) {}
            }
        }

        for (size_t i = 0; i < entries.size(); i++) {
            f << "  {\"value\": \"";
            for (unsigned char c : entries[i].value) {
                switch (c) {
                    case '"': f << "\\\""; break;
                    case '\\': f << "\\\\"; break;
                    case '\n': f << "\\n"; break;
                    case '\r': f << "\\r"; break;
                    case '\t': f << "\\t"; break;
                    case '\b': f << "\\b"; break;
                    case '\f': f << "\\f"; break;
                    default:
                        if (c < 0x20) {
                            char hex[8];
                            snprintf(hex, sizeof(hex), "\\u%04x", c);
                            f << hex;
                        } else {
                            f << static_cast<char>(c);
                        }
                        break;
                }
            }
            if (entries[i].address > 0)
                f << "\", \"address\": \"0x" << std::hex << entries[i].address << std::dec << "\"}";
            else
                f << "\", \"address\": \"0x0\"}";
            if (i + 1 < entries.size()) f << ",";
            f << "\n";
        }
        f << "]\n";
        f.close();
        LOGI("stringliteral.json written: %zu entries", entries.size());
    }

    // Write il2cpp.h
    {
        auto path = outputDir + "/il2cpp.h";
        std::ofstream f(path, std::ios::binary);
        char buf[65536];
        f.rdbuf()->pubsetbuf(buf, sizeof(buf));
        f << HeaderConstants::GenericHeader;
        auto ver = il2Cpp.version;
        if (ver == 22) f << HeaderConstants::HeaderV22;
        else if (ver == 23 || ver == 24) f << HeaderConstants::HeaderV240;
        else if (ver == 24.1) f << HeaderConstants::HeaderV241;
        else if (ver >= 24.2 && ver <= 24.5) f << HeaderConstants::HeaderV242;
        else if (ver >= 27 && ver <= 27.2) f << HeaderConstants::HeaderV27;
        else if (ver >= 29) f << HeaderConstants::HeaderV29;
        else {
            LOGI("WARNING: il2cpp version %.1f does not support generating .h files", ver);
            f.close();
            return;
        }

        // Build lookup map and use RecursionStructInfo for proper ordering
        for (auto& info : structInfoList) {
            structInfoWithStructName[info.typeName + "_o"] = &info;
        }
        structCache.clear();
        for (auto& info : structInfoList) {
            recursionStructInfoToStream(info, f);
        }

        f.close();
        LOGI("il2cpp.h written: %zu structs", structInfoList.size());
    }

    LOGI("Struct generation complete");
}

// Simplified implementations for remaining methods
std::string StructGenerator::parseType(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VOID: return "void";
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN: return "bool";
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR: return "uint16_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I1: return "int8_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U1: return "uint8_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I2: return "int16_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U2: return "uint16_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I4: return "int32_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U4: return "uint32_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_I8: return "int64_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U8: return "uint64_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_R4: return "float";
        case Il2CppTypeEnum::IL2CPP_TYPE_R8: return "double";
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING: return "System_String_o*";
        case Il2CppTypeEnum::IL2CPP_TYPE_I: return "intptr_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_U: return "uintptr_t";
        case Il2CppTypeEnum::IL2CPP_TYPE_OBJECT: return "Il2CppObject*";
        case Il2CppTypeEnum::IL2CPP_TYPE_TYPEDBYREF: return "Il2CppObject*";
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE: {
            auto typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
            if (typeDef.isEnum()) {
                // For enums, return the underlying type
                auto& elemType = il2Cpp.types[typeDef.elementTypeIndex];
                return parseType(elemType, context);
            }
            auto structName = getIl2CppStructName(il2CppType, context);
            return structName + "_o";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS: {
            auto structName = getIl2CppStructName(il2CppType, context);
            return structName + "_o*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY: {
            return "Il2CppArray*";
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_GENERICINST: {
            auto structName = getIl2CppStructName(il2CppType, context);
            auto typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
            if (typeDef.isValueType()) {
                if (typeDef.isEnum()) {
                    auto& elemType = il2Cpp.types[typeDef.elementTypeIndex];
                    return parseType(elemType, context);
                }
                return structName + "_o";
            }
            return structName + "_o*";
        }
        default: return "void*";
    }
}

std::string StructGenerator::getIl2CppStructName(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS: {
            if (il2Cpp.version >= 27 && il2Cpp.isDumped) {
                auto typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
                uint64_t key = (static_cast<uint64_t>(typeDef.nameIndex) << 32) | typeDef.namespaceIndex;
                auto idxIt = typeIdentityToIndex.find(key);
                if (idxIt != typeIdentityToIndex.end()) {
                    auto nameIt = structNameDic.find(idxIt->second);
                    if (nameIt != structNameDic.end()) return nameIt->second;
                }
            } else {
                auto idx = il2CppType.klassIndex();
                if (idx >= 0) {
                    auto it = structNameDic.find(static_cast<int>(idx));
                    if (it != structNameDic.end()) return it->second;
                }
            }
            return "System_Object";
        }
        default: return "System_Object";
    }
}

bool StructGenerator::isValueType(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    if (il2CppType.type == Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE) {
        auto typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
        return !typeDef.isEnum();
    }
    return false;
}

bool StructGenerator::isCustomType(const Il2CppType& il2CppType, const Il2CppGenericContext* context) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING:
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS:
        case Il2CppTypeEnum::IL2CPP_TYPE_ARRAY:
        case Il2CppTypeEnum::IL2CPP_TYPE_SZARRAY:
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
            return true;
        default:
            return false;
    }
}

void StructGenerator::createStructNameDic(const Il2CppTypeDefinition& typeDef, int typeDefIndex) {
    auto typeName = executor.getTypeDefName(typeDef, true, true);
    auto typeStructName = fixName(typeName);
    auto uniqueName = getUniqueName(typeStructName);
    structNameDic[typeDefIndex] = uniqueName;
    uint64_t key = (static_cast<uint64_t>(typeDef.nameIndex) << 32) | typeDef.namespaceIndex;
    typeIdentityToIndex.emplace(key, typeDefIndex);
}

std::string StructGenerator::getUniqueName(const std::string& name) {
    auto fixName = name;
    int i = 1;
    while (!structNameHashSet.insert(fixName).second) {
        fixName = name + "_" + std::to_string(i++);
    }
    return fixName;
}

void StructGenerator::addStruct(const Il2CppTypeDefinition& typeDef, int typeDefIndex) {
    StructInfo info;
    auto it = structNameDic.find(typeDefIndex);
    info.typeName = (it != structNameDic.end()) ? it->second : "Unknown";
    info.isValueType = typeDef.isValueType();
    addParents(typeDef, info);
    addFields(typeDef, info, nullptr);
    addVTableMethod(info, typeDef);
    addRGCTX(info, typeDef);
    structInfoList.push_back(info);
}

void StructGenerator::addGenericClassStruct(uint64_t /*pointer*/) {}

void StructGenerator::addParents(const Il2CppTypeDefinition& typeDef, StructInfo& info) {
    if (!typeDef.isValueType() && !typeDef.isEnum()) {
        if (typeDef.parentIndex >= 0) {
            auto& parent = il2Cpp.types[typeDef.parentIndex];
            if (parent.type != Il2CppTypeEnum::IL2CPP_TYPE_OBJECT) {
                info.parent = getIl2CppStructName(parent);
            }
        }
    }
}

void StructGenerator::addFields(const Il2CppTypeDefinition& typeDef, StructInfo& info, const Il2CppGenericContext* /*context*/) {
    if (typeDef.field_count > 0) {
        auto fieldEnd = typeDef.fieldStart + typeDef.field_count;
        std::unordered_set<std::string> cache;
        for (int i = typeDef.fieldStart; i < fieldEnd; i++) {
            auto& fieldDef = metadata.fieldDefs[i];
            auto& fieldType = il2Cpp.types[fieldDef.typeIndex];
            if (fieldType.attrs & FIELD_ATTRIBUTE_LITERAL) continue;
            StructFieldInfo fi;
            fi.fieldTypeName = parseType(fieldType);
            auto fieldName = fixName(metadata.getStringFromIndex(fieldDef.nameIndex));
            if (!cache.insert(fieldName).second) {
                fieldName = "_" + std::to_string(i - typeDef.fieldStart) + "_" + fieldName;
            }
            fi.fieldName = fieldName;
            fi.isValueType = isValueType(fieldType);
            fi.isCustomType = isCustomType(fieldType);
            if (fieldType.attrs & FIELD_ATTRIBUTE_STATIC)
                info.staticFields.push_back(fi);
            else
                info.fields.push_back(fi);
        }
    }
}

void StructGenerator::addVTableMethod(StructInfo& info, const Il2CppTypeDefinition& typeDef) {
    std::map<int, std::string> dic;
    for (int i = 0; i < typeDef.vtable_count; i++) {
        auto vTableIndex = typeDef.vtableStart + i;
        if (static_cast<size_t>(vTableIndex) >= metadata.vtableMethods.size()) break;
        auto encodedMethodIndex = metadata.vtableMethods[vTableIndex];
        auto usage = Metadata::getEncodedIndexType(encodedMethodIndex);
        auto index = metadata.getDecodedMethodIndex(encodedMethodIndex);
        int32_t slot = -1;
        std::string methodName;
        if (usage == 6 && static_cast<size_t>(index) < il2Cpp.methodSpecs.size()) {
            auto& methodSpec = il2Cpp.methodSpecs[index];
            if (static_cast<size_t>(methodSpec.methodDefinitionIndex) < metadata.methodDefs.size()) {
                auto& methodDef = metadata.methodDefs[methodSpec.methodDefinitionIndex];
                slot = methodDef.slot;
                methodName = fixName(metadata.getStringFromIndex(methodDef.nameIndex));
            }
        } else if (static_cast<size_t>(index) < metadata.methodDefs.size()) {
            auto& methodDef = metadata.methodDefs[index];
            slot = methodDef.slot;
            methodName = fixName(metadata.getStringFromIndex(methodDef.nameIndex));
        }
        if (slot >= 0 && slot != 0xFFFF) {
            dic[slot] = methodName;
        }
    }
    if (!dic.empty()) {
        int maxSlot = dic.rbegin()->first;
        info.vTableMethod.resize(maxSlot + 1);
        for (auto& [slot, name] : dic) {
            info.vTableMethod[slot].methodName = name;
        }
    }
}

void StructGenerator::addRGCTX(StructInfo& /*info*/, const Il2CppTypeDefinition& /*typeDef*/) {}
void StructGenerator::parseArrayClassStruct(const Il2CppType& /*il2CppType*/, const Il2CppGenericContext* /*context*/) {}

std::string StructGenerator::recursionStructInfo(StructInfo& info) {
    std::ostringstream out;
    recursionStructInfoToStream(info, out);
    return out.str();
}

void StructGenerator::recursionStructInfoToStream(StructInfo& info, std::ostream& out) {
    if (!structCache.insert(reinterpret_cast<size_t>(&info)).second) {
        return;
    }

    // Resolve parent dependency first
    if (!info.parent.empty()) {
        auto parentKey = info.parent + "_o";
        auto it = structInfoWithStructName.find(parentKey);
        if (it != structInfoWithStructName.end()) {
            recursionStructInfoToStream(*it->second, out);
        }
        out << "struct " << info.typeName << "_Fields : " << info.parent << "_Fields {\n";
    } else {
        out << "struct " << info.typeName << "_Fields {\n";
    }

    // Fields
    for (auto& field : info.fields) {
        if (field.isValueType) {
            auto it = structInfoWithStructName.find(field.fieldTypeName);
            if (it != structInfoWithStructName.end()) {
                recursionStructInfoToStream(*it->second, out);
            }
        }
        if (field.isCustomType)
            out << "\tstruct " << field.fieldTypeName << " " << field.fieldName << ";\n";
        else
            out << "\t" << field.fieldTypeName << " " << field.fieldName << ";\n";
    }
    out << "};\n";

    // RGCTXs
    if (!info.rgctxs.empty()) {
        out << "struct " << info.typeName << "_RGCTXs {\n";
        for (size_t i = 0; i < info.rgctxs.size(); i++) {
            auto& rgctx = info.rgctxs[i];
            switch (rgctx.type) {
                case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_TYPE:
                    out << "\tIl2CppType* _" << i << "_" << rgctx.typeName << ";\n"; break;
                case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_CLASS:
                    out << "\tIl2CppClass* _" << i << "_" << rgctx.className << ";\n"; break;
                case Il2CppRGCTXDataType::IL2CPP_RGCTX_DATA_METHOD:
                    out << "\tMethodInfo* _" << i << "_" << rgctx.methodName << ";\n"; break;
                default: break;
            }
        }
        out << "};\n";
    }

    // VTable
    if (!info.vTableMethod.empty()) {
        out << "struct " << info.typeName << "_VTable {\n";
        for (size_t i = 0; i < info.vTableMethod.size(); i++) {
            out << "\tVirtualInvokeData _" << i << "_";
            if (!info.vTableMethod[i].methodName.empty())
                out << info.vTableMethod[i].methodName;
            else
                out << "unknown";
            out << ";\n";
        }
        out << "};\n";
    }

    // Class metadata struct (_c)
    out << "struct " << info.typeName << "_c {\n";
    out << "\tIl2CppClass_1 _1;\n";
    if (!info.staticFields.empty())
        out << "\tstruct " << info.typeName << "_StaticFields* static_fields;\n";
    else
        out << "\tvoid* static_fields;\n";
    if (!info.rgctxs.empty())
        out << "\t" << info.typeName << "_RGCTXs* rgctx_data;\n";
    else
        out << "\tIl2CppRGCTXData* rgctx_data;\n";
    out << "\tIl2CppClass_2 _2;\n";
    if (!info.vTableMethod.empty())
        out << "\t" << info.typeName << "_VTable vtable;\n";
    else
        out << "\tVirtualInvokeData vtable[32];\n";
    out << "};\n";

    // Object struct (_o)
    out << "struct " << info.typeName << "_o {\n";
    if (!info.isValueType) {
        out << "\t" << info.typeName << "_c *klass;\n";
        out << "\tvoid *monitor;\n";
    }
    out << "\t" << info.typeName << "_Fields fields;\n";
    out << "};\n";

    // Static fields struct
    if (!info.staticFields.empty()) {
        out << "struct " << info.typeName << "_StaticFields {\n";
        for (auto& field : info.staticFields) {
            if (field.isValueType) {
                auto it = structInfoWithStructName.find(field.fieldTypeName);
                if (it != structInfoWithStructName.end()) {
                    recursionStructInfoToStream(*it->second, out);
                }
            }
            if (field.isCustomType)
                out << "\tstruct " << field.fieldTypeName << " " << field.fieldName << ";\n";
            else
                out << "\t" << field.fieldTypeName << " " << field.fieldName << ";\n";
        }
        out << "};\n";
    }
}
void StructGenerator::generateMethodInfo(const std::string&, const std::string&, const std::vector<StructRGCTXInfo>&) {}
std::vector<StructRGCTXInfo> StructGenerator::generateRGCTX(const std::string&, const Il2CppMethodDefinition&) { return {}; }
std::string StructGenerator::getMethodTypeSignature(const std::vector<Il2CppTypeEnum>&) { return ""; }

void StructGenerator::addMetadataUsageTypeInfo(ScriptJson&, uint32_t, uint64_t) {}
void StructGenerator::addMetadataUsageIl2CppType(ScriptJson&, uint32_t, uint64_t) {}
void StructGenerator::addMetadataUsageMethodDef(ScriptJson&, uint32_t, uint64_t) {}
void StructGenerator::addMetadataUsageFieldInfo(ScriptJson&, uint32_t, uint64_t) {}
void StructGenerator::addMetadataUsageStringLiteral(ScriptJson&, uint32_t, uint64_t) {}
void StructGenerator::addMetadataUsageMethodRef(ScriptJson&, uint32_t, uint64_t) {}

} // namespace il2cpp_dumper
