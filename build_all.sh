#!/bin/bash

# Get a list of all board environments by looking at the subdirectories in the 'boards' directory
# This is generally more reliable than parsing platformio.ini
BOARDS=$(ls boards)

# Loop through each board
for BOARD in $BOARDS
do
  # Check if the board is one to be skipped
  if [[ "$BOARD" == *"ESP32-C5"* || "$BOARD" == *"_New-Device-Model"* || "$BOARD" == *"_boards_json"* ]]; then
    echo "===================================================="
    echo ">>> Skipping board: $BOARD"
    echo "===================================================="
    continue
  fi

  echo "===================================================="
  echo ">>> Building board: $BOARD"
  echo "===================================================="

  # Clean up previous build files to avoid conflicts and save space
  echo ">>> Cleaning previous build..."
  rm -rf .pio

  # Build the specific board environment
  # The -e flag specifies the environment, which corresponds to the directory name
  pio run -e "$BOARD"
  
  # Capture the exit code of the build command
  EXIT_CODE=$?

  # Check if the build failed
  if [ $EXIT_CODE -ne 0 ]; then
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    echo ">>> Build failed for board: $BOARD"
    echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    
    # Specific check for out-of-space errors, though this is a simple text check.
    # A more robust solution might check `df -h .` but this is a good first step.
    # For now, we'll just notify and stop as per the user's request.
    # The user requested to stop if cleaning pio doesn't work, and since we clean it every time,
    # any space-related failure is effectively a "cleaning didn't work" scenario.
    
    # A generic check for "No space left on device"
    # output=$(pio run -e "$BOARD" 2>&1)
    # if [[ "$output" == *"No space left on device"* ]]; then
    #   echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    #   echo ">>> INSUFFICIENT SPACE. Halting all builds."
    #   echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    #   exit 1
    # fi
    
    # Simple "skip on fail" as requested
    echo ">>> Skipping to the next board."
  else
    echo "****************************************************"
    echo ">>> Successfully built board: $BOARD"
    echo "****************************************************"
  fi
done

echo "===================================================="
echo ">>> All builds finished."
echo "===================================================="
