$scriptpath = Split-Path $MyInvocation.MyCommand.Path
Set-Location $scriptpath
$prjroot = "..\..\"

# Array of excluded c files and header files
$cfilepatharray = @("sd_mmc\sd_mmc.c", "hal\src\hal_mci_sync.c", "hpl\dmac\hpl_dmac.c", "..\hpl\sdhc\hpl_sdhc.c")
$headerpatharray = @("sd_mmc\sd_mmc.h", "hal\include\hal_mci_sync.h", "hal\include\hpl_mci_sync.h")


For ($i=0; $i -lt $cfilepatharray.Length; $i++) {
    if (Test-Path -Path ($prjroot + $cfilepatharray[$i]) -PathType Leaf) {
        $filepath = Get-ChildItem ($prjroot + $cfilepatharray[$i])
        if (Test-Path -Path ($prjroot +  $cfilepatharray[$i] + "tmp") -PathType Leaf) {
            Remove-Item ($prjroot +  $cfilepatharray[$i] + "tmp")
            Write-Host ($prjroot +  $cfilepatharray[$i] + "tmp has been deleted.")
        }
        try {
            Rename-Item -Path ($prjroot + $cfilepatharray[$i]) -NewName ($filepath.Name + "tmp")
            Write-Host ($filepath.Name + "tmp has been created.")
        }
        catch {
            throw $_.Exception.Message
        }
    }
}

For ($i=0; $i -lt $headerpatharray.Length; $i++) {
    if (Test-Path -Path ($prjroot + $headerpatharray[$i]) -PathType Leaf) {
        $filepath = Get-ChildItem ($prjroot + $headerpatharray[$i])
        if (Test-Path -Path ($prjroot +  $headerpatharray[$i] + "tmp") -PathType Leaf) {
            Remove-Item ($prjroot +  $headerpatharray[$i] + "tmp")
            Write-Host ($prjroot +  $headerpatharray[$i] + "tmp has been deleted.")
        }
        try {
            Rename-Item -Path ($prjroot + $headerpatharray[$i]) -NewName ($filepath.Name + "tmp")
            Write-Host ($filepath.Name + "tmp has been created.")
        }
        catch {
            throw $_.Exception.Message
        }
    }
}