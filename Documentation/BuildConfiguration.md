# Crimson GUI Build Configuration
# Build Configuration & Process
## Before you start:
### Pre-Configure Steps
#### Changing the Release Version
In `C:\crimson\NonSuperBuild.cmake`, edit these variables:

https://github.com/carthurs/CRIMSONGUI/blob/b0f35f1388b77847be4390a6a3686ed55b98bff9/CMakeLists.txt#L176-L179

```cmake
set(${MY_PROJECT_NAME}_VERSION_MAJOR "2020")
set(${MY_PROJECT_NAME}_VERSION_MINOR "03")
set(${MY_PROJECT_NAME}_VERSION_PATCH "12")  
```
Note that the 0 in “03” is important if you have a 1 digit day or month

### Additional Notes

#### Keep the build directory as short as possible
- We recommend keeping the source code in a directory like `C:\crimson`
- We recommend building to a directory like `C:\cr`, because it is very easy to hit the max path limit in Windows

#### You will not be able to move the source or build directory after you start
- The CMake build process will write absolute paths to a lot of configuration files as the build process progresses, unfortunately that means that if you move the build directory, you’ll have to start over.
- We don't recommend trying to transfer the build directory to a different PC, either.

#### Prevent unexpected reboots / power off
- We recommend pausing Windows 10 automatic updates during the compilation process
- Consider changing power settings on your device so Windows doesn’t put the computer to sleep in the middle of the build

## CMake configuration
### Notes about cmake-gui
- CMake expects forward slashes (`/`) for all directory and file paths, not back slashes (`\`), which are what is usually used on Windows.
    - **That means that you should not just copy/paste paths from Windows Explorer into CMake.**
    - If you use CMake’s built in file browser, it will auto-correct the slashes
    - One trick is to paste a path into the file browser, hit enter, and select that and CMake will correct the paths for you.
    - The file browser will not let you specify directories that don’t exist, though (like the presolver), in that case you will just have to type it out and rely on CMake’s autocomplete.
- In cmake-gui, if you click anywhere on the right side of the table in the configuration options on a boolean option (not just on the checkbox), CMake will toggle that option
    - **So be very careful about accidentally clicking in this area**, it may take 5 hours for you to find out that you accidentally clicked something, most of the check boxes will completely break the build if you accidentally enable or disable something

### Conventions used in this guide:
- In the instructions that follow this page, I will refer to a build that takes place in C:\cr\, if you see C:\cr and you built to a different directory, adjust that path to the path that you used.

### Process
1. Acquire the source
```
git clone https://github.com/carthurs/CRIMSONGUI.git
```

2. I renamed the directory to crimson, for me I put it in `C:\crimson`
3. Start cmake-gui (you should be able to start that from “run”, win+R)
4. For the source directory choose `C:/crimson`
5. For the build directory, make a directory named `c:/cr`

Configure 1:

6. Click configure
7. For the generator, choose `Visual Studio 12 2013 Win64`, leave everything else at defaults then click finish
8. It will error out
9. Check `CMAKE_BUILD_TYPE`, use `Debug` builds for development.
10. Check `CRIMSON_BUILD_TRIAL_VERSION` if you are making a trial release (we distribute trial releases through the mailing list). The expiration date is set automatically in the code using expiration dates, you do not need to edit this manually.
11. Set `Qt5_DIR` to `C:/Qt/Qt5.7.0/5.7/msvc2013_64/lib/cmake/Qt5`

Configure 2:

12. Click configure again
13. For `presolver_executable`, manually type in something like  `C:/cr/CMakeExternals/Source/presolver/presolver.exe` (note forward slashes)
14. For `presolver_url`, browse for the presolver zip file you downloaded earlier, the path should be something like `C:/_FILES/Installs/presolver_20210317_1442.zip`
15. Hit configure again, it will succeed
16. Click Generate
