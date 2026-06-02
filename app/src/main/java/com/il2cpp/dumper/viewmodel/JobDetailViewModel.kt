package com.il2cpp.dumper.viewmodel

import android.app.Application
import android.content.Intent
import android.net.Uri
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.data.db.DatabaseProvider
import com.il2cpp.dumper.data.repository.DumpJobRepository
import com.il2cpp.dumper.util.ExportUtil
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import java.io.File

class JobDetailViewModel(
    application: Application,
    private val jobId: Long
) : AndroidViewModel(application) {

    private val repository = DumpJobRepository(DatabaseProvider.getDao(application))
    private val jobsDir = File(application.filesDir, "jobs")

    private val _job = MutableStateFlow<DumpJobEntity?>(null)
    val job: StateFlow<DumpJobEntity?> = _job.asStateFlow()

    private val _exportResult = MutableStateFlow<String?>(null)
    val exportResult: StateFlow<String?> = _exportResult.asStateFlow()

    private val _shareIntent = MutableStateFlow<Intent?>(null)
    val shareIntent: StateFlow<Intent?> = _shareIntent.asStateFlow()

    init {
        viewModelScope.launch {
            repository.getJobById(jobId).collect { _job.value = it }
        }
    }

    fun exportFile(fileName: String) {
        val context = getApplication<Application>()
        val jobData = _job.value ?: return
        val outputDir = jobData.outputDirPath?.let { File(it) } ?: return
        val sourceFile = File(outputDir, fileName)
        if (!sourceFile.exists()) {
            _exportResult.value = "File not found: $fileName"
            return
        }
        viewModelScope.launch {
            val uri = ExportUtil.saveFileToDownloads(context, sourceFile, fileName)
            _exportResult.value = if (uri != null) {
                "Saved to Downloads/Il2CppDumper/$fileName"
            } else {
                "Failed to save $fileName"
            }
        }
    }

    fun exportAllAsZip() {
        val context = getApplication<Application>()
        val jobData = _job.value ?: return
        val outputDir = jobData.outputDirPath?.let { File(it) } ?: return
        if (!outputDir.exists()) {
            _exportResult.value = "Output directory not found"
            return
        }
        viewModelScope.launch {
            val zipName = "${ExportUtil.generateJobFolderName(jobData.timestamp)}.zip"
            val cacheZip = File(context.cacheDir, "export/$zipName")
            cacheZip.parentFile?.mkdirs()
            val zipFile = ExportUtil.createZipFromDirectory(outputDir, cacheZip)
            if (zipFile != null) {
                val uri = ExportUtil.saveFileToDownloads(context, zipFile, zipName)
                _exportResult.value = if (uri != null) {
                    "ZIP saved to Downloads/Il2CppDumper/$zipName"
                } else {
                    "Failed to save ZIP"
                }
                zipFile.delete()
            } else {
                _exportResult.value = "Failed to create ZIP"
            }
        }
    }

    fun shareFile(fileName: String) {
        val context = getApplication<Application>()
        val jobData = _job.value ?: return
        val outputDir = jobData.outputDirPath?.let { File(it) } ?: return
        val sourceFile = File(outputDir, fileName)
        val intent = ExportUtil.shareFile(context, sourceFile, getMimeType(fileName))
        if (intent != null) {
            _shareIntent.value = Intent.createChooser(intent, "Share $fileName")
        } else {
            _exportResult.value = "Failed to share $fileName"
        }
    }

    fun shareAllAsZip() {
        val context = getApplication<Application>()
        val jobData = _job.value ?: return
        val outputDir = jobData.outputDirPath?.let { File(it) } ?: return
        if (!outputDir.exists()) return
        viewModelScope.launch {
            val zipName = "${ExportUtil.generateJobFolderName(jobData.timestamp)}.zip"
            val cacheZip = File(context.cacheDir, "export/$zipName")
            cacheZip.parentFile?.mkdirs()
            val zipFile = ExportUtil.createZipFromDirectory(outputDir, cacheZip)
            if (zipFile != null) {
                val intent = ExportUtil.shareZip(context, zipFile)
                if (intent != null) {
                    _shareIntent.value = Intent.createChooser(intent, "Share dump files")
                }
            }
        }
    }

    fun deleteJob(onDone: () -> Unit) {
        viewModelScope.launch {
            repository.deleteJobWithFiles(jobId, jobsDir)
            onDone()
        }
    }

    fun clearExportResult() {
        _exportResult.value = null
    }

    fun clearShareIntent() {
        _shareIntent.value = null
    }

    private fun getMimeType(fileName: String): String = when {
        fileName.endsWith(".json") -> "application/json"
        fileName.endsWith(".cs") -> "text/plain"
        fileName.endsWith(".h") -> "text/plain"
        else -> "application/octet-stream"
    }
}
