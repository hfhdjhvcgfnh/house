#include "horse.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

// Structure for Warnsdorff sorting, used internally by the traversal function.
struct Move {
    int x;
    int y;
    int degree;
};

// --- Static Member Definitions from horse.h ---

int horse::begin_x = 0;
int horse::begin_y = 0;
bool horse::closedTour = true;
int horse::dir_x[size_x] = {1, 2, 2, 1, -1, -2, -2, -1};
int horse::dir_y[size_y] = {2, 1, -1, -2, -2, -1, 1, 2};
int horse::j_c_Warnsdorff_sortarray[size_x] = {0, 1, 2, 3, 4, 5, 6, 7};
int horse::grid[size_x][size_y] = {{0}};

// Global pointer required by horse.h, but no longer used by the fixed algorithm.
const int *ptr = nullptr;

// --- Function Implementations ---

void horse::initBoard() {
    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            grid[i][j] = 0;
        }
    }
}

void horse::setClosedTour(bool closed) {
    horse::closedTour = closed;
}

void horse::init(int x, int y) {
    initBoard();
    if (x <= 0 || x > size_x || y <= 0 || y > size_y) return;
    begin_x = x - 1;
    begin_y = y - 1;
    grid[begin_x][begin_y] = 1;
}

// Checks if a square is on the board and has not been visited yet.
int horse::in_grid(int x, int y) {
    if (x >= 0 && x < size_x && y >= 0 && y < size_y && grid[x][y] == 0) {
        return 1;
    }
    return 0;
}

// Helper function to calculate the "degree" of a square for the Warnsdorff rule.
// This is not part of the class but is used by the traversal function.
static int get_degree(int x, int y) {
    int count = 0;
    for (int i = 0; i < able_step; ++i) {
        if (horse::in_grid(x + horse::dir_x[i], y + horse::dir_y[i])) {
            count++;
        }
    }
    return count;
}

// Main solver function that initiates the tour.
bool horse::solve(int startX, int startY) {
    initBoard();
    if (startX < 0 || startX >= size_x || startY < 0 || startY >= size_y) {
        return false;
    }
    begin_x = startX;
    begin_y = startY;
    grid[startX][startY] = 1;
    return hores_traversal(startX, startY, 2) == 1;
}

// The core recursive traversal function with the corrected Warnsdorff implementation.
int horse::hores_traversal(int x, int y, int deep) {
    if (deep > Total_step) {
        if (!closedTour) return 1;
        int dx = std::abs(x - begin_x);
        int dy = std::abs(y - begin_y);
        return ((dx == 1 && dy == 2) || (dx == 2 && dy == 1)) ? 1 : 0;
    }

    std::vector<Move> moves;
    moves.clear();
    for (int i = 0; i < able_step; ++i) {
        int next_x = x + dir_x[i];
        int next_y = y + dir_y[i];
        if (in_grid(next_x, next_y)) {
            moves.push_back({next_x, next_y, get_degree(next_x, next_y)});
        }
    }

    std::sort(moves.begin(), moves.end(), [](const Move& a, const Move& b) {
        if (a.degree != b.degree) return a.degree < b.degree;
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });

    for (const auto& move : moves) {
        grid[move.x][move.y] = deep;
        if (hores_traversal(move.x, move.y, deep + 1) == 1) {
            return 1;
        }
        grid[move.x][move.y] = 0; // Backtrack
    }

    return 0;
}

// --- Dummy implementations for unused functions from horse.h ---

void horse::print_result() {
    std::printf("\n");
    for (int i = 0; i < size_y; i++) {
        for (int j = 0; j < size_x; j++) {
            std::printf("%3d", grid[j][i]);
        }
        std::printf("\n");
    }
}

// These functions are part of the old, flawed implementation.
// They are no longer called by the corrected hores_traversal,
// but they must exist to satisfy the linker.
int horse::compare(const void*, const void*) { return 0; }
void horse::every_init(int) {}
void horse::sort_index(const int[], int[], int) {}
void horse::sort_j_c_Warnsdorff(int, int) {}
