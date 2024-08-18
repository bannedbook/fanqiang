package com.nononsenseapps.feeder.ui

import android.content.Context
import androidx.room.Room
import com.nononsenseapps.feeder.db.room.AppDatabase
import org.junit.rules.ExternalResource

class TestDatabaseRule(private val context: Context) : ExternalResource() {
    lateinit var db: AppDatabase

    override fun before() {
        db =
            Room.inMemoryDatabaseBuilder(
                context,
                AppDatabase::class.java,
            ).build().also {
                // Ensure all classes use test database
                AppDatabase.setInstance(it)
            }
    }

    override fun after() {
        // Don't close the databse or tests fail
    }
}
