package com.gsensor.app.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.FiberManualRecord
import androidx.compose.material.icons.filled.History
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material.icons.filled.Stop
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.gsensor.app.data.ble.ConnectionState
import com.gsensor.app.data.model.AccelData
import com.gsensor.app.ui.components.AccelChart
import com.gsensor.app.viewmodel.LiveViewModel
import java.util.Locale

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LiveScreen(
    viewModel: LiveViewModel,
    onNavigateToRecordings: () -> Unit,
    onDisconnected: () -> Unit
) {
    val connectionState by viewModel.connectionState.collectAsState()
    val latestData by viewModel.latestData.collectAsState()
    val chartData by viewModel.chartData.collectAsState()
    val peakValue by viewModel.peakValue.collectAsState()
    val isRecording by viewModel.isRecording.collectAsState()
    val sampleCount by viewModel.recordingSampleCount.collectAsState()
    val recordingStartTime by viewModel.recordingStartTime.collectAsState()

    // Handle disconnection
    LaunchedEffect(connectionState) {
        if (connectionState is ConnectionState.Disconnected) {
            onDisconnected()
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("gSENSOR Live") },
                navigationIcon = {
                    IconButton(onClick = {
                        viewModel.disconnect()
                        onDisconnected()
                    }) {
                        Icon(
                            imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                            contentDescription = "Disconnect"
                        )
                    }
                },
                actions = {
                    IconButton(onClick = onNavigateToRecordings) {
                        Icon(
                            imageVector = Icons.Default.History,
                            contentDescription = "Recordings"
                        )
                    }
                }
            )
        }
    ) { padding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Connection status
            ConnectionStatusBar(connectionState)

            // Magnitude display (large)
            MagnitudeCard(
                magnitude = latestData?.magnitude ?: 0f,
                peak = peakValue,
                onResetPeak = { viewModel.resetPeak() }
            )

            // X, Y, Z values
            AccelValuesRow(latestData)

            // Chart
            AccelChart(
                data = chartData,
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f)
            )

            // Recording controls
            RecordingControls(
                isRecording = isRecording,
                sampleCount = sampleCount,
                startTime = recordingStartTime,
                onStartRecording = { viewModel.startRecording() },
                onStopRecording = { viewModel.stopRecording() }
            )
        }
    }
}

@Composable
private fun ConnectionStatusBar(state: ConnectionState) {
    val (text, color) = when (state) {
        is ConnectionState.Connected -> "Connected" to MaterialTheme.colorScheme.primary
        is ConnectionState.Connecting -> "Connecting..." to MaterialTheme.colorScheme.secondary
        is ConnectionState.Disconnected -> "Disconnected" to MaterialTheme.colorScheme.error
        is ConnectionState.Error -> "Error: ${state.message}" to MaterialTheme.colorScheme.error
    }

    Surface(
        color = color.copy(alpha = 0.1f),
        shape = MaterialTheme.shapes.small
    ) {
        Text(
            text = text,
            color = color,
            modifier = Modifier.padding(horizontal = 12.dp, vertical = 4.dp),
            style = MaterialTheme.typography.labelMedium
        )
    }
}

@Composable
private fun MagnitudeCard(
    magnitude: Float,
    peak: Float,
    onResetPeak: () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.primaryContainer
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = "Magnitude",
                style = MaterialTheme.typography.titleMedium,
                color = MaterialTheme.colorScheme.onPrimaryContainer
            )
            Text(
                text = String.format(Locale.US, "%.2f g", magnitude),
                style = MaterialTheme.typography.displayMedium.copy(
                    fontWeight = FontWeight.Bold
                ),
                color = MaterialTheme.colorScheme.onPrimaryContainer
            )
            Spacer(modifier = Modifier.height(8.dp))
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                Text(
                    text = "Peak: ${String.format(Locale.US, "%.2f g", peak)}",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onPrimaryContainer.copy(alpha = 0.8f)
                )
                IconButton(
                    onClick = onResetPeak,
                    modifier = Modifier.size(32.dp)
                ) {
                    Icon(
                        imageVector = Icons.Default.Refresh,
                        contentDescription = "Reset Peak",
                        tint = MaterialTheme.colorScheme.onPrimaryContainer,
                        modifier = Modifier.size(20.dp)
                    )
                }
            }
        }
    }
}

