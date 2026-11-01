# Tetris Clone

A classic Tetris clone built with modern C++17 and [Raylib](https://www.raylib.com/).

## Features
- Complete 10×20 playing grid
- All 7 standard tetrominoes (I, O, T, S, Z, J, L) with distinctive color schemes
- Smooth controls: movement, wall-kick rotations, soft drop, and hard drop
- Line clearing mechanics and scoring system
- Game over state detection with restart capability
- Visual preview for upcoming tetrominoes

## Controls
- **Left / Right Arrow**: Move piece left / right
- **Up Arrow**: Rotate piece
- **Down Arrow**: Soft drop (faster fall)
- **Space**: Hard drop (instant drop to bottom)
- **R**: Restart game (on Game Over)

## Building & Running

### Prerequisites
- [Raylib](https://www.raylib.com/) and MinGW-w64 (e.g., via `w64devkit`) installed on Windows.

### Build via Command Line
```bash
make
./game.exe
```
Or on Windows with w64devkit:
```cmd
C:\raylib\w64devkit\bin\mingw32-make.exe
.\game.exe
```

### Build & Debug in VS Code
- Press **F5** to build and run with the debugger attached.
- Press **Ctrl + Shift + B** to build the project.
