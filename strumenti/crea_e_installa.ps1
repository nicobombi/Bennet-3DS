# La Valle dei Germogli - crea il file .cia e lo offre a FBI con un codice QR.
# Si avvia con CREA_E_INSTALLA.bat (doppio clic). Con -SoloServi offre il .cia gia' pronto senza ricompilare.
param([switch]$SoloServi, [int]$Porta = 8080)
$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$Radice = Split-Path -Parent $PSScriptRoot
$Strum = Join-Path $PSScriptRoot 'win'
$Nome = 'ValleDeiGermogli'
$Cia = Join-Path $Radice "$Nome.cia"
Set-Location $Radice
function Passo($t) { Write-Host ""; Write-Host "== $t" -ForegroundColor Yellow }
function Errore($t) { Write-Host ""; Write-Host "ERRORE: $t" -ForegroundColor Red; exit 1 }

# ---------- strumenti esterni: makerom (obbligatorio) e bannertool (per il banner del menu HOME) ----------
function Scarica-Strumento($repo, $filtro, $exe) {
  $dest = Join-Path $Strum $exe
  if (Test-Path $dest) { return $dest }
  $sulPath = Get-Command $exe -ErrorAction SilentlyContinue
  if ($sulPath) { return $sulPath.Source }
  New-Item -ItemType Directory -Force -Path $Strum | Out-Null
  try {
    Write-Host "Scarico $exe da github.com/$repo ..."
    $rel = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases" -Headers @{ 'User-Agent' = 'ValleDeiGermogli' }
    $asset = $null
    foreach ($r in $rel) { $asset = $r.assets | Where-Object { $_.name -match $filtro } | Select-Object -First 1; if ($asset) { break } }
    if (-not $asset) { throw "nessun file adatto" }
    $zip = Join-Path $env:TEMP $asset.name
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zip -UseBasicParsing
    $tmp = Join-Path $env:TEMP ("vdg_" + [guid]::NewGuid().ToString('N'))
    Expand-Archive -Path $zip -DestinationPath $tmp -Force
    $trovato = Get-ChildItem -Path $tmp -Recurse -Filter $exe | Where-Object { $_.FullName -notmatch 'linux|mac|osx' } | Select-Object -First 1
    if (-not $trovato) { $trovato = Get-ChildItem -Path $tmp -Recurse -Filter $exe | Select-Object -First 1 }
    if (-not $trovato) { throw "$exe non trovato nell'archivio" }
    Copy-Item $trovato.FullName $dest
    return $dest
  } catch {
    Write-Host "Non sono riuscito a scaricare $exe ($($_.Exception.Message))." -ForegroundColor DarkYellow
    return $null
  }
}

if (-not $SoloServi) {
  # ---------- compilazione con devkitPro ----------
  Passo "Compilo il gioco (devkitPro)"
  $dkp = if ($env:DEVKITPRO -and (Test-Path ($env:DEVKITPRO -replace '^/opt/devkitpro', 'C:\devkitPro'))) { $env:DEVKITPRO -replace '^/opt/devkitpro', 'C:\devkitPro' } else { 'C:\devkitPro' }
  $bash = Join-Path $dkp 'msys2\usr\bin\bash.exe'
  if (-not (Test-Path $bash)) { Errore "devkitPro non trovato in $dkp. Installalo da https://devkitpro.org/wiki/Getting_Started (seleziona '3DS Development') e riprova." }
  $posix = '/' + $Radice.Substring(0, 1).ToLower() + ($Radice.Substring(2) -replace '\\', '/')
  & $bash -lc "cd '$posix' && make -j4"
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path "$Nome.elf")) { Errore "la compilazione non e' andata a buon fine (vedi i messaggi sopra)." }

  # ---------- banner e .cia ----------
  Passo "Preparo gli strumenti per il .cia"
  $makerom = Scarica-Strumento '3DSGuy/Project_CTR' 'makerom.*win.*\.zip$' 'makerom.exe'
  if (-not $makerom) { Errore "manca makerom.exe. Scaricalo da https://github.com/3DSGuy/Project_CTR/releases (file makerom ... win), mettilo in strumenti\win\ e rilancia." }
  $bannertool = Scarica-Strumento 'carstene1ns/3ds-bannertool' '\.zip$' 'bannertool.exe'
  $argBanner = @()
  if ($bannertool) {
    & $bannertool makebanner -i cia\banner.png -a cia\banner.wav -o build\banner.bnr | Out-Null
    if ($LASTEXITCODE -eq 0 -and (Test-Path build\banner.bnr)) { $argBanner = @('-banner', 'build\banner.bnr') }
  }
  if (-not $argBanner) { Write-Host "Nota: senza bannertool il .cia funziona lo stesso, ma nel menu HOME lo schermo in alto restera' vuoto." -ForegroundColor DarkYellow }

  Passo "Creo $Nome.cia"
  if (Test-Path $Cia) { Remove-Item $Cia }
  & $makerom -f cia -o $Cia -elf "$Nome.elf" -rsf cia\app.rsf -icon "$Nome.smdh" @argBanner -exefslogo -target t
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path $Cia)) { Errore "makerom non ha creato il .cia." }
  $mb = [math]::Round((Get-Item $Cia).Length / 1MB, 1)
  Write-Host "Fatto: $Nome.cia ($mb MB)" -ForegroundColor Green
}
if (-not (Test-Path $Cia)) { Errore "non trovo $Nome.cia: lancia prima CREA_E_INSTALLA.bat." }

