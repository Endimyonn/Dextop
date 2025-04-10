# Dextop
Work-in-progress desktop client for MangaDex.

## Goals
The objective is a relatively lightweight (<25MB download) and cross-platform MangaDex client with a UI design emphasizing compactness.

The primary focus is on finding and reading content. I don't currently plan to implement features related to uploading and editing manga, but other features that require a login (e.g. following and update viewing) will be available.

## Building
This project is not ready for normal use and things will break/not work. To build it, open the project in VS Code and let it configure, then build it, or:
1. Install CMake
2. If using Windows, install curl through vcpkg. Otherwise, ensure libcurl is present on your system.
2. In the root directory, run `cmake ./ "-B./build" -T host=x64 -A x64` to configure the project
3. Run `cmake --build "./build" --config Release --target ALL_BUILD -j 26` to build it