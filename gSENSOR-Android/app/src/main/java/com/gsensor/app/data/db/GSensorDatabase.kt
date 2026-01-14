package com.gsensor.app.data.db

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import com.gsensor.app.data.model.AccelSample
import com.gsensor.app.data.model.RecordingSession

/**
 * Room database for gSENSOR app.
 */
@Database(
    entities = [RecordingSession::class, AccelSample::class],
    version = 1,
    exportSchema = false
)
abstract class GSensorDatabase : RoomDatabase() {

    abstract fun recordingDao(): RecordingDao

    companion object {
        @Volatile
        private var INSTANCE: GSensorDatabase? = null

        fun getDatabase(context: Context): GSensorDatabase {
            return INSTANCE ?: synchronized(this) {
                val instance = Room.databaseBuilder(
                    context.applicationContext,
                    GSensorDatabase::class.java,
                    "gsensor_database"
                ).build()
                INSTANCE = instance
                instance
            }
        }
    }
}
