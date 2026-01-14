package com.gsensor.app.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import com.gsensor.app.data.model.AccelData

@Composable
fun AccelChart(
    data: List<AccelData>,
    modifier: Modifier = Modifier
) {
    Card(
        modifier = modifier,
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.surface
        )
    ) {
        if (data.isEmpty()) {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = "Waiting for data...",
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        } else {
            Box(modifier = Modifier.fillMaxSize().padding(8.dp)) {
                Canvas(modifier = Modifier.fillMaxSize()) {
                    val width = size.width
                    val height = size.height
                    val centerY = height / 2

                    // Find min/max for scaling
                    val allValues = data.flatMap { listOf(it.x, it.y, it.z, it.magnitude) }
                    val maxVal = allValues.maxOrNull()?.coerceAtLeast(1f) ?: 1f
                    val minVal = allValues.minOrNull()?.coerceAtMost(-1f) ?: -1f
                    val range = (maxVal - minVal).coerceAtLeast(0.1f)

                    fun valueToY(value: Float): Float {
                        return height - ((value - minVal) / range * height)
                    }

                    val lineColors = listOf(
                        Color(0xFFE57373) to { d: AccelData -> d.x },     // X - Red
                        Color(0xFF81C784) to { d: AccelData -> d.y },     // Y - Green
                        Color(0xFF64B5F6) to { d: AccelData -> d.z },     // Z - Blue
                        Color(0xFFFFB74D) to { d: AccelData -> d.magnitude } // Magnitude - Orange
                    )

                    // Draw each line
                    lineColors.forEach { (color, getValue) ->
                        val path = Path()
                        data.forEachIndexed { index, sample ->
                            val x = (index.toFloat() / (data.size - 1).coerceAtLeast(1)) * width
                            val y = valueToY(getValue(sample))

                            if (index == 0) {
                                path.moveTo(x, y)
                            } else {
                                path.lineTo(x, y)
                            }
                        }
                        drawPath(
                            path = path,
                            color = color,
                            style = Stroke(width = 2.dp.toPx())
                        )
                    }

                    // Draw zero line
                    val zeroY = valueToY(0f)
                    drawLine(
                        color = Color.Gray.copy(alpha = 0.3f),
                        start = Offset(0f, zeroY),
                        end = Offset(width, zeroY),
                        strokeWidth = 1.dp.toPx()
                    )
                }
            }
        }
    }
}
