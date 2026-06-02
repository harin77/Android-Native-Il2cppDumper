#pragma once

#include <cstdint>
#include <vector>

namespace il2cpp_dumper {

enum class SearchSectionType {
    Exec,
    Data,
    Bss
};

struct SearchSection {
    uint64_t offset = 0;
    uint64_t offsetEnd = 0;
    uint64_t address = 0;
    uint64_t addressEnd = 0;
};

} // namespace il2cpp_dumper
