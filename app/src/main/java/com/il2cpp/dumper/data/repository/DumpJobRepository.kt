package com.il2cpp.dumper.data.repository

import com.il2cpp.dumper.data.db.DumpJobDao
import com.il2cpp.dumper.data.db.DumpJobEntity
import kotlinx.coroutines.flow.Flow
import java.io.File
import java.io.FileInputStream
import java.security.MessageDigest

class DumpJobRepository(private val dao: DumpJobDao) {

    fun getAllJobs(): Flow<List<DumpJobEntity>> = dao.getAllJobs()

    fun getJobById(id: Long): Flow<DumpJobEntity?> = dao.getJobById(id)

    fun getJobCount(): Flow<Int> = dao.getJobCount()

    suspend fun insertJob(job: DumpJobEntity): Long = dao.insertJob(job)

    suspend fun updateJob(job: DumpJobEntity) = dao.updateJob(job)

    suspend fun deleteJob(id: Long) = dao.deleteJob(id)

    suspend fun deleteAllJobs() = dao.deleteAllJobs()

    suspend fun deleteJobWithFiles(id: Long, jobsDir: File) {
        dao.deleteJob(id)
        val jobDir = File(jobsDir, id.toString())
        if (jobDir.exists()) jobDir.deleteRecursively()
    }

    suspend fun deleteAllJobsWithFiles(jobsDir: File) {
        dao.deleteAllJobs()
        if (jobsDir.exists()) jobsDir.deleteRecursively()
    }

    companion object {
        fun computeFileHash(file: File): String? {
            return try {
                val digest = MessageDigest.getInstance("SHA-256")
                FileInputStream(file).use { fis ->
                    val buffer = ByteArray(8192)
                    var read: Int
                    while (fis.read(buffer).also { read = it } > 0) {
                        digest.update(buffer, 0, read)
                    }
                }
                digest.digest().joinToString("") { "%02x".format(it) }
            } catch (_: Exception) {
                null
            }
        }
    }
}
