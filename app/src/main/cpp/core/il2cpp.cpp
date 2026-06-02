#include "il2cpp.h"
#include "elf.h"
#include "section_helper.h"
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

// --- Struct read() implementations ---

void Il2CppCodeRegistration::read(BinaryStream& stream, double version) {
    if (version <= 24.1) {
        methodPointersCount = stream.readUIntPtr();
        methodPointers = stream.readUIntPtr();
    }
    if (version <= 21) {
        delegateWrappersFromNativeToManagedCount = stream.readUIntPtr();
        delegateWrappersFromNativeToManaged = stream.readUIntPtr();
    }
    if (version >= 22) {
        reversePInvokeWrapperCount = stream.readUIntPtr();
        reversePInvokeWrappers = stream.readUIntPtr();
    }
    if (version <= 22) {
        delegateWrappersFromManagedToNativeCount = stream.readUIntPtr();
        delegateWrappersFromManagedToNative = stream.readUIntPtr();
        marshalingFunctionsCount = stream.readUIntPtr();
        marshalingFunctions = stream.readUIntPtr();
    }
    if (version >= 21 && version <= 22) {
        ccwMarshalingFunctionsCount = stream.readUIntPtr();
        ccwMarshalingFunctions = stream.readUIntPtr();
    }
    genericMethodPointersCount = stream.readUIntPtr();
    genericMethodPointers = stream.readUIntPtr();
    if (version == 24.5 || version >= 27.1) {
        genericAdjustorThunks = stream.readUIntPtr();
    }
    invokerPointersCount = stream.readUIntPtr();
    invokerPointers = stream.readUIntPtr();
    if (version <= 24.5) {
        customAttributeCount = stream.readUIntPtr();
        customAttributeGenerators = stream.readUIntPtr();
    }
    if (version >= 21 && version <= 22) {
        guidCount = stream.readUIntPtr();
        guids = stream.readUIntPtr();
    }
    if (version >= 22) {
        unresolvedVirtualCallCount = stream.readUIntPtr();
        unresolvedVirtualCallPointers = stream.readUIntPtr();
    }
    if (version >= 29.1) {
        unresolvedInstanceCallPointers = stream.readUIntPtr();
        unresolvedStaticCallPointers = stream.readUIntPtr();
    }
    if (version >= 23) {
        interopDataCount = stream.readUIntPtr();
        interopData = stream.readUIntPtr();
    }
    if (version >= 24.3) {
        windowsRuntimeFactoryCount = stream.readUIntPtr();
        windowsRuntimeFactoryTable = stream.readUIntPtr();
    }
    if (version >= 24.2) {
        codeGenModulesCount = stream.readUIntPtr();
        codeGenModules = stream.readUIntPtr();
    }
}

void Il2CppMetadataRegistration::read(BinaryStream& stream, double version) {
    genericClassesCount = stream.readIntPtr();
    genericClasses = stream.readUIntPtr();
    genericInstsCount = stream.readIntPtr();
    genericInsts = stream.readUIntPtr();
    genericMethodTableCount = stream.readIntPtr();
    genericMethodTable = stream.readUIntPtr();
    typesCount = stream.readIntPtr();
    types = stream.readUIntPtr();
    methodSpecsCount = stream.readIntPtr();
    methodSpecs = stream.readUIntPtr();
    if (version <= 16) {
        methodReferencesCount = stream.readIntPtr();
        methodReferences = stream.readUIntPtr();
    }
    fieldOffsetsCount = stream.readIntPtr();
    fieldOffsets = stream.readUIntPtr();
    typeDefinitionsSizesCount = stream.readIntPtr();
    typeDefinitionsSizes = stream.readUIntPtr();
    if (version >= 19) {
        metadataUsagesCount = stream.readUIntPtr();
        metadataUsages = stream.readUIntPtr();
    }
}

void Il2CppType::read(BinaryStream& stream, double /*version*/) {
    datapoint = stream.readUIntPtr();
    bits = stream.readUInt32();
}

void Il2CppGenericClass::read(BinaryStream& stream, double version) {
    if (version <= 24.5) {
        typeDefinitionIndex = stream.readIntPtr();
    } else {
        type = stream.readUIntPtr();
    }
    context_class_inst = stream.readUIntPtr();
    context_method_inst = stream.readUIntPtr();
    cached_class = stream.readUIntPtr();
}

