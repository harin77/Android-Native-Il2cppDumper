package com.il2cpp.dumper.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.il2cpp.dumper.data.db.DumpJobEntity
import com.il2cpp.dumper.data.db.DatabaseProvider
import com.il2cpp.dumper.data.repository.DumpJobRepository
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import java.io.File

class JobHistoryViewModel(application: Application) : AndroidViewModel(application) {

    private val repository = DumpJobRepository(DatabaseProvider.getDao(application))

    val jobs: StateFlow<List<DumpJobEntity>> = repository.getAllJobs()
        .stateIn(viewModelScope, SharingStarted.WhileSubscribed(5000), emptyList())

    private val jobsDir = File(application.filesDir, "jobs")

    fun deleteJob(id: Long) {
        viewModelScope.launch {
            repository.deleteJobWithFiles(id, jobsDir)
        }
    }

    fun clearAll() {
        viewModelScope.launch {
            repository.deleteAllJobsWithFiles(jobsDir)
        }
    }
}
