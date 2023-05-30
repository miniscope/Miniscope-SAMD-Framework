$currentpath = $PWD
Write-Host ("original path: "+$currentpath)
$scriptpath = Split-Path $MyInvocation.MyCommand.Path
Write-Host ("command path: "+$scriptpath)
Set-Location $scriptpath
$prjroot = "..\..\"

# Array of excluded c files and header files
$cfilepatharray = @("sd_mmc\sd_mmc.c", "hal\src\hal_mci_sync.c", "hpl\dmac\hpl_dmac.c", "..\hpl\sdhc\hpl_sdhc.c")
$headerpatharray = @("sd_mmc\sd_mmc.h", "hal\include\hal_mci_sync.h", "hal\include\hpl_mci_sync.h")


For ($i=0; $i -lt $cfilepatharray.Length; $i++) {
    if (Test-Path -Path ($prjroot + $cfilepatharray[$i] + "tmp") -PathType Leaf) {
        $filepath = Get-ChildItem ($prjroot +  $cfilepatharray[$i] + "tmp")
        if (Test-Path -Path ($prjroot + $cfilepatharray[$i]) -PathType Leaf) {
            Remove-Item ($prjroot + $cfilepatharray[$i])
            Write-Host ($prjroot +  $cfilepatharray[$i] + "has been deleted.")
        }
        try {
            Rename-Item -Path ($filepath) -NewName ($filepath.BaseName + ".c")
            Write-Host ($filepath.BaseName + ".c has been created.")
        }
        catch {
            throw $_.Exception.Message
        }
    }
}

For ($i=0; $i -lt $headerpatharray.Length; $i++) {
    if (Test-Path -Path ($prjroot + $headerpatharray[$i] + "tmp") -PathType Leaf) {
        $filepath = Get-ChildItem ($prjroot +  $headerpatharray[$i] + "tmp")
        if (Test-Path -Path ($prjroot + $headerpatharray[$i]) -PathType Leaf) {
            Remove-Item ($prjroot + $headerpatharray[$i])
            Write-Host ($prjroot +  $headerpatharray[$i] + "has been deleted.")
        }
        try {
            Rename-Item -Path ($filepath) -NewName ($filepath.BaseName + ".h")
            Write-Host ($filepath.BaseName + ".h has been created.")
        }
        catch {
            throw $_.Exception.Message
        }
    }
}
Write-Host ("MS_framework_init.ps1: done")
Set-Location $currentpath