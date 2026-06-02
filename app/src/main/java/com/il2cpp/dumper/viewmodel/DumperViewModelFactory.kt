package com.il2cpp.dumper.viewmodel

import android.app.Application
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import com.il2cpp.dumper.data.db.DatabaseProvider
import com.il2cpp.dumper.data.repository.DumpJobRepository

class DumperViewModelFactory(private val application: Application) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(DumperViewModel::class.java)) {
            val dao = DatabaseProvider.getDao(application)
            val repository = DumpJobRepository(dao)
            return DumperViewModel(application, repository) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class: ${modelClass.name}")
    }
}
