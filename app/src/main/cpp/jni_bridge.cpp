#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "core/metadata.h"
#include "core/il2cpp.h"
#include "core/section_helper.h"
#include "core/il2cpp_executor.h"
#include "core/il2cpp_decompiler.h"
#include "core/struct_generator.h"
#include "core/config.h"

#define LOG_TAG "Il2CppDumper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace il2cpp_dumper;

// Global state
static Metadata* g_metadata = nullptr;
static Il2CppEngine* g_il2Cpp = nullptr;

// JNI callback for logging
static JavaVM* g_jvm = nullptr;
static jobject g_callback = nullptr;
static jmethodID g_onLogMethod = nullptr;

static void logToJava(JNIEnv* env, const std::string& msg) {
    if (g_callback && g_onLogMethod) {
        jstring jmsg = env->NewStringUTF(msg.c_str());
        env->CallVoidMethod(g_callback, g_onLogMethod, jmsg);
        env->DeleteLocalRef(jmsg);
    }
}

static void logCallback(const char* msg) {
    JNIEnv* env;
    bool needsDetach = false;
    int status = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (status == JNI_EDETACHED) {
        g_jvm->AttachCurrentThread(&env, nullptr);
        needsDetach = true;
    }
    if (env && g_callback && g_onLogMethod) {
        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(g_callback, g_onLogMethod, jmsg);
        env->DeleteLocalRef(jmsg);
    }
    if (needsDetach) {
        g_jvm->DetachCurrentThread();
    }
}

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/) {
    g_jvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSetCallback(JNIEnv* env, jobject /* thiz */, jobject callback) {
    if (g_callback) {
        env->DeleteGlobalRef(g_callback);
        g_callback = nullptr;
    }
    if (callback) {
        g_callback = env->NewGlobalRef(callback);
        jclass cls = env->GetObjectClass(callback);
        g_onLogMethod = env->GetMethodID(cls, "onLog", "(Ljava/lang/String;)V");
    }
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeInit(
    JNIEnv* env, jobject /* thiz */,
    jstring metadataPath, jstring il2cppPath) {

    // Clean up previous state
    delete g_metadata;
    delete g_il2Cpp;
    g_metadata = nullptr;
    g_il2Cpp = nullptr;

    const char* metaPath = env->GetStringUTFChars(metadataPath, nullptr);
    const char* il2Path = env->GetStringUTFChars(il2cppPath, nullptr);

    LOGI("Initializing: metadata=%s, il2cpp=%s", metaPath, il2Path);

    // Read files into memory
    FILE* fMeta = fopen(metaPath, "rb");
    FILE* fIl2 = fopen(il2Path, "rb");

    env->ReleaseStringUTFChars(metadataPath, metaPath);
    env->ReleaseStringUTFChars(il2cppPath, il2Path);

    if (!fMeta || !fIl2) {
        LOGE("Failed to open files");
        if (fMeta) fclose(fMeta);
        if (fIl2) fclose(fIl2);
        return JNI_FALSE;
    }

    fseek(fMeta, 0, SEEK_END);
    auto metaSize = ftell(fMeta);
    fseek(fMeta, 0, SEEK_SET);

    fseek(fIl2, 0, SEEK_END);
    auto il2Size = ftell(fIl2);
    fseek(fIl2, 0, SEEK_SET);

    std::vector<uint8_t> metaBytes(metaSize);
    std::vector<uint8_t> il2Bytes(il2Size);

    fread(metaBytes.data(), 1, metaSize, fMeta);
    fread(il2Bytes.data(), 1, il2Size, fIl2);
    fclose(fMeta);
    fclose(fIl2);

    try {
        // Initialize metadata
        g_metadata = new Metadata(std::move(metaBytes));
        LOGI("Metadata loaded: version=%.1f", g_metadata->version);

        // Determine il2cpp format
        if (il2Bytes.size() < 4) {
            LOGE("Il2Cpp file too small");
            return JNI_FALSE;
        }

        uint32_t magic;
        std::memcpy(&magic, il2Bytes.data(), 4);

        if (magic == 0x464c457f) { // ELF
            // Check ELF class
            bool isElf64 = (il2Bytes[4] == 2);
            g_il2Cpp = new ElfIl2Cpp(std::move(il2Bytes), isElf64);
            LOGI("ELF %s loaded", isElf64 ? "64-bit" : "32-bit");
        } else {
            LOGE("Unsupported file format: 0x%08x", magic);
            return JNI_FALSE;
        }

        // Set properties
        g_il2Cpp->setProperties(g_metadata->version, g_metadata->metadataUsagesCount);
        LOGI("Il2Cpp version: %.1f", g_il2Cpp->version);

        return JNI_TRUE;
    } catch (const std::exception& e) {
        LOGE("Init error: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSearch(JNIEnv* env, jobject /* thiz */) {
    if (!g_metadata || !g_il2Cpp) return JNI_FALSE;

    try {
        // Count valid methods
        int methodCount = 0;
        for (auto& md : g_metadata->methodDefs) {
            if (md.methodIndex >= 0) methodCount++;
        }

        int typeDefCount = g_metadata->typeDefs.size();
        int imageCount = g_metadata->imageDefs.size();

        LOGI("Search: methods=%d, types=%d, images=%d, version=%.1f",
             methodCount, typeDefCount, imageCount, g_il2Cpp->version);

        // Check if this is a dump file
        bool isDump = g_il2Cpp->checkDump();
        if (isDump) {
            LOGI("Detected this may be a dump file. Setting isDumped=true.");
            g_il2Cpp->isDumped = true;
        }

        // Try PlusSearch first (searches for registration structures in data/exec sections)
        bool found = false;
        auto* helper = g_il2Cpp->getSectionHelper(methodCount, typeDefCount, imageCount);
        auto codeReg = helper->findCodeRegistration();
        auto metaReg = helper->findMetadataRegistration();
        delete helper;

        LOGI("PlusSearch result: codeReg=0x%llx, metaReg=0x%llx",
             (unsigned long long)codeReg, (unsigned long long)metaReg);

        if (codeReg != 0 && metaReg != 0) {
            found = g_il2Cpp->autoPlusInit(codeReg, metaReg);
        }

        if (!found) {
            LOGI("Trying pattern search...");
            found = g_il2Cpp->search();
            if (found) {
                LOGI("Pattern search succeeded, running autoPlusInit");
                helper = g_il2Cpp->getSectionHelper(methodCount, typeDefCount, imageCount);
                codeReg = helper->findCodeRegistration();
                metaReg = helper->findMetadataRegistration();
                delete helper;
                if (codeReg != 0 && metaReg != 0) {
                    found = g_il2Cpp->autoPlusInit(codeReg, metaReg);
                }
            }
        }

        if (!found) {
            LOGI("Trying symbol search...");
            found = g_il2Cpp->symbolSearch();
        }

        if (!found) {
            LOGI("ERROR: All search methods failed");
        }

        // For dump files with version >= 27, calculate ImageBase from type handles
        if (found && g_il2Cpp->version >= 27 && g_il2Cpp->isDumped && !g_metadata->typeDefs.empty()) {
            auto& firstTypeDef = g_metadata->typeDefs[0];
            auto byvalTypeIndex = firstTypeDef.byvalTypeIndex;
            if (byvalTypeIndex >= 0 && static_cast<size_t>(byvalTypeIndex) < g_il2Cpp->types.size()) {
                auto& il2CppType = g_il2Cpp->types[byvalTypeIndex];
                g_metadata->imageBase = il2CppType.typeHandle() - g_metadata->header.typeDefinitionsOffset;
                LOGI("Calculated ImageBase for dump: 0x%llx (typeHandle=0x%llx, offset=0x%x)",
                     (unsigned long long)g_metadata->imageBase,
                     (unsigned long long)il2CppType.typeHandle(),
                     g_metadata->header.typeDefinitionsOffset);
            }
        }

        return found ? JNI_TRUE : JNI_FALSE;
    } catch (const std::exception& e) {
        LOGE("Search error: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeDump(
    JNIEnv* env, jobject /* thiz */,
    jstring outputDir, jstring configJson) {

    if (!g_metadata || !g_il2Cpp) return JNI_FALSE;

    const char* outDir = env->GetStringUTFChars(outputDir, nullptr);
    const char* cfgJson = env->GetStringUTFChars(configJson, nullptr);

    Config config = Config::fromJson(cfgJson);

    env->ReleaseStringUTFChars(outputDir, outDir);
    env->ReleaseStringUTFChars(configJson, cfgJson);

    try {
        Il2CppExecutor executor(*g_metadata, *g_il2Cpp);

        if (config.dumpMethod || config.dumpField || config.dumpProperty) {
            Il2CppDecompiler decompiler(executor);
            decompiler.decompile(config, outDir);
        }

        if (config.generateStruct) {
            StructGenerator gen(executor);
            gen.writeScript(outDir);
        }

        return JNI_TRUE;
    } catch (const std::exception& e) {
        LOGE("Dump error: %s", e.what());
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeGetVersion(JNIEnv* env, jobject /* thiz */) {
    if (!g_metadata) return env->NewStringUTF("Unknown");
    char buf[64];
    snprintf(buf, sizeof(buf), "Metadata: %.1f", g_metadata->version);
    return env->NewStringUTF(buf);
}

JNIEXPORT jboolean JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeIsDumpFile(JNIEnv* /* env */, jobject /* thiz */) {
    if (!g_il2Cpp) return JNI_FALSE;
    return g_il2Cpp->checkDump() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeSetDumpAddress(
    JNIEnv* /* env */, jobject /* thiz */, jlong address) {
    if (!g_il2Cpp) return;
    if (address != 0) {
        g_il2Cpp->imageBase = static_cast<uint64_t>(address);
        g_il2Cpp->isDumped = true;
        LOGI("Set dump address: 0x%llx", (unsigned long long)address);
    }
}

JNIEXPORT void JNICALL
Java_com_il2cpp_dumper_NativeDumper_nativeCleanup(JNIEnv* /* env */, jobject /* thiz */) {
    delete g_metadata;
    delete g_il2Cpp;
    g_metadata = nullptr;
    g_il2Cpp = nullptr;
}

} // extern "C"
