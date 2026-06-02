package com.il2cpp.dumper.viewmodel

import android.app.Application
import android.net.Uri
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.il2cpp.dumper.NativeDumper
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import org.json.JSONObject
import java.io.File

data class DumpConfig(
    val dumpMethod: Boolean = true,
    val dumpField: Boolean = true,
    val dumpProperty: Boolean = false,
    val dumpAttribute: Boolean = false,
    val dumpFieldOffset: Boolean = true,
    val dumpMethodOffset: Boolean = true,
    val dumpTypeDefIndex: Boolean = true,
    val generateStruct: Boolean = true,
    val forceIl2CppVersion: Boolean = false,
    val forceVersion: Double = 24.3,
    val forceDump: Boolean = false
) {
    fun toJson(): String {
        val json = JSONObject()
        json.put("DumpMethod", dumpMethod)
        json.put("DumpField", dumpField)
        json.put("DumpProperty", dumpProperty)
        json.put("DumpAttribute", dumpAttribute)
        json.put("DumpFieldOffset", dumpFieldOffset)
        json.put("DumpMethodOffset", dumpMethodOffset)
        json.put("DumpTypeDefIndex", dumpTypeDefIndex)
        json.put("GenerateStruct", generateStruct)
        json.put("ForceIl2CppVersion", forceIl2CppVersion)
        json.put("ForceVersion", forceVersion)
        json.put("ForceDump", forceDump)
        json.put("GenerateDummyDll", false)
        return json.toString()
    }
}

sealed class DumpState {
    data object Idle : DumpState()
    data object Initializing : DumpState()
    data object Searching : DumpState()
    data object Dumping : DumpState()
    data class Success(val outputDir: String) : DumpState()
    data class Error(val message: String) : DumpState()
}

class DumperViewModel(application: Application) : AndroidViewModel(application) {

    private val _state = MutableStateFlow<DumpState>(DumpState.Idle)
    val state: StateFlow<DumpState> = _state.asStateFlow()

    private val _logs = MutableStateFlow<List<String>>(emptyList())
    val logs: StateFlow<List<String>> = _logs.asStateFlow()

    private val _config = MutableStateFlow(DumpConfig())
    val config: StateFlow<DumpConfig> = _config.asStateFlow()

    private val _metadataUri = MutableStateFlow<Uri?>(null)
    val metadataUri: StateFlow<Uri?> = _metadataUri.asStateFlow()

    private val _il2cppUri = MutableStateFlow<Uri?>(null)
    val il2cppUri: StateFlow<Uri?> = _il2cppUri.asStateFlow()

    private val _isDumpFile = MutableStateFlow(false)
    val isDumpFile: StateFlow<Boolean> = _isDumpFile.asStateFlow()

    private val _dumpAddress = MutableStateFlow<Long?>(null)
    val dumpAddress: StateFlow<Long?> = _dumpAddress.asStateFlow()

    private val logCallback = object : NativeDumper.LogCallback {
        override fun onLog(message: String) {
            _logs.value = _logs.value + message
        }
    }

    init {
        NativeDumper.nativeSetCallback(logCallback)
    }

    fun setMetadataUri(uri: Uri) {
        _metadataUri.value = uri
    }

    fun setIl2cppUri(uri: Uri) {
        _il2cppUri.value = uri
    }

    fun updateConfig(config: DumpConfig) {
        _config.value = config
    }

    fun setDumpAddress(address: Long) {
        _dumpAddress.value = address
    }

    fun startDump() {
        val metaUri = _metadataUri.value
        val il2Uri = _il2cppUri.value
        if (metaUri == null || il2Uri == null) {
            _state.value = DumpState.Error("Please select both files")
            return
        }

        viewModelScope.launch {
            _logs.value = emptyList()
            _state.value = DumpState.Initializing

            try {
                // Copy files to internal storage for native access
                val context = getApplication<Application>()
                val metaFile = copyUriToFile(metaUri, "global-metadata.dat")
                val il2File = copyUriToFile(il2Uri, "libil2cpp.so")

                addLog("Initializing...")
                val initResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeInit(metaFile.absolutePath, il2File.absolutePath)
                }
                if (!initResult) {
                    _state.value = DumpState.Error("Failed to initialize")
                    return@launch
                }

                addLog("Version: ${NativeDumper.nativeGetVersion()}")

                // Check if this is a dump file
                val isDump = withContext(Dispatchers.IO) { NativeDumper.nativeIsDumpFile() }
                _isDumpFile.value = isDump
                if (isDump) {
                    addLog("Detected this may be a dump file.")
                    // If user provided a dump address, set it
                    val addr = _dumpAddress.value
                    if (addr != null && addr != 0L) {
                        withContext(Dispatchers.IO) { NativeDumper.nativeSetDumpAddress(addr) }
                        addLog("Using dump address: 0x${addr.toString(16)}")
                    } else {
                        addLog("Set dump address via settings if needed, or continuing with auto-detect...")
                    }
                }

                _state.value = DumpState.Searching
                addLog("Searching for registration structures...")
                val searchResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeSearch()
                }
                if (!searchResult) {
                    _state.value = DumpState.Error("Failed to find registration structures")
                    return@launch
                }

                _state.value = DumpState.Dumping
                val outputDir = File(context.filesDir, "output")
                outputDir.mkdirs()

                addLog("Dumping to ${outputDir.absolutePath}...")
                val dumpResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeDump(outputDir.absolutePath, _config.value.toJson())
                }

                if (dumpResult) {
                    _state.value = DumpState.Success(outputDir.absolutePath)
                    addLog("Done!")
                } else {
                    _state.value = DumpState.Error("Dump failed")
                }
            } catch (e: Exception) {
                _state.value = DumpState.Error(e.message ?: "Unknown error")
                addLog("Error: ${e.message}")
            }
        }
    }

    private fun addLog(message: String) {
        _logs.value = _logs.value + message
    }

    private suspend fun copyUriToFile(uri: Uri, fileName: String): File {
        return withContext(Dispatchers.IO) {
            val context = getApplication<Application>()
            val file = File(context.cacheDir, fileName)
            context.contentResolver.openInputStream(uri)?.use { input ->
                file.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
            file
        }
    }

    fun getOutputFiles(): List<Pair<String, String>> {
        val outputDir = File(getApplication<Application>().filesDir, "output")
        if (!outputDir.exists()) return emptyList()
        return outputDir.listFiles()?.map { it.name to it.absolutePath } ?: emptyList()
    }

    fun reset() {
        _state.value = DumpState.Idle
        _logs.value = emptyList()
        _isDumpFile.value = false
        _dumpAddress.value = null
        NativeDumper.nativeCleanup()
    }

    override fun onCleared() {
        super.onCleared()
        NativeDumper.nativeSetCallback(null)
        NativeDumper.nativeCleanup()
    }
}
