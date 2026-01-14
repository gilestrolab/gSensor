package com.gsensor.app.data.export

import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.core.content.FileProvider
import com.gsensor.app.data.model.AccelSample
import com.gsensor.app.data.model.RecordingSession
import java.io.File
import java.io.FileWriter
import java.text.SimpleDateFormat
import java.util.*

/**
 * Exports recording data to CSV format.
 */
class CsvExporter(private val context: Context) {

    private val dateFormat = SimpleDateFormat("yyyy-MM-dd_HH-mm-ss", Locale.US)

    /**
     * Export recording session to CSV file.
     *
     * @param session The recording session metadata
     * @param samples The accelerometer samples
     * @return File object pointing to the created CSV
     */
    fun exportToCsv(session: RecordingSession, samples: List<AccelSample>): File {
        val exportDir = File(context.cacheDir, "exports").apply { mkdirs() }
        val fileName = "gsensor_${dateFormat.format(Date(session.startTime))}.csv"
        val file = File(exportDir, fileName)

        FileWriter(file).use { writer ->
            // Write header
            writer.appendLine("timestamp,x,y,z,magnitude")

            // Write data rows
            samples.forEach { sample ->
                writer.appendLine("${sample.timestamp},${sample.x},${sample.y},${sample.z},${sample.magnitude}")
            }
        }

        return file
    }

    /**
     * Create a share intent for the CSV file.
     *
     * @param file The CSV file to share
     * @return Intent for sharing
     */
    fun createShareIntent(file: File): Intent {
        val uri: Uri = FileProvider.getUriForFile(
            context,
            "${context.packageName}.fileprovider",
            file
        )

        return Intent(Intent.ACTION_SEND).apply {
            type = "text/csv"
            putExtra(Intent.EXTRA_STREAM, uri)
            putExtra(Intent.EXTRA_SUBJECT, "gSENSOR Recording Export")
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        }
    }

    /**
     * Export and immediately share.
     *
     * @param session The recording session metadata
     * @param samples The accelerometer samples
     * @return Intent for sharing
     */
    fun exportAndShare(session: RecordingSession, samples: List<AccelSample>): Intent {
        val file = exportToCsv(session, samples)
        return createShareIntent(file)
    }
}
