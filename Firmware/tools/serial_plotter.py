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

import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np


# Default settings
DEFAULT_BAUD = 115200
DEFAULT_PORT = "/dev/ttyACM0"
WINDOW_SIZE = 500  # Number of samples to display


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

        # Stats
        self.sample_count = 0
        self.start_time = None

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
            return True

        except (ValueError, IndexError):
            return False

    def read_data(self):
        """Read and parse available data from serial port."""
        if not self.serial or not self.serial.is_open:
            return

        while self.serial.in_waiting > 0:
            try:
                line = self.serial.readline().decode("utf-8", errors="ignore")
                if line.strip():
                    parsed = self.parse_line(line)
                    if not parsed and self.sample_count == 0:
                        # Show unparseable lines only at startup for debugging
                        print(f"Skipping: {line.strip()[:60]}")
            except Exception as e:
                print(f"Read error: {e}")

    def setup_plot(self):
        """Set up the matplotlib figure and axes."""
        plt.style.use("dark_background")

        self.fig, (self.ax_xyz, self.ax_mag) = plt.subplots(
            2, 1, figsize=(12, 8), sharex=True
        )

        self.fig.suptitle("gSENSOR Real-Time Data", fontsize=14)

        # XYZ subplot
        self.ax_xyz.set_ylabel("Acceleration (g)")
        self.ax_xyz.set_ylim(-10, 10)
        self.ax_xyz.grid(True, alpha=0.3)
        self.ax_xyz.legend(loc="upper right")

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

        plt.tight_layout()

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
                self.fig.suptitle(
                    f"gSENSOR Real-Time Data | {rate:.1f} Hz | Peak: {self.peak_data[-1]:.2f}g",
                    fontsize=14,
                )
        else:
            self.fig.suptitle(
                f"gSENSOR - Waiting for data... ({self.sample_count} samples)",
                fontsize=14,
            )

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
    print("Press Ctrl+C to stop")
    print("=" * 40)

    plotter = GsensorPlotter(args.port, args.baud)
    plotter.run(args.duration)


if __name__ == "__main__":
    main()
