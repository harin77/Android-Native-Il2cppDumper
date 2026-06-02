package com.il2cpp.dumper.ui.screens

import android.content.Intent
import android.net.Uri
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.ContentCopy
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Download
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.FolderZip
import androidx.compose.material.icons.filled.OpenInNew
import androidx.compose.material.icons.filled.Share
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.data.db.JobStatus
import com.il2cpp.dumper.ui.components.LogViewer
import com.il2cpp.dumper.ui.theme.DumpGreen
import com.il2cpp.dumper.ui.theme.DumpRed
import com.il2cpp.dumper.ui.theme.DumpOrange
import com.il2cpp.dumper.util.DebugInfoCollector
import com.il2cpp.dumper.viewmodel.DumpConfig
import com.il2cpp.dumper.viewmodel.JobDetailViewModel
import org.json.JSONObject

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun JobDetailScreen(
    viewModel: JobDetailViewModel,
    onBack: () -> Unit,
    modifier: Modifier = Modifier
) {
    val job by viewModel.job.collectAsState()
    val exportResult by viewModel.exportResult.collectAsState()
    val shareIntent by viewModel.shareIntent.collectAsState()
    val snackbarHostState = remember { SnackbarHostState() }
    val context = LocalContext.current

    LaunchedEffect(exportResult) {
        exportResult?.let {
            snackbarHostState.showSnackbar(it)
            viewModel.clearExportResult()
        }
    }

    LaunchedEffect(shareIntent) {
        shareIntent?.let {
            context.startActivity(it)
            viewModel.clearShareIntent()
        }
    }

    Box(modifier = modifier.fillMaxSize()) {
        Column(modifier = Modifier.fillMaxSize()) {
            TopAppBar(
                title = { Text("Job Details") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    IconButton(onClick = {
                        viewModel.deleteJob(onBack)
                    }) {
                        Icon(Icons.Default.Delete, contentDescription = "Delete")
                    }
                }
            )

            val jobData = job
            if (jobData == null) {
                Text(
                    text = "Loading...",
                    modifier = Modifier
                        .fillMaxSize()
                        .padding(16.dp)
                )
                return@Box
            }

            Column(
                modifier = Modifier
                    .fillMaxSize()
                    .verticalScroll(rememberScrollState())
                    .padding(16.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp)
            ) {
            // Status banner
            StatusBanner(jobData)

            // Input Files section
            SectionCard("Input Files") {
                InfoRow("Metadata", jobData.metadataFileName)
                if (jobData.metadataSize > 0) {
                    InfoRow("Metadata Size", formatFileSize(jobData.metadataSize))
                }
                if (jobData.metadataHash != null) {
                    InfoRow("Metadata SHA-256", jobData.metadataHash.take(16) + "...")
                }
                InfoRow("Il2Cpp", jobData.il2cppFileName)
                if (jobData.il2cppSize > 0) {
                    InfoRow("Il2Cpp Size", formatFileSize(jobData.il2cppSize))
                }
                if (jobData.il2cppHash != null) {
                    InfoRow("Il2Cpp SHA-256", jobData.il2cppHash.take(16) + "...")
                }
            }

            // Metadata section
            SectionCard("Metadata") {
                if (jobData.metadataVersion != null) {
                    InfoRow("Metadata Version", jobData.metadataVersion.toString())
                }
                if (jobData.il2cppVersion != null) {
                    InfoRow("Il2Cpp Version", jobData.il2cppVersion)
                }
                if (jobData.durationMs != null) {
                    InfoRow("Duration", formatDuration(jobData.durationMs))
                }
            }

            // Configuration section
            if (jobData.configJson.isNotBlank()) {
                SectionCard("Configuration") {
                    val config = parseConfig(jobData.configJson)
                    config.forEach { (key, value) ->
                        InfoRow(key, value)
                    }
                }
            }

            // Output Files section
            if (jobData.status == JobStatus.SUCCESS && jobData.outputFileNames.isNotEmpty()) {
                SectionCard("Output Files") {
                    jobData.outputFileNames.forEach { fileName ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(vertical = 4.dp),
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            Text(
                                text = fileName,
                                style = MaterialTheme.typography.bodyMedium,
                                modifier = Modifier.weight(1f)
                            )
                            IconButton(onClick = { viewModel.exportFile(fileName) }) {
                                Icon(Icons.Default.Download, contentDescription = "Export", modifier = Modifier.padding(4.dp))
                            }
                            IconButton(onClick = { viewModel.shareFile(fileName) }) {
                                Icon(Icons.Default.Share, contentDescription = "Share", modifier = Modifier.padding(4.dp))
                            }
                        }
                    }

                    Spacer(modifier = Modifier.height(8.dp))

                    Row(
                        modifier = Modifier.fillMaxWidth(),
                        horizontalArrangement = Arrangement.spacedBy(8.dp)
                    ) {
                        Button(
                            onClick = { viewModel.exportAllAsZip() },
                            modifier = Modifier.weight(1f)
                        ) {
                            Icon(Icons.Default.FolderZip, contentDescription = null)
                            Spacer(modifier = Modifier.width(4.dp))
                            Text("Export ZIP")
                        }
                        OutlinedButton(
                            onClick = { viewModel.shareAllAsZip() },
                            modifier = Modifier.weight(1f)
                        ) {
                            Icon(Icons.Default.Share, contentDescription = null)
                            Spacer(modifier = Modifier.width(4.dp))
                            Text("Share ZIP")
                        }
                    }
                }
            }

            // Error section
            if (jobData.status == JobStatus.FAILED && jobData.errorMessage != null) {
                SectionCard("Error") {
                    Text(
                        text = jobData.errorMessage,
                        style = MaterialTheme.typography.bodyMedium,
                        color = DumpRed
                    )
                    Spacer(modifier = Modifier.height(8.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        OutlinedButton(onClick = {
                            val debugInfo = DebugInfoCollector.collect(
                                metadataFileName = jobData.metadataFileName,
                                il2cppFileName = jobData.il2cppFileName,
                                metadataSize = jobData.metadataSize,
                                il2cppSize = jobData.il2cppSize,
                                config = parseConfigToDumpConfig(jobData.configJson),
                                logs = jobData.logs,
                                metadataVersion = jobData.metadataVersion,
                                il2cppVersion = jobData.il2cppVersion,
                                errorMessage = jobData.errorMessage
                            )
                            DebugInfoCollector.copyToClipboard(context, debugInfo)
                        }) {
                            Icon(Icons.Default.ContentCopy, contentDescription = null, modifier = Modifier.padding(end = 4.dp))
                            Text("Copy Debug Info")
                        }
                        OutlinedButton(onClick = {
                            val debugInfo = DebugInfoCollector.collect(
                                metadataFileName = jobData.metadataFileName,
                                il2cppFileName = jobData.il2cppFileName,
                                logs = jobData.logs,
                                errorMessage = jobData.errorMessage
                            )
                            DebugInfoCollector.copyToClipboard(context, debugInfo)
                            context.startActivity(Intent(Intent.ACTION_VIEW, Uri.parse("https://t.me/+GN7d4fxJAVgxMjg1")))
                        }) {
                            Icon(Icons.Default.OpenInNew, contentDescription = null, modifier = Modifier.padding(end = 4.dp))
                            Text("Report Error")
                        }
                    }
                }
            }

            // Logs section
            if (jobData.logs.isNotEmpty()) {
                var logsExpanded by remember { mutableStateOf(false) }
                SectionCard("Logs", expandable = true, expanded = logsExpanded, onToggle = { logsExpanded = !logsExpanded }) {
                    if (logsExpanded) {
                        LogViewer(logs = jobData.logs)
                    }
                }
            }

            Spacer(modifier = Modifier.height(16.dp))
            }
        }

        SnackbarHost(
            hostState = snackbarHostState,
            modifier = Modifier.align(Alignment.BottomCenter)
        )
    }
}

