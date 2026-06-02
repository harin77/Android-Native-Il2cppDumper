#include "binary_stream.h"
#include <cstring>
#include <stdexcept>
#include <android/log.h>

#define LOG_TAG "Il2CppDumper"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace il2cpp_dumper {

BinaryStream::BinaryStream(const uint8_t* data, size_t size)
    : data(const_cast<uint8_t*>(data)), dataSize(size), position(0) {}

BinaryStream::BinaryStream(std::vector<uint8_t>&& data)
    : ownedData(std::move(data)), position(0) {
    this->data = ownedData.data();
    this->dataSize = ownedData.size();
}

void BinaryStream::setPosition(uint64_t pos) {
    if (pos > dataSize) {
        LOGE("setPosition out of bounds: pos=%llu, size=%zu", (unsigned long long)pos, dataSize);
        position = dataSize;
        return;
    }
    position = pos;
}

bool BinaryStream::readBoolean() {
    return readByte() != 0;
}

uint8_t BinaryStream::readByte() {
    if (position >= dataSize) {
        LOGE("readByte out of bounds: pos=%llu, size=%zu", (unsigned long long)position, dataSize);
        return 0;
    }
    return data[position++];
}

std::vector<uint8_t> BinaryStream::readBytes(int count) {
    if (count < 0 || position + static_cast<uint64_t>(count) > dataSize) {
        LOGE("readBytes out of bounds: pos=%llu, count=%d, size=%zu",
             (unsigned long long)position, count, dataSize);
        return {};
    }
    std::vector<uint8_t> result(data + position, data + position + count);
    position += count;
    return result;
}

int8_t BinaryStream::readSByte() {
    return static_cast<int8_t>(readByte());
}

int16_t BinaryStream::readInt16() {
    if (position + 2 > dataSize) return 0;
    int16_t value;
    std::memcpy(&value, data + position, 2);
    position += 2;
    return value;
}

uint16_t BinaryStream::readUInt16() {
    if (position + 2 > dataSize) return 0;
    uint16_t value;
    std::memcpy(&value, data + position, 2);
    position += 2;
    return value;
}

int32_t BinaryStream::readInt32() {
    if (position + 4 > dataSize) return 0;
    int32_t value;
    std::memcpy(&value, data + position, 4);
    position += 4;
    return value;
}

uint32_t BinaryStream::readUInt32() {
    if (position + 4 > dataSize) return 0;
    uint32_t value;
    std::memcpy(&value, data + position, 4);
    position += 4;
    return value;
}

int64_t BinaryStream::readInt64() {
    if (position + 8 > dataSize) return 0;
    int64_t value;
    std::memcpy(&value, data + position, 8);
    position += 8;
    return value;
}

uint64_t BinaryStream::readUInt64() {
    if (position + 8 > dataSize) return 0;
    uint64_t value;
    std::memcpy(&value, data + position, 8);
    position += 8;
    return value;
}

float BinaryStream::readFloat() {
    if (position + 4 > dataSize) return 0;
    float value;
    std::memcpy(&value, data + position, 4);
    position += 4;
    return value;
}

double BinaryStream::readDouble() {
    if (position + 8 > dataSize) return 0;
    double value;
    std::memcpy(&value, data + position, 8);
    position += 8;
    return value;
}

uint32_t BinaryStream::readULeb128() {
    uint32_t value = readByte();
    if (value >= 0x80) {
        int bitshift = 0;
        value &= 0x7f;
        while (true) {
            uint8_t b = readByte();
            bitshift += 7;
            value |= static_cast<uint32_t>((b & 0x7f) << bitshift);
            if (b < 0x80)
                break;
        }
    }
    return value;
}

uint32_t BinaryStream::readCompressedUInt32() {
    uint32_t val;
    uint8_t read = readByte();

    if ((read & 0x80) == 0) {
        val = read;
    } else if ((read & 0xC0) == 0x80) {
        val = (read & ~0x80u) << 8;
        val |= readByte();
    } else if ((read & 0xE0) == 0xC0) {
        val = (read & ~0xC0u) << 24;
        val |= (static_cast<uint32_t>(readByte()) << 16);
        val |= (static_cast<uint32_t>(readByte()) << 8);
        val |= readByte();
    } else if (read == 0xF0) {
        val = readUInt32();
    } else if (read == 0xFE) {
        val = UINT32_MAX - 1;
    } else if (read == 0xFF) {
        val = UINT32_MAX;
    } else {
        throw std::runtime_error("Invalid compressed integer format");
    }
    return val;
}

int32_t BinaryStream::readCompressedInt32() {
    uint32_t encoded = readCompressedUInt32();
    if (encoded == UINT32_MAX)
        return INT32_MIN;
    bool isNegative = (encoded & 1) != 0;
    encoded >>= 1;
    if (isNegative)
        return -static_cast<int32_t>(encoded + 1);
    return static_cast<int32_t>(encoded);
}

int64_t BinaryStream::readIntPtr() {
    return is32Bit ? readInt32() : readInt64();
}

uint64_t BinaryStream::readUIntPtr() {
    return is32Bit ? readUInt32() : readUInt64();
}

void BinaryStream::writeUInt32(uint32_t value) {
    if (position + 4 > dataSize) return;
    std::memcpy(data + position, &value, 4);
    position += 4;
}

void BinaryStream::writeUInt64(uint64_t value) {
    if (position + 8 > dataSize) return;
    std::memcpy(data + position, &value, 8);
    position += 8;
}

std::string BinaryStream::readStringToNull(uint64_t addr) {
    if (addr >= dataSize) return "";
    setPosition(addr);
    std::string result;
    while (position < dataSize) {
        uint8_t b = data[position++];
        if (b == 0) break;
        result += static_cast<char>(b);
    }
    return result;
}

std::string BinaryStream::readString(int numChars) {
    // Read UTF-8 string with numChars characters
    std::string result;
    result.reserve(numChars);
    for (int i = 0; i < numChars && position < dataSize; i++) {
        result += static_cast<char>(data[position++]);
    }
    return result;
}

} // namespace il2cpp_dumper
