
Debug mode to find memory locations and such:

```powershell
.\mame.exe -debug -console roms/galaga.zip -autoboot_script mothership_emu.lua
```

Record an `INP` for automated testing:

```powershell
.\mame.exe -console roms/galaga.zip -autoboot_script mothership_emu.lua -input_directory inp -record test
```

Run an `INP` for automated testing:

```powershell
.\mame.exe -console roms/galaga.zip -autoboot_script mothership_emu.lua -input_directory inp -playback test -window
```