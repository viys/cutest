[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("update", "cmake", "make", "run", "clean", "delete")]
    [string]$Action
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $repoRoot "build"
$testSourceDir = Join-Path $repoRoot "test"
$allTestsPath = Join-Path $testSourceDir "AllTests.c"
$makeTestsScriptPath = Join-Path $repoRoot "scripts\\make-tests.py"
$makeTestsConfigPath = Join-Path $repoRoot "scripts\\make-tests.json"

function Get-CommandPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    return $null
}

function Invoke-Configure {
    $cmakeArgs = @("-S", $testSourceDir, "-B", $buildDir)
    $ninjaPath = Get-CommandPath -Name "ninja"
    $gccPath = Get-CommandPath -Name "gcc"

    if ($ninjaPath -and $gccPath) {
        $cmakeArgs += @("-G", "Ninja", "-D", "CMAKE_C_COMPILER=$gccPath")
    }

    & cmake @cmakeArgs
}

function Invoke-UpdateAllTests {
    # Regenerate AllTests.c from test sources before building or running the suite.
    $testFiles = Get-ChildItem -LiteralPath $testSourceDir -Filter "*.c" |
        Where-Object { $_.Name -ne "AllTests.c" } |
        Sort-Object -Property Name |
        ForEach-Object { $_.FullName }

    if (-not $testFiles) {
        throw "No test source files found under: $testSourceDir"
    }

    $pythonPath = Get-CommandPath -Name "python"
    if (-not $pythonPath) {
        throw "Python is required to generate AllTests.c but was not found in PATH."
    }

    & $pythonPath $makeTestsScriptPath --config $makeTestsConfigPath --output $allTestsPath --files $testFiles
}

function Invoke-Make {
    if (-not (Test-Path -LiteralPath $buildDir)) {
        Invoke-Configure
    }
    cmake --build $buildDir
}

function Invoke-Run {
    $cutestPath = Join-Path $buildDir "cutest.exe"
    if (-not (Test-Path -LiteralPath $cutestPath)) {
        throw "Missing test executable: $cutestPath. Run '.\test.ps1 make' first."
    }
    & $cutestPath
}

if (-not $PSBoundParameters.ContainsKey("Action")) {
    Invoke-UpdateAllTests
    Invoke-Configure
    Invoke-Make
    Invoke-Run
    return
}

switch ($Action) {
    "update" {
        Invoke-UpdateAllTests
    }
    "cmake" {
        Invoke-Configure
    }
    "make" {
        Invoke-Make
    }
    "run" {
        Invoke-Run
    }
    "clean" {
        if (Test-Path -LiteralPath $buildDir) {
            cmake --build $buildDir --target clean
        }
    }
    "delete" {
        if (Test-Path -LiteralPath $buildDir) {
            Remove-Item -LiteralPath $buildDir -Recurse -Force
        }
    }
}