void Il2CppArrayType::read(BinaryStream& stream, double /*version*/) {
    etype = stream.readUIntPtr();
    rank = stream.readByte();
    numsizes = stream.readByte();
    numlobounds = stream.readByte();
    // Padding
    stream.readByte();
    sizes = stream.readUIntPtr();
    lobounds = stream.readUIntPtr();
}

void Il2CppGenericMethodFunctionsDefinitions::read(BinaryStream& stream, double version) {
    genericMethodIndex = stream.readInt32();
    indices_methodIndex = stream.readInt32();
    indices_invokerIndex = stream.readInt32();
    if (version == 24.5 || version >= 27.1) {
        indices_adjustorThunk = stream.readInt32();
    }
}

void Il2CppCodeGenModule::read(BinaryStream& stream, double version) {
    moduleName = stream.readUIntPtr();
    methodPointerCount = stream.readIntPtr();
    methodPointers = stream.readUIntPtr();
    if (version == 24.5 || version >= 27.1) {
        adjustorThunkCount = stream.readIntPtr();
        adjustorThunks = stream.readUIntPtr();
    }
    invokerIndices = stream.readUIntPtr();
    reversePInvokeWrapperCount = stream.readUIntPtr();
    reversePInvokeWrapperIndices = stream.readUIntPtr();
    rgctxRangesCount = stream.readIntPtr();
    rgctxRanges = stream.readUIntPtr();
    rgctxsCount = stream.readIntPtr();
    rgctxs = stream.readUIntPtr();
    debuggerMetadata = stream.readUIntPtr();
    if (version >= 27 && version <= 27.2) {
        customAttributeCacheGenerator = stream.readUIntPtr();
    }
    if (version >= 27) {
        moduleInitializer = stream.readUIntPtr();
        staticConstructorTypeIndices = stream.readUIntPtr();
        metadataRegistration = stream.readUIntPtr();
        codeRegistration = stream.readUIntPtr();
    }
}

// --- Il2CppEngine ---

Il2CppEngine::Il2CppEngine(const uint8_t* data, size_t size, bool isElf64)
    : BinaryStream(data, size) {
    is32Bit = !isElf64;
}

Il2CppEngine::Il2CppEngine(std::vector<uint8_t>&& data, bool isElf64)
    : BinaryStream(std::move(data)) {
    is32Bit = !isElf64;
}

void Il2CppEngine::setProperties(double ver, int64_t muCount) {
    version = ver;
    metadataUsagesCount = muCount;
}

