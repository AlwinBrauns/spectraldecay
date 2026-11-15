# SPDX-License-Identifier: AGPL-3.0-or-later
param(
    [string]$BuildDirectory = "out/windows-2026-x64",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
if ($env:OS -ne "Windows_NT") {
    throw "This export requires a Windows x86-64 build, not a Linux VST3."
}

$repository = Split-Path $PSScriptRoot -Parent
$build = (Resolve-Path $BuildDirectory).Path
$source = Join-Path $build "SpectralDecay_artefacts/$Configuration/VST3/Spectral Decay Neutral.vst3"
$binaryRelative = "Contents/x86_64-win/Spectral Decay Neutral.vst3"
if (-not (Test-Path (Join-Path $source $binaryRelative) -PathType Leaf)) {
    throw "The complete Windows x86-64 VST3 bundle is missing: $source"
}

$listing = & ctest --test-dir $build -C $Configuration --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw "Cannot enumerate the configured tests." }
$names = (($listing -join [Environment]::NewLine) | ConvertFrom-Json).tests.name
foreach ($required in @("neutral_offline", "neutral_processor", "neutral_vst3_host")) {
    if ($names -notcontains $required) { throw "Required test is not configured: $required" }
}
& ctest --test-dir $build -C $Configuration --output-on-failure --no-tests=error
if ($LASTEXITCODE -ne 0) { throw "Tests failed; no export was created." }

$exportDirectory = Join-Path $repository "plugin-export"
$destination = Join-Path $exportDirectory "spectral-decay.vst3"
if (Test-Path $destination) {
    throw "Export already exists. Move it aside explicitly before exporting again: $destination"
}
New-Item -ItemType Directory -Force $exportDirectory | Out-Null
Copy-Item -Recurse $source $destination
# A VST3 bundle and its architecture-specific binary must share the basename.
Rename-Item (Join-Path $destination $binaryRelative) "spectral-decay.vst3"

$hostTest = Join-Path $build "$Configuration/spectral_decay_vst3_host_tests.exe"
if (-not (Test-Path $hostTest -PathType Leaf)) {
    $hostTest = Join-Path $build "spectral_decay_vst3_host_tests.exe"
}
if (-not (Test-Path $hostTest -PathType Leaf)) {
    throw "The VST3 host-test executable is missing."
}
& $hostTest $destination
if ($LASTEXITCODE -ne 0) { throw "The renamed export failed its VST3 loading/audio test." }

$licenses = Join-Path $destination "Contents/Resources/Licenses"
New-Item -ItemType Directory -Force $licenses | Out-Null
Copy-Item (Join-Path $repository "LICENSE") (Join-Path $licenses "AGPL-3.0.txt")
Copy-Item (Join-Path $repository "THIRD_PARTY_NOTICES.md") $licenses
Copy-Item (Join-Path $build "_deps/juce-src/LICENSE.md") (Join-Path $licenses "JUCE.md")
Copy-Item (Join-Path $build "_deps/juce-src/JUCE.spdx.json") $licenses
Copy-Item (Join-Path $build "_deps/pffft-src/LICENSE.txt") (Join-Path $licenses "PFFFT.txt")
Copy-Item (Join-Path $build "_deps/juce-src/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt") (Join-Path $licenses "VST3-SDK.txt")
Write-Output "Exported and loaded successfully: $destination"
Write-Output "Neutrality tolerances and the Ableton test remain open. See docs/verification/export.md."
