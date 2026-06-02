#include "custom_attribute_reader.h"
#include "il2cpp_executor.h"
#include <sstream>

namespace il2cpp_dumper {

CustomAttributeDataReader::CustomAttributeDataReader(Il2CppExecutor* executor,
                                                       const uint8_t* data, size_t size)
    : executor(executor), stream(data, size), count_(0) {
    if (size >= 4) {
        count_ = static_cast<int>(stream.readCompressedUInt32());
    }
}

BlobValue CustomAttributeDataReader::readBlobValue() {
    BlobValue bv;
    auto type = static_cast<Il2CppTypeEnum>(stream.readByte());
    bv.il2cppTypeEnum = type;

    switch (type) {
        case Il2CppTypeEnum::IL2CPP_TYPE_BOOLEAN:
            bv.value = stream.readBoolean();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_U1:
            bv.value = stream.readByte();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_I1:
            bv.value = stream.readSByte();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_CHAR: {
            auto bytes = stream.readBytes(2);
            char16_t c;
            std::memcpy(&c, bytes.data(), 2);
            bv.value = c;
            break;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_U2:
            bv.value = stream.readUInt16();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_I2:
            bv.value = stream.readInt16();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_U4:
            bv.value = stream.readCompressedUInt32();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_I4:
            bv.value = stream.readCompressedInt32();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_U8:
            bv.value = stream.readUInt64();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_I8:
            bv.value = stream.readInt64();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_R4:
            bv.value = stream.readFloat();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_R8:
            bv.value = stream.readDouble();
            break;
        case Il2CppTypeEnum::IL2CPP_TYPE_STRING: {
            int length = stream.readCompressedInt32();
            if (length == -1) {
                bv.value = nullptr;
            } else {
                bv.value = stream.readString(length);
            }
            break;
        }
        case Il2CppTypeEnum::IL2CPP_TYPE_IL2CPP_TYPE_INDEX: {
            int typeIndex = stream.readCompressedInt32();
            if (typeIndex == -1) {
                bv.value = nullptr;
            } else {
                bv.value = static_cast<uint64_t>(typeIndex);
            }
            break;
        }
        default:
            bv.value = std::monostate{};
            break;
    }
    return bv;
}

CustomAttributeVisitor CustomAttributeDataReader::visitCustomAttributeData() {
    CustomAttributeVisitor visitor;
    if (currentIndex >= count_) return visitor;

    visitor.ctorIndex = static_cast<int>(stream.readCompressedUInt32());

    // Read constructor arguments
    auto argCount = stream.readCompressedUInt32();
    for (uint32_t i = 0; i < argCount; i++) {
        CustomAttributeArgumentInfo arg;
        arg.index = static_cast<int>(stream.readCompressedUInt32());
        arg.value = readBlobValue();
        visitor.arguments.push_back(std::move(arg));
    }

    // Read named fields
    auto fieldCount = stream.readCompressedUInt32();
    for (uint32_t i = 0; i < fieldCount; i++) {
        CustomAttributeArgumentInfo field;
        field.index = static_cast<int>(stream.readCompressedUInt32());
        field.value = readBlobValue();
        visitor.fields.push_back(std::move(field));
    }

    // Read named properties
    auto propCount = stream.readCompressedUInt32();
    for (uint32_t i = 0; i < propCount; i++) {
        CustomAttributeArgumentInfo prop;
        prop.index = static_cast<int>(stream.readCompressedUInt32());
        prop.value = readBlobValue();
        visitor.properties.push_back(std::move(prop));
    }

    currentIndex++;
    return visitor;
}

std::string CustomAttributeDataReader::getStringCustomAttributeData() {
    // Simplified version - returns a placeholder string
    // The full implementation would need access to the executor for type name resolution
    return "[CustomAttribute]";
}

} // namespace il2cpp_dumper
