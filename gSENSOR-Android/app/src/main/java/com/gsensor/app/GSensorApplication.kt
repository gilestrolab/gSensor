package com.gsensor.app

import android.app.Application
import com.gsensor.app.data.repository.GSensorRepository

/**
 * Application class for gSENSOR app.
 *
 * Provides application-wide repository instance.
 */
class GSensorApplication : Application() {

    lateinit var repository: GSensorRepository
        private set

    override fun onCreate() {
        super.onCreate()
        repository = GSensorRepository(this)
    }

    override fun onTerminate() {
        super.onTerminate()
        repository.close()
    }
}
