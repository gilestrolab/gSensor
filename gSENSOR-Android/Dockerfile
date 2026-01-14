# Use pre-built Android SDK image
FROM cimg/android:2024.01

WORKDIR /app

# Copy project files
COPY --chown=circleci:circleci . .

# Make gradlew executable
RUN chmod +x gradlew

# Build the project
CMD ["./gradlew", "assembleDebug", "--no-daemon"]
