# UPaste

UPaste is a small GTK 3 clipboard-history application. It stores the latest 500 copied text entries locally in an SQLite database and provides history search, copy, delete, clear-history, and pause/resume monitoring controls.

Clipboard contents can include sensitive information. History is stored locally under the user's data directory (`$XDG_DATA_HOME/clipboard-manager/history.sqlite3`, or `~/.local/share/clipboard-manager/history.sqlite3` when `XDG_DATA_HOME` is unset). Use **Pause Monitoring** when needed and **Clear History** to remove saved entries.

## Build and run on Ubuntu

Install the compiler and development dependencies:

```sh
sudo apt update
sudo apt install build-essential cmake pkg-config libgtk-3-dev libsqlite3-dev
```

From the repository root, configure and build:

```sh
cmake -S . -B build
cmake --build build
```

Launch the app from a graphical desktop session:

```sh
./build/clipboard_manager
```

Close the window to exit. Clipboard access on Wayland depends on the compositor and session; verify background capture on each supported desktop/compositor.

## Install for the current user

Install the executable and application-menu entry under `~/.local`:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build
cmake --install build
```

You can then launch **UPaste** from your desktop application menu, or run `~/.local/bin/clipboard_manager` in a graphical session. This installs only for the current user; it does not create a system package such as a `.deb`.

## Build and test the Snap

Install Snapcraft, then build from the repository root (Snapcraft may ask you to configure a build provider):

```sh
sudo snap install snapcraft --classic
snapcraft
```

Snapcraft creates a `.snap` package. On a system with `snapd`, test-install it locally with:

```sh
sudo snap install --dangerous ./upaste_0.1.0_amd64.snap
```

Launch **UPaste** from the application menu or run `snap run upaste`. Remove the test install with `sudo snap remove upaste`. The generated filename includes the target architecture, so it may differ from the example.

## Publish through Ubuntu App Center

Ubuntu App Center obtains Snap applications from the Snap Store. Create a publisher account in the Snap Store, register the unique name `upaste` (if available), and upload the tested Snap. The recipe currently uses the `devel` grade, so it is for testing rather than a stable release; change it to `stable` when the app is ready for general release, then upload and release to the desired channel. Test clipboard capture on both X11 and Wayland under strict confinement before publishing. Choose and add an open-source license before presenting the project as open source.