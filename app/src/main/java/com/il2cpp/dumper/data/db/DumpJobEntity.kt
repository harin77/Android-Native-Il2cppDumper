package com.il2cpp.dumper.data.db

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "dump_jobs")
data class DumpJobEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val timestamp: Long = System.currentTimeMillis(),
    val metadataFileName: String = "",
    val il2cppFileName: String = "",
    val metadataHash: String? = null,
    val il2cppHash: String? = null,
    val metadataSize: Long = 0,
    val il2cppSize: Long = 0,
    val configJson: String = "",
    val status: JobStatus = JobStatus.RUNNING,
    val outputDirPath: String? = null,
    val logs: List<String> = emptyList(),
    val durationMs: Long? = null,
    val metadataVersion: Double? = null,
    val il2cppVersion: String? = null,
    val errorMessage: String? = null,
    val outputFileNames: List<String> = emptyList()
)
