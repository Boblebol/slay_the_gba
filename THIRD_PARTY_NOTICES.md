# Third-party notices

The ROMs are built with the [Butano](https://github.com/GValiente/butano) engine (git submodule `butano/`) and the
[devkitARM](https://devkitpro.org/) toolchain. Butano and the libraries it bundles, some of which are compiled into
the cartridges depending on the features used, are distributed under their own permissive licenses. Their full texts
are in [`butano/licenses/`](https://github.com/GValiente/butano/tree/master/licenses).

| Component | Copyright | License |
|---|---|---|
| [Butano](https://github.com/GValiente/butano) | © 2020-2026 Gustavo Valiente | zlib |
| [maxmod](https://github.com/devkitPro/maxmod) (music and sound) | © 2008-2009 Mukunda Johnson, © 2021-2025 Antonio Niño Díaz | ISC |
| [libtonc](https://github.com/devkitPro/libtonc) | © 2005-2009 J Vijn | MIT |
| [agbabi](https://github.com/felixjones/agbabi) | Felix Jones and contributors | zlib |
| [stdgba](https://github.com/felixjones/stdgba) | Felix Jones and contributors | zlib |
| [Embedded Template Library](https://github.com/ETLCPP/etl) | © 2016 jwellbelove | MIT |
| [ctti](https://github.com/Manu343726/ctti) | © 2015 Manuel Sánchez | MIT |
| [gba-modern](https://github.com/JoaoBaptMG/gba-modern) | © 2019 João Baptista de Paula e Silva | MIT |
| [Cult of GBA BIOS](https://github.com/Cult-of-GBA/BIOS) | © 2020-2021 DenSinH and fleroviux | MIT |
| [gba-link-connection](https://github.com/afska/gba-link-connection) | © 2020 Rodrigo Alfonso | MIT |
| [GBT Player](https://github.com/AntonioND/gbt-player) | © 2009-2022 Antonio Niño Díaz | MIT |
| [ugba](https://github.com/AntonioND/libugba) | © 2011-2015, 2019-2020 Antonio Niño Díaz | MIT |
| [Apex Audio System](https://github.com/stuij/apex-audio-system) | © 2003-2021 James Daniels, © 2021 Ties Stuij | MIT |
| lineclipping-julia | © 2019 Michael Hirsch | MIT |
| posprintf | Dan Posluns | Public domain |
| [devkitARM crt0](https://github.com/devkitPro/devkitarm-crtls) (cartridge startup code) | devkitPro | MPL-2.0 (source at the link) |
| wonderful-toolchain crt0 | © 2022 Luna Mittelbach, © 2023 Adrian Siekierka | MIT |

Other credits: [Tonc: GBA Programming Guide](https://www.coranac.com/tonc/text/) by J Vijn; the code of conduct is
adapted from the [Contributor Covenant](https://www.contributor-covenant.org/) 2.1 (CC BY 4.0). Python packages
used by the build scripts (Pillow, NumPy, Markdown, pytest) are only used at build time and are not distributed.
