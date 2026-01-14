package com.gsensor.app

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.navigation.compose.rememberNavController
import com.gsensor.app.ui.navigation.NavGraph
import com.gsensor.app.ui.theme.GSensorTheme
import com.gsensor.app.viewmodel.LiveViewModel
import com.gsensor.app.viewmodel.RecordingsViewModel
import com.gsensor.app.viewmodel.ScanViewModel

class MainActivity : ComponentActivity() {

    private val scanViewModel: ScanViewModel by viewModels {
        ScanViewModel.provideFactory((application as GSensorApplication).repository)
    }

    private val liveViewModel: LiveViewModel by viewModels {
        LiveViewModel.provideFactory((application as GSensorApplication).repository)
    }

    private val recordingsViewModel: RecordingsViewModel by viewModels {
        RecordingsViewModel.provideFactory((application as GSensorApplication).repository)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            GSensorTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    val navController = rememberNavController()
                    NavGraph(
                        navController = navController,
                        scanViewModel = scanViewModel,
                        liveViewModel = liveViewModel,
                        recordingsViewModel = recordingsViewModel
                    )
                }
            }
        }
    }
}
