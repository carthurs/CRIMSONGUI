# Crimson GUI Compilation System Requirements and Required Software

# System Requirements for Compiling
- 500 GB SSD or larger recommended, as your C drive. An external drive does not count.

- 64 bit Windows
    - The build has only been tested on Windows 10, but it might work on Windows 7
    - This guide was tested on a clean install of Windows 10 Pro 64 bit version 1909, with all the latest updates as of Nov. 5th 2020.
    - I ran all these tests with UAC set to default levels and Windows Firewall active and set to default settings, no special exceptions were made.

- My compilation PC’s specs:
    - Intel Core i3-2100 CPU
    - 8 GB of RAM
    - Intel HD Graphics 2000 (no dedicated GPU)

- How PC specs affect performance in compiling:
    - The dual core / 4 HT CPU was definitely working hard, at around 100% all of the time.
        - This CPU was not particularly great even 10 years ago
    - The PC pretty steadily used around 5 GB of RAM during the build, I don’t think RAM really makes much of a difference.
    - The build script does not make particularly good use of multicore CPUs in general though, I don’t think you should go out and buy a Ryzen Threadripper for this.

- How PC specs affect running Crimson itself:
    - You may experience slow performance in the UI without a dedicated GPU, my Core i3 PC was OK for very simple models, though.
        - Meshing and other demanding operations like running the flowsolver will be much faster with more RAM and a better CPU.
    - I have very quickly used all my RAM on fine mesh operations. 
    - Scale back your expectations about how fine your mesh can be if you are running the Crimson UI on a weak machine, or be prepared to wait a very long time for meshing to finish.

# Installing Required Software
## Visual Studio 2013 Community, Update 5
- We recommend compiling Crimson on a PC that has only ever had Visual Studio 2013 installed on it, and never had any other version installed on it.
- Installing other versions of Visual Studio (newer or older) on the same PC (before or after installing Visual Studio 2013) may cause issues and is not recommended
- Attempting to uninstall a different version of Visual Studio to then install Visual Studio 2013 may also cause issues and is not recommended
- Note that Visual Studio 2013 [has issues in recent versions of Windows 10](https://developercommunity2.visualstudio.com/t/Unexpected-VS-crash-when-docking-or-spli/1323017).

## Visual Studio Installation Procedure
1. Download the Visual Studio installer. We recommend using the iso installer. You can get a link for downloading Visual Studio by signing up for Visual Studio Dev Essentials (free at the time of writing)
- https://visualstudio.microsoft.com/

2. Mount the file by right clicking on it and clicking mount.
3. It will show up as a virtual DVD drive, double click on vs_community.exe
4. For my setup the only option I had checked in “Optional Features to Install” was “Microsoft Foundation Classes for C++”, you don’t need any of the other optional features
5. Wait for the setup to finish
6. Sign in to Visual Studio to register it with your Microsoft Account. 

If you don’t sign in, Visual Studio will stop working completely in 30 days. You can sign out as soon as it gets a license if you like, though. You may have to sign in to Visual Studio occasionally to “renew” your license.

## Software required for compilation
### Qt 5.7.0
1. Download link: https://download.qt.io/new_archive/qt/5.7/5.7.0/qt-opensource-windows-x86-msvc2013_64-5.7.0.exe
2. You can skip the Qt login
3. Let Qt install to the default location
4. In “Select Components” leave everything at its defaults

### CMake 3.13.5
1. Download Link: https://cmake.org/files/v3.13/cmake-3.13.5-win64-x64.zip
2. Extract this zip file to a location of your choice, I put it in `C:\_FILES\prog\cmake-3.13.5\`
3. Add the bin folder to `PATH` in that directory (the build script will need to have cmake.exe in the path), and we will be using cmake-gui.exe often too.

### Git for Windows
The superbuild script needs this in many steps. **This is not just for cloning the Crimson source from github.** Any recent version of this will do.

Install with all default options

### Presolver
- Download link: https://umich.box.com/s/pa5s9ua248g79mbd2er47j4keyd6v6j4
- Just download this zip file somewhere on your computer, don't extract it. We will use it later.

### After installing these components, I recommend restarting the computer
It seems that CMake had trouble finding Git until I restarted.
