PARAM(
    [bool]$StopOnError = $true,
    [bool]$DoVisualStudio = $true,
    [bool]$DoMinGw = $true,
    [bool]$DoStm = $false,
    [bool]$KeepOutput = $false
)

$env:CTEST_OUTPUT_ON_FAILURE=1

$Global:KeepOutput = $KeepOutput

Function RemoveDirs
{
    $SolutionDir = $PSScriptRoot+"\Solution"
    $OutputDir   = $PSScriptRoot+"\Output"
    # Fist Clean Solution if Existing
    if((Test-Path $SolutionDir))
    {
        Remove-Item $SolutionDir -Recurse -Force
    }
    # Fist Clean Solution if Existing
    if( (Test-Path $OutputDir) -eq $true -and
        $Global:KeepOutput -eq $false)
    {
        Remove-Item $OutputDir -Recurse -Force
    }
}

Function ResetDirs
{
    $SolutionDir = $PSScriptRoot+"\Solution"
    $OutputDir   = $PSScriptRoot+"\Output"
    RemoveDirs
    New-Item $SolutionDir -ItemType Directory
    if((Test-Path $OutputDir) -eq $false)
    {
        New-Item $OutputDir -ItemType Directory
    }
}

function StartBuildProcess
{
    PARAM(
        $VisualStudio,
        $Architecture,
        $Configuration,
        $Static,
        $ExtendedArchtecture
    )

    # Enable parallel build
    $ENV:CL="/MP"

    $CurrentDir  = (Get-Item .\).FullName
    $TestLog     = $CurrentDir+"/Test.log"
    $CcOSRootDir = $PSScriptRoot+"/../Sources"
    $SolutionDir = $PSScriptRoot+"/Solution"
    $OutputDir   = $PSScriptRoot+"/Output"
    ResetDirs

    cd $SolutionDir

    $VisualStudioString = $VisualStudio
    $AppendVS19 = ""
    $AppendVS19_2 = ""
    if($Architecture -eq "x64")
    {
        if($ExtendedArchtecture)
        {
            $AppendVS19 = "-A"
            $AppendVS19_2 = "x64"
        }
        else
        {
            $VisualStudioString += " Win64"
        }
    }
    else
    {
        if($ExtendedArchtecture)
        {
            $AppendVS19 = "-A"
            $AppendVS19_2 = $Architecture
        }
    }
    try
    {
        $AppendCmake = ""
        $AppendCmake2 = ""
        if($Static -eq "Static")
        {
            $AppendCmake = "-DCC_LINK_TYPE=STATIC"
        }
        if($Configuration -eq "Release")
        {
            $AppendCmake2 = "-DCC_WARNING_AS_ERROR=TRUE"
        }
        & "cmake.exe" "$CcOSRootDir" "-G" $VisualStudioString $AppendVS19 $AppendVS19_2 "-DCC_OUTPUT_DIR=`"$OutputDir`"" "$AppendCmake" "$AppendCmake2"
        if($LASTEXITCODE -ne 0)
        {
            cd $CurrentDir
            $Msg = "Failed: cmake generation with $VisualStudioString $Configuration $AppendCmake"
            Add-Content $TestLog $Msg
            throw $Msg
        }
        cmake.exe --build . --config $Configuration
        if($LASTEXITCODE -ne 0)
        {
            cd $CurrentDir
            $Msg = "Failed: cmake build with $VisualStudioString $Configuration $AppendCmake"
            Add-Content $TestLog $Msg
            throw $Msg
        }
        ctest --output-on-failure -C $Configuration
        if($LASTEXITCODE -ne 0)
        {
            cd $CurrentDir
            $Msg = "Failed: ctest with $VisualStudioString $Configuration $AppendCmake"
            Add-Content $TestLog $Msg
            throw $Msg
        }
        Add-Content $TestLog "Success: $VisualStudioString $Configuration $AppendCmake"
    }
    catch
    {
        cd $CurrentDir
        if($StopOnError)
        {
            throw $Msg
        }
        else
        {
            Write-Host $_.Exception.Message
        }

    }
    cd $CurrentDir
    RemoveDirs
}

Function Test-VisualStudio()
{
    $VisualStudios = @()
    if((Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe") -and
       (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2017"))
    {
        $VisualStudio15 += @{ Name     = "Visual Studio 15 2017"};
        $VisualStudio15 += @{ Extended = $false};
        $VisualStudios += $VisualStudio15
    }
    if((Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe") -and
       (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2019"))
    {
        $VisualStudio16 += @{ Name     = "Visual Studio 16 2019"};
        $VisualStudio16 += @{ Extended = $true};
        $VisualStudios += $VisualStudio16
    }
    if( (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe") -and
       ((Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2022") -or
        (Test-Path "C:\Program Files\Microsoft Visual Studio\2022")) )
    {
        $VisualStudio17 += @{ Name     = "Visual Studio 17 2022"};
        $VisualStudio17 += @{ Extended = $true};
        $VisualStudios += $VisualStudio17
    }

    $Architectures  = @("win32", "x64")
    $Configurations = @("Debug", "Release") # Not required but possible to test : "RelWithDebInfo", "MinSizeRel")
    $Statics = @("Static", "Shared")

    $CurrentDir  = (Get-Item .\).FullName
    $TestLog     = $CurrentDir+"\Test.log"
    if((Test-Path $TestLog))
    {
        Remove-Item $TestLog
    }

    foreach($VisualStudio in $VisualStudios)
    {
        foreach($Architecture in $Architectures)
        {
            foreach($Configuration in $Configurations)
            {
                foreach($Static in $Statics)
                {
                    # ExampleCall StartBuildProcess "Visual Studio 12" "win32" "Release" "Shared"
                    # ExampleCall StartBuildProcess "Visual Studio 12" "x64" "Debug" "Static"
                    StartBuildProcess $VisualStudio.Name $Architecture $Configuration $Static $VisualStudio.Extended
                }
            }
        }
    }
}

Function Test-MinGW()
{
    $oProcInfos = Get-WmiObject -class Win32_processor
    $iCores = 1
    foreach($oProcInfo in $oProcInfos)
    {
        $iCores = $oProcInfo.NumberOfCores
    }

    $CurrentDir  = (Get-Item .\).FullName
    $TestLog     = $CurrentDir+"/Test.log"
    $CcOSRootDir = $PSScriptRoot+"/../Sources"
    $SolutionDir = $PSScriptRoot+"/Solution"
    $OutputDir   = $PSScriptRoot+"/Output"
    ResetDirs

    $Versions  = @("6") # "4.8.2"
    $Architectures  = @("x86", "x64")
    $Configurations = @("Debug", "Release")
    foreach($Version in $Versions)
    {
        foreach($Architecture in $Architectures)
        {
            foreach($Configuration in $Configurations)
            {
                cd $CurrentDir
                ResetDirs
                cd $SolutionDir


                cmake.exe "$CcOSRootDir" "-G" "`"MinGW Makefiles`"" "-DCCOS_BOARD=MinGW" "-DCMAKE_BUILD_TYPE=$Configuration" "-DCC_OUTPUT_DIR=`"$OutputDir`"" "-DCMAKE_SYSTEM_PROCESSOR=`"$Architecture`"" "-DMINGW_VERSION=`"$Version`""
                if($LASTEXITCODE -ne 0)
                {
                    cd $CurrentDir
                    $Msg = "Failed: cmake generation of MinGW $Configuration $Architecture $Version failed"
                    Add-Content $TestLog $Msg
                    throw $Msg
                }
                cmake.exe --build . --config $Configuration -- "-j$iCores"
                if($LASTEXITCODE -ne 0)
                {
                    cd $CurrentDir
                    $Msg = "Failed: cmake build wit MinGW $Configuration $Architecture $Version failed"
                    Add-Content $TestLog $Msg
                    throw $Msg
                }
                ctest --output-on-failure -C $Configuration
                if($LASTEXITCODE -ne 0)
                {
                    cd $CurrentDir
                    $Msg = "Failed: ctest with MinGW $Configuration $Architecture $Version failed"
                    Add-Content $TestLog $Msg
                    throw $Msg
                }
                Add-Content $TestLog "Success: MinGW $Configuration $Architecture $Version"
            }
        }
    }
    cd $CurrentDir
    RemoveDirs
}

