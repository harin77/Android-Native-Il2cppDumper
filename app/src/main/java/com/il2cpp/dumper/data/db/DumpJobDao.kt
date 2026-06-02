package com.il2cpp.dumper.data.db

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Update
import kotlinx.coroutines.flow.Flow

@Dao
interface DumpJobDao {
    @Query("SELECT * FROM dump_jobs ORDER BY timestamp DESC")
    fun getAllJobs(): Flow<List<DumpJobEntity>>

    @Query("SELECT * FROM dump_jobs WHERE id = :id")
    fun getJobById(id: Long): Flow<DumpJobEntity?>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    suspend fun insertJob(job: DumpJobEntity): Long

    @Update
    suspend fun updateJob(job: DumpJobEntity)

    @Query("DELETE FROM dump_jobs WHERE id = :id")
    suspend fun deleteJob(id: Long)

    @Query("DELETE FROM dump_jobs")
    suspend fun deleteAllJobs()

    @Query("SELECT COUNT(*) FROM dump_jobs")
    fun getJobCount(): Flow<Int>
}
