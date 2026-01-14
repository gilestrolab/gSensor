#!/usr/bin/env python3
"""
gSENSOR Serial Plotter

Real-time plotting of accelerometer data from gSENSOR device.

Usage:
    python serial_plotter.py [--port PORT] [--baud BAUD] [--duration SECONDS]

Requirements:
    pip install pyserial matplotlib numpy
"""

import argparse
import sys
import time
from collections import deque
from datetime import datetime

import csv
import tkinter as tk
from tkinter import filedialog

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.widgets import Button
import numpy as np


# Default settings
DEFAULT_BAUD = 115200
DEFAULT_PORT = "/dev/ttyACM0"
WINDOW_SIZE = 500  # Number of samples to display

# Sample rate options (must match firmware config.h)
# Note: At 115200 baud with ~45 bytes/line, max sustainable rate is ~250 Hz
SAMPLE_RATES = {
    1: 100,   # s1 - default/display rate
    2: 200,   # s2 - recommended for recording at 115200 baud
    3: 400,   # s3 - may cause buffer overflow
    4: 800,   # s4 - requires higher baud rate
}
DEFAULT_DISPLAY_RATE = 1   # 100 Hz for normal display
DEFAULT_RECORDING_RATE = 2  # 200 Hz - safe for 115200 baud