@Composable
private fun AccelValuesRow(data: AccelData?) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        AccelValueCard(
            label = "X",
            value = data?.x ?: 0f,
            color = Color(0xFFE57373),
            modifier = Modifier.weight(1f)
        )
        AccelValueCard(
            label = "Y",
            value = data?.y ?: 0f,
            color = Color(0xFF81C784),
            modifier = Modifier.weight(1f)
        )
        AccelValueCard(
            label = "Z",
            value = data?.z ?: 0f,
            color = Color(0xFF64B5F6),
            modifier = Modifier.weight(1f)
        )
    }
}

@Composable
private fun AccelValueCard(
    label: String,
    value: Float,
    color: Color,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(
            containerColor = color.copy(alpha = 0.15f)
        )
    ) {
        Column(
            modifier = Modifier
                .fillMaxWidth()
                .padding(12.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = label,
                style = MaterialTheme.typography.labelMedium,
                color = color
            )
            Text(
                text = String.format(Locale.US, "%.2f", value),
                style = MaterialTheme.typography.titleLarge.copy(
                    fontWeight = FontWeight.Bold
                ),
                color = color
            )
        }
    }
}

@Composable
private fun RecordingControls(
    isRecording: Boolean,
    sampleCount: Int,
    startTime: Long?,
    onStartRecording: () -> Unit,
    onStopRecording: () -> Unit
) {
    var elapsedSeconds by remember { mutableLongStateOf(0L) }

    // Update elapsed time every second while recording
    LaunchedEffect(isRecording, startTime) {
        if (isRecording && startTime != null) {
            while (true) {
                elapsedSeconds = (System.currentTimeMillis() - startTime) / 1000
                kotlinx.coroutines.delay(1000)
            }
        } else {
            elapsedSeconds = 0
        }
    }

    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = if (isRecording) {
                MaterialTheme.colorScheme.errorContainer
            } else {
                MaterialTheme.colorScheme.surfaceVariant
            }
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically,
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Column {
                if (isRecording) {
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Icon(
                            imageVector = Icons.Default.FiberManualRecord,
                            contentDescription = null,
                            tint = MaterialTheme.colorScheme.error,
                            modifier = Modifier.size(12.dp)
                        )
                        Spacer(modifier = Modifier.width(4.dp))
                        Text(
                            text = "Recording",
                            style = MaterialTheme.typography.titleMedium,
                            color = MaterialTheme.colorScheme.onErrorContainer
                        )
                    }
                    Text(
                        text = formatDuration(elapsedSeconds) + " • $sampleCount samples",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onErrorContainer.copy(alpha = 0.8f)
                    )
                } else {
                    Text(
                        text = "Ready to Record",
                        style = MaterialTheme.typography.titleMedium
                    )
                    Text(
                        text = "Tap button to start",
                        style = MaterialTheme.typography.bodyMedium,
                        color = MaterialTheme.colorScheme.onSurfaceVariant
                    )
                }
            }

            Button(
                onClick = if (isRecording) onStopRecording else onStartRecording,
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (isRecording) {
                        MaterialTheme.colorScheme.error
                    } else {
                        MaterialTheme.colorScheme.primary
                    }
                )
            ) {
                Icon(
                    imageVector = if (isRecording) Icons.Default.Stop else Icons.Default.FiberManualRecord,
                    contentDescription = null,
                    modifier = Modifier.size(20.dp)
                )
                Spacer(modifier = Modifier.width(8.dp))
                Text(if (isRecording) "Stop" else "Record")
            }
        }
    }
}

private fun formatDuration(seconds: Long): String {
    val mins = seconds / 60
    val secs = seconds % 60
    return String.format(Locale.US, "%02d:%02d", mins, secs)
}
