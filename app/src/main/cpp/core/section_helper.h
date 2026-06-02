#pragma once

#include <cstdint>
#include <vector>
#include "search_section.h"
#include "elf_class.h"

namespace il2cpp_dumper {

// Forward declarations
class ElfBase;

class SectionHelper {
public:
    SectionHelper(ElfBase* il2Cpp, int methodCount, int typeDefinitionsCount,
                  int64_t metadataUsagesCount, int imageCount);

    void setSection(SearchSectionType type, const std::vector<Elf32_Phdr>& sections);
    void setSection(SearchSectionType type, const std::vector<Elf64_Phdr>& sections);
    void setSection(SearchSectionType type, const std::vector<SearchSection>& sections);

    uint64_t findCodeRegistration();
    uint64_t findMetadataRegistration();

    const std::vector<SearchSection>& getExec() const { return exec; }
    const std::vector<SearchSection>& getData() const { return data; }
    const std::vector<SearchSection>& getBss() const { return bss; }

private:
    ElfBase* il2Cpp;
    int methodCount;
    int typeDefinitionsCount;
    int64_t metadataUsagesCount;
    int imageCount;
    bool pointerInExec = false;

    std::vector<SearchSection> exec;
    std::vector<SearchSection> data;
    std::vector<SearchSection> bss;

    uint64_t findCodeRegistrationOld();
    uint64_t findMetadataRegistrationOld();
    uint64_t findMetadataRegistrationV21();
    uint64_t findCodeRegistrationData();
    uint64_t findCodeRegistrationExec();
    uint64_t findCodeRegistration2019(const std::vector<SearchSection>& secs);

    bool checkPointerRangeDataRa(uint64_t pointer);
    bool checkPointerRangeExecVa(const std::vector<uint64_t>& pointers);
    bool checkPointerRangeDataVa(const std::vector<uint64_t>& pointers);
    bool checkPointerRangeBssVa(const std::vector<uint64_t>& pointers);

    std::vector<uint64_t> findReference(uint64_t addr);
};

} // namespace il2cpp_dumper
