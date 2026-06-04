#include "section_helper.h"
#include "elf.h"
#include "boyer_moore.h"
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

static const uint8_t featureBytes[] = {
    0x6D, 0x73, 0x63, 0x6F, 0x72, 0x6C, 0x69, 0x62,
    0x2E, 0x64, 0x6C, 0x6C, 0x00
}; // "mscorlib.dll\0"

SectionHelper::SectionHelper(ElfBase* il2Cpp, int methodCount, int typeDefinitionsCount,
                             int64_t metadataUsagesCount, int imageCount)
    : il2Cpp(il2Cpp), methodCount(methodCount), typeDefinitionsCount(typeDefinitionsCount),
      metadataUsagesCount(metadataUsagesCount), imageCount(imageCount) {}

void SectionHelper::setSection(SearchSectionType type, const std::vector<Elf32_Phdr>& sections) {
    std::vector<SearchSection> secs;
    for (auto& s : sections) {
        secs.push_back({
            s.p_offset,
            s.p_offset + s.p_filesz,
            s.p_vaddr,
            s.p_vaddr + s.p_memsz
        });
    }
    setSection(type, secs);
}

void SectionHelper::setSection(SearchSectionType type, const std::vector<Elf64_Phdr>& sections) {
    std::vector<SearchSection> secs;
    for (auto& s : sections) {
        secs.push_back({
            s.p_offset,
            s.p_offset + s.p_filesz,
            s.p_vaddr,
            s.p_vaddr + s.p_memsz
        });
    }
    setSection(type, secs);
}

void SectionHelper::setSection(SearchSectionType type, const std::vector<SearchSection>& secs) {
    switch (type) {
        case SearchSectionType::Exec: exec = secs; break;
        case SearchSectionType::Data: data = secs; break;
        case SearchSectionType::Bss: bss = secs; break;
    }
}

uint64_t SectionHelper::findCodeRegistration() {
    LOGI("findCodeRegistration: version=%.1f, exec=%zu, data=%zu",
         il2Cpp->version, exec.size(), data.size());
    if (il2Cpp->version >= 24.2) {
        uint64_t codeRegistration = findCodeRegistrationExec();
        if (codeRegistration == 0) {
            LOGI("  Exec search failed, trying data...");
            codeRegistration = findCodeRegistrationData();
        } else {
            pointerInExec = true;
            LOGI("  Found in exec: 0x%llx", (unsigned long long)codeRegistration);
        }
        return codeRegistration;
    }
    return findCodeRegistrationOld();
}

uint64_t SectionHelper::findMetadataRegistration() {
    if (il2Cpp->version < 19) return 0;
    if (il2Cpp->version >= 27) return findMetadataRegistrationV21();
    return findMetadataRegistrationOld();
}

uint64_t SectionHelper::findCodeRegistrationOld() {
    auto ptrSize = il2Cpp->getPointerSize();
    for (auto& section : data) {
        il2Cpp->setPosition(section.offset);
        while (il2Cpp->getPosition() < section.offsetEnd) {
            auto addr = il2Cpp->getPosition();
            if (il2Cpp->readIntPtr() == methodCount) {
                try {
                    auto pointer = il2Cpp->mapVATR(il2Cpp->readUIntPtr());
                    if (checkPointerRangeDataRa(pointer)) {
                        auto pointers = il2Cpp->readPointerArray(pointer, methodCount);
                        if (checkPointerRangeExecVa(pointers)) {
                            return addr - section.offset + section.address;
                        }
                    }
                } catch (...) {}
            }
            il2Cpp->setPosition(addr + ptrSize);
        }
    }
    return 0;
}

uint64_t SectionHelper::findMetadataRegistrationOld() {
    auto ptrSize = il2Cpp->getPointerSize();
    for (auto& section : data) {
        il2Cpp->setPosition(section.offset);
        auto end = std::min(section.offsetEnd, il2Cpp->getLength()) - ptrSize;
        while (il2Cpp->getPosition() < end) {
            auto addr = il2Cpp->getPosition();
            if (il2Cpp->readIntPtr() == typeDefinitionsCount) {
                try {
                    il2Cpp->setPosition(il2Cpp->getPosition() + ptrSize * 2);
                    auto pointer = il2Cpp->mapVATR(il2Cpp->readUIntPtr());
                    if (checkPointerRangeDataRa(pointer)) {
                        auto pointers = il2Cpp->readPointerArray(pointer, metadataUsagesCount);
                        if (checkPointerRangeBssVa(pointers)) {
                            return addr - ptrSize * 12 - section.offset + section.address;
                        }
                    }
                } catch (...) {}
            }
            il2Cpp->setPosition(addr + ptrSize);
        }
    }
    return 0;
}

