# LibreLoop

LibreLoop is a free, open-source digital audio workstation for **Windows,
macOS and Linux**. It has built-in instruments and effects, a piano roll,
an arrangement timeline, audio and MIDI recording, and a mixer.

It is written in C11 using SDL3 and miniaudio. It is still being developed,
so expect rough edges.

## Build and run

You only need to install the build tools once. After that, one command prepares
the dependencies, builds LibreLoop and puts the app in a local folder.

### 1. Install the tools for your OS

<details>
<summary><strong>Windows</strong></summary>

Install [MSYS2](https://www.msys2.org/). Open **MSYS2 UCRT64** from the Start
menu and update its packages:

```sh
pacman -Syu
```

If it asks you to close the terminal, reopen UCRT64 and run that command again.
Then install the build tools:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-ca-certificates
```

Keep using the UCRT64 terminal for the build steps below.

</details>

<details>
<summary><strong>macOS</strong></summary>

Open Terminal and install Apple's command-line tools:

```sh
xcode-select --install
```

Install [Homebrew](https://brew.sh/) if you do not already have it, then run:

```sh
brew install cmake ninja python
```

</details>

<details>
<summary><strong>Linux: Ubuntu / Debian</strong></summary>

Open a terminal and run:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build python3 pkg-config \
  libasound2-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev \
  libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev \
  libwayland-dev wayland-protocols libdecor-0-dev libegl1-mesa-dev \
  libgl1-mesa-dev libudev-dev
```

For Gentoo and other distributions, see the [Linux build notes](docs/building.md#linux-dependencies).

</details>

### 2. Get the source code

[Download the source ZIP](https://github.com/abel-fyi/libreloop/archive/refs/heads/main.zip)
and extract it, or clone this repository if you already use Git.

In your terminal, use `cd` to enter the extracted folder. This is the folder
containing this README. On Windows, MSYS2 writes `C:\` as `/c/`, for example:

```sh
cd /c/Users/you/Downloads/libreloop-main
```

Replace that example with your folder's location.

### 3. Build

On **Windows**, run:

```sh
python tools/build.py
```

On **macOS or Linux**, run:

```sh
python3 tools/build.py
```

The first build downloads SDL and checks its checksum before using it. Later
builds reuse it. Building can take a while on a slow computer.

### 4. Open LibreLoop

| OS | Run from the same terminal |
| --- | --- |
| Windows | `./stage-windows/bin/libreloop.exe` |
| macOS | `open stage/LibreLoop.app` |
| Linux | `./stage/bin/libreloop` |

On Windows, you can also double-click `libreloop.exe` inside `stage-windows/bin`.
Keep the whole `stage-windows` folder together; `share` contains the app's fonts,
samples and other resources.

**To rebuild after changing the code:** close LibreLoop and run the same build
command again. It updates the same app folder.

If something goes wrong, check the error just above “Build stopped” and see the
[detailed build guide](docs/building.md). It also covers manual CMake commands,
offline builds, system SDL, tests and packaging.

## Using LibreLoop

Choose **Pattern** or **Song** beside Play to decide what plays. The separate
View switch chooses what you see while editing. Record arms a take; Play starts
it. See [controls and behavior](docs/behavior-spec.md) for more.

## More

- [Website](https://libreloop.org)
- [Development roadmap](docs/roadmap.md)
- [Windows performance measurements](docs/windows-performance.md)
- [Third-party licenses](docs/third-party.md)

LibreLoop is shared under GPL-3.0-only. Bundled libraries, fonts and artwork
keep their own documented licenses.
