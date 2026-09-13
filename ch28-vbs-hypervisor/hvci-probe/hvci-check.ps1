# Run from an elevated PowerShell prompt on Windows 11 24H2.
$dg = Get-CimInstance -ClassName Win32_DeviceGuard `
      -Namespace root\Microsoft\Windows\DeviceGuard                # 1

'VBS running        : ' + ($dg.VirtualizationBasedSecurityStatus -eq 2)  # 2
'HVCI running       : ' + ($dg.SecurityServicesRunning -contains 2)      # 3
'Credential Guard   : ' + ($dg.SecurityServicesRunning -contains 1)      # 4
'Required security  : ' + ($dg.RequiredSecurityProperties -join ',')     # 5
