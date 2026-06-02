#pragma once

#include <cstdint>
#include <cstring>

namespace il2cpp_dumper {

// ELF constants
namespace ElfConstants {
    // e_machine
    inline constexpr int EM_386 = 3;
    inline constexpr int EM_ARM = 40;
    inline constexpr int EM_X86_64 = 62;
    inline constexpr int EM_AARCH64 = 183;

    // p_type
    inline constexpr int PT_LOAD = 1;
    inline constexpr int PT_DYNAMIC = 2;

    // p_flags
    inline constexpr int PF_X = 1;

    // d_tag
    inline constexpr int DT_PLTGOT = 3;
    inline constexpr int DT_HASH = 4;
    inline constexpr int DT_STRTAB = 5;
    inline constexpr int DT_SYMTAB = 6;
    inline constexpr int DT_RELA = 7;
    inline constexpr int DT_RELASZ = 8;
    inline constexpr int DT_INIT = 12;
    inline constexpr int DT_FINI = 13;
    inline constexpr int DT_REL = 17;
    inline constexpr int DT_RELSZ = 18;
    inline constexpr int DT_JMPREL = 23;
    inline constexpr int DT_INIT_ARRAY = 25;
    inline constexpr int DT_FINI_ARRAY = 26;
    inline constexpr int DT_GNU_HASH = 0x6ffffef5;

    // sh_type
    inline constexpr uint32_t SHT_LOUSER = 0x80000000;

    // ARM relocs
    inline constexpr int R_ARM_ABS32 = 2;

    // i386 relocs
    inline constexpr int R_386_32 = 1;

    // AArch64 relocs
    inline constexpr int R_AARCH64_ABS64 = 257;
    inline constexpr int R_AARCH64_RELATIVE = 1027;

    // AMD x86-64 relocations
    inline constexpr int R_X86_64_64 = 1;
    inline constexpr int R_X86_64_RELATIVE = 8;
}

// 32-bit ELF structures
struct Elf32_Ehdr {
    uint32_t ei_mag;
    uint8_t ei_class;
    uint8_t ei_data;
    uint8_t ei_version;
    uint8_t ei_osabi;
    uint8_t ei_abiversion;
    uint8_t ei_pad[7];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf32_Phdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};

struct Elf32_Shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
};

struct Elf32_Sym {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
};

struct Elf32_Dyn {
    int32_t d_tag;
    uint32_t d_un;
};

struct Elf32_Rel {
    uint32_t r_offset;
    uint32_t r_info;
};

// 64-bit ELF structures
struct Elf64_Ehdr {
    uint32_t ei_mag;
    uint8_t ei_class;
    uint8_t ei_data;
    uint8_t ei_version;
    uint8_t ei_osabi;
    uint8_t ei_abiversion;
    uint8_t ei_pad[7];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct Elf64_Phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct Elf64_Shdr {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
};

struct Elf64_Sym {
    uint32_t st_name;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
};

struct Elf64_Dyn {
    int64_t d_tag;
    uint64_t d_un;
};

struct Elf64_Rela {
    uint64_t r_offset;
    uint64_t r_info;
    int64_t r_addend;
};

} // namespace il2cpp_dumper
