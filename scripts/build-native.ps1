# Builds gplcompression.dll for Windows by compiling the JNI bindings and the
# full LZO library sources into a single DLL with MSVC (cl.exe), and drops it
# where the JAR expects it:
#
#   build/native-resources/native/Windows-<arch>-64/lib/gplcompression.dll
#
# The layout and library name match what GPLNativeCodeLoader unpacks at runtime.
# LZO is compiled in directly, so the DLL depends only on the system libraries.
#
# cl.exe must be on PATH (use ilammy/msvc-dev-cmd in CI). The architecture cl.exe
# targets is whichever the active MSVC environment selects, so TARGET_ARCH here
# only names the output directory: pick the matching msvc-dev-cmd arch (amd64 for
# x64, amd64_arm64 to cross-compile arm64) to agree with it.
#
# Environment:
#   JAVA_HOME    required, provides jni.h and win32/jni_md.h
#   TARGET_ARCH  output architecture, amd64 or arm64 (default: amd64)

$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $RepoRoot

if (-not $env:JAVA_HOME) { throw "JAVA_HOME must be set" }
$HeadersDir  = "build/native-headers"
$OutputRoot  = "build/native-resources"
$LzoTarball  = "provided/lzo-2.10.tar.gz"

# Map the target to the os.arch value Java reports on Windows, which names the
# resource directory GPLNativeCodeLoader reads at runtime.
$TargetArch = if ($env:TARGET_ARCH) { $env:TARGET_ARCH } else { "amd64" }
switch ($TargetArch) {
  { $_ -in @("amd64", "x64", "x86_64") } { $JavaArch = "amd64" }
  { $_ -in @("arm64", "aarch64") }       { $JavaArch = "aarch64" }
  default { throw "Unsupported Windows target arch: $TargetArch" }
}
$PlatformDir = "Windows-$JavaArch-64"

if (-not (Test-Path $HeadersDir)) {
  throw "JNI headers not found in $HeadersDir. Run './gradlew compileJava' first."
}

$BuildDir = "build/native-build"
$ObjDir   = Join-Path $BuildDir "obj"
$OutLibDir = Join-Path $OutputRoot "native/$PlatformDir/lib"

Write-Host "==> Building gplcompression.dll for $PlatformDir"
Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $ObjDir, $OutLibDir | Out-Null

# Unpack the LZO sources; they are compiled straight into the DLL below.
tar xf $LzoTarball -C $BuildDir
if ($LASTEXITCODE -ne 0) { throw "tar failed to extract $LzoTarball" }
$LzoSrc = (Get-ChildItem -Path $BuildDir -Directory -Filter "lzo-*" | Select-Object -First 1)
if (-not $LzoSrc) { throw "Could not find extracted lzo-* directory in $BuildDir" }
$LzoSrc = $LzoSrc.FullName

# JNI bindings plus every LZO source file. LZO uses its own portable feature
# detection unless LZO_HAVE_CONFIG_H is set, so we don't define it.
$sources = @(
  "src/main/native/impl/lzo/LzoCompressor.c",
  "src/main/native/impl/lzo/LzoDecompressor.c"
)
$sources += Get-ChildItem -Path (Join-Path $LzoSrc "src") -Filter *.c | ForEach-Object { $_.FullName }

$includes = @(
  "/I$env:JAVA_HOME\include",
  "/I$env:JAVA_HOME\include\win32",
  "/I$HeadersDir",
  "/Isrc\main\native\impl",
  "/I$LzoSrc\include",
  "/I$LzoSrc\src"
)

$dllInBuild = Join-Path $BuildDir "gplcompression.dll"

# /LD -> DLL, JNIEXPORT (__declspec(dllexport)) exports the Java_* entry points.
$clArgs = @(
  "/nologo", "/O2", "/MD", "/LD",
  "/D_JNI_IMPLEMENTATION_", "/D_CRT_SECURE_NO_WARNINGS",
  "/Fo$ObjDir\"
) + $includes + $sources + @("/Fe:$dllInBuild")

Write-Host "==> Compiling $($sources.Count) source files"
& cl @clArgs
if ($LASTEXITCODE -ne 0) { throw "cl.exe failed with exit code $LASTEXITCODE" }

# Copy only the DLL into the resource tree (leave .lib/.exp/.obj behind so they
# never end up in the JAR).
Copy-Item -Force $dllInBuild (Join-Path $OutLibDir "gplcompression.dll")

Write-Host "==> Done: $(Join-Path $OutLibDir 'gplcompression.dll')"
Get-Item (Join-Path $OutLibDir "gplcompression.dll") | Select-Object FullName, Length
