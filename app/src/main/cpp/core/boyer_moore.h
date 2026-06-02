#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace il2cpp_dumper {

class BoyerMooreHorspool {
public:
    // Search for exact byte pattern
    static std::vector<int> search(const uint8_t* source, size_t sourceLen,
                                   const uint8_t* pattern, size_t patternLen) {
        std::vector<int> results;
        if (!source || !pattern || sourceLen == 0 || patternLen == 0 || patternLen > sourceLen)
            return results;

        int badCharacters[256];
        for (int i = 0; i < 256; i++)
            badCharacters[i] = static_cast<int>(patternLen);

        auto lastPatternByte = static_cast<int>(patternLen - 1);
        for (int i = 0; i < lastPatternByte; i++)
            badCharacters[pattern[i]] = lastPatternByte - i;

        size_t index = 0;
        while (index <= sourceLen - patternLen) {
            int i = lastPatternByte;
            while (source[index + i] == pattern[i]) {
                if (i == 0) {
                    results.push_back(static_cast<int>(index));
                    break;
                }
                i--;
            }
            index += badCharacters[source[index + lastPatternByte]];
        }
        return results;
    }

    // Search with wildcard pattern string (space-separated hex bytes, "?" for wildcards)
    static std::vector<int> search(const uint8_t* source, size_t sourceLen,
                                   const std::string& stringPattern) {
        std::vector<int> results;
        if (!source || sourceLen == 0 || stringPattern.empty())
            return results;

        // Parse pattern string
        std::vector<std::string> pattern;
        std::string token;
        for (char c : stringPattern) {
            if (c == ' ') {
                if (!token.empty()) {
                    pattern.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
        if (!token.empty())
            pattern.push_back(token);

        size_t patternLen = pattern.size();
        if (patternLen == 0 || patternLen > sourceLen)
            return results;

        int badCharacters[256];
        for (int i = 0; i < 256; i++)
            badCharacters[i] = static_cast<int>(patternLen);

        auto lastPatternByte = static_cast<int>(patternLen - 1);
        for (int i = 0; i < lastPatternByte; i++) {
            if (pattern[i] != "?") {
                int val = std::stoi(pattern[i], nullptr, 16);
                badCharacters[val] = lastPatternByte - i;
            }
        }

        size_t index = 0;
        while (index <= sourceLen - patternLen) {
            int i = lastPatternByte;
            while (checkEqual(source, pattern, index, i)) {
                if (i == 0) {
                    results.push_back(static_cast<int>(index));
                    break;
                }
                i--;
            }
            if (pattern[lastPatternByte] != "?") {
                int val = std::stoi(pattern[lastPatternByte], nullptr, 16);
                index += badCharacters[val];
            } else {
                index += 1;
            }
        }
        return results;
    }

private:
    static bool checkEqual(const uint8_t* source, const std::vector<std::string>& pattern,
                           size_t index, int i) {
        if (pattern[i] != "?") {
            int val = std::stoi(pattern[i], nullptr, 16);
            return source[index + i] == static_cast<uint8_t>(val);
        }
        return true;
    }
};

} // namespace il2cpp_dumper