class GsensorPlotter:
    """
    Real-time plotter for gSENSOR data.

    Reads CSV data from serial port and displays live plots.
    """

    def __init__(self, port: str, baud: int, window_size: int = WINDOW_SIZE):
        """
        Initialize the plotter.

        Args:
            port: Serial port path
            baud: Baud rate
            window_size: Number of samples to display in the plot
        """
        self.port = port
        self.baud = baud
        self.window_size = window_size

        # Data buffers
        self.timestamps = deque(maxlen=window_size)
        self.x_data = deque(maxlen=window_size)
        self.y_data = deque(maxlen=window_size)
        self.z_data = deque(maxlen=window_size)
        self.mag_data = deque(maxlen=window_size)
        self.peak_data = deque(maxlen=window_size)

        # Serial connection
        self.serial = None

        # Plot objects
        self.fig = None
        self.ax_xyz = None
        self.ax_mag = None
        self.lines = {}
        self.buttons = {}
        self.status_text = None

        # Stats
        self.sample_count = 0
        self.start_time = None

        # Recording state
        self.recording = False
        self.recorded_data = []
        self.record_start_time = None

        # Sample rate settings
        self.display_rate = DEFAULT_DISPLAY_RATE
        self.recording_rate = DEFAULT_RECORDING_RATE
        self.current_rate = self.display_rate

    def connect(self) -> bool:
        """
        Connect to the serial port.

        Returns:
            True if connection successful, False otherwise.
        """
        try:
            self.serial = serial.Serial(self.port, self.baud, timeout=0.1)
            print(f"Connected to {self.port} at {self.baud} baud")

            # Clear any buffered data
            self.serial.reset_input_buffer()
            time.sleep(0.5)

            # Read a few lines to verify data is coming through
            print("Waiting for data...")
            for _ in range(20):
                if self.serial.in_waiting > 0:
                    line = self.serial.readline().decode("utf-8", errors="ignore")
                    print(f"  Received: {line.strip()[:70]}")
                    if self.parse_line(line):
                        print("  -> Parsed successfully!")
                        break
                time.sleep(0.1)

            if self.sample_count == 0:
                print("Warning: No valid data parsed yet. Check sensor connection.")
            else:
                print(f"Data stream OK ({self.sample_count} samples)")

            return True
        except serial.SerialException as e:
            print(f"Error connecting to {self.port}: {e}")
            return False

    def disconnect(self):
        """Close the serial connection."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            print("Disconnected from serial port")

    def parse_line(self, line: str) -> bool:
        """
        Parse a CSV line from the sensor.

        Expected format: timestamp,x,y,z,magnitude,peak

        Args:
            line: CSV line to parse

        Returns:
            True if parsing successful, False otherwise.
        """
        try:
            parts = line.strip().split(",")
            if len(parts) != 6:
                return False

            timestamp = int(parts[0])
            x = float(parts[1])
            y = float(parts[2])
            z = float(parts[3])
            mag = float(parts[4])
            peak = float(parts[5])

            # Convert timestamp to seconds from start
            if self.start_time is None:
                self.start_time = timestamp

            t = (timestamp - self.start_time) / 1000.0

            self.timestamps.append(t)
            self.x_data.append(x)
            self.y_data.append(y)
            self.z_data.append(z)
            self.mag_data.append(mag)
            self.peak_data.append(peak)

            self.sample_count += 1

            # Store data if recording
            if self.recording:
                self.recorded_data.append({
                    "timestamp_ms": timestamp,
                    "x": x,
                    "y": y,
                    "z": z,
                    "magnitude": mag,
                    "peak": peak,
                })

            return True

        except (ValueError, IndexError):
            return False

    def read_data(self):
        """Read and parse available data from serial port."""
        if not self.serial or not self.serial.is_open:
            return

        try:
            while self.serial.in_waiting > 0:
                try:
                    line = self.serial.readline().decode("utf-8", errors="ignore")
                    if line.strip():
                        parsed = self.parse_line(line)
                        if not parsed and self.sample_count == 0:
                            # Show unparseable lines only at startup for debugging
                            print(f"Skipping: {line.strip()[:60]}")
                except UnicodeDecodeError:
                    pass  # Skip malformed data
        except OSError as e:
            # Serial port disconnected or buffer overflow
            print(f"Serial error: {e}")
            print("Device may have disconnected or buffer overflow occurred.")
            if self.recording:
                print(f"Recording stopped with {len(self.recorded_data)} samples.")
                self.recording = False

    def setup_plot(self):
        """Set up the matplotlib figure and axes."""
        plt.style.use("dark_background")

        self.fig = plt.figure(figsize=(12, 9))

        # Create gridspec for layout: plots on top, buttons at bottom
        gs = self.fig.add_gridspec(
            3, 1, height_ratios=[1, 1, 0.12], hspace=0.3, top=0.92, bottom=0.08
        )

        self.ax_xyz = self.fig.add_subplot(gs[0])
        self.ax_mag = self.fig.add_subplot(gs[1], sharex=self.ax_xyz)

        self.fig.suptitle("gSENSOR Real-Time Data", fontsize=14)

        # XYZ subplot
        self.ax_xyz.set_ylabel("Acceleration (g)")
        self.ax_xyz.set_ylim(-10, 10)
        self.ax_xyz.grid(True, alpha=0.3)

        (self.lines["x"],) = self.ax_xyz.plot([], [], "r-", label="X", linewidth=1)
        (self.lines["y"],) = self.ax_xyz.plot([], [], "g-", label="Y", linewidth=1)
        (self.lines["z"],) = self.ax_xyz.plot([], [], "b-", label="Z", linewidth=1)
        self.ax_xyz.legend(loc="upper right")

        # Magnitude subplot
        self.ax_mag.set_xlabel("Time (s)")
        self.ax_mag.set_ylabel("Magnitude (g)")
        self.ax_mag.set_ylim(0, 20)
        self.ax_mag.grid(True, alpha=0.3)

        (self.lines["mag"],) = self.ax_mag.plot(
            [], [], "cyan", label="Magnitude", linewidth=1.5
        )
        (self.lines["peak"],) = self.ax_mag.plot(
            [], [], "orange", label="Peak", linewidth=1, linestyle="--"
        )
        self.ax_mag.legend(loc="upper right")

        # Add control buttons
        button_style = {"color": "#333333", "hovercolor": "#555555"}

        ax_record = self.fig.add_axes([0.15, 0.02, 0.12, 0.04])
        self.buttons["record"] = Button(ax_record, "Record", **button_style)
        self.buttons["record"].on_clicked(self.toggle_recording)

        ax_save = self.fig.add_axes([0.30, 0.02, 0.12, 0.04])
        self.buttons["save"] = Button(ax_save, "Save CSV", **button_style)
        self.buttons["save"].on_clicked(self.save_csv)

        ax_clear = self.fig.add_axes([0.45, 0.02, 0.12, 0.04])
        self.buttons["clear"] = Button(ax_clear, "Clear", **button_style)
        self.buttons["clear"].on_clicked(self.clear_data)

        ax_reset = self.fig.add_axes([0.60, 0.02, 0.12, 0.04])
        self.buttons["reset"] = Button(ax_reset, "Reset Peak", **button_style)
        self.buttons["reset"].on_clicked(self.reset_peak)

        # Status text on the right side
        self.status_text = self.fig.text(
            0.85, 0.035, "", fontsize=9, ha="center", va="center", color="#888888"
        )

        # Connect keyboard handler
        self.fig.canvas.mpl_connect("key_press_event", self.on_key)

    def update_plot(self, frame):
        """
        Update function for animation.

        Args:
            frame: Animation frame number (unused)

        Returns:
            List of updated line objects.
        """
        self.read_data()

        if len(self.timestamps) < 2:
            return self.lines.values()

        t = list(self.timestamps)

        # Update XYZ lines
        self.lines["x"].set_data(t, list(self.x_data))
        self.lines["y"].set_data(t, list(self.y_data))
        self.lines["z"].set_data(t, list(self.z_data))

        # Update magnitude lines
        self.lines["mag"].set_data(t, list(self.mag_data))
        self.lines["peak"].set_data(t, list(self.peak_data))

        # Adjust x-axis limits
        if t:
            xmin = max(0, t[-1] - 10)  # Show last 10 seconds
            xmax = t[-1] + 0.5
            self.ax_xyz.set_xlim(xmin, xmax)
            self.ax_mag.set_xlim(xmin, xmax)

        # Auto-scale magnitude y-axis
        if self.mag_data:
            mag_max = max(max(self.mag_data), max(self.peak_data)) * 1.2
            mag_max = max(mag_max, 5)  # Minimum 5g scale
            self.ax_mag.set_ylim(0, mag_max)

        # Update title with sample rate
        if self.sample_count > 0 and self.timestamps:
            elapsed = t[-1] if t else 0
            if elapsed > 0:
                rate = self.sample_count / elapsed
                rec_indicator = " [REC]" if self.recording else ""
                self.fig.suptitle(
                    f"gSENSOR Real-Time Data | {rate:.1f} Hz | "
                    f"Peak: {self.peak_data[-1]:.2f}g{rec_indicator}",
                    fontsize=14,
                    color="#ff4444" if self.recording else "white",
                )
        else:
            self.fig.suptitle(
                f"gSENSOR - Waiting for data... ({self.sample_count} samples)",
                fontsize=14,
            )

        # Update status text
        if self.status_text:
            if self.recording:
                rec_count = len(self.recorded_data)
                rec_hz = SAMPLE_RATES[self.recording_rate]
                self.status_text.set_text(f"REC {rec_hz}Hz: {rec_count}")
                self.status_text.set_color("#ff4444")
            else:
                self.status_text.set_text("Space: Record")
                self.status_text.set_color("#888888")

        return self.lines.values()

    def run(self, duration: float = None):
        """
        Run the plotter.

        Args:
            duration: Optional duration in seconds to run (None = indefinite)
        """
        if not self.connect():
            return

        self.setup_plot()

        # Create animation
        ani = animation.FuncAnimation(
            self.fig,
            self.update_plot,
            interval=50,  # 20 FPS
            blit=False,
            cache_frame_data=False,
        )

        try:
            plt.show()
        except KeyboardInterrupt:
            print("\nStopped by user")
        finally:
            self.disconnect()

    def send_command(self, cmd: str):
        """
        Send a command to the sensor.

        Args:
            cmd: Command character to send
        """
        if self.serial and self.serial.is_open:
            self.serial.write(cmd.encode())

    def set_sample_rate(self, rate_key: int):
        """
        Set the sensor sample rate.

        Args:
            rate_key: Rate key (1=100Hz, 2=200Hz, 3=400Hz, 4=800Hz)
        """
        if rate_key not in SAMPLE_RATES:
            print(f"Invalid rate key: {rate_key}")
            return

        self.send_command(f"s{rate_key}")
        if self.serial and self.serial.is_open:
            self.serial.flush()  # Ensure command is sent
            time.sleep(0.1)  # Wait for firmware to process
        self.current_rate = rate_key
        hz = SAMPLE_RATES[rate_key]
        print(f"Sample rate set to {hz} Hz")

    def toggle_recording(self, event=None):
        """
        Toggle recording state.

        Automatically switches to high sample rate when recording starts,
        and back to display rate when recording stops.

        Args:
            event: Button click event (unused).
        """
        self.recording = not self.recording
        if self.recording:
            # Switch to high sample rate for recording
            self.set_sample_rate(self.recording_rate)
            self.recorded_data = []
            self.record_start_time = datetime.now()
            rec_hz = SAMPLE_RATES[self.recording_rate]
            print(f"Recording at {rec_hz} Hz started at {self.record_start_time.strftime('%H:%M:%S')}")
            if "record" in self.buttons:
                self.buttons["record"].label.set_text("Stop")
                self.buttons["record"].color = "#aa3333"
                self.buttons["record"].hovercolor = "#cc4444"
        else:
            # Switch back to display rate
            self.set_sample_rate(self.display_rate)
            duration = (datetime.now() - self.record_start_time).total_seconds()
            actual_hz = len(self.recorded_data) / duration if duration > 0 else 0
            print(
                f"Recording stopped. {len(self.recorded_data)} samples "
                f"in {duration:.1f}s ({actual_hz:.1f} Hz actual)"
            )
            if "record" in self.buttons:
                self.buttons["record"].label.set_text("Record")
                self.buttons["record"].color = "#333333"
                self.buttons["record"].hovercolor = "#555555"

    def save_csv(self, event=None):
        """
        Save recorded data to CSV file.

        Args:
            event: Button click event (unused).
        """
        if not self.recorded_data:
            print("No data to save. Start recording first.")
            return

        # Create hidden Tk root for file dialog
        root = tk.Tk()
        root.withdraw()

        # Default filename with timestamp
        default_name = f"gsensor_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

        filepath = filedialog.asksaveasfilename(
            defaultextension=".csv",
            filetypes=[("CSV files", "*.csv"), ("All files", "*.*")],
            initialfile=default_name,
            title="Save accelerometer data",
        )

        root.destroy()

        if not filepath:
            print("Save cancelled.")
            return

        try:
            with open(filepath, "w", newline="") as f:
                # Write header comment
                f.write(f"# gSENSOR Data Export\n")
                f.write(f"# Port: {self.port}\n")
                f.write(f"# Baud: {self.baud}\n")
                f.write(f"# Sample Rate: {SAMPLE_RATES[self.recording_rate]} Hz\n")
                if self.record_start_time:
                    f.write(
                        f"# Start: {self.record_start_time.strftime('%Y-%m-%d %H:%M:%S')}\n"
                    )
                f.write(f"# Samples: {len(self.recorded_data)}\n")

                writer = csv.DictWriter(
                    f,
                    fieldnames=["timestamp_ms", "x", "y", "z", "magnitude", "peak"],
                )
                writer.writeheader()
                writer.writerows(self.recorded_data)

            print(f"Saved {len(self.recorded_data)} samples to {filepath}")
        except IOError as e:
            print(f"Error saving file: {e}")

    def clear_data(self, event=None):
        """
        Clear all data buffers.

        Args:
            event: Button click event (unused).
        """
        self.timestamps.clear()
        self.x_data.clear()
        self.y_data.clear()
        self.z_data.clear()
        self.mag_data.clear()
        self.peak_data.clear()
        self.recorded_data = []
        self.sample_count = 0
        self.start_time = None
        print("Data cleared.")

    def reset_peak(self, event=None):
        """
        Send reset peak command to sensor.

        Args:
            event: Button click event (unused).
        """
        self.send_command("r")
        print("Peak reset command sent.")

    def on_key(self, event):
        """
        Handle keyboard shortcuts.

        Args:
            event: Key press event.
        """
        if event.key == " ":
            self.toggle_recording()
        elif event.key.lower() == "s":
            self.save_csv()
        elif event.key.lower() == "c":
            self.clear_data()
        elif event.key.lower() == "r":
            self.reset_peak()


def list_ports():
    """List available serial ports."""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found")
        return

    print("Available serial ports:")
    for port in ports:
        print(f"  {port.device}: {port.description}")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="gSENSOR Serial Plotter - Real-time accelerometer visualization"
    )
    parser.add_argument(
        "--port",
        "-p",
        default=DEFAULT_PORT,
        help=f"Serial port (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--baud",
        "-b",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Baud rate (default: {DEFAULT_BAUD})",
    )
    parser.add_argument(
        "--list",
        "-l",
        action="store_true",
        help="List available serial ports",
    )
    parser.add_argument(
        "--duration",
        "-d",
        type=float,
        default=None,
        help="Recording duration in seconds (default: indefinite)",
    )

    args = parser.parse_args()

    if args.list:
        list_ports()
        return

    print("gSENSOR Serial Plotter")
    print("=" * 40)
    print(f"Port: {args.port}")
    print(f"Baud: {args.baud}")
    print(f"Recording rate: {SAMPLE_RATES[DEFAULT_RECORDING_RATE]} Hz")
    print("Keyboard: Space=Record, S=Save, C=Clear, R=Reset")
    print("Press Ctrl+C to stop")
    print("=" * 40)

    plotter = GsensorPlotter(args.port, args.baud)
    plotter.run(args.duration)


if __name__ == "__main__":
    main()
