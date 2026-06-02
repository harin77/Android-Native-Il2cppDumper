#include "il2cpp_decompiler.h"
#include "il2cpp_constants.h"
#include "custom_attribute_reader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

Il2CppDecompiler::Il2CppDecompiler(Il2CppExecutor& executor)
    : executor(executor), metadata(executor.metadata), il2Cpp(executor.il2Cpp) {}

void Il2CppDecompiler::decompile(const Config& config, const std::string& outputDir) {
    LOGI("Dumping...");
    auto path = outputDir + "/dump.cs";
    std::ofstream writer(path, std::ios::binary);
    if (!writer.is_open()) {
        LOGE("Failed to open output file: %s", path.c_str());
        return;
    }

    // Write BOM for UTF-8
    writer << "\xEF\xBB\xBF";

    // Dump images
    for (size_t imageIndex = 0; imageIndex < metadata.imageDefs.size(); imageIndex++) {
        auto& imageDef = metadata.imageDefs[imageIndex];
        writer << "// Image " << imageIndex << ": "
               << metadata.getStringFromIndex(imageDef.nameIndex)
               << " - " << imageDef.typeStart << "\n";
    }

    // Dump types
    for (auto& imageDef : metadata.imageDefs) {
        try {
            auto imageName = metadata.getStringFromIndex(imageDef.nameIndex);
            auto typeEnd = imageDef.typeStart + imageDef.typeCount;
            for (int typeDefIndex = imageDef.typeStart; typeDefIndex < typeEnd; typeDefIndex++) {
                auto& typeDef = metadata.typeDefs[typeDefIndex];
                std::vector<std::string> extends;

                if (typeDef.parentIndex >= 0) {
                    auto& parent = il2Cpp.types[typeDef.parentIndex];
                    auto parentName = executor.getTypeName(parent, false, false);
                    if (!typeDef.isValueType() && !typeDef.isEnum() && parentName != "object") {
                        extends.push_back(parentName);
                    }
                }
                for (int i = 0; i < typeDef.interfaces_count; i++) {
                    auto& iface = il2Cpp.types[metadata.interfaceIndices[typeDef.interfacesStart + i]];
                    extends.push_back(executor.getTypeName(iface, false, false));
                }

                writer << "\n// Namespace: " << metadata.getStringFromIndex(typeDef.namespaceIndex) << "\n";

                if (config.dumpAttribute) {
                    writer << getCustomAttribute(imageDef, typeDef.customAttributeIndex, typeDef.token);
                }

                // Visibility
                auto visibility = typeDef.flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
                switch (visibility) {
                    case TYPE_ATTRIBUTE_PUBLIC:
                    case TYPE_ATTRIBUTE_NESTED_PUBLIC: writer << "public "; break;
                    case TYPE_ATTRIBUTE_NOT_PUBLIC:
                    case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
                    case TYPE_ATTRIBUTE_NESTED_ASSEMBLY: writer << "internal "; break;
                    case TYPE_ATTRIBUTE_NESTED_PRIVATE: writer << "private "; break;
                    case TYPE_ATTRIBUTE_NESTED_FAMILY: writer << "protected "; break;
                    case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM: writer << "protected internal "; break;
                }

                if ((typeDef.flags & TYPE_ATTRIBUTE_ABSTRACT) && (typeDef.flags & TYPE_ATTRIBUTE_SEALED))
                    writer << "static ";
                else if (!(typeDef.flags & TYPE_ATTRIBUTE_INTERFACE) && (typeDef.flags & TYPE_ATTRIBUTE_ABSTRACT))
                    writer << "abstract ";
                else if (!typeDef.isValueType() && !typeDef.isEnum() && (typeDef.flags & TYPE_ATTRIBUTE_SEALED))
                    writer << "sealed ";

                if (typeDef.flags & TYPE_ATTRIBUTE_INTERFACE) writer << "interface ";
                else if (typeDef.isEnum()) writer << "enum ";
                else if (typeDef.isValueType()) writer << "struct ";
                else writer << "class ";

                auto typeName = executor.getTypeDefName(typeDef, false, true);
                writer << typeName;
                if (!extends.empty()) {
                    writer << " : ";
                    for (size_t i = 0; i < extends.size(); i++) {
                        if (i > 0) writer << ", ";
                        writer << extends[i];
                    }
                }
                if (config.dumpTypeDefIndex)
                    writer << " // TypeDefIndex: " << typeDefIndex << "\n{\n";
                else
                    writer << "\n{\n";

                // Dump fields
                if (config.dumpField && typeDef.field_count > 0) {
                    writer << "\t// Fields\n";
                    auto fieldEnd = typeDef.fieldStart + typeDef.field_count;
                    for (int i = typeDef.fieldStart; i < fieldEnd; i++) {
                        auto& fieldDef = metadata.fieldDefs[i];
                        auto& fieldType = il2Cpp.types[fieldDef.typeIndex];
                        bool isStatic = false, isConst = false;

                        if (config.dumpAttribute) {
                            writer << getCustomAttribute(imageDef, fieldDef.customAttributeIndex, fieldDef.token, "\t");
                        }
                        writer << "\t";

                        auto access = fieldType.attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
                        switch (access) {
                            case FIELD_ATTRIBUTE_PRIVATE: writer << "private "; break;
                            case FIELD_ATTRIBUTE_PUBLIC: writer << "public "; break;
                            case FIELD_ATTRIBUTE_FAMILY: writer << "protected "; break;
                            case FIELD_ATTRIBUTE_ASSEMBLY:
                            case FIELD_ATTRIBUTE_FAM_AND_ASSEM: writer << "internal "; break;
                            case FIELD_ATTRIBUTE_FAM_OR_ASSEM: writer << "protected internal "; break;
                        }

                        if (fieldType.attrs & FIELD_ATTRIBUTE_LITERAL) {
                            isConst = true;
                            writer << "const ";
                        } else {
                            if (fieldType.attrs & FIELD_ATTRIBUTE_STATIC) {
                                isStatic = true;
                                writer << "static ";
                            }
                            if (fieldType.attrs & FIELD_ATTRIBUTE_INIT_ONLY) writer << "readonly ";
                        }

                        writer << executor.getTypeName(fieldType, false, false) << " "
                               << metadata.getStringFromIndex(fieldDef.nameIndex);

                        if (config.dumpFieldOffset && !isConst) {
                            writer << "; // 0x" << std::hex
                                   << il2Cpp.getFieldOffsetFromIndex(typeDefIndex, i - typeDef.fieldStart, i, typeDef.isValueType(), isStatic)
                                   << std::dec << "\n";
                        } else {
                            writer << ";\n";
                        }
                    }
                }

                // Dump properties
                if (config.dumpProperty && typeDef.property_count > 0) {
                    writer << "\t// Properties\n";
                    auto propEnd = typeDef.propertyStart + typeDef.property_count;
                    for (int i = typeDef.propertyStart; i < propEnd; i++) {
                        auto& propDef = metadata.propertyDefs[i];
                        if (config.dumpAttribute) {
                            writer << getCustomAttribute(imageDef, propDef.customAttributeIndex, propDef.token, "\t");
                        }
                        writer << "\t";
                        if (propDef.get >= 0) {
                            auto& methodDef = metadata.methodDefs[typeDef.methodStart + propDef.get];
                            writer << getModifiers(methodDef);
                            auto& propType = il2Cpp.types[methodDef.returnType];
                            writer << executor.getTypeName(propType, false, false) << " "
                                   << metadata.getStringFromIndex(propDef.nameIndex) << " { ";
                        }
                        if (propDef.get >= 0) writer << "get; ";
                        if (propDef.set >= 0) writer << "set; ";
                        writer << "}\n";
                    }
                }

                // Dump methods
                if (config.dumpMethod && typeDef.method_count > 0) {
                    writer << "\t// Methods\n";
                    auto methodEnd = typeDef.methodStart + typeDef.method_count;
                    for (int i = typeDef.methodStart; i < methodEnd; i++) {
                        writer << "\n";
                        auto& methodDef = metadata.methodDefs[i];
                        bool isAbstract = (methodDef.flags & METHOD_ATTRIBUTE_ABSTRACT) != 0;

                        if (config.dumpAttribute) {
                            writer << getCustomAttribute(imageDef, methodDef.customAttributeIndex, methodDef.token, "\t");
                        }
                        if (config.dumpMethodOffset) {
                            auto methodPointer = il2Cpp.getMethodPointer(imageName, methodDef);
                            if (!isAbstract && methodPointer > 0) {
                                auto fixedPtr = il2Cpp.getRVA(methodPointer);
                                writer << "\t// RVA: 0x" << std::hex << fixedPtr
                                       << " Offset: 0x" << il2Cpp.mapVATR(methodPointer)
                                       << " VA: 0x" << methodPointer << std::dec;
                            } else {
                                writer << "\t// RVA: -1 Offset: -1";
                            }
                            if (methodDef.slot != 0xFFFF) {
                                writer << " Slot: " << methodDef.slot;
                            }
                            writer << "\n";
                        }
                        writer << "\t" << getModifiers(methodDef);
                        auto& methodReturnType = il2Cpp.types[methodDef.returnType];
                        auto methodName = metadata.getStringFromIndex(methodDef.nameIndex);
                        if (methodDef.genericContainerIndex >= 0) {
                            auto& container = metadata.genericContainers[methodDef.genericContainerIndex];
                            methodName += executor.getGenericContainerParams(container);
                        }
                        if (methodReturnType.byref == 1) writer << "ref ";
                        writer << executor.getTypeName(methodReturnType, false, false) << " " << methodName << "(";

                        std::vector<std::string> paramStrs;
                        for (int j = 0; j < methodDef.parameterCount; j++) {
                            auto& paramDef = metadata.parameterDefs[methodDef.parameterStart + j];
                            auto paramStr = executor.getTypeName(il2Cpp.types[paramDef.typeIndex], false, false);
                            paramStr += " " + metadata.getStringFromIndex(paramDef.nameIndex);
                            paramStrs.push_back(paramStr);
                        }
                        for (size_t j = 0; j < paramStrs.size(); j++) {
                            if (j > 0) writer << ", ";
                            writer << paramStrs[j];
                        }
                        writer << (isAbstract ? ");\n" : ") { }\n");
                    }
                }
                writer << "}\n";
            }
        } catch (const std::exception& e) {
            LOGE("Error dumping: %s", e.what());
            writer << "/*" << e.what() << "*/\n}\n";
        }
    }

    writer.close();
    LOGI("Dump complete: %s", path.c_str());
}

