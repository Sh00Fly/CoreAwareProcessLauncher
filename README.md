# Core Aware Process Launcher (CAPL)

Core Aware Process Launcher is a Windows utility designed to launch processes with intelligent CPU core affinity management, optimizing performance on hybrid CPU architectures.

## Table of Contents
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Planned Improvements](#planned-improvements)
- [License](#license)
- [Contact](#contact)

## Features
- **Core Affinity Control:** Launch processes with specific core affinity settings.
- **Hybrid CPU Support:** Automatic detection and management of P-cores, E-cores, and LP E-cores using CPUID instructions.
- **Command-Line Interface:** Easy-to-use CLI for target process startup.
- **Console-Free Execution:** GUI version can be used in batch files or shortcuts without opening a console window.
- **Logging:** Global logging instance for error tracking and diagnostics.
- **Detailed CPU Information:** Query system capabilities and core types.

## Requirements
- **Operating System:** Windows
- **Development Environment:** Visual Studio 2022 (for building)
- **Hardware:** CPU with hybrid architecture (for P/E/LP-core detection)

## Installation
1. Clone the repository to your local machine.
2. Open the solution in Visual Studio 2022.
3. Select the desired configuration (Debug/Release).
4. Build the solution.

## Usage
### Command-Line Interface
#### Usage
```bash
caplcli.exe [options] -- <program> [program arguments]
```
or
```bash
caplgui.exe [options] -- <program> [program arguments]
```
#### Options
- `--help`, `-h`, `-?`, `/?`: Display help information.
- `--query`, `-q`: Show detailed system CPU information.
- `--mode`, `-m <mode>`: Set core affinity mode. Modes include:
  - `p`: P-cores only.
  - `e`: E-cores only.
  - `lp`: LP E-cores only.
  - `alle`: All E-cores.
  - `all`: All cores.
- `--cores <list>`: Custom core selection (comma-separated list).
- `--invert`, `-i`: Invert core selection.
- `--dir`, `-d <path>`: Set working directory for the target process.
- `--log`, `-l`: Enable logging.
- `--logpath <path>`: Specify log file path.

#### Process
- `-- <program> [args]`: Program to launch with its arguments

### Examples
```bash
caplgui.exe --mode p -- notepad.exe "file.txt"
caplcli.exe --mode lp --dir "C:\Work" -- program.exe -arg1 -arg2
caplcli.exe --mode alle -- cmd.exe /c "batch.cmd"
caplgui.exe --cores 0,2,4 -- program.exe
caplcli.exe --query
```

### Notes
- Either `--mode` or `--cores` must be specified for launching a process.
- Core numbers must be non-negative and within system limits.
- `--mode all` explicitly locks process to all cores, preventing Windows from dynamically restricting core usage.

### GUI Version
- Use the GUI executable in batch files or shortcuts to avoid opening a console window.
- The GUI version uses message boxes for help and error messages.
- Example usage in a batch file:
   ```batch
   start "" caplgui.exe --mode lp --dir "C:\Work" -- program.exe -arg1 -arg2
   ```

## Planned Improvements
- Implement INI file support and configuration persistence.
- Extend core detection patterns and verify on more CPU models.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE.txt) file for details.

## Contact
For questions or support, please use the GitHub issue tracker.
