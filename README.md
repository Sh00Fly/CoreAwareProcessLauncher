# Core Aware Process Launcher (CAPL)

A Windows utility for launching processes with intelligent CPU core affinity management.

## Features
- Automatic P-core detection for hybrid CPU architectures
- Process launching with specific core affinity
- INI-based configuration

## Requirements
- Windows operating system
- Visual Studio 2022 (for building)
- CPU with hybrid architecture (for P-core detection)

## Building
1. Open the solution in Visual Studio 2022
2. Select configuration (Debug/Release)
3. Build the solution

## Usage
1. Rename the executable to match your target program
2. Create an INI file with the same name
3. Specify the target program path in the INI file
4. Launch the utility
