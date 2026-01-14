package com.gsensor.app.ui.navigation

import androidx.compose.runtime.Composable
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import com.gsensor.app.ui.screens.AboutScreen
import com.gsensor.app.ui.screens.LiveScreen
import com.gsensor.app.ui.screens.RecordingsScreen
import com.gsensor.app.ui.screens.ScanScreen
import com.gsensor.app.viewmodel.LiveViewModel
import com.gsensor.app.viewmodel.RecordingsViewModel
import com.gsensor.app.viewmodel.ScanViewModel

sealed class Screen(val route: String) {
    data object Scan : Screen("scan")
    data object Live : Screen("live")
    data object Recordings : Screen("recordings")
    data object About : Screen("about")
}

@Composable
fun NavGraph(
    navController: NavHostController,
    scanViewModel: ScanViewModel,
    liveViewModel: LiveViewModel,
    recordingsViewModel: RecordingsViewModel
) {
    NavHost(
        navController = navController,
        startDestination = Screen.Scan.route
    ) {
        composable(Screen.Scan.route) {
            ScanScreen(
                viewModel = scanViewModel,
                onConnected = {
                    navController.navigate(Screen.Live.route) {
                        popUpTo(Screen.Scan.route)
                    }
                },
                onNavigateToAbout = {
                    navController.navigate(Screen.About.route)
                }
            )
        }

        composable(Screen.Live.route) {
            LiveScreen(
                viewModel = liveViewModel,
                onNavigateToRecordings = {
                    navController.navigate(Screen.Recordings.route)
                },
                onDisconnected = {
                    navController.navigate(Screen.Scan.route) {
                        popUpTo(Screen.Scan.route) { inclusive = true }
                    }
                }
            )
        }

        composable(Screen.Recordings.route) {
            RecordingsScreen(
                viewModel = recordingsViewModel,
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }

        composable(Screen.About.route) {
            AboutScreen(
                onNavigateBack = {
                    navController.popBackStack()
                }
            )
        }
    }
}
