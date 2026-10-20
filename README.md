# 🐍 Snake Game — C++ Terminal Edition

A classic Snake game built with C++17 for Linux terminals, featuring real-time keyboard controls, colored output, score tracking, and progressively increasing difficulty.

## ✨ Features

- **Real-time controls:** Move with W, A, S, D.
- **Colored terminal UI:** Green snake, red food, and a cyan title.
- **Scoring system:** Earn 10 points for every food item.
- **Progressive difficulty:** The snake gets faster as you eat.
- **Levels:** Advance every five food items.
- **Pause and resume:** Press P to pause or continue.
- **Restart:** Press R to start a new game.
- **Collision detection:** Walls and the snake's body end the game.
- **Random food placement:** Food never spawns inside the snake.
- **Terminal restoration:** The original terminal settings are restored on exit.

## 🛠️ Requirements

- Linux terminal
- C++17-compatible compiler (such as G++)
- `poll()` and POSIX terminal support

## 🚀 Installation

Clone the repository:

```bash
git clone https://github.com/YOUR_USERNAME/snake-game.git
cd snake-game
```

Compile:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic snake.cpp -o snake
```

Run:

```bash
./snake
```

## 🎮 Controls

| Key | Action |
|---|---|
| W | Move up |
| A | Move left |
| S | Move down |
| D | Move right |
| P | Pause / Resume |
| R | Restart |
| Q | Quit |

## 📚 Concepts Practiced

- C++ classes and structures
- Vectors and algorithms
- Enumerations
- Game loops and state management
- Collision detection
- Random number generation
- Linux terminal input with `termios`
- Event polling with `poll()`
- ANSI escape sequences
- RAII and resource cleanup

## 🎯 Project Goal

This project was created to practice C++ programming, problem-solving, and Linux system interfaces through a small interactive game.

## 📄 License

This project is available under the MIT License.