bool Il2CppEngine::autoPlusInit(uint64_t codeRegistration, uint64_t metadataRegistration) {
    if (codeRegistration != 0) {
        uint64_t limit = 0x50000;
        if (version >= 24.2) {
            pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
            if (version == 31) {
                if (pCodeRegistration.genericMethodPointersCount > limit) {
                    codeRegistration -= getPointerSize() * 2;
                } else {
                    version = 29;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 29) {
                if (pCodeRegistration.genericMethodPointersCount > limit) {
                    version = 29.1;
                    codeRegistration -= getPointerSize() * 2;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 27) {
                if (pCodeRegistration.reversePInvokeWrapperCount > limit) {
                    version = 27.1;
                    codeRegistration -= getPointerSize();
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 24.4) {
                codeRegistration -= getPointerSize() * 2;
                if (pCodeRegistration.reversePInvokeWrapperCount > limit) {
                    version = 24.5;
                    codeRegistration -= getPointerSize();
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
            if (version == 24.2) {
                if (pCodeRegistration.interopDataCount == 0) {
                    version = 24.3;
                    codeRegistration -= getPointerSize() * 2;
                    LOGI("Change il2cpp version to: %.1f", version);
                }
            }
        }
    }

    LOGI("CodeRegistration : %llx", (unsigned long long)codeRegistration);
    LOGI("MetadataRegistration : %llx", (unsigned long long)metadataRegistration);

    if (codeRegistration != 0 && metadataRegistration != 0) {
        init(codeRegistration, metadataRegistration);
        return true;
    }
    return false;
}

void Il2CppEngine::init(uint64_t codeRegistration, uint64_t metadataRegistration) {
    pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));

    uint64_t limit = 0x50000;
    if (version == 27 && pCodeRegistration.invokerPointersCount > limit) {
        version = 27.1;
        LOGI("Change il2cpp version to: %.1f", version);
        pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
    }

    if (version == 27.1) {
        auto pCodeGenModules = readPrimitiveArray<uint64_t>(
            mapVATR(pCodeRegistration.codeGenModules), pCodeRegistration.codeGenModulesCount);
        for (auto pCodeGenModule : pCodeGenModules) {
            auto codeGenModule = readClass<Il2CppCodeGenModule>(mapVATR(pCodeGenModule));
            if (codeGenModule.rgctxsCount > 0) {
                // Check RGCTX entries
                break;
            }
        }
    }

    if (version == 24.4 && pCodeRegistration.invokerPointersCount > limit) {
        version = 24.5;
        LOGI("Change il2cpp version to: %.1f", version);
        pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
    }

    if (version == 24.2 && pCodeRegistration.codeGenModules == 0) {
        version = 24.3;
        LOGI("Change il2cpp version to: %.1f", version);
        pCodeRegistration = readClass<Il2CppCodeRegistration>(mapVATR(codeRegistration));
    }

    pMetadataRegistration = readClass<Il2CppMetadataRegistration>(mapVATR(metadataRegistration));

    genericMethodPointers = readPrimitiveArray<uint64_t>(
        mapVATR(pCodeRegistration.genericMethodPointers), pCodeRegistration.genericMethodPointersCount);
    invokerPointers = readPrimitiveArray<uint64_t>(
        mapVATR(pCodeRegistration.invokerPointers), pCodeRegistration.invokerPointersCount);

    if (version < 27) {
        customAttributeGenerators = readPrimitiveArray<uint64_t>(
            mapVATR(pCodeRegistration.customAttributeGenerators), pCodeRegistration.customAttributeCount);
    }

    if (version > 16 && version < 27) {
        metadataUsages = readPrimitiveArray<uint64_t>(
            mapVATR(pMetadataRegistration.metadataUsages), metadataUsagesCount);
    }

    if (version >= 22) {
        if (pCodeRegistration.reversePInvokeWrapperCount != 0)
            reversePInvokeWrappers = readPrimitiveArray<uint64_t>(
                mapVATR(pCodeRegistration.reversePInvokeWrappers), pCodeRegistration.reversePInvokeWrapperCount);
        if (pCodeRegistration.unresolvedVirtualCallCount != 0)
            unresolvedVirtualCallPointers = readPrimitiveArray<uint64_t>(
                mapVATR(pCodeRegistration.unresolvedVirtualCallPointers), pCodeRegistration.unresolvedVirtualCallCount);
    }

    genericInstPointers = readPrimitiveArray<uint64_t>(
        mapVATR(pMetadataRegistration.genericInsts), pMetadataRegistration.genericInstsCount);
    genericInsts.resize(genericInstPointers.size());
    for (size_t i = 0; i < genericInstPointers.size(); i++) {
        genericInsts[i].read(*this, is32Bit);
    }

    fieldOffsetsArePointers = version > 21;
    if (version == 21) {
        auto fieldTest = readPrimitiveArray<uint32_t>(mapVATR(pMetadataRegistration.fieldOffsets), 6);
        fieldOffsetsArePointers = fieldTest[0] == 0 && fieldTest[1] == 0 && fieldTest[2] == 0 &&
                                   fieldTest[3] == 0 && fieldTest[4] == 0 && fieldTest[5] > 0;
    }
    if (fieldOffsetsArePointers) {
        fieldOffsets = readPrimitiveArray<uint64_t>(
            mapVATR(pMetadataRegistration.fieldOffsets), pMetadataRegistration.fieldOffsetsCount);
    } else {
        auto offsets32 = readPrimitiveArray<uint32_t>(
            mapVATR(pMetadataRegistration.fieldOffsets), pMetadataRegistration.fieldOffsetsCount);
        fieldOffsets.resize(offsets32.size());
        for (size_t i = 0; i < offsets32.size(); i++) fieldOffsets[i] = offsets32[i];
    }

    auto pTypes = readPrimitiveArray<uint64_t>(
        mapVATR(pMetadataRegistration.types), pMetadataRegistration.typesCount);
    LOGI("CRITICAL: is32Bit=%d, version=%.1f at types read", is32Bit, version);
    LOGI("pTypes array: addr=0x%llx, count=%lld",
         (unsigned long long)mapVATR(pMetadataRegistration.types), (long long)pMetadataRegistration.typesCount);
    if (!pTypes.empty()) {
        LOGI("  pTypes[0]=0x%llx, pTypes[1]=0x%llx",
             (unsigned long long)pTypes[0], (unsigned long long)pTypes[1]);
        auto typeFileOffset = mapVATR(pTypes[0]);
        LOGI("  pTypes[0] file offset: 0x%llx", (unsigned long long)typeFileOffset);
        // Read raw bytes at the type location - DIRECT TEST
        if (typeFileOffset > 0 && typeFileOffset + 12 <= getLength()) {
            setPosition(typeFileOffset);
            LOGI("  Position before readUInt64: %llu, data ptr=%p, dataSize=%zu",
                 (unsigned long long)getPosition(), (void*)getData(), (size_t)getLength());
            auto rawDp = readUInt64();
            LOGI("  readUInt64 result: 0x%llx, position after: %llu",
                 (unsigned long long)rawDp, (unsigned long long)getPosition());
            auto rawBits = readUInt32();
            LOGI("  Raw type[0]: datapoint=0x%llx, bits=0x%x", (unsigned long long)rawDp, rawBits);

            // Now test readClass
            setPosition(typeFileOffset);
            auto testType = readClass<Il2CppType>();
            LOGI("  readClass<Il2CppType> result: datapoint=0x%llx, bits=0x%x",
                 (unsigned long long)testType.datapoint, testType.bits);
        }
    }
    types.resize(pMetadataRegistration.typesCount);
    for (int64_t i = 0; i < pMetadataRegistration.typesCount; i++) {
        types[i] = readClass<Il2CppType>(mapVATR(pTypes[i]));
        types[i].init(version);
        typeDic[pTypes[i]] = static_cast<size_t>(i);
    }
    LOGI("Types loaded: count=%lld, typeDic.size=%zu", (long long)pMetadataRegistration.typesCount, typeDic.size());

    if (version >= 24.2) {
        auto pCodeGenModules = readPrimitiveArray<uint64_t>(
            mapVATR(pCodeRegistration.codeGenModules), pCodeRegistration.codeGenModulesCount);
        for (auto pCodeGenModule : pCodeGenModules) {
            auto codeGenModule = readClass<Il2CppCodeGenModule>(mapVATR(pCodeGenModule));
            auto moduleName = readStringToNull(mapVATR(codeGenModule.moduleName));
            codeGenModules[moduleName] = codeGenModule;
            try {
                auto methodPointers = readPrimitiveArray<uint64_t>(
                    mapVATR(codeGenModule.methodPointers), codeGenModule.methodPointerCount);
                codeGenModuleMethodPointers[moduleName] = std::move(methodPointers);
            } catch (...) {
                codeGenModuleMethodPointers[moduleName].resize(codeGenModule.methodPointerCount, 0);
            }
        }
    } else {
        methodPointers = readPrimitiveArray<uint64_t>(
            mapVATR(pCodeRegistration.methodPointers), pCodeRegistration.methodPointersCount);
    }

    // Read generic method table
    auto genericMethodTable = readClassArray<Il2CppGenericMethodFunctionsDefinitions>(
        mapVATR(pMetadataRegistration.genericMethodTable), pMetadataRegistration.genericMethodTableCount);
    methodSpecs = readClassArray<Il2CppMethodSpec>(
        mapVATR(pMetadataRegistration.methodSpecs), pMetadataRegistration.methodSpecsCount);

    for (auto& table : genericMethodTable) {
        auto& methodSpec = methodSpecs[table.genericMethodIndex];
        auto methodDefIndex = methodSpec.methodDefinitionIndex;
        methodDefinitionMethodSpecs[methodDefIndex].push_back(methodSpec);
        // Hash methodSpec for use as key
        uint64_t hash = static_cast<uint64_t>(methodSpec.methodDefinitionIndex) ^
                        (static_cast<uint64_t>(methodSpec.classIndexIndex) << 16) ^
                        (static_cast<uint64_t>(methodSpec.methodIndexIndex) << 32);
        methodSpecGenericMethodPointers[hash] = genericMethodPointers[table.indices_methodIndex];
    }

    LOGI("Il2Cpp initialized: version=%.1f, types=%lld, methods loaded",
         version, (long long)pMetadataRegistration.typesCount);
}

Il2CppType* Il2CppEngine::getIl2CppType(uint64_t pointer) {
    auto it = typeDic.find(pointer);
    if (it != typeDic.end()) return &types[it->second];
    return nullptr;
}

uint64_t Il2CppEngine::getMethodPointer(const std::string& imageName, const Il2CppMethodDefinition& methodDef) {
    if (version >= 24.2) {
        auto it = codeGenModuleMethodPointers.find(imageName);
        if (it == codeGenModuleMethodPointers.end()) return 0;
        auto methodToken = methodDef.token;
        auto methodPointerIndex = methodToken & 0x00FFFFFFu;
        if (methodPointerIndex == 0 || methodPointerIndex > it->second.size()) return 0;
        return it->second[methodPointerIndex - 1];
    } else {
        auto methodIndex = methodDef.methodIndex;
        if (methodIndex >= 0 && static_cast<size_t>(methodIndex) < methodPointers.size()) {
            return methodPointers[methodIndex];
        }
    }
    return 0;
}

int Il2CppEngine::getFieldOffsetFromIndex(int typeIndex, int fieldIndexInType, int fieldIndex,
                                            bool isValueType, bool isStatic) {
    try {
        int offset = -1;
        if (fieldOffsetsArePointers) {
            auto ptr = fieldOffsets[typeIndex];
            if (ptr > 0) {
                setPosition(mapVATR(ptr) + 4ull * fieldIndexInType);
                offset = readInt32();
            }
        } else {
            offset = static_cast<int>(fieldOffsets[fieldIndex]);
        }
        if (offset > 0) {
            if (isValueType && !isStatic) {
                offset -= is32Bit ? 8 : 16;
            }
        }
        return offset;
    } catch (...) {
        return -1;
    }
}

// --- ElfIl2Cpp ---

ElfIl2Cpp::ElfIl2Cpp(const uint8_t* data, size_t size, bool isElf64)
    : Il2CppEngine(data, size, isElf64) {
    initElf();
}

ElfIl2Cpp::ElfIl2Cpp(std::vector<uint8_t>&& data, bool isElf64)
    : Il2CppEngine(std::move(data), isElf64) {
    initElf();
}

void ElfIl2Cpp::initElf() {
    // Check ELF class
    setPosition(4);
    auto elfClass = readByte();
    bool actually64 = (elfClass == 2);
    // CRITICAL: Set BinaryStream::is32Bit so readClass<Il2CppType>() reads correct size
    is32Bit = !actually64;

    // Create ELF parser by re-reading from owned data
    elfParser = new Elf(ownedData.data(), ownedData.size());
    elfParser->version = version;
}

uint64_t ElfIl2Cpp::mapVATR(uint64_t addr) {
    elfParser->version = version;
    return elfParser->mapVATR(addr);
}

uint64_t ElfIl2Cpp::mapRTVA(uint64_t addr) {
    elfParser->version = version;
    return elfParser->mapRTVA(addr);
}

bool ElfIl2Cpp::search() {
    elfParser->version = version;
    elfParser->isDumped = isDumped;
    return elfParser->search();
}

bool ElfIl2Cpp::plusSearch(int methodCount, int typeDefinitionsCount, int imageCount) {
    elfParser->version = version;
    elfParser->isDumped = isDumped;
    elfParser->setMetadataUsagesCount(metadataUsagesCount);
    return elfParser->plusSearch(methodCount, typeDefinitionsCount, imageCount);
}

bool ElfIl2Cpp::symbolSearch() {
    elfParser->version = version;
    return elfParser->symbolSearch();
}

SectionHelper* ElfIl2Cpp::getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount) {
    elfParser->version = version;
    return elfParser->getSectionHelper(methodCount, typeDefinitionsCount, imageCount, metadataUsagesCount);
}

SectionHelper* ElfIl2Cpp::getSectionHelper(int methodCount, int typeDefinitionsCount, int imageCount, int64_t muCount) {
    elfParser->version = version;
    return elfParser->getSectionHelper(methodCount, typeDefinitionsCount, imageCount, muCount);
}

bool ElfIl2Cpp::checkDump() {
    return elfParser->checkDump();
}

uint64_t ElfIl2Cpp::getRVA(uint64_t pointer) {
    return elfParser->getRVA(pointer);
}

} // namespace il2cpp_dumper
