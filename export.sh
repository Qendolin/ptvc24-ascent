#!/bin/bash

# Define the list of glob patterns of files that should be exported
globPatterns=(
    "assets/CREDITS.txt"
    "assets/fonts/*.ttf"
    "assets/models/*.glb"
    "assets/shaders/*.*"
    "assets/shaders/gtao/*.*"
    "assets/shaders/objects/*.*"
    "assets/shaders/terrain/*.*"
    "assets/shaders/water/*.*"
    "assets/shaders/particles/*.*"
    "assets/textures/*.png"
    "assets/textures/*.f32"
    "assets/textures/*.dds"
    "assets/textures/skybox/*.iblenv"
    "assets/textures/ui/*.png"
    "assets/textures/input/*.png"
    "assets/textures/input/*.atlas"
    "assets/textures/particle/*.png"
    "assets/audio/music/*.ogg"
    "assets/audio/music/*.wav"
    "assets/audio/sound/*.ogg"
    "assets/audio/sound/*.wav"
)

# Define the output tar file name
outputTarFile="_bin/release/ptvc24-ascent.tar"
compressedTarFile="${outputTarFile}.xz"

# Ensure that the directory for the output tar file exists
outputDirectory=$(dirname "$outputTarFile")
if [ ! -d "$outputDirectory" ]; then
    mkdir -p "$outputDirectory"
fi

# Check if the output tar file already exists, and delete it if it does
if [ -f "$outputTarFile" ]; then
    rm -f "$outputTarFile"
    echo "Deleted existing $outputTarFile"
fi

# Check if the compressed tar file already exists, and delete it if it does
if [ -f "$compressedTarFile" ]; then
    rm -f "$compressedTarFile"
    echo "Deleted existing $compressedTarFile"
fi

# Create an empty tar file
touch "$outputTarFile"

# Add files matching each glob pattern to the tar file
for pattern in "${globPatterns[@]}"; do
    files=$(find . -path "./$pattern")
    for file in $files; do
        entryName="${file:2}"
        echo "Packing $entryName"
        tar --append --file="$outputTarFile" "$file"
    done
done

# Add README.md and LICENSE files to the tar file
if [ -f "README.md" ]; then
    echo "Packing README.md"
    tar --append --file="$outputTarFile" "README.md"
fi
if [ -f "LICENSE" ]; then
    echo "Packing LICENSE"
    tar --append --file="$outputTarFile" "LICENSE"
fi

# Check if the executable exists in the release directory
exePath="_bin/linux/release/PTVC_Project_GL"
if [ -f "$exePath" ]; then
    lastCompileTime=$(stat -c %Y "$exePath")
    currentTime=$(date +%s)
    if (( (currentTime - lastCompileTime) > 3600 )); then
        echo "Warning: The compiled exe is older than an hour, may be outdated."
    fi
else
    echo "Executable not found. Running 'make release'..."
    # Run make release
    make release
fi

if [ -f "$exePath" ]; then
    chmod +x $exePath
    echo "Packing Ascent Binary"
    tar --append --file="$outputTarFile" --transform="flags=r;s|$exePath|Ascent|" "$exePath"
fi

echo "Compressing with xz"

# Compress the tar file using xz
xz -z "$outputTarFile"

echo "Assets packed as $compressedTarFile"
