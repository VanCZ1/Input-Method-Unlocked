# Input-Method-Unlocked

An SKSE plugin that allows the use of Input Method Editor in Skyrim.

## Requirements

- [Git](https://git-scm.com)
  - Add to your `PATH`
- [Visual Studio Community 2026](https://visualstudio.microsoft.com)
  - Desktop development with C++

## User Requirements

- [Address Library for SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
  - Needed for SSE
- [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
  - Needed for VR

## Register Visual Studio as a Generator

- Open `x64 Native Tools Command Prompt`
- Run `cmake`
- Close the cmd window

## Building

```
git clone https://github.com/VanCZ1/Input-Method-Unlocked.git
cd Input-Method-Unlocked
git submodule update --init --recursive
```

```
cmake --preset release
cmake --build --preset release
```
