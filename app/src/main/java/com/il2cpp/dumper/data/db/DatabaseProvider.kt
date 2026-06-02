package com.il2cpp.dumper.data.db

import android.content.Context
import androidx.room.Room

object DatabaseProvider {
    @Volatile
    private var database: AppDatabase? = null

    fun getDatabase(context: Context): AppDatabase {
        return database ?: synchronized(this) {
            database ?: Room.databaseBuilder(
                context.applicationContext,
                AppDatabase::class.java,
                "il2cpp_dumper.db"
            ).build().also { database = it }
        }
    }

    fun getDao(context: Context): DumpJobDao = getDatabase(context).dumpJobDao()
}
