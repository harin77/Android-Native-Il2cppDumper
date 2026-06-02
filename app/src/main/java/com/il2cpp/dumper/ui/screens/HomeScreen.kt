package com.il2cpp.dumper.ui.screens

import android.net.Uri
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.ui.components.ConfigPanel
import com.il2cpp.dumper.ui.components.FileSelector
import com.il2cpp.dumper.ui.components.LogViewer
import com.il2cpp.dumper.viewmodel.DumpState
import com.il2cpp.dumper.viewmodel.DumperViewModel

@Composable
fun HomeScreen(
    viewModel: DumperViewModel,
    modifier: Modifier = Modifier
) {
    val state by viewModel.state.collectAsState()
    val logs by viewModel.logs.collectAsState()
    val config by viewModel.config.collectAsState()
    val metadataUri by viewModel.metadataUri.collectAsState()
    val il2cppUri by viewModel.il2cppUri.collectAsState()

    var metadataPicker by remember { mutableStateOf(false) }
    var il2cppPicker by remember { mutableStateOf(false) }

    val metadataLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        uri?.let { viewModel.setMetadataUri(it) }
    }

    val il2cppLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.OpenDocument()
    ) { uri: Uri? ->
        uri?.let { viewModel.setIl2cppUri(it) }
    }

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Text(
            text = "Il2Cpp Dumper",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )

        Text(
            text = "Select Il2Cpp binary and metadata files to dump",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        Spacer(modifier = Modifier.height(4.dp))

        FileSelector(
            title = "Il2Cpp Binary",
            subtitle = "libil2cpp.so (ELF)",
            selectedUri = il2cppUri,
            onSelect = { il2cppLauncher.launch(arrayOf("*/*")) }
        )

        FileSelector(
            title = "Global Metadata",
            subtitle = "global-metadata.dat",
            selectedUri = metadataUri,
            onSelect = { metadataLauncher.launch(arrayOf("*/*")) }
        )

        // Dump address input (for dump files)
        var dumpAddrText by remember { mutableStateOf("") }
        OutlinedTextField(
            value = dumpAddrText,
            onValueChange = {
                dumpAddrText = it
                // Parse hex address
                val addr = it.toLongOrNull(16) ?: 0L
                viewModel.setDumpAddress(addr)
            },
            label = { Text("Dump Address (optional)") },
            placeholder = { Text("0x0 (leave empty for auto-detect)") },
            modifier = Modifier.fillMaxWidth(),
            singleLine = true,
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Password),
            supportingText = { Text("For dump files: enter the image base address in hex") }
        )

        ConfigPanel(
            config = config,
            onConfigChange = { viewModel.updateConfig(it) }
        )

        Spacer(modifier = Modifier.height(4.dp))

        Button(
            onClick = { viewModel.startDump() },
            modifier = Modifier.fillMaxWidth(),
            enabled = state is DumpState.Idle || state is DumpState.Success || state is DumpState.Error
        ) {
            Icon(
                imageVector = Icons.Default.PlayArrow,
                contentDescription = null
            )
            Spacer(modifier = Modifier.padding(4.dp))
            Text(
                text = when (state) {
                    is DumpState.Initializing -> "Initializing..."
                    is DumpState.Searching -> "Searching..."
                    is DumpState.Dumping -> "Dumping..."
                    else -> "Start Dump"
                }
            )
        }

        if (state is DumpState.Initializing || state is DumpState.Searching || state is DumpState.Dumping) {
            LinearProgressIndicator(modifier = Modifier.fillMaxWidth())
        }

        if (logs.isNotEmpty()) {
            Text(
                text = "Log Output",
                style = MaterialTheme.typography.titleSmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
            LogViewer(logs = logs)
        }

        when (val s = state) {
            is DumpState.Success -> {
                Text(
                    text = "Dump completed successfully!",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.primary
                )
                Text(
                    text = "Output: ${s.outputDir}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
                Button(
                    onClick = { viewModel.reset() },
                    modifier = Modifier.fillMaxWidth(),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = MaterialTheme.colorScheme.secondary
                    )
                ) {
                    Text("Reset")
                }
            }
            is DumpState.Error -> {
                Text(
                    text = "Error: ${s.message}",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.error
                )
                Button(
                    onClick = { viewModel.reset() },
                    modifier = Modifier.fillMaxWidth()
                ) {
                    Text("Try Again")
                }
            }
            else -> {}
        }

        Spacer(modifier = Modifier.height(16.dp))
    }
}
