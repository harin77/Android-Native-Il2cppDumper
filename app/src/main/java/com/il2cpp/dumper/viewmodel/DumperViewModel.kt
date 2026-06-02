package com.il2cpp.dumper.viewmodel

import android.app.Application
import android.net.Uri
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.il2cpp.dumper.NativeDumper
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.data.db.JobStatus
import com.il2cpp.dumper.data.repository.DumpJobRepository
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
    data class Success(val outputDir: String, val jobId: Long) : DumpState()
    data class Error(val message: String) : DumpState()
}

class DumperViewModel(
    application: Application,
    private val repository: DumpJobRepository
) : AndroidViewModel(application) {

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

    private var currentJobId: Long = 0

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

            val startTime = System.currentTimeMillis()
            val context = getApplication<Application>()

            // Get file info for the job
            val metaFileName = getFileName(metaUri)
            val il2FileName = getFileName(il2Uri)

            // Create job entity
            val job = DumpJobEntity(
                timestamp = startTime,
                metadataFileName = metaFileName,
                il2cppFileName = il2FileName,
                configJson = _config.value.toJson(),
                status = JobStatus.RUNNING
            )
            currentJobId = repository.insertJob(job)

            try {
                // Copy files to internal storage
                val metaFile = copyUriToFile(metaUri, "global-metadata.dat")
                val il2File = copyUriToFile(il2Uri, "libil2cpp.so")

                // Compute hashes
                val metaHash = DumpJobRepository.computeFileHash(metaFile)
                val il2Hash = DumpJobRepository.computeFileHash(il2File)

                addLog("Initializing...")
                val initResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeInit(metaFile.absolutePath, il2File.absolutePath)
                }
                if (!initResult) {
                    failJob("Failed to initialize", startTime, metaFileName, il2FileName, metaHash, il2Hash, metaFile.length(), il2File.length())
                    return@launch
                }

                val versionStr = NativeDumper.nativeGetVersion()
                addLog("Version: $versionStr")

                val isDump = withContext(Dispatchers.IO) { NativeDumper.nativeIsDumpFile() }
                _isDumpFile.value = isDump
                if (isDump) {
                    addLog("Detected this may be a dump file.")
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
                    failJob("Failed to find registration structures", startTime, metaFileName, il2FileName, metaHash, il2Hash, metaFile.length(), il2File.length())
                    return@launch
                }

                _state.value = DumpState.Dumping

                // Create job-specific output directory
                val jobOutputDir = File(context.filesDir, "jobs/$currentJobId")
                jobOutputDir.mkdirs()

                addLog("Dumping to ${jobOutputDir.absolutePath}...")
                val dumpResult = withContext(Dispatchers.IO) {
                    NativeDumper.nativeDump(jobOutputDir.absolutePath, _config.value.toJson())
                }

                val duration = System.currentTimeMillis() - startTime

                if (dumpResult) {
                    val outputFiles = jobOutputDir.listFiles()?.map { it.name } ?: emptyList()
                    val entity = DumpJobEntity(
                        id = currentJobId,
                        timestamp = startTime,
                        metadataFileName = metaFileName,
                        il2cppFileName = il2FileName,
                        metadataHash = metaHash,
                        il2cppHash = il2Hash,
                        metadataSize = metaFile.length(),
                        il2cppSize = il2File.length(),
                        configJson = _config.value.toJson(),
                        status = JobStatus.SUCCESS,
                        outputDirPath = jobOutputDir.absolutePath,
                        logs = _logs.value,
                        durationMs = duration,
                        metadataVersion = parseVersion(versionStr),
                        il2cppVersion = versionStr,
                        outputFileNames = outputFiles
                    )
                    repository.updateJob(entity)
                    _state.value = DumpState.Success(jobOutputDir.absolutePath, currentJobId)
                    addLog("Done! (${duration}ms)")
                } else {
                    failJob("Dump failed", startTime, metaFileName, il2FileName, metaHash, il2Hash, metaFile.length(), il2File.length(), duration)
                }
            } catch (e: Exception) {
                val duration = System.currentTimeMillis() - startTime
                failJob(e.message ?: "Unknown error", startTime, metaFileName, il2FileName, null, null, 0, 0, duration)
            }
        }
    }

    private suspend fun failJob(
        error: String,
        startTime: Long,
        metaFileName: String,
        il2FileName: String,
        metaHash: String?,
        il2Hash: String?,
        metaSize: Long,
        il2Size: Long,
        duration: Long = System.currentTimeMillis() - startTime
    ) {
        _state.value = DumpState.Error(error)
        addLog("Error: $error")
        val entity = DumpJobEntity(
            id = currentJobId,
            timestamp = startTime,
            metadataFileName = metaFileName,
            il2cppFileName = il2FileName,
            metadataHash = metaHash,
            il2cppHash = il2Hash,
            metadataSize = metaSize,
            il2cppSize = il2Size,
            configJson = _config.value.toJson(),
            status = JobStatus.FAILED,
            logs = _logs.value,
            durationMs = duration,
            errorMessage = error
        )
        repository.updateJob(entity)
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

    private fun getFileName(uri: Uri): String {
        val context = getApplication<Application>()
        var name = "unknown"
        context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            val nameIndex = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
            if (cursor.moveToFirst() && nameIndex >= 0) {
                name = cursor.getString(nameIndex) ?: "unknown"
            }
        }
        return name
    }

    private fun parseVersion(versionStr: String): Double? {
        val match = Regex("""[\d.]+""").find(versionStr)
        return match?.value?.toDoubleOrNull()
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
