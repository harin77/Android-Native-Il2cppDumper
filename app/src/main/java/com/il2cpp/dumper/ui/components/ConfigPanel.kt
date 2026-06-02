package com.il2cpp.dumper.ui.components

import androidx.compose.animation.AnimatedVisibility
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ExpandLess
import androidx.compose.material.icons.filled.ExpandMore
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import com.il2cpp.dumper.viewmodel.DumpConfig

@Composable
fun ConfigPanel(
    config: DumpConfig,
    onConfigChange: (DumpConfig) -> Unit,
    modifier: Modifier = Modifier
) {
    var expanded by remember { mutableStateOf(false) }

    Card(
        modifier = modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surfaceVariant
        )
    ) {
        Column(modifier = Modifier.padding(16.dp)) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier.fillMaxWidth()
            ) {
                Icon(
                    imageVector = Icons.Default.Settings,
                    contentDescription = null,
                    tint = MaterialTheme.colorScheme.primary
                )
                Spacer(modifier = Modifier.width(12.dp))
                Text(
                    text = "Dump Options",
                    style = MaterialTheme.typography.titleSmall,
                    modifier = Modifier.weight(1f)
                )
                IconButton(onClick = { expanded = !expanded }) {
                    Icon(
                        imageVector = if (expanded) Icons.Default.ExpandLess else Icons.Default.ExpandMore,
                        contentDescription = if (expanded) "Collapse" else "Expand"
                    )
                }
            }

            AnimatedVisibility(visible = expanded) {
                Column {
                    Spacer(modifier = Modifier.height(8.dp))
                    ConfigSwitch("Dump Methods", config.dumpMethod) {
                        onConfigChange(config.copy(dumpMethod = it))
                    }
                    ConfigSwitch("Dump Fields", config.dumpField) {
                        onConfigChange(config.copy(dumpField = it))
                    }
                    ConfigSwitch("Dump Properties", config.dumpProperty) {
                        onConfigChange(config.copy(dumpProperty = it))
                    }
                    ConfigSwitch("Dump Attributes", config.dumpAttribute) {
                        onConfigChange(config.copy(dumpAttribute = it))
                    }
                    ConfigSwitch("Field Offsets", config.dumpFieldOffset) {
                        onConfigChange(config.copy(dumpFieldOffset = it))
                    }
                    ConfigSwitch("Method Offsets", config.dumpMethodOffset) {
                        onConfigChange(config.copy(dumpMethodOffset = it))
                    }
                    ConfigSwitch("TypeDef Index", config.dumpTypeDefIndex) {
                        onConfigChange(config.copy(dumpTypeDefIndex = it))
                    }
                    ConfigSwitch("Generate Struct (il2cpp.h)", config.generateStruct) {
                        onConfigChange(config.copy(generateStruct = it))
                    }
                    ConfigSwitch("Force Dump", config.forceDump) {
                        onConfigChange(config.copy(forceDump = it))
                    }
                    ConfigSwitch("Force Il2Cpp Version", config.forceIl2CppVersion) {
                        onConfigChange(config.copy(forceIl2CppVersion = it))
                    }
                    AnimatedVisibility(visible = config.forceIl2CppVersion) {
                        OutlinedTextField(
                            value = config.forceVersion.toString(),
                            onValueChange = { value ->
                                val v = value.toDoubleOrNull() ?: return@OutlinedTextField
                                onConfigChange(config.copy(forceVersion = v))
                            },
                            label = { Text("Il2Cpp Version") },
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(start = 16.dp, top = 4.dp, bottom = 4.dp),
                            singleLine = true,
                            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Decimal)
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun ConfigSwitch(
    label: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
    ) {
        Text(
            text = label,
            style = MaterialTheme.typography.bodyMedium,
            modifier = Modifier.weight(1f)
        )
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange
        )
    }
}
