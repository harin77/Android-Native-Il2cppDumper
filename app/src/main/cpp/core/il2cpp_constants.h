#pragma once

namespace il2cpp_dumper {

// Field Attributes (21.1.5)
inline constexpr int FIELD_ATTRIBUTE_FIELD_ACCESS_MASK = 0x0007;
inline constexpr int FIELD_ATTRIBUTE_COMPILER_CONTROLLED = 0x0000;
inline constexpr int FIELD_ATTRIBUTE_PRIVATE = 0x0001;
inline constexpr int FIELD_ATTRIBUTE_FAM_AND_ASSEM = 0x0002;
inline constexpr int FIELD_ATTRIBUTE_ASSEMBLY = 0x0003;
inline constexpr int FIELD_ATTRIBUTE_FAMILY = 0x0004;
inline constexpr int FIELD_ATTRIBUTE_FAM_OR_ASSEM = 0x0005;
inline constexpr int FIELD_ATTRIBUTE_PUBLIC = 0x0006;

inline constexpr int FIELD_ATTRIBUTE_STATIC = 0x0010;
inline constexpr int FIELD_ATTRIBUTE_INIT_ONLY = 0x0020;
inline constexpr int FIELD_ATTRIBUTE_LITERAL = 0x0040;

// Method Attributes (22.1.9)
inline constexpr int METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK = 0x0007;
inline constexpr int METHOD_ATTRIBUTE_COMPILER_CONTROLLED = 0x0000;
inline constexpr int METHOD_ATTRIBUTE_PRIVATE = 0x0001;
inline constexpr int METHOD_ATTRIBUTE_FAM_AND_ASSEM = 0x0002;
inline constexpr int METHOD_ATTRIBUTE_ASSEM = 0x0003;
inline constexpr int METHOD_ATTRIBUTE_FAMILY = 0x0004;
inline constexpr int METHOD_ATTRIBUTE_FAM_OR_ASSEM = 0x0005;
inline constexpr int METHOD_ATTRIBUTE_PUBLIC = 0x0006;

inline constexpr int METHOD_ATTRIBUTE_STATIC = 0x0010;
inline constexpr int METHOD_ATTRIBUTE_FINAL = 0x0020;
inline constexpr int METHOD_ATTRIBUTE_VIRTUAL = 0x0040;

inline constexpr int METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK = 0x0100;
inline constexpr int METHOD_ATTRIBUTE_REUSE_SLOT = 0x0000;
inline constexpr int METHOD_ATTRIBUTE_NEW_SLOT = 0x0100;

inline constexpr int METHOD_ATTRIBUTE_ABSTRACT = 0x0400;
inline constexpr int METHOD_ATTRIBUTE_PINVOKE_IMPL = 0x2000;

// Type Attributes (21.1.13)
inline constexpr int TYPE_ATTRIBUTE_VISIBILITY_MASK = 0x00000007;
inline constexpr int TYPE_ATTRIBUTE_NOT_PUBLIC = 0x00000000;
inline constexpr int TYPE_ATTRIBUTE_PUBLIC = 0x00000001;
inline constexpr int TYPE_ATTRIBUTE_NESTED_PUBLIC = 0x00000002;
inline constexpr int TYPE_ATTRIBUTE_NESTED_PRIVATE = 0x00000003;
inline constexpr int TYPE_ATTRIBUTE_NESTED_FAMILY = 0x00000004;
inline constexpr int TYPE_ATTRIBUTE_NESTED_ASSEMBLY = 0x00000005;
inline constexpr int TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM = 0x00000006;
inline constexpr int TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM = 0x00000007;

inline constexpr int TYPE_ATTRIBUTE_INTERFACE = 0x00000020;
inline constexpr int TYPE_ATTRIBUTE_ABSTRACT = 0x00000080;
inline constexpr int TYPE_ATTRIBUTE_SEALED = 0x00000100;
inline constexpr int TYPE_ATTRIBUTE_SERIALIZABLE = 0x00002000;

// Flags for Params (22.1.12)
inline constexpr int PARAM_ATTRIBUTE_IN = 0x0001;
inline constexpr int PARAM_ATTRIBUTE_OUT = 0x0002;
inline constexpr int PARAM_ATTRIBUTE_OPTIONAL = 0x0010;

} // namespace il2cpp_dumper
