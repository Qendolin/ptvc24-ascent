# Define the list of glob patterns of files that should be exported
$globPatterns = @(
    "assets/fonts/*.ttf",
    "assets/models/*.glb",
    "assets/shaders/*.*",
    "assets/textures/*.*",
    "assets/textures/skybox/*.*",
    "assets/textures/ui/*.*"
)

# Define the output zip file name
$outputZipFile = "_bin/release/ptvc24-ascent.zip"

# Ensure that the directory for the output zip file exists
$outputDirectory = Split-Path -parent $outputZipFile
if (-not (Test-Path $outputDirectory)) {
    New-Item -Path $outputDirectory -ItemType Directory -Force | Out-Null
}

# Check if the output zip file already exists, and delete it if it does
if (Test-Path $outputZipFile) {
    Remove-Item $outputZipFile -Force
    Write-Host "Deleted existing $outputZipFile"
}

# Create a new empty zip file
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zipFile = [System.IO.Compression.ZipFile]::Open($outputZipFile, 'Create')

function AddFileToZip($zipFile, $file, $entryName) {
    Write-Host "Packing $entryName"
    $entry = $zipFile.CreateEntry($entryName)
    $entryStream = $entry.Open()
    $fileStream = [System.IO.File]::OpenRead($file.FullName)
    $fileStream.CopyTo($entryStream)
    $fileStream.Close()
    $entryStream.Close()
}

# Add files matching each glob pattern to the zip file
foreach ($pattern in $globPatterns) {
    $files = Get-ChildItem -Path "." -Filter $pattern -File
    foreach ($file in $files) {
        $entryName = $file.FullName.Substring($PWD.Path.Length + 1)
        AddFileToZip $zipFile $file $entryName
    }
}

AddFileToZip $zipFile (Get-ChildItem "README.md") "README.md"
AddFileToZip $zipFile (Get-ChildItem "LICENSE") "LICENSE"


# Check if the executable exists in the release directory
$exePath = "_bin/release/PTVC_Project_GL.exe"
if (-not (Test-Path $exePath)) {
    Write-Host "Executable not found. Running 'make release'..."
    # Run make release
    ./make release
}

AddFileToZip $zipFile (Get-ChildItem $exePath) "Ascent.exe"

# Close the zip file
$zipFile.Dispose()

Write-Host "Assets zipped as $outputZipFile"