# Changelog

All notable changes to kpacket2 are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.1.0-rc.1] - 2026-06-17

First pre-release of the C++ Ashita packet capture plugin for the kparser2 stack.

### Added

- ZeroMQ relay on ports **5555** (PUB/SUB), **5556** (REQ/REP), **5557** (PUSH/PULL)
- Wire protocol **v1**: multipart topic + JSON meta + raw packet bytes
- Topic format `kpacket.v1.world.s2c.0xHHHH` / `kpacket.v1.world.c2s.0xHHHH`
- JSON command interface: `hello`, `status`, `stats`, `inject`, `filter`
- `hello` / `status` expose `release_version` (git semver) separate from wire `version` (`v1`)
- `hello` includes `player_name` when the local party leader name is available
- PowerShell build script (`build.ps1`) for HorizonXI deploy
- Opcode name table updates for battle and trophy packets

### Notes

- Pair with [kparser2 v0.1.0-rc.1](https://github.com/poroburu/kparser2/releases/tag/v0.1.0-rc.1) for RC testing
- Ashita `GetVersion()` remains `1.0` (Ashita plugin API convention)

[0.1.0-rc.1]: https://github.com/poroburu/kpacket2/releases/tag/v0.1.0-rc.1
