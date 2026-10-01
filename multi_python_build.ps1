param(
    [switch]$ConfigureOnly,
    [switch]$ListPython
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Invoke-Checked {
    param([string]$File, [string[]]$Arguments)

    & $File @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$File failed with exit code $LASTEXITCODE"
    }
}

function Get-PythonInfo {
    param([string]$Executable)

    $code = "import sys,sysconfig; print('|'.join((str(sys.version_info.major),str(sys.version_info.minor),str(64 if sys.maxsize > 2**32 else 32),sys.implementation.name,sys.base_prefix,sysconfig.get_path('include'),sysconfig.get_config_var('EXT_SUFFIX'))))"
    $output = & $Executable -c $code
    if ($LASTEXITCODE -ne 0) {
        throw "Cannot inspect Python: $Executable"
    }
    $fields = $output.Split('|')
    if ($fields.Count -ne 7) {
        throw "Unexpected Python details from ${Executable}: $output"
    }
    $info = [pscustomobject]@{
        version = "$($fields[0]).$($fields[1])"
        major = [int]$fields[0]
        minor = [int]$fields[1]
        bits = [int]$fields[2]
        implementation = $fields[3]
        prefix = $fields[4]
        includePath = $fields[5]
        suffix = $fields[6]
    }
    if ($info.implementation -ne 'cpython' -or $info.major -ne 3 -or $info.bits -ne 64 -or
        $info.suffix -notmatch '^\.cp\d+-win_amd64\.pyd$') {
        throw "Expected 64-bit CPython with a versioned .pyd suffix: $Executable"
    }
    return $info
}

function Get-PythonConfigurations {
    $configurations = @()
    $seenVersions = @{}
    foreach ($entry in [Environment]::GetEnvironmentVariables('Process').GetEnumerator()) {
        if ($entry.Key -notmatch '^OPENSEES_PYTHON_\d+$') {
            continue
        }
        $fields = ([string]$entry.Value).Split('|')
        if ($fields.Count -ne 4) {
            throw "Invalid $($entry.Key): expected version|python.exe|Include|pythonXY.lib"
        }
        $version = $fields[0].Trim()
        try {
            $versionNumber = [version]$version
        } catch {
            throw "Invalid Python version in $($entry.Key): $version"
        }
        if ($seenVersions.ContainsKey($version)) {
            throw "Python $version is listed more than once in build.bat"
        }
        $seenVersions[$version] = $true
        $configurations += [pscustomobject]@{
            Version = $version
            VersionNumber = $versionNumber
            Executable = $fields[1].Trim()
            IncludeDir = $fields[2].Trim()
            Library = $fields[3].Trim()
        }
    }
    if ($configurations.Count -eq 0) {
        throw 'Add at least one OPENSEES_PYTHON_<version> row to build.bat'
    }
    return $configurations | Sort-Object VersionNumber
}

