package com.il2cpp.dumper.util

import android.content.ContentValues
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.MediaStore
import androidx.core.content.FileProvider
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.zip.ZipEntry
import java.util.zip.ZipOutputStream

object ExportUtil {

    fun generateJobFolderName(timestamp: Long): String {
        val sdf = SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.US)
        return "Il2CppDump_${sdf.format(Date(timestamp))}"
    }

    fun saveFileToDownloads(context: Context, sourceFile: File, displayName: String): Uri? {
        if (!sourceFile.exists()) return null
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            saveWithMediaStore(context, sourceFile, displayName, getMimeType(displayName))
        } else {
            saveLegacy(sourceFile, displayName)
        }
    }

    fun createZipFromDirectory(sourceDir: File, zipFile: File): File? {
        if (!sourceDir.exists() || !sourceDir.isDirectory) return null
        return try {
            ZipOutputStream(FileOutputStream(zipFile)).use { zos ->
                sourceDir.listFiles()?.forEach { file ->
                    FileInputStream(file).use { fis ->
                        zos.putNextEntry(ZipEntry(file.name))
                        fis.copyTo(zos)
                        zos.closeEntry()
                    }
                }
            }
            zipFile
        } catch (_: Exception) {
            null
        }
    }

    fun shareFile(context: Context, file: File, mimeType: String): Intent? {
        if (!file.exists()) return null
        val uri = FileProvider.getUriForFile(
            context,
            "${context.packageName}.fileprovider",
            file
        )
        return Intent(Intent.ACTION_SEND).apply {
            type = mimeType
            putExtra(Intent.EXTRA_STREAM, uri)
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
    }

    fun shareZip(context: Context, zipFile: File): Intent? {
        return shareFile(context, zipFile, "application/zip")
    }

    private fun saveWithMediaStore(context: Context, sourceFile: File, displayName: String, mimeType: String): Uri? {
        val contentValues = ContentValues().apply {
            put(MediaStore.Downloads.DISPLAY_NAME, displayName)
            put(MediaStore.Downloads.MIME_TYPE, mimeType)
            put(MediaStore.Downloads.RELATIVE_PATH, "${Environment.DIRECTORY_DOWNLOADS}/Il2CppDumper")
        }
        val uri = context.contentResolver.insert(MediaStore.Downloads.EXTERNAL_CONTENT_URI, contentValues) ?: return null
        return try {
            context.contentResolver.openOutputStream(uri)?.use { output ->
                FileInputStream(sourceFile).use { input ->
                    input.copyTo(output)
                }
            }
            uri
        } catch (_: Exception) {
            context.contentResolver.delete(uri, null, null)
            null
        }
    }

    private fun saveLegacy(sourceFile: File, displayName: String): Uri? {
        return try {
            val downloadsDir = File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "Il2CppDumper"
            )
            downloadsDir.mkdirs()
            val targetFile = File(downloadsDir, displayName)
            sourceFile.copyTo(targetFile, overwrite = true)
            Uri.fromFile(targetFile)
        } catch (_: Exception) {
            null
        }
    }

    private fun getMimeType(fileName: String): String {
        return when {
            fileName.endsWith(".json") -> "application/json"
            fileName.endsWith(".cs") -> "text/plain"
            fileName.endsWith(".h") -> "text/plain"
            fileName.endsWith(".zip") -> "application/zip"
            else -> "application/octet-stream"
        }
    }
}