std::string Il2CppDecompiler::getCustomAttribute(const Il2CppImageDefinition& imageDef,
                                                    int customAttributeIndex, uint32_t token,
                                                    const std::string& padding) {
    if (il2Cpp.version < 21) return "";
    auto attributeIndex = metadata.getCustomAttributeIndex(imageDef, customAttributeIndex, token);
    if (attributeIndex >= 0) {
        if (il2Cpp.version < 29) {
            if (static_cast<size_t>(attributeIndex) >= executor.customAttributeGenerators.size()) return "";
            auto methodPointer = executor.customAttributeGenerators[attributeIndex];
            auto fixedPtr = il2Cpp.getRVA(methodPointer);
            if (static_cast<size_t>(attributeIndex) >= metadata.attributeTypeRanges.size()) return "";
            auto& range = metadata.attributeTypeRanges[attributeIndex];
            std::ostringstream ss;
            for (int i = 0; i < range.count; i++) {
                auto typeIndex = metadata.attributeTypes[range.start + i];
                ss << padding << "[" << executor.getTypeName(il2Cpp.types[typeIndex], false, false)
                   << "] // RVA: 0x" << std::hex << fixedPtr
                   << " Offset: 0x" << il2Cpp.mapVATR(methodPointer)
                   << " VA: 0x" << methodPointer << std::dec << "\n";
            }
            return ss.str();
        } else {
            if (static_cast<size_t>(attributeIndex) >= metadata.attributeDataRanges.size()) return "";
            auto& startRange = metadata.attributeDataRanges[attributeIndex];
            auto& endRange = metadata.attributeDataRanges[attributeIndex + 1];
            metadata.setPosition(metadata.header.attributeDataOffset + startRange.startOffset);
            auto buff = metadata.readBytes(static_cast<int>(endRange.startOffset - startRange.startOffset));
            CustomAttributeDataReader reader(&executor, buff.data(), buff.size());
            if (reader.count() == 0) return "";
            std::ostringstream ss;
            for (int i = 0; i < reader.count(); i++) {
                ss << padding << reader.getStringCustomAttributeData() << "\n";
            }
            return ss.str();
        }
    }
    return "";
}

