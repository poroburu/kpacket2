# Releasing kpacket2

Steps to publish a versioned GitHub release with the Ashita plugin DLL.

## Versioning

- Git semver lives in the root [`VERSION`](VERSION) file (e.g. `0.1.0-rc.1`).
- Wire protocol version stays in `zmq_relay` config (`protocol_version = "v1"`) until a breaking change.
- Ashita `GetVersion()` returns `1.0` (plugin API convention).

Update [`CHANGELOG.md`](CHANGELOG.md) before tagging. Keep git tags aligned with [kparser2](https://github.com/poroburu/kparser2) for RC/GA releases.

## Pre-release checklist

```powershell
cd C:\path\to\kpacket2
.\build.ps1 -Configuration Release
# optional oracle:
.\build.ps1 -Configuration Release -BuildExamples
```

Verify `hello` / `status` on `:5556` report `release_version` matching `VERSION`.

## Build release zip

```powershell
$ver = Get-Content VERSION | ForEach-Object { $_.Trim() }
$out = "dist/v$ver"
New-Item -ItemType Directory -Force -Path $out | Out-Null

Copy-Item bin\kpacket.dll $out\
Copy-Item README.md, CHANGELOG.md, LICENSE.md $out\

Compress-Archive -Path "$out/*" -DestinationPath "dist/kpacket2-v$ver-win-x86.zip" -Force
```

Copy any vcpkg runtime DLLs required beside `kpacket.dll` if your build links them dynamically.

## Tag and GitHub release

```powershell
git add VERSION CHANGELOG.md docs/RELEASING.md
git commit -m "Release v$ver"
git tag -a "v$ver" -m "kpacket2 v$ver"
git push origin zmq-design2
git push origin "v$ver"

gh release create "v$ver" "dist/kpacket2-v$ver-win-x86.zip" `
  --title "kpacket2 v$ver" `
  --prerelease `
  --notes-file CHANGELOG.md
```

Tag **kpacket2 before kparser2** when releasing a paired RC.

## Install

1. Extract `kpacket.dll` (and any bundled runtime DLLs) to your Ashita `plugins/` folder.
2. `/load kpacket` in-game.
3. Connect clients to `tcp://localhost:5555` and `tcp://localhost:5556`.