# ---------- server per FBI ----------
Passo "Offro il gioco a FBI"
$ip = $null
try { $ip = @((Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway -and $_.NetAdapter.Status -eq 'Up' } | Select-Object -First 1).IPv4Address.IPAddress)[0] } catch {}
if (-not $ip) { $a = [System.Net.Dns]::GetHostAddresses([System.Net.Dns]::GetHostName()) | Where-Object { $_.AddressFamily -eq 'InterNetwork' -and $_.ToString() -notmatch '^(127|169\.254)\.' } | Select-Object -First 1; if ($a) { $ip = $a.ToString() } }
if (-not $ip) { Errore "non trovo l'indirizzo del PC nella rete locale." }

$ascolto = $null
for ($p = $Porta; $p -lt $Porta + 20; $p++) {
  try { $ascolto = New-Object System.Net.Sockets.TcpListener([System.Net.IPAddress]::Any, $p); $ascolto.Start(); $Porta = $p; break } catch { $ascolto = $null }
}
if (-not $ascolto) { Errore "nessuna porta libera tra $Porta e $($Porta + 19)." }
$url = "http://${ip}:$Porta/$Nome.cia"
$pagina = (Get-Content (Join-Path $PSScriptRoot 'pagina_qr.html') -Raw -Encoding UTF8) -replace '%%URL%%', $url
$paginaB = [Text.Encoding]::UTF8.GetBytes($pagina)
$qrjsB = [IO.File]::ReadAllBytes((Join-Path $PSScriptRoot 'qr.js'))

Write-Host "Indirizzo per FBI: $url" -ForegroundColor Green
Write-Host "Apro la pagina con il codice QR nel browser..."
Write-Host "Se Windows chiede il permesso del firewall, consenti l'accesso sulle reti private."
Write-Host "Premi Ctrl+C per chiudere quando FBI ha finito."
Start-Process "http://localhost:$Porta/"

function Rispondi($stream, $codice, $tipo, [byte[]]$corpo, $head) {
  $h = "HTTP/1.1 $codice`r`nContent-Type: $tipo`r`nContent-Length: $($corpo.Length)`r`nConnection: close`r`n`r`n"
  $hb = [Text.Encoding]::ASCII.GetBytes($h); $stream.Write($hb, 0, $hb.Length)
  if (-not $head) { $stream.Write($corpo, 0, $corpo.Length) }
}
while ($true) {
  if (-not $ascolto.Pending()) { Start-Sleep -Milliseconds 100; continue }
  $cli = $ascolto.AcceptTcpClient()
  try {
    $cli.ReceiveTimeout = 5000; $s = $cli.GetStream()
    $r = New-Object System.IO.StreamReader($s, [Text.Encoding]::ASCII, $false, 1024, $true)
    $riga = $r.ReadLine(); while ($true) { $l = $r.ReadLine(); if ($l -eq $null -or $l -eq '') { break } }
    if (-not $riga) { continue }
    $parti = $riga.Split(' '); $metodo = $parti[0]; $via = $parti[1]; $head = $metodo -eq 'HEAD'
    $chi = $cli.Client.RemoteEndPoint.Address.ToString()
    if ($via -like "/$Nome.cia*") {
      $f = [IO.File]::OpenRead($Cia)
      try {
        $h = "HTTP/1.1 200 OK`r`nContent-Type: application/octet-stream`r`nContent-Length: $($f.Length)`r`nConnection: close`r`n`r`n"
        $hb = [Text.Encoding]::ASCII.GetBytes($h); $s.Write($hb, 0, $hb.Length)
        if (-not $head) { Write-Host "$(Get-Date -Format HH:mm:ss)  $chi scarica il gioco..."; $f.CopyTo($s); Write-Host "$(Get-Date -Format HH:mm:ss)  invio completato: conferma l'installazione su FBI." -ForegroundColor Green }
      } finally { $f.Close() }
    } elseif ($via -eq '/' -or $via.StartsWith('/?')) { Rispondi $s '200 OK' 'text/html; charset=utf-8' $paginaB $head }
    elseif ($via -eq '/qr.js') { Rispondi $s '200 OK' 'application/javascript' $qrjsB $head }
    else { Rispondi $s '404 Not Found' 'text/plain' ([Text.Encoding]::ASCII.GetBytes('non trovato')) $head }
  } catch { Write-Host "Connessione interrotta: $($_.Exception.Message)" -ForegroundColor DarkYellow }
  finally { $cli.Close() }
}
