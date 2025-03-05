# Dextop
Work-in-progress desktop client for MangaDex.

## Goals
The primary objective is a relatively lightweight (<~50MB) and cross-platform MangaDex client with a UI design emphasizing compactness.

This serves as a learning vehicle for areas of software development I have minimal experience in.

## Building
This project is not ready for normal use and things will break/not work. To build it, open the project in VS Code and let it configure, then build it, or:
1. In the root directory, run `cmake ./ "-B./build -T host=x64 -A x64` to configure the project
2. Run `cmake --build "./build" --config Release --target ALL_BUILD -j 26` to build it