Function Test-Stm()
{
    $oProcInfos = Get-WmiObject -class Win32_processor
    $iCores = 1
    foreach($oProcInfo in $oProcInfos)
    {
        $iCores = $oProcInfo.NumberOfCores
    }

    $CurrentDir  = (Get-Item .\).FullName
    $TestLog     = $CurrentDir+"\Test.log"
    $CcOSRootDir = $PSScriptRoot+"\..\Sources"
    $SolutionDir = $PSScriptRoot+"\Solution"
    $OutputDir   = $PSScriptRoot+"\Output"

    ResetDirs
    cd $SolutionDir

    $oBoards = @("STM32F4Discovery", "stm32f3discovery")

    foreach($sBoard in $oBoards)
    {
        cmake -G "Eclipse CDT4 - Unix Makefiles" "$CcOSRootDir" "-DCMAKE_ECLIPSE_VERSION=4.9" "-DCCOS_BOARD=$sBoard" "-DCMAKE_BUILD_TYPE=Debug" "-DCC_OUTPUT_DIR=`"$OutputDir`""
        if($LASTEXITCODE -ne 0)
        {
            cd $CurrentDir
            $Msg = "Failed: cmake generation of $sBoard failed"
            Add-Content $TestLog $Msg
            throw $Msg
        }
        cmake.exe --build . --config Release -- "-j$iCores"
        if($LASTEXITCODE -ne 0)
        {
            cd $CurrentDir
            $Msg = "Failed: cmake build wit $sBoard failed"
            Add-Content $TestLog $Msg
            throw $Msg
        }
        cmake.exe --build . --config Debug -- "-j$iCores"
        if($LASTEXITCODE -ne 0)
        {
            cd $CurrentDir
            $Msg = "Failed: cmake build wit $sBoard failed"
            Add-Content $TestLog $Msg
            throw $Msg
        }
        Add-Content $TestLog "Success: $sBoard"
    }

    cd $CurrentDir

    RemoveDirs
}

$ENV:TEST_CCOS="TRUE"

if($DoVisualStudio)
{
    Test-VisualStudio
}

if($DoMinGw)
{
    Test-MinGW
}

if($DoStm)
{
    Test-Stm
}