function Assert-CMakePython {
    param([string]$BuildDirectory, [string]$ExpectedExecutable, [string]$ExpectedInclude, [string]$ExpectedLibrary)

    $cachePath = Join-Path $BuildDirectory 'CMakeCache.txt'
    $cache = Get-Content -LiteralPath $cachePath -Raw
    $match = [regex]::Match($cache, '(?m)^_Python_EXECUTABLE:INTERNAL=(.+)\r?$')
    if (-not $match.Success) {
        throw "CMake did not record a Python interpreter in $cachePath"
    }
    $actual = [IO.Path]::GetFullPath($match.Groups[1].Value.Trim())
    $expected = [IO.Path]::GetFullPath($ExpectedExecutable)
    if (-not $actual.Equals($expected, [StringComparison]::OrdinalIgnoreCase)) {
        throw "CMake selected $actual instead of $expected"
    }
    $includeMatch = [regex]::Match($cache, '(?m)^_Python_INCLUDE_DIR:INTERNAL=(.+)\r?$')
    if (-not $includeMatch.Success) {
        $includeMatch = [regex]::Match($cache, '(?m)^Python_INCLUDE_DIR:[^=]+=(.+)\r?$')
    }
    if (-not $includeMatch.Success) {
        throw "CMake did not record a Python include directory in $cachePath"
    }
    $actualInclude = [IO.Path]::GetFullPath($includeMatch.Groups[1].Value.Trim())
    $expectedIncludePath = [IO.Path]::GetFullPath($ExpectedInclude)
    if (-not $actualInclude.Equals($expectedIncludePath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "CMake selected $actualInclude instead of $expectedIncludePath"
    }
    $libraryMatch = [regex]::Match($cache, '(?m)^_Python_LIBRARY_RELEASE:INTERNAL=(.+)\r?$')
    if (-not $libraryMatch.Success) {
        $libraryMatch = [regex]::Match($cache, '(?m)^Python_LIBRARY:[^=]+=(.+)\r?$')
    }
    if (-not $libraryMatch.Success) {
        throw "CMake did not record a Python development library in $cachePath"
    }
    $actualLibrary = [IO.Path]::GetFullPath($libraryMatch.Groups[1].Value.Trim())
    $expectedLibraryPath = [IO.Path]::GetFullPath($ExpectedLibrary)
    if (-not $actualLibrary.Equals($expectedLibraryPath, [StringComparison]::OrdinalIgnoreCase)) {
        throw "CMake selected $actualLibrary instead of $expectedLibraryPath"
    }
}

$repoRoot = $PSScriptRoot
$toolchain = Join-Path $repoRoot 'build\Release\generators\conan_toolchain.cmake'
$moduleDirectory = Join-Path $repoRoot 'build\python-modules'
$openMpDll = 'D:\oneAPI\compiler\2024.2\bin\libiomp5md.dll'

try {
    Push-Location $repoRoot
    try {
        $pythonBuilds = @()
        foreach ($configuration in (Get-PythonConfigurations)) {
            if (-not $configuration.Executable -or -not $configuration.IncludeDir -or -not $configuration.Library) {
                throw "Complete the OPENSEES_PYTHON row for Python $($configuration.Version) in build.bat"
            }
            $resolved = (Resolve-Path -LiteralPath $configuration.Executable).Path
            $includePath = (Resolve-Path -LiteralPath $configuration.IncludeDir).Path
            $libraryPath = (Resolve-Path -LiteralPath $configuration.Library).Path
            if (-not (Test-Path -LiteralPath (Join-Path $includePath 'Python.h'))) {
                throw "Python.h was not found in $includePath"
            }
            $info = Get-PythonInfo -Executable $resolved
            if ($info.version -ne $configuration.Version) {
                throw "Configured Python $($configuration.Version) uses a $($info.version) interpreter: $resolved"
            }
            $expectedInclude = [IO.Path]::GetFullPath($info.includePath)
            if (-not $includePath.Equals($expectedInclude, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Python $($info.version) expects headers in $expectedInclude, not $includePath"
            }
            $expectedLibrary = [IO.Path]::GetFullPath((Join-Path $info.prefix "libs\python$($info.major)$($info.minor).lib"))
            if (-not $libraryPath.Equals($expectedLibrary, [StringComparison]::OrdinalIgnoreCase)) {
                throw "Python $($info.version) expects $expectedLibrary, not $libraryPath"
            }
            $pythonBuilds += [pscustomobject]@{
                Executable = $resolved
                IncludeDir = $includePath
                Library = $libraryPath
                Info = $info
            }
        }

        if ($pythonBuilds.Count -eq 0) {
            throw 'No supported Python installations found.'
        }

        if ($ListPython) {
            foreach ($build in $pythonBuilds) {
                Write-Host "$($build.Info.version): $($build.Executable) -> opensees$($build.Info.suffix)"
                Write-Host "  Include: $($build.IncludeDir)"
                Write-Host "  Library: $($build.Library)"
            }
            return
        }

        & conan profile path default *> $null
        if ($LASTEXITCODE -ne 0) {
            Invoke-Checked 'conan' @('profile', 'detect')
        }
        Invoke-Checked 'conan' @('install', '.', '-s', 'arch=x86_64', '-s', 'compiler.runtime=static',
            '-s', 'compiler.version=193',
            '--build=missing', '-c', 'tools.cmake.cmaketoolchain:generator=Ninja')
        if (-not (Test-Path -LiteralPath $toolchain)) {
            throw "Conan toolchain was not generated: $toolchain"
        }

        New-Item -ItemType Directory -Force -Path $moduleDirectory | Out-Null
        $builtExecutable = $false
        foreach ($build in $pythonBuilds) {
            $info = $build.Info
            $tag = "py$($info.major)$($info.minor)"
            $buildDirectory = Join-Path $repoRoot "build\$tag\Release"
            Write-Host "=== Building Python $($info.version) in $buildDirectory ==="

            $cmakeArguments = @(
                '-S', $repoRoot, '-B', $buildDirectory, '-G', 'Ninja',
                "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
                '-DCMAKE_BUILD_TYPE=Release',
                '-DCMAKE_POLICY_VERSION_MINIMUM=3.5',
                '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded',
                '-DCMAKE_Fortran_COMPILER=ifx',
                '-DOPS_Use_Graphics_Option=OpenGL',
                '-DBLA_STATIC=ON', '-DMKL_LINK=static', '-DMKL_INTERFACE_FULL=intel_lp64',
                "-DMUMPS_DIR=$(Join-Path $repoRoot 'mumps\build')",
                '-DCMAKE_EXE_LINKER_FLAGS=/FORCE:MULTIPLE',
                '-DCMAKE_SHARED_LINKER_FLAGS=/FORCE:MULTIPLE',
                '-DCMAKE_NINJA_FORCE_RESPONSE_FILE=ON',
                "-DPython_EXECUTABLE=$($build.Executable)",
                "-DPython_INCLUDE_DIR=$($build.IncludeDir)",
                "-DPython_LIBRARY=$($build.Library)",
                "-DPython_ROOT_DIR=$($info.prefix)",
                '-DPython_FIND_STRATEGY=LOCATION'
            )
            if ((Test-Path -LiteralPath (Join-Path $buildDirectory 'CMakeCache.txt')) -and
                -not (Test-Path -LiteralPath (Join-Path $buildDirectory 'build.ninja'))) {
                Write-Host 'Incomplete CMake configuration found; retrying with --fresh.'
                $cmakeArguments = @('--fresh') + $cmakeArguments
            }
            Invoke-Checked 'cmake.exe' $cmakeArguments
            Assert-CMakePython -BuildDirectory $buildDirectory -ExpectedExecutable $build.Executable -ExpectedInclude $build.IncludeDir -ExpectedLibrary $build.Library

            if ($ConfigureOnly) {
                continue
            }
            if (-not $builtExecutable) {
                Invoke-Checked 'cmake.exe' @('--build', $buildDirectory, '--config', 'Release', '--target', 'OpenSees', '-j8')
                $builtExecutable = $true
            }
            Invoke-Checked 'cmake.exe' @('--build', $buildDirectory, '--config', 'Release', '--target', 'OpenSeesPy', '-j8')

            $sourceDll = Join-Path $buildDirectory 'OpenSeesPy.dll'
            if (-not (Test-Path -LiteralPath $sourceDll)) {
                throw "OpenSeesPy output was not found: $sourceDll"
            }
            $destination = Join-Path $moduleDirectory "opensees$($info.suffix)"
            Copy-Item -LiteralPath $sourceDll -Destination $destination -Force
            if (Test-Path -LiteralPath $openMpDll) {
                Copy-Item -LiteralPath $openMpDll -Destination $moduleDirectory -Force
            }

            $previousPythonPath = $env:PYTHONPATH
            try {
                $env:PYTHONPATH = $moduleDirectory
                Invoke-Checked $build.Executable @('-c', 'import opensees; print(opensees.__file__)')
            } finally {
                $env:PYTHONPATH = $previousPythonPath
            }
            Write-Host "Created $destination"
        }

        if ($ConfigureOnly) {
            Write-Host 'All Python versions configured successfully; no binaries were built.'
        } else {
            Write-Host "All versioned modules are in $moduleDirectory"
        }
    } finally {
        Pop-Location
    }
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
