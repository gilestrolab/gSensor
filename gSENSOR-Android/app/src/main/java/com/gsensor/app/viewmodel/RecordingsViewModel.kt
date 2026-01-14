package com.gsensor.app.viewmodel

import android.content.Intent
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.gsensor.app.data.model.RecordingSession
import com.gsensor.app.data.repository.GSensorRepository
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.launch

/**
 * ViewModel for the recordings list screen.
 */
class RecordingsViewModel(private val repository: GSensorRepository) : ViewModel() {

    // All recording sessions
    val sessions: Flow<List<RecordingSession>> = repository.allSessions

    // Share intent events
    private val _shareIntent = MutableSharedFlow<Intent>()
    val shareIntent: SharedFlow<Intent> = _shareIntent.asSharedFlow()

    /**
     * Delete a recording session.
     */
    fun deleteSession(session: RecordingSession) {
        viewModelScope.launch {
            repository.deleteSession(session)
        }
    }

    /**
     * Export a session to CSV and trigger share.
     */
    fun exportSession(sessionId: Long) {
        viewModelScope.launch {
            val intent = repository.exportSession(sessionId)
            if (intent != null) {
                _shareIntent.emit(intent)
            }
        }
    }

    companion object {
        fun provideFactory(repository: GSensorRepository): ViewModelProvider.Factory {
            return object : ViewModelProvider.Factory {
                @Suppress("UNCHECKED_CAST")
                override fun <T : ViewModel> create(modelClass: Class<T>): T {
                    return RecordingsViewModel(repository) as T
                }
            }
        }
    }
}
