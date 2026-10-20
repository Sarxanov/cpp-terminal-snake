#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <poll.h>
#include <random>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

constexpr int WIDTH = 60;
constexpr int HEIGHT = 22;
constexpr int START_SPEED = 140;
constexpr int MIN_SPEED = 65;
constexpr int LEVEL_FOOD = 5;

struct Position {
    int x;
    int y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

enum class Direction { Up, Down, Left, Right };
enum class State { Playing, Paused, GameOver, Quit };

class Terminal {
private:
    termios original{};
    bool configured = false;
    bool alternateScreen = false;

public:
    Terminal() {
        if (!isatty(STDIN_FILENO) ||
            tcgetattr(STDIN_FILENO, &original) == -1) {
            throw runtime_error(
                "Run this game inside a valid terminal."
            );
        }

        termios raw = original;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_iflag &= ~(IXON | ICRNL);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
            throw runtime_error(
                "Could not configure terminal input."
            );
        }

        configured = true;

        cout << "\033[?1049h\033[?25l\033[2J\033[H";
        cout.flush();
        alternateScreen = true;
    }

    ~Terminal() {
        if (configured)
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);

        if (alternateScreen) {
            cout << "\033[0m\033[?25h\033[?1049l";
            cout.flush();
        }
    }

    Terminal(const Terminal&) = delete;
    Terminal& operator=(const Terminal&) = delete;
};

class SnakeGame {
private:
    vector<Position> snake;
    Position food{};

    Direction direction = Direction::Right;
    Direction nextDirection = Direction::Right;
    State state = State::Playing;

    int score = 0;
    int foodsEaten = 0;
    int bestScore = 0;
    int speed = START_SPEED;

    // At most one queued turn per movement tick.
    bool turnQueued = false;

    mt19937 rng{random_device{}()};

    bool isOpposite(Direction a, Direction b) const {
        return
            (a == Direction::Up && b == Direction::Down) ||
            (a == Direction::Down && b == Direction::Up) ||
            (a == Direction::Left && b == Direction::Right) ||
            (a == Direction::Right && b == Direction::Left);
    }

    bool insideBoard(Position p) const {
        return p.x >= 0 && p.x < WIDTH &&
               p.y >= 0 && p.y < HEIGHT;
    }

    bool occupiesSnake(Position p) const {
        for (const auto& segment : snake) {
            if (segment == p)
                return true;
        }

        return false;
    }

