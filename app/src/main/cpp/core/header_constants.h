#pragma once

#include <string>

namespace il2cpp_dumper {

namespace HeaderConstants {

inline const std::string GenericHeader = R"(#include <stdint.h>
#include <stdbool.h>

typedef struct {
    Il2CppMethodPointer methodPointer;
    void* invoker_method;
    const char* name;
    Il2CppClass* klass;
    const Il2CppType* return_type;
    const void* parameters;
    const Il2CppRGCTXData* rgctx_data;
    union {
        const void* genericMethod;
        const void* genericContainer;
    };
    int32_t customAttributeIndex;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint8_t parameters_count;
    uint8_t bitflags;
} MethodInfo;

typedef void (*Il2CppMethodPointer)();
typedef void (*InvokerMethod)(Il2CppMethodPointer, const MethodInfo*, void*, void**);

)";

inline const std::string HeaderV22 = R"(// IL2CPP v22
typedef struct { void* klass; void* monitor; } Il2CppObject;

)";

inline const std::string HeaderV240 = R"(// IL2CPP v24.0
typedef struct { void* klass; void* monitor; } Il2CppObject;

)";

inline const std::string HeaderV241 = R"(// IL2CPP v24.1
typedef struct { void* klass; void* monitor; } Il2CppObject;

)";

inline const std::string HeaderV242 = R"(// IL2CPP v24.2-24.5
typedef struct { void* klass; void* monitor; } Il2CppObject;

)";

inline const std::string HeaderV27 = R"(// IL2CPP v27
typedef struct { void* klass; void* monitor; } Il2CppObject;

)";

inline const std::string HeaderV29 = R"(// IL2CPP v29+
typedef struct { void* klass; void* monitor; } Il2CppObject;

)";

} // namespace HeaderConstants

} // namespace il2cpp_dumper
