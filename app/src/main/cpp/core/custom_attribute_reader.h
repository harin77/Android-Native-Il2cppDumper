#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "blob_value.h"
#include "binary_stream.h"

namespace il2cpp_dumper {

// Forward declarations
class Il2CppExecutor;

struct CustomAttributeArgumentInfo {
    int index = 0;
    BlobValue value;
};

struct CustomAttributeVisitor {
    int ctorIndex = 0;
    std::vector<CustomAttributeArgumentInfo> arguments;
    std::vector<CustomAttributeArgumentInfo> fields;
    std::vector<CustomAttributeArgumentInfo> properties;
};

class CustomAttributeDataReader {
public:
    CustomAttributeDataReader(Il2CppExecutor* executor, const uint8_t* data, size_t size);

    int count() const { return count_; }
    CustomAttributeVisitor visitCustomAttributeData();
    std::string getStringCustomAttributeData();

private:
    Il2CppExecutor* executor;
    BinaryStream stream;
    int count_ = 0;
    int currentIndex = 0;

    BlobValue readBlobValue();
    Il2CppTypeEnum readEncodedTypeEnum(uint64_t& enumTypePtr);
};

} // namespace il2cpp_dumper