std::string Il2CppDecompiler::getModifiers(const Il2CppMethodDefinition& methodDef) {
    auto hash = methodDef.nameIndex; // Simple hash for cache
    auto it = methodModifiers.find(hash);
    if (it != methodModifiers.end()) return it->second;

    std::string str;
    auto access = methodDef.flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access) {
        case METHOD_ATTRIBUTE_PRIVATE: str += "private "; break;
        case METHOD_ATTRIBUTE_PUBLIC: str += "public "; break;
        case METHOD_ATTRIBUTE_FAMILY: str += "protected "; break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM: str += "internal "; break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM: str += "protected internal "; break;
    }
    if (methodDef.flags & METHOD_ATTRIBUTE_STATIC) str += "static ";
    if (methodDef.flags & METHOD_ATTRIBUTE_ABSTRACT) {
        str += "abstract ";
        if ((methodDef.flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            str += "override ";
    } else if (methodDef.flags & METHOD_ATTRIBUTE_FINAL) {
        if ((methodDef.flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            str += "sealed override ";
    } else if (methodDef.flags & METHOD_ATTRIBUTE_VIRTUAL) {
        if ((methodDef.flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT)
            str += "virtual ";
        else
            str += "override ";
    }
    if (methodDef.flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) str += "extern ";

    methodModifiers[hash] = str;
    return str;
}

} // namespace il2cpp_dumper
