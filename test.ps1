[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("update", "cmake", "make", "run", "coverage", "clean", "delete", "help")]
    [string]$Action
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $repoRoot "build\test"
$middlewareBuildDir = Join-Path $buildDir "middleware"
$testSourceDir = Join-Path $repoRoot "test"
$allTestsPath = Join-Path $testSourceDir "AllTests.c"
$makeTestsScriptPath = Join-Path $repoRoot "src\scripts\make-tests.py"
$makeTestsConfigPath = Join-Path $testSourceDir "make-tests.json"
$coverageScriptPath = Join-Path $repoRoot "src\scripts\generate_coverage_report.py"

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

# Display the available test and coverage actions.
function Show-Help {
    Write-Host "CuTest test commands:"
    Write-Host "  .\test.ps1                  Update, build, and run standard and middleware tests"
    Write-Host "  .\test.ps1 update           Regenerate test/AllTests.c"
    Write-Host "  .\test.ps1 cmake            Configure standard tests in build/test"
    Write-Host "  .\test.ps1 make             Build standard tests"
    Write-Host "  .\test.ps1 run              Run standard tests"
    Write-Host "  .\test.ps1 coverage         Build tests and show detailed coverage output"
    Write-Host "  .\test.ps1 clean            Clean standard and middleware builds"
    Write-Host "  .\test.ps1 delete           Delete the build directory"
    Write-Host "  .\test.ps1 help             Show this help"
}

function Invoke-Configure {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SelectedBuildDir,
        [switch]$UseMemoryMiddleware
    )

    $cmakeArgs = @("-S", $testSourceDir, "-B", $SelectedBuildDir)
    $ninjaPath = Get-CommandPath -Name "ninja"
    $gccPath = Get-CommandPath -Name "gcc"

    if ($ninjaPath -and $gccPath) {
        $cmakeArgs += @("-G", "Ninja", "-D", "CMAKE_C_COMPILER=$gccPath")
    }

    if ($UseMemoryMiddleware) {
        $cmakeArgs += @(
            "-D", "CUTEST_USE_MEMORY_MIDDLEWARE=ON",
            "-D", "CUTEST_MEMORY_HEAP_SIZE=65536"
        )
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
    param(
        [Parameter(Mandatory = $true)]
        [string]$SelectedBuildDir,
        [string]$TargetName
    )

    if (-not (Test-Path -LiteralPath $SelectedBuildDir)) {
        $useMemoryMiddleware = $SelectedBuildDir -eq $middlewareBuildDir
        Invoke-Configure -SelectedBuildDir $SelectedBuildDir -UseMemoryMiddleware:$useMemoryMiddleware
    }

    $buildArgs = @("--build", $SelectedBuildDir)
    if ($TargetName) {
        $buildArgs += @("--target", $TargetName)
    }

    cmake @buildArgs
}

function Invoke-RunExecutable {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SelectedBuildDir,
        [Parameter(Mandatory = $true)]
        [string]$ExecutableName,
        [Parameter(Mandatory = $true)]
        [string]$VariantLabel
    )

    $cutestPath = Join-Path $SelectedBuildDir $ExecutableName
    if (-not (Test-Path -LiteralPath $cutestPath)) {
        throw "Missing $VariantLabel test executable: $cutestPath. Run '.\test.ps1 make' first."
    }
    & $cutestPath
}

function Invoke-Run {
    Invoke-RunExecutable -SelectedBuildDir $buildDir -ExecutableName "cutest.exe" -VariantLabel "default"
}

function Invoke-RunMiddleware {
    Invoke-RunExecutable -SelectedBuildDir $middlewareBuildDir -ExecutableName "cutest_middleware.exe" -VariantLabel "middleware"
}

if (-not $PSBoundParameters.ContainsKey("Action")) {
    Invoke-UpdateAllTests

    Invoke-Configure -SelectedBuildDir $buildDir
    Invoke-Make -SelectedBuildDir $buildDir -TargetName "cutest"
    Invoke-Run

    Invoke-Configure -SelectedBuildDir $middlewareBuildDir -UseMemoryMiddleware
    Invoke-Make -SelectedBuildDir $middlewareBuildDir -TargetName "cutest"
    Invoke-RunMiddleware
    return
}

switch ($Action) {
    "help" {
        Show-Help
    }
    "update" {
        Invoke-UpdateAllTests
    }
    "cmake" {
        Invoke-Configure -SelectedBuildDir $buildDir
    }
    "make" {
        Invoke-Make -SelectedBuildDir $buildDir -TargetName "cutest"
    }
    "run" {
        Invoke-Run
    }
    "coverage" {
        $pythonPath = Get-CommandPath -Name "python"
        if (-not $pythonPath) {
            throw "Python is required to generate the coverage report."
        }

        $coverageArguments = @(
            $coverageScriptPath,
            "--source-root", $repoRoot,
            "--cmake-source-dir", "test",
            "--build-dir", "build/test/coverage",
            "--report-dir", "build/test/coverage-report",
            "--filter", ".*src.*\.c$",
            "--cmake-arg=-DCUTEST_ENABLE_COVERAGE=ON"
        )
        $coverageArguments += "--verbose"

        & $pythonPath @coverageArguments
        if ($LASTEXITCODE -ne 0) {
            throw "Coverage report generation failed."
        }
    }
    "clean" {
        if (Test-Path -LiteralPath $buildDir) {
            cmake --build $buildDir --target clean
        }
        if (Test-Path -LiteralPath $middlewareBuildDir) {
            cmake --build $middlewareBuildDir --target clean
        }
    }
    "delete" {
        $allBuildDir = Join-Path $repoRoot "build"
        if (Test-Path -LiteralPath $allBuildDir) {
            Remove-Item -LiteralPath $allBuildDir -Recurse -Force
        }
    }
}
