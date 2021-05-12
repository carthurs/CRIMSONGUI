# Crimson GUI Compilation Process

## 1. [System Requirements and Required Software](Documentation/SystemRequirementsAndRequiredSoftware.md)
## 2. [Build Configuration](Documentation/BuildConfiguration.md)

## 3. Crimson Superbuild
## Notes
- The Crimson GUI uses a Superbuild system, which means that part of the Crimson GUI build process is compiling its dependencies. 
- Note that one of the libraries being built by Crimson, MITK, is also a superbuild, which also builds its own dependencies.
- The Crimson UI Superbuild does not compile the presolver, it just uses a prebuilt binary. The presolver contains Fortran code and is not built with Visual Studio.
- Occasionally you may get an internal error during the superbuild or an access violation
    - If this happens, we recommend restarting your computer, not just Visual Studio.
    - It may not be a bad idea to just restart after every Debug build.

## Process
1. Rename `C:\Program Files (x86)\Windows Kits\8.1\Include\shared\fttypes.h` to `C:\Program Files (x86)\Windows Kits\8.1\Include\shared\fttypes_old.h`
2. Open `C:\cr\CRIMSON-superbuild.sln` in Visual Studio
3. **Check the build type in Visual Studio!!**
    - If you are doing a release build you will probably need to set the build type to Release before building!
    - The CMake and Visual studio build types must match, and Visual Studio will open in debug mode by default.

4. Right click on the `ALL_BUILD` project, and build it
5. Wait for the superbuild to complete, on my PC it took about 6 hours
    - Note that the progress bar in the bottom right is a bit deceptive, it very quickly gets to around 50%, then spends a lot of time there.
6. Close Visual Studio. The whole program, not just “Close Solution”.
    - **Yes, this is a real step, you must close Visual Studio now.**
    - Using File-> Open to open the CRIMSON.sln solution without restarting Visual Studio may cause problems.

## 4. Testing your build
If the compilation was successful, at this point you will have a binary and a batch file that you can run. For me this batch file is in:
```
C:\cr\Crimson-build\bin\startCRIMSON_release.bat
```

Note that there is one batch file for Release and Debug build types, run the batch file that matches your build.

## Debugging Crimson
If you want to debug Crimson, 
1. Start the debug batch file,
2. open the solution in 
C:\cr\Crimson-build\CRIMSON.sln (not the superbuild one)

3. **Verify that the build type (release or debug) matches the build type you selected in CMake**, it may show as Debug when you open it in visual studio by default.
    - This is mostly only important if you decide you want to edit the source code, you can compile Crimson from this solution instead after the superbuild is ran to completion at least once
4. In Visual Studio, do Debug -> Attach to Process to start debugging the running Crimson application.
