package com.il2cpp.dumper.util

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Build
import android.widget.Toast
import com.il2cpp.dumper.BuildConfig
import com.il2cpp.dumper.viewmodel.DumpConfig

object DebugInfoCollector {

    fun collect(
        metadataFileName: String? = null,
        il2cppFileName: String? = null,
        metadataSize: Long? = null,
        il2cppSize: Long? = null,
        config: DumpConfig? = null,
        logs: List<String> = emptyList(),
        metadataVersion: Double? = null,
        il2cppVersion: String? = null,
        errorMessage: String? = null,
        maxLogLines: Int = 50
    ): String {
        val sb = StringBuilder()
        sb.appendLine("=== Il2CppDumper Debug Info ===")
        sb.appendLine()
        sb.appendLine("--- App ---")
        sb.appendLine("Version: v${BuildConfig.VERSION_NAME} (${BuildConfig.VERSION_CODE})")
        sb.appendLine()
        sb.appendLine("--- Device ---")
        sb.appendLine("Manufacturer: ${Build.MANUFACTURER}")
        sb.appendLine("Model: ${Build.MODEL} (${Build.DEVICE})")
        sb.appendLine("Android: ${Build.VERSION.RELEASE} (SDK ${Build.VERSION.SDK_INT})")
        sb.appendLine("ABIs: ${Build.SUPPORTED_ABIS.joinToString()}")
        sb.appendLine()

        if (metadataFileName != null || il2cppFileName != null) {
            sb.appendLine("--- Files ---")
            if (metadataFileName != null) {
                val sizeStr = metadataSize?.let { formatFileSize(it) } ?: "unknown"
                sb.appendLine("metadata: $metadataFileName ($sizeStr)")
            }
            if (il2cppFileName != null) {
                val sizeStr = il2cppSize?.let { formatFileSize(it) } ?: "unknown"
                sb.appendLine("il2cpp: $il2cppFileName ($sizeStr)")
            }
            sb.appendLine()
        }

        if (config != null) {
            sb.appendLine("--- Config ---")
            sb.appendLine(config.toJson())
            sb.appendLine()
        }

        if (metadataVersion != null || il2cppVersion != null) {
            sb.appendLine("--- Versions ---")
            if (metadataVersion != null) sb.appendLine("Metadata: $metadataVersion")
            if (il2cppVersion != null) sb.appendLine("Il2Cpp: $il2cppVersion")
            sb.appendLine()
        }

        if (errorMessage != null) {
            sb.appendLine("--- Error ---")
            sb.appendLine(errorMessage)
            sb.appendLine()
        }

        if (logs.isNotEmpty()) {
            sb.appendLine("--- Logs (last $maxLogLines lines) ---")
            val logSubset = if (logs.size > maxLogLines) logs.takeLast(maxLogLines) else logs
            logSubset.forEach { sb.appendLine(it) }
        }

        return sb.toString()
    }

    fun copyToClipboard(context: Context, text: String) {
        val clipboard = context.getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        clipboard.setPrimaryClip(ClipData.newPlainText("Il2CppDumper Debug Info", text))
        Toast.makeText(context, "Debug info copied to clipboard", Toast.LENGTH_SHORT).show()
    }

    private fun formatFileSize(bytes: Long): String {
        return when {
            bytes < 1024 -> "$bytes B"
            bytes < 1024 * 1024 -> "%.1f KB".format(bytes / 1024.0)
            bytes < 1024 * 1024 * 1024 -> "%.1f MB".format(bytes / (1024.0 * 1024))
            else -> "%.2f GB".format(bytes / (1024.0 * 1024 * 1024))
        }
    }
}
