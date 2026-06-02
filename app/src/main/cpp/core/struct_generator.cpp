#include "struct_generator.h"
#include "header_constants.h"
#include "il2cpp_constants.h"
#include <fstream>
#include <sstream>
#include <algorithm>
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
                    sm.name = typeName + "$$" + metadata.getStringFromIndex(methodDef.nameIndex);
                    auto& returnType = il2Cpp.types[methodDef.returnType];
                    sm.signature = executor.getTypeName(returnType, false, false) + " " +
                                   fixName(sm.name) + "(const MethodInfo* method);";
                    json.scriptMethod.push_back(sm);
                }
            }
        }
    }

    // Write script.json
    {
        auto path = outputDir + "/script.json";
        std::ofstream f(path);
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
        std::ofstream f(path);
        f << "[\n";
        for (size_t i = 0; i < metadata.stringLiterals.size(); i++) {
            try {
                auto value = metadata.getStringLiteralFromIndex(static_cast<uint32_t>(i));
                f << "  {\"Value\": \"";
                // Escape special chars
                for (char c : value) {
                    switch (c) {
                        case '"': f << "\\\""; break;
                        case '\\': f << "\\\\"; break;
                        case '\n': f << "\\n"; break;
                        case '\r': f << "\\r"; break;
                        case '\t': f << "\\t"; break;
                        default: f << c; break;
                    }
                }
                f << "\", \"Address\": \"0x0\"}";
                if (i + 1 < metadata.stringLiterals.size()) f << ",";
                f << "\n";
            } catch (...) {}
        }
        f << "]\n";
        f.close();
        LOGI("stringliteral.json written: %zu literals, stringLiteralDataOffset=0x%x",
             metadata.stringLiterals.size(), metadata.header.stringLiteralDataOffset);
    }

    // Write il2cpp.h
    {
        auto path = outputDir + "/il2cpp.h";
        std::ofstream f(path);
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

        // Write struct definitions
        for (auto& info : structInfoList) {
            // Fields struct
            f << "struct " << info.typeName << "_Fields {\n";
            for (auto& field : info.fields) {
                f << "\t" << field.fieldTypeName << " " << field.fieldName << ";\n";
            }
            f << "};\n\n";

            // Object struct
            f << "struct " << info.typeName << "_o {\n";
            if (!info.isValueType) {
                f << "\tvoid* klass;\n";
                f << "\tvoid* monitor;\n";
            }
            f << "\t" << info.typeName << "_Fields fields;\n";
            f << "};\n\n";

            // Static fields if any
            if (!info.staticFields.empty()) {
                f << "struct " << info.typeName << "_StaticFields {\n";
                for (auto& field : info.staticFields) {
                    f << "\t" << field.fieldTypeName << " " << field.fieldName << ";\n";
                }
                f << "};\n\n";
            }
        }

        f.close();
        LOGI("il2cpp.h written: %zu structs, structNameDic=%zu entries",
             structInfoList.size(), structNameDic.size());
        if (!structInfoList.empty()) {
            LOGI("  First struct: '%s', fields=%zu, staticFields=%zu",
                 structInfoList[0].typeName.c_str(),
                 structInfoList[0].fields.size(),
                 structInfoList[0].staticFields.size());
        }
    }

    LOGI("Struct generation complete");
}

// Simplified implementations for remaining methods
std::string StructGenerator::parseType(const Il2CppType& il2CppType, const Il2CppGenericContext* /*context*/) {
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

std::string StructGenerator::getIl2CppStructName(const Il2CppType& il2CppType, const Il2CppGenericContext* /*context*/) {
    switch (il2CppType.type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE:
        case Il2CppTypeEnum::IL2CPP_TYPE_CLASS: {
            auto typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
            // Find the typeDefIndex by searching metadata.typeDefs
            for (size_t i = 0; i < metadata.typeDefs.size(); i++) {
                if (metadata.typeDefs[i].nameIndex == typeDef.nameIndex &&
                    metadata.typeDefs[i].namespaceIndex == typeDef.namespaceIndex) {
                    auto it = structNameDic.find(static_cast<int>(i));
                    if (it != structNameDic.end()) return it->second;
                    break;
                }
            }
            return "System_Object";
        }
        default: return "System_Object";
    }
}

bool StructGenerator::isValueType(const Il2CppType& il2CppType, const Il2CppGenericContext* /*context*/) {
    if (il2CppType.type == Il2CppTypeEnum::IL2CPP_TYPE_VALUETYPE) {
        auto typeDef = executor.getTypeDefinitionFromIl2CppType(il2CppType);
        return !typeDef.isEnum();
    }
    return false;
}

bool StructGenerator::isCustomType(const Il2CppType& il2CppType, const Il2CppGenericContext* /*context*/) {
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
void StructGenerator::addParents(const Il2CppTypeDefinition& /*typeDef*/, StructInfo& /*info*/) {}

void StructGenerator::addFields(const Il2CppTypeDefinition& typeDef, StructInfo& info, const Il2CppGenericContext* /*context*/) {
    if (typeDef.field_count > 0) {
        auto fieldEnd = typeDef.fieldStart + typeDef.field_count;
        for (int i = typeDef.fieldStart; i < fieldEnd; i++) {
            auto& fieldDef = metadata.fieldDefs[i];
            auto& fieldType = il2Cpp.types[fieldDef.typeIndex];
            if (fieldType.attrs & FIELD_ATTRIBUTE_LITERAL) continue;
            StructFieldInfo fi;
            fi.fieldTypeName = parseType(fieldType);
            fi.fieldName = fixName(metadata.getStringFromIndex(fieldDef.nameIndex));
            fi.isValueType = isValueType(fieldType);
            fi.isCustomType = isCustomType(fieldType);
            if (fieldType.attrs & FIELD_ATTRIBUTE_STATIC)
                info.staticFields.push_back(fi);
            else
                info.fields.push_back(fi);
        }
    }
}

void StructGenerator::addVTableMethod(StructInfo& /*info*/, const Il2CppTypeDefinition& /*typeDef*/) {}
void StructGenerator::addRGCTX(StructInfo& /*info*/, const Il2CppTypeDefinition& /*typeDef*/) {}
void StructGenerator::parseArrayClassStruct(const Il2CppType& /*il2CppType*/, const Il2CppGenericContext* /*context*/) {}
std::string StructGenerator::recursionStructInfo(StructInfo& /*info*/) { return ""; }
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
