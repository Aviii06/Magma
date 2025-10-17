#!/bin/bash

set -e

IMGUI_VERSION="v1.90.4"  # change this to latest tag if needed
BASE_URL="https://raw.githubusercontent.com/ocornut/imgui/${IMGUI_VERSION}"

FILES=(
  "imconfig.h"
  "imgui.cpp"
  "imgui_draw.cpp"
  "imgui_widgets.cpp"
  "imgui_tables.cpp"
  "imgui.h"
  "imgui_internal.h"
  "imstb_textedit.h"
  "imstb_rectpack.h"
  "imstb_textedit.h"
  "imstb_truetype.h"
  "imgui_demo.cpp"
)

BACKEND_FILES=(
  "backends/imgui_impl_glfw.cpp"
  "backends/imgui_impl_glfw.h"
  "backends/imgui_impl_vulkan.cpp"
  "backends/imgui_impl_vulkan.h"
)

EXAMPLE_FILE="examples/example_glfw_vulkan/main.cpp"

declare -a PIDS=()

download_with_progress() {
    local src=$1
    local dest=$2
    local dir=$(dirname "$dest")

    # Create directory if it doesn't exist
    mkdir -p "$dir"

    local temp_file=$(mktemp)

    (curl -# -L "${BASE_URL}/${src}" -o "$dest" > "$temp_file" 2>&1; echo "$? $src $dest" > "$temp_file.done") &

    local pid=$!
    PIDS+=($pid)

    echo "$temp_file $pid"
}

echo "Starting parallel downloads of ImGui files..."

declare -a TEMP_FILES=()

# Download core files
for file in "${FILES[@]}"; do
    dest="./vendor/imgui/${file}"
    echo "Queueing: $file -> $dest"
    TEMP_FILES+=("$(download_with_progress "$file" "$dest")")
done

# Download backend files
for file in "${BACKEND_FILES[@]}"; do
    dest="./vendor/imgui/${file}"
    echo "Queueing: $file -> $dest"
    TEMP_FILES+=("$(download_with_progress "$file" "$dest")")
done

# Download example file
dest="./vendor/imgui/example/main.cpp"
echo "Queueing: $EXAMPLE_FILE -> $dest"
TEMP_FILES+=("$(download_with_progress "$EXAMPLE_FILE" "$dest")")

# Function to display progress
display_progress() {
    echo ""
    local running=true

    while $running; do
        running=false
        clear
        echo "ImGui v${IMGUI_VERSION} Download Progress:"
        echo "----------------------------------------"

        for temp_file_info in "${TEMP_FILES[@]}"; do
            read -r temp_file pid <<< "$temp_file_info"

            # Check if process is still running
            if kill -0 $pid 2>/dev/null; then
                running=true

                # Display the last line of progress
                if [ -f "$temp_file" ]; then
                    tail -n 1 "$temp_file"
                fi
            elif [ -f "$temp_file.done" ]; then
                # Process is done, read status
                read -r status src dest < "$temp_file.done"
                if [ "$status" -eq 0 ]; then
                    echo "[DONE] $src -> $dest"
                else
                    echo "[FAILED] $src -> $dest"
                fi
            fi
        done

        if $running; then
            sleep 0.2
        fi
    done

    # Clean up temp files
    for temp_file_info in "${TEMP_FILES[@]}"; do
        read -r temp_file pid <<< "$temp_file_info"
        rm -f "$temp_file" "$temp_file.done"
    done
}

# Display progress while downloads are running
display_progress

# Wait for all downloads to complete
for pid in "${PIDS[@]}"; do
    wait $pid
done

echo "All files downloaded successfully!"
echo "ImGui files are in the ./imgui directory"
echo "Example file is in ./example/main.cpp"