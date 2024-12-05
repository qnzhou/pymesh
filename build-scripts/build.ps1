# If you have a proxy, you may need to set the proxy like this:
# $ENV:HTTP_PROXY="http://proxy_host:proxy_port"
# $ENV:HTTPS_PROXY="http://proxy_host:proxy_port"
# you may need to clear the vcpkg cache by remove the folder "$ENV:LOCALAPPDATA\vcpkg\archives"
$ENV:_CL_HACK_CRT=1
$ENV:VCPKG_KEEP_ENV_VARS="_CL_HACK_CRT;_CL_"

Set-Location $PSScriptRoot\..
Set-Variable PYMESH_PATH "$(Get-Location)"
Set-Variable VCPKG_PATH "$(Get-Location)\vcpkg"
Set-Variable PATCH_PATH "$(Get-Location)\build-scripts\patches"
$ENV:VSLANG=1033

# if vs2022 env is not activated, activate it.
where.exe link >nul 2>&1
if ( ! $?) {
    $installPath = &"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version 16.0 -property installationpath
    Import-Module (Join-Path $installPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
    Enter-VsDevShell -VsInstallPath $installPath -SkipAutomaticLocation -DevCmdArguments "-host_arch=x64 -arch=x64"
    where.exe link >nul 2>&1
    if ( ! $?) {
        Write-Output "Link.exe not found. Please install Visual Studio 2022"
        exit 1
    }
}

function find_patches {
    Push-Location $PATCH_PATH
    $patches = Get-ChildItem -Path . -Filter "*.patch" -Recurse
    $relativePaths = @()
    $currentDir = Get-Location
    foreach ($file in $patches) {
        $relativePath = $file.FullName.Substring($currentDir.Path.Length + 1)
        $pos = $relativePath.LastIndexOf("\")
        if ($pos -gt 0) {
            $folder = $relativePath.Substring(0, $pos)
            $name = $relativePath.Substring($pos + 1)
        } else {
            $folder = "."
            $name = $relativePath
        }
        $relativePaths += [PSCustomObject]@{
            Folder = $folder
            Name = $name
        }
    }
    Pop-Location
    return $relativePaths
}

function NormalizeEnv {
    param([String] $KeyName)
    $pathComponents = [System.Environment]::GetEnvironmentVariable($KeyName) -split ';'
    $uniquePaths = @()
    $seenPaths = [System.Collections.Generic.HashSet[string]]::new()

    # Normalize each path component
    foreach ($component in $pathComponents) {
        # Trim any whitespace and skip empty components
        $component = $component.Trim()
        if (-not [string]::IsNullOrEmpty($component)) {
            try {
                $normalizedPath = [System.IO.Path]::GetFullPath($component).TrimEnd('\')
                if ($seenPaths.Add($normalizedPath)) {
                    $uniquePaths += $normalizedPath
                }
            } catch {
            }
        }
    }
    $newPath = [string]::Join(';', $uniquePaths)
    [System.Environment]::SetEnvironmentVariable($KeyName, $newPath)
    # Write-Host "Normalized: $newPath"
}

# apply patch
if (-Not (Test-Path -Path "$PYMESH_PATH\build\.patch_done_tag")) {
    Push-Location $PYMESH_PATH\build-scripts\patches
    $patches = find_patches
    foreach ($patch in $patches) {
        $folder = $patch.Folder
        $name = $patch.Name
        Set-Location "$PYMESH_PATH\third_party\$folder"
        git apply "$PATCH_PATH\$folder\$name"
    }
    New-Item -ItemType File -Path "$PYMESH_PATH/build/.patch_done_tag" -Force
    Pop-Location
}

if (-Not (Test-Path -Path "$VCPKG_PATH\vcpkg.exe")) {
    git clone --depth 1 https://github.com/microsoft/vcpkg.git $VCPKG_PATH
    Set-Location $VCPKG_PATH
    .\bootstrap-vcpkg.bat
}

if (Test-Path -Path "$VCPKG_PATH\installed\x64-windows-static\include\gmp.h") {
    $ENV:GMP_INC="$VCPKG_PATH\installed\x64-windows-static\include"
    $ENV:GMP_LIB="$VCPKG_PATH\installed\x64-windows-static\lib"
    $ENV:GMP_INC_DIR="$ENV:GMP_INC"
    $ENV:GMP_LIB_DIR="$ENV:GMP_LIB"
    # Write-Output $ENV:GMP_INC $ENV:GMP_LIB
} else {
    &"$VCPKG_PATH\vcpkg.exe" install gmp:x64-windows-static
}

if (Test-Path -Path "$VCPKG_PATH\installed\x64-windows-static\include\mpfr.h") {
    $ENV:MPFR_INC="$VCPKG_PATH\installed\x64-windows-static\include"
    $ENV:MPFR_LIB="$VCPKG_PATH\installed\x64-windows-static\lib"
    $ENV:MPFR_INC_DIR="$ENV:MPFR_INC"
    $ENV:MPFR_LIB_DIR="$ENV:MPFR_LIB"
} else {
    &"$VCPKG_PATH\vcpkg.exe" install mpfr:x64-windows-static
}

# need boost.
if (-Not (Test-Path -Path "$VCPKG_PATH\installed\x64-windows-static\include\boost\system.hpp")) {
    $ENV:_CL_='/D_ALLOW_RUNTIME_LIBRARY_MISMATCH'
    $req=@("filesystem","system","program-options","chrono","date-time","thread","foreach",
        "math","multiprecision","property-map","graph","heap","logic","format","ptr-container","callable-traits")
    foreach ($itm in $req) {
        &"$VCPKG_PATH\vcpkg.exe" install "boost-${itm}:x64-windows-static"
    }
}

# don't use tool chain file, we'd like to use only static libraries.
# $ENV:CMAKE_TOOLCHAIN_FILE="$VCPKG_PATH\scripts\buildsystems\vcpkg.cmake"
$ENV:CMAKE_PREFIX_PATH="$VCPKG_PATH\installed\x64-windows-static"

# $ENV:EIGEN_PATH=
if (-Not (Test-Path -Path "$PYMESH_PATH/third_party/build/.all_done_tag")) {
    Set-Location $PYMESH_PATH/third_party
    python build.py all
    if ($?) {
        New-Item -ItemType File -Path "$PYMESH_PATH/third_party/build/.all_done_tag" -Force
    } else {
        exit 1
    }
}

if (-Not (Test-Path -Path "$PYMESH_PATH/build/PyMesh.sln")) {
    Set-Location "$PYMESH_PATH\build"
    cmake ..
}

if (-Not (Test-Path -Path "$PYMESH_PATH/build/.done_tag")) {
    # build the release version
    Set-Location "$PYMESH_PATH\build"
    $ENV:_CL_='/MD'
    cmake --build . --config Release
    Remove-Item ENV:_CL_
    if ($?) {
        Set-Location "$PYMESH_PATH\build\tests"
        $ENV:PATH="$PYMESH_PATH\python\pymesh\lib\Release;$PYMESH_PATH\python\pymesh\third_party\bin;$ENV:PATH"
        NormalizeEnv "PATH"
        cmake --build . --config Release --target tests

        # build the All-in-one python package
        Set-Location "$PYMESH_PATH"
        $LIBS_PATH=(Resolve-Path ((python -c "import sysconfig; print(sysconfig.get_path('include'))") + "\..\libs")).path
        $EXT_SUFFIX=(python -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'))")
        link /nologo /dll /debug /LTCG /out:PyMesh$EXT_SUFFIX /LIBPATH:$LIBS_PATH /LIBPATH:"$VCPKG_PATH\installed\x64-windows-static\lib" "@build-scripts\objs.txt" /nodefaultlib:tbb.lib gmp.lib mpfr.lib

        if ($?) {
            New-Item -ItemType File -Path "$PYMESH_PATH/build/.done_tag" -Force
        } else {
            Set-Location $PSScriptRoot
            exit 1
        }
        Set-Location $PSScriptRoot
    } else {
        exit 1
    }
}

# final step: run the tests
$ENV:PYTHONPATH="$PYMESH_PATH;$PYMESH_PATH/python;$ENV:PYTHONPATH"
NormalizeEnv "PYTHONPATH"
python -c "import pymesh;pymesh.test(verbose=3)"
if ( $? ) {
    Write-Output "Cong! ALL DONE! (Note: expected failure is NOT an error)"
    Set-Location $PSScriptRoot
    exit 0
} else {
    Set-Location $PSScriptRoot
    exit 1
}