uint64_t SectionHelper::findMetadataRegistrationV21() {
    LOGI("findMetadataRegistrationV21: typeDefs=%d, data sections=%zu", typeDefinitionsCount, data.size());
    auto ptrSize = il2Cpp->getPointerSize();
    for (auto& section : data) {
        il2Cpp->setPosition(section.offset);
        auto end = std::min(section.offsetEnd, il2Cpp->getLength()) - ptrSize;
        LOGI("  Searching data section: offset=0x%llx, end=0x%llx, size=0x%llx",
             (unsigned long long)section.offset, (unsigned long long)end,
             (unsigned long long)(end - section.offset));
        while (il2Cpp->getPosition() < end) {
            auto addr = il2Cpp->getPosition();
            if (il2Cpp->readIntPtr() == typeDefinitionsCount) {
                il2Cpp->setPosition(il2Cpp->getPosition() + ptrSize);
                if (il2Cpp->readIntPtr() == typeDefinitionsCount) {
                    try {
                        auto pointer = il2Cpp->mapVATR(il2Cpp->readUIntPtr());
                        if (checkPointerRangeDataRa(pointer)) {
                            auto pointers = il2Cpp->readPointerArray(pointer, typeDefinitionsCount);
                            bool flag = pointerInExec ? checkPointerRangeExecVa(pointers) : checkPointerRangeDataVa(pointers);
                            if (flag) {
                                return addr - ptrSize * 10 - section.offset + section.address;
                            }
                        }
                    } catch (...) {}
                }
            }
            il2Cpp->setPosition(addr + ptrSize);
        }
    }
    return 0;
}

uint64_t SectionHelper::findCodeRegistrationData() {
    return findCodeRegistration2019(data);
}

uint64_t SectionHelper::findCodeRegistrationExec() {
    return findCodeRegistration2019(exec);
}

uint64_t SectionHelper::findCodeRegistration2019(const std::vector<SearchSection>& secs) {
    LOGI("findCodeRegistration2019: searching %zu sections", secs.size());
    for (auto& sec : secs) {
        auto readSize = static_cast<int>(std::min(sec.offsetEnd - sec.offset,
                                                   il2Cpp->getLength() - sec.offset));
        if (readSize <= 0) continue;
        il2Cpp->setPosition(sec.offset);
        auto buff = il2Cpp->readBytes(readSize);
        LOGI("  Section: offset=0x%llx, address=0x%llx, size=%d",
             (unsigned long long)sec.offset, (unsigned long long)sec.address, readSize);
        auto results = BoyerMooreHorspool::search(buff.data(), buff.size(), featureBytes, sizeof(featureBytes));
        LOGI("  mscorlib.dll matches: %zu", results.size());
        for (auto index : results) {
            auto dllva = static_cast<uint64_t>(index) + sec.address;
            for (auto refva : findReference(dllva)) {
                for (auto refva2 : findReference(refva)) {
                    if (il2Cpp->version >= 27) {
                        for (int i = imageCount - 1; i >= 0; i--) {
                            for (auto refva3 : findReference(refva2 - static_cast<uint64_t>(i) * il2Cpp->getPointerSize())) {
                                il2Cpp->setPosition(il2Cpp->mapVATR(refva3 - il2Cpp->getPointerSize()));
                                if (il2Cpp->readIntPtr() == imageCount) {
                                    if (il2Cpp->version >= 29) return refva3 - il2Cpp->getPointerSize() * 14;
                                    return refva3 - il2Cpp->getPointerSize() * 13;
                                }
                            }
                        }
                    } else {
                        for (int i = 0; i < imageCount; i++) {
                            for (auto refva3 : findReference(refva2 - static_cast<uint64_t>(i) * il2Cpp->getPointerSize())) {
                                return refva3 - il2Cpp->getPointerSize() * 13;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}

std::vector<uint64_t> SectionHelper::findReference(uint64_t addr) {
    std::vector<uint64_t> results;
    auto ptrSize = il2Cpp->getPointerSize();
    for (auto& dataSec : data) {
        auto position = dataSec.offset;
        auto end = std::min(dataSec.offsetEnd, il2Cpp->getLength()) - ptrSize;
        while (position < end) {
            il2Cpp->setPosition(position);
            if (il2Cpp->readUIntPtr() == addr) {
                results.push_back(position - dataSec.offset + dataSec.address);
            }
            position += ptrSize;
        }
    }
    /*if (results.empty()) {
        LOGI("    findReference(0x%llx): no references found", (unsigned long long)addr);
    }*/
    return results;
}

bool SectionHelper::checkPointerRangeDataRa(uint64_t pointer) {
    return std::any_of(data.begin(), data.end(), [pointer](const SearchSection& s) {
        return pointer >= s.offset && pointer <= s.offsetEnd;
    });
}

bool SectionHelper::checkPointerRangeExecVa(const std::vector<uint64_t>& pointers) {
    return std::all_of(pointers.begin(), pointers.end(), [this](uint64_t x) {
        return std::any_of(exec.begin(), exec.end(), [x](const SearchSection& s) {
            return x >= s.address && x <= s.addressEnd;
        });
    });
}

bool SectionHelper::checkPointerRangeDataVa(const std::vector<uint64_t>& pointers) {
    return std::all_of(pointers.begin(), pointers.end(), [this](uint64_t x) {
        return std::any_of(data.begin(), data.end(), [x](const SearchSection& s) {
            return x >= s.address && x <= s.addressEnd;
        });
    });
}

bool SectionHelper::checkPointerRangeBssVa(const std::vector<uint64_t>& pointers) {
    return std::all_of(pointers.begin(), pointers.end(), [this](uint64_t x) {
        return std::any_of(bss.begin(), bss.end(), [x](const SearchSection& s) {
            return x >= s.address && x <= s.addressEnd;
        });
    });
}

} // namespace il2cpp_dumper
