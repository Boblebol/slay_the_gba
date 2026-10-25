# Security policy

Slay the GBA is an offline Game Boy Advance homebrew game and a static website: there is no server, account or
personal data. Security issues are still possible, for example in:

* the GitHub Actions workflows (`.github/workflows/`) that build and publish releases;
* the website generator (`scripts/build_site.py`) and the published site;
* a ROM that could harm an emulator or a flash cart through a malformed save.

## Supported versions

Only the [latest release](https://github.com/Boblebol/slay_the_gba/releases/latest) and `main` get fixes.

## Reporting a vulnerability

Please do not open a public issue. Use GitHub's
[private vulnerability reporting](https://github.com/Boblebol/slay_the_gba/security/advisories/new), or write to
[alexandre.enouf@gmail.com](mailto:alexandre.enouf@gmail.com) with the subject "Slay the GBA security".

You will get an answer within a week. Gameplay bugs, crashes and softlocks are not security issues: report them
with the [bug report form](https://github.com/Boblebol/slay_the_gba/issues/new?template=bug_report.yml).