@Composable
private fun StatusBanner(job: DumpJobEntity) {
    val (color, text) = when (job.status) {
        JobStatus.SUCCESS -> DumpGreen to "Completed Successfully"
        JobStatus.FAILED -> DumpRed to "Failed"
        JobStatus.RUNNING -> DumpOrange to "Running"
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(containerColor = color.copy(alpha = 0.15f))
    ) {
        Text(
            text = text,
            style = MaterialTheme.typography.titleMedium,
            color = color,
            modifier = Modifier.padding(16.dp)
        )
    }
}

@Composable
private fun SectionCard(
    title: String,
    expandable: Boolean = false,
    expanded: Boolean = true,
    onToggle: () -> Unit = {},
    content: @Composable () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceContainerLow
        )
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = title,
                    style = MaterialTheme.typography.titleSmall,
                    color = MaterialTheme.colorScheme.primary,
                    modifier = Modifier.weight(1f)
                )
                if (expandable) {
                    IconButton(onClick = onToggle) {
                        Icon(
                            if (expanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                            contentDescription = if (expanded) "Collapse" else "Expand"
                        )
                    }
                }
            }
            if (!expandable || expanded) {
                Spacer(modifier = Modifier.height(8.dp))
                content()
            }
        }
    }
}

@Composable
private fun InfoRow(label: String, value: String) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 2.dp)
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
            modifier = Modifier.width(120.dp)
        )
        Text(
            text = value,
            style = MaterialTheme.typography.bodySmall
        )
    }
}

private fun parseConfig(json: String): List<Pair<String, String>> {
    return try {
        val obj = JSONObject(json)
        obj.keys().asSequence().map { key ->
            key to obj.get(key).toString()
        }.toList()
    } catch (_: Exception) {
        emptyList()
    }
}

private fun parseConfigToDumpConfig(json: String): DumpConfig? {
    return try {
        val obj = JSONObject(json)
        DumpConfig(
            dumpMethod = obj.optBoolean("DumpMethod", true),
            dumpField = obj.optBoolean("DumpField", true),
            dumpProperty = obj.optBoolean("DumpProperty", false),
            dumpAttribute = obj.optBoolean("DumpAttribute", false),
            dumpFieldOffset = obj.optBoolean("DumpFieldOffset", true),
            dumpMethodOffset = obj.optBoolean("DumpMethodOffset", true),
            dumpTypeDefIndex = obj.optBoolean("DumpTypeDefIndex", true),
            generateStruct = obj.optBoolean("GenerateStruct", true),
            forceIl2CppVersion = obj.optBoolean("ForceIl2CppVersion", false),
            forceVersion = obj.optDouble("ForceVersion", 24.3),
            forceDump = obj.optBoolean("ForceDump", false)
        )
    } catch (_: Exception) {
        null
    }
}

private fun formatFileSize(bytes: Long): String {
    return when {
        bytes < 1024 -> "$bytes B"
        bytes < 1024 * 1024 -> "%.1f KB".format(bytes / 1024.0)
        bytes < 1024 * 1024 * 1024 -> "%.1f MB".format(bytes / (1024.0 * 1024))
        else -> "%.2f GB".format(bytes / (1024.0 * 1024 * 1024))
    }
}

private fun formatDuration(ms: Long): String {
    return when {
        ms < 1000 -> "${ms}ms"
        ms < 60000 -> "%.1fs".format(ms / 1000.0)
        else -> "%dm %ds".format(ms / 60000, (ms % 60000) / 1000)
    }
}