    void spawnFood() {
        vector<Position> emptyCells;

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                Position p{x, y};

                if (!occupiesSnake(p))
                    emptyCells.push_back(p);
            }
        }

        if (emptyCells.empty()) {
            state = State::GameOver;
            return;
        }

        uniform_int_distribution<size_t> dist(
            0, emptyCells.size() - 1
        );

        food = emptyCells[dist(rng)];
    }

    void reset() {
        snake.clear();

        snake.push_back({WIDTH / 2, HEIGHT / 2});
        snake.push_back({WIDTH / 2 - 1, HEIGHT / 2});
        snake.push_back({WIDTH / 2 - 2, HEIGHT / 2});

        direction = Direction::Right;
        nextDirection = Direction::Right;

        score = 0;
        foodsEaten = 0;
        speed = START_SPEED;
        turnQueued = false;
        state = State::Playing;

        // Keep the best score for the current program session.
        spawnFood();
    }

    void handleInput(char key) {
        if (key == 'q' || key == 'Q') {
            state = State::Quit;
            return;
        }

        if (state == State::GameOver) {
            if (key == 'r' || key == 'R')
                reset();

            return;
        }

        if (key == 'p' || key == 'P') {
            if (state == State::Playing)
                state = State::Paused;
            else if (state == State::Paused)
                state = State::Playing;

            return;
        }

        if (key == 'r' || key == 'R') {
            reset();
            return;
        }

        if (state != State::Playing || turnQueued)
            return;

        Direction requested = direction;

        switch (key) {
            case 'w':
            case 'W':
                requested = Direction::Up;
                break;

            case 's':
            case 'S':
                requested = Direction::Down;
                break;

            case 'a':
            case 'A':
                requested = Direction::Left;
                break;

            case 'd':
            case 'D':
                requested = Direction::Right;
                break;

            default:
                return;
        }

        if (requested == direction ||
            isOpposite(requested, direction)) {
            return;
        }

        nextDirection = requested;
        turnQueued = true;
    }

    void move() {
        direction = nextDirection;
        turnQueued = false;

        Position head = snake.front();

        switch (direction) {
            case Direction::Up:
                --head.y;
                break;

            case Direction::Down:
                ++head.y;
                break;

            case Direction::Left:
                --head.x;
                break;

            case Direction::Right:
                ++head.x;
                break;
        }

        if (!insideBoard(head)) {
            state = State::GameOver;
            return;
        }

        const bool eating = (head == food);

        // The tail vacates its cell if the snake is not eating.
        for (size_t i = 0; i < snake.size(); ++i) {
            if (!eating && i == snake.size() - 1)
                continue;

            if (snake[i] == head) {
                state = State::GameOver;
                return;
            }
        }

        snake.insert(snake.begin(), head);

        if (eating) {
            score += 10;
            ++foodsEaten;

            bestScore = max(bestScore, score);

            speed = max(
                MIN_SPEED,
                START_SPEED - (foodsEaten / LEVEL_FOOD) * 8
            );

            spawnFood();
        } else {
            snake.pop_back();
        }
    }

    void draw() const {
        // Build the board once per frame instead of scanning
        // the entire snake separately for every cell.
        vector<string> board(
            HEIGHT, string(WIDTH, ' ')
        );

        for (size_t i = 1; i < snake.size(); ++i) {
            const auto& p = snake[i];

            if (insideBoard(p))
                board[p.y][p.x] = 'o';
        }

        if (insideBoard(food))
            board[food.y][food.x] = '*';

        if (!snake.empty() && insideBoard(snake.front())) {
            const auto& head = snake.front();
            board[head.y][head.x] = '@';
        }

        cout << "\033[H";

        cout << "\033[1;36m"
             << "  S N A K E | C++17 TERMINAL EDITION"
             << "\033[0m\n";

        cout << "  Score: " << score
             << "   Best: " << bestScore
             << "   Level: " << (foodsEaten / LEVEL_FOOD) + 1
             << "   Speed: " << speed << "ms\n";

        cout << "  +" << string(WIDTH, '-') << "+\n";

        for (int y = 0; y < HEIGHT; ++y) {
            cout << "  |";

            for (int x = 0; x < WIDTH; ++x) {
                const char cell = board[y][x];

                if (cell == '@')
                    cout << "\033[1;32m@\033[0m";
                else if (cell == 'o')
                    cout << "\033[32mo\033[0m";
                else if (cell == '*')
                    cout << "\033[1;31m*\033[0m";
                else
                    cout << ' ';
            }

            cout << "|\n";
        }

        cout << "  +" << string(WIDTH, '-') << "+\n";
        cout << "  WASD: Move   P: Pause   R: Restart   Q: Quit\n";

        if (state == State::Paused) {
            cout << "\n  \033[1;33mPAUSED - Press P to continue\033[0m\n";
        } else if (state == State::GameOver) {
            cout << "\n  \033[1;31mGAME OVER!\033[0m"
                 << " Score: " << score
                 << " | Press R to retry or Q to quit.\n";
        } else {
            cout << "\n\n";
        }

        cout.flush();
    }

public:
    SnakeGame() {
        reset();
    }

    void run() {
        while (state != State::Quit) {
            draw();

            pollfd fd{};
            fd.fd = STDIN_FILENO;
            fd.events = POLLIN;

            const int timeout =
                (state == State::Playing) ? speed : -1;

            const int result = poll(&fd, 1, timeout);

            if (result < 0) {
                if (errno == EINTR)
                    continue;

                throw runtime_error("Terminal polling failed.");
            }

            if (result == 0) {
                if (state == State::Playing)
                    move();

                continue;
            }

            if (fd.revents & POLLNVAL)
                throw runtime_error("Invalid terminal file descriptor.");

            if (fd.revents & (POLLERR | POLLHUP)) {
                state = State::Quit;
                break;
            }

            if (fd.revents & POLLIN) {
                char key = '\0';

                // Read exactly one byte. Do not loop on blocking read().
                const ssize_t bytesRead = read(
                    STDIN_FILENO, &key, 1
                );

                if (bytesRead == 1) {
                    handleInput(key);
                } else if (bytesRead == 0) {
                    state = State::Quit;
                } else if (errno != EINTR && errno != EAGAIN) {
                    throw runtime_error("Terminal input failed.");
                }
            }
        }
    }
};

int main() {
    try {
        Terminal terminal;
        SnakeGame game;
        game.run();
    } catch (const exception& e) {
        cerr << "\nError: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    cout << "\nThanks for playing Snake!\n";
    return EXIT_SUCCESS;
}
