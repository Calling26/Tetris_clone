// =============================================================================
// Tetris Clone — Raylib + Modern C++17
// =============================================================================
// A complete, playable Tetris game with:
//   • 10×20 play grid
//   • All 7 standard tetrominoes (I, O, T, S, Z, J, L) with distinct colours
//   • Movement, wall‑kick rotation, gravity, hard drop, line clear, scoring
//   • Game‑over detection & restart
// =============================================================================

#include "raylib.h"
#include <array>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int GRID_COLS    = 10;
static constexpr int GRID_ROWS    = 20;
static constexpr int CELL_SIZE    = 30;
static constexpr int GRID_OFFSET_X = 11;   // pixels from left edge of window
static constexpr int GRID_OFFSET_Y = 11;   // pixels from top edge of window

static constexpr int SIDEBAR_X    = GRID_OFFSET_X + GRID_COLS * CELL_SIZE + 20;
static constexpr int SCREEN_W     = SIDEBAR_X + 170;
static constexpr int SCREEN_H     = GRID_OFFSET_Y + GRID_ROWS * CELL_SIZE + 11;

// Colour palette (index 0 = empty)
static const Color CELL_COLORS[] = {
    {  26,  31,  40, 255 },   // 0  dark background (empty cell)
    {   0, 232, 216, 255 },   // 1  cyan        — I
    { 226, 196,   0, 255 },   // 2  yellow      — O
    { 166,   0, 247, 255 },   // 3  purple      — T
    {  47, 230,  23, 255 },   // 4  green       — S
    { 232,  18,  18, 255 },   // 5  red         — Z
    {  13,  64, 216, 255 },   // 6  blue        — J
    { 226, 116,  17, 255 },   // 7  orange      — L
};

// ---------------------------------------------------------------------------
// Tetromino data — each piece has 4 rotation states stored as (row,col) offsets
// ---------------------------------------------------------------------------
struct Position { int row; int col; };

using RotationState = std::vector<Position>;

struct TetrominoData {
    int id;                                   // colour id (1‑7)
    std::array<RotationState, 4> rotations;   // 4 orientations
};

// Helper to build data for all 7 pieces
static std::vector<TetrominoData> BuildTetrominoes()
{
    std::vector<TetrominoData> pieces(7);

    // I‑piece (id 1)
    pieces[0].id = 1;
    pieces[0].rotations[0] = { {1,0},{1,1},{1,2},{1,3} };
    pieces[0].rotations[1] = { {0,2},{1,2},{2,2},{3,2} };
    pieces[0].rotations[2] = { {2,0},{2,1},{2,2},{2,3} };
    pieces[0].rotations[3] = { {0,1},{1,1},{2,1},{3,1} };

    // O‑piece (id 2)
    pieces[1].id = 2;
    for (auto& r : pieces[1].rotations)
        r = { {0,0},{0,1},{1,0},{1,1} };

    // T‑piece (id 3)
    pieces[2].id = 3;
    pieces[2].rotations[0] = { {0,1},{1,0},{1,1},{1,2} };
    pieces[2].rotations[1] = { {0,1},{1,1},{1,2},{2,1} };
    pieces[2].rotations[2] = { {1,0},{1,1},{1,2},{2,1} };
    pieces[2].rotations[3] = { {0,1},{1,0},{1,1},{2,1} };

    // S‑piece (id 4)
    pieces[3].id = 4;
    pieces[3].rotations[0] = { {0,1},{0,2},{1,0},{1,1} };
    pieces[3].rotations[1] = { {0,0},{1,0},{1,1},{2,1} };
    pieces[3].rotations[2] = { {1,1},{1,2},{2,0},{2,1} };
    pieces[3].rotations[3] = { {0,0},{1,0},{1,1},{2,1} };

    // Z‑piece (id 5)
    pieces[4].id = 5;
    pieces[4].rotations[0] = { {0,0},{0,1},{1,1},{1,2} };
    pieces[4].rotations[1] = { {0,2},{1,1},{1,2},{2,1} };
    pieces[4].rotations[2] = { {1,0},{1,1},{2,1},{2,2} };
    pieces[4].rotations[3] = { {0,1},{1,0},{1,1},{2,0} };

    // J‑piece (id 6)
    pieces[5].id = 6;
    pieces[5].rotations[0] = { {0,0},{1,0},{1,1},{1,2} };
    pieces[5].rotations[1] = { {0,1},{0,2},{1,1},{2,1} };
    pieces[5].rotations[2] = { {1,0},{1,1},{1,2},{2,2} };
    pieces[5].rotations[3] = { {0,1},{1,1},{2,0},{2,1} };

    // L‑piece (id 7)
    pieces[6].id = 7;
    pieces[6].rotations[0] = { {0,2},{1,0},{1,1},{1,2} };
    pieces[6].rotations[1] = { {0,1},{1,1},{2,1},{2,2} };
    pieces[6].rotations[2] = { {1,0},{1,1},{1,2},{2,0} };
    pieces[6].rotations[3] = { {0,0},{0,1},{1,1},{2,1} };

    return pieces;
}

// ---------------------------------------------------------------------------
// Grid — manages the 10×20 locked‑cell array
// ---------------------------------------------------------------------------
class Grid {
public:
    Grid()  { Clear(); }

    void Clear()
    {
        for (auto& row : cells)
            row.fill(0);
    }

    // Returns true if (r,c) is inside the grid boundaries
    bool IsInside(int r, int c) const
    {
        return r >= 0 && r < GRID_ROWS && c >= 0 && c < GRID_COLS;
    }

    // Returns true if cell is empty (0) AND inside the grid
    bool IsCellEmpty(int r, int c) const
    {
        return IsInside(r, c) && cells[r][c] == 0;
    }

    // Lock a colour id into a specific cell
    void SetCell(int r, int c, int value)
    {
        if (IsInside(r, c))
            cells[r][c] = value;
    }

    int GetCell(int r, int c) const
    {
        if (IsInside(r, c)) return cells[r][c];
        return -1;
    }

    // Clear all completed rows, return count cleared
    int ClearFullRows()
    {
        int cleared = 0;
        // Scan from bottom to top
        for (int r = GRID_ROWS - 1; r >= 0; --r) {
            if (IsRowFull(r)) {
                ClearRow(r);
                MoveRowsDown(r);
                ++cleared;
                ++r;  // re‑check this index (rows shifted down)
            }
        }
        return cleared;
    }

    // Draw all locked cells
    void Draw() const
    {
        for (int r = 0; r < GRID_ROWS; ++r)
            for (int c = 0; c < GRID_COLS; ++c)
                DrawCell(r, c, cells[r][c]);
    }

private:
    std::array<std::array<int, GRID_COLS>, GRID_ROWS> cells{};

    bool IsRowFull(int r) const
    {
        for (int c = 0; c < GRID_COLS; ++c)
            if (cells[r][c] == 0) return false;
        return true;
    }

    void ClearRow(int r)
    {
        cells[r].fill(0);
    }

    void MoveRowsDown(int emptyRow)
    {
        for (int r = emptyRow; r > 0; --r)
            cells[r] = cells[r - 1];
        cells[0].fill(0);
    }

    static void DrawCell(int r, int c, int value)
    {
        int x = GRID_OFFSET_X + c * CELL_SIZE;
        int y = GRID_OFFSET_Y + r * CELL_SIZE;
        DrawRectangle(x, y, CELL_SIZE - 1, CELL_SIZE - 1, CELL_COLORS[value]);
    }
};

// ---------------------------------------------------------------------------
// Piece — a live tetromino on the grid
// ---------------------------------------------------------------------------
class Piece {
public:
    Piece() = default;
    Piece(const TetrominoData* data, int startCol)
        : data_(data), rotation_(0), rowOffset_(0), colOffset_(startCol) {}

    int Id()       const { return data_->id; }
    int Rotation() const { return rotation_; }

    // Current world‑space cells
    std::vector<Position> GetCells() const
    {
        std::vector<Position> out;
        for (auto& p : data_->rotations[rotation_])
            out.push_back({ p.row + rowOffset_, p.col + colOffset_ });
        return out;
    }

    // Cells after a hypothetical rotation
    std::vector<Position> GetCellsRotated(int newRot) const
    {
        std::vector<Position> out;
        for (auto& p : data_->rotations[newRot])
            out.push_back({ p.row + rowOffset_, p.col + colOffset_ });
        return out;
    }

    void MoveDown()  { ++rowOffset_; }
    void MoveLeft()  { --colOffset_; }
    void MoveRight() { ++colOffset_; }
    void MoveUp()    { --rowOffset_; }

    void Rotate()
    {
        rotation_ = (rotation_ + 1) % 4;
    }

    void UndoRotate()
    {
        rotation_ = (rotation_ + 3) % 4;   // +3 mod 4 == −1 mod 4
    }

    void Draw() const
    {
        for (auto& p : GetCells()) {
            int x = GRID_OFFSET_X + p.col * CELL_SIZE;
            int y = GRID_OFFSET_Y + p.row * CELL_SIZE;
            DrawRectangle(x, y, CELL_SIZE - 1, CELL_SIZE - 1, CELL_COLORS[data_->id]);
        }
    }

    // Draw inside the "Next" preview box
    void DrawPreview(int px, int py) const
    {
        const int previewCell = 25;
        for (auto& p : data_->rotations[0]) {
            int x = px + p.col * previewCell;
            int y = py + p.row * previewCell;
            DrawRectangle(x, y, previewCell - 1, previewCell - 1, CELL_COLORS[data_->id]);
        }
    }

private:
    const TetrominoData* data_ = nullptr;
    int rotation_   = 0;
    int rowOffset_   = 0;
    int colOffset_   = 0;
};

// ---------------------------------------------------------------------------
// Game — orchestrates everything
// ---------------------------------------------------------------------------
class Game {
public:
    Game()
    {
        tetrominoes_ = BuildTetrominoes();
        Reset();
    }

    void Reset()
    {
        grid_.Clear();
        score_     = 0;
        gameOver_  = false;
        dropTimer_ = 0.0;
        dropInterval_ = 0.5;   // seconds between automatic drops
        FillBag();
        SpawnPiece();
        SpawnNextPiece();
    }

    // ------ Input / Update ------

    void HandleInput()
    {
        if (gameOver_) {
            if (IsKeyPressed(KEY_ENTER)) Reset();
            return;
        }

        if (IsKeyPressed(KEY_LEFT)  || IsKeyPressedRepeat(KEY_LEFT))  TryMoveLeft();
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) TryMoveRight();
        if (IsKeyPressed(KEY_DOWN)  || IsKeyPressedRepeat(KEY_DOWN))  TryMoveDown();
        if (IsKeyPressed(KEY_UP))    TryRotate();
        if (IsKeyPressed(KEY_SPACE)) HardDrop();
    }

    void Update()
    {
        if (gameOver_) return;

        dropTimer_ += GetFrameTime();
        if (dropTimer_ >= dropInterval_) {
            dropTimer_ = 0.0;
            if (!TryMoveDown())
                LockAndAdvance();
        }
    }

    // ------ Drawing ------

    void Draw() const
    {
        // Background panel behind grid
        DrawRectangle(GRID_OFFSET_X - 1, GRID_OFFSET_Y - 1,
                      GRID_COLS * CELL_SIZE + 2, GRID_ROWS * CELL_SIZE + 2,
                      {  0, 150, 136, 80 });

        // Draw ghost piece (landing preview)
        DrawGhost();

        grid_.Draw();
        currentPiece_.Draw();

        // --- Sidebar ---
        int sy = GRID_OFFSET_Y;

        DrawText("SCORE", SIDEBAR_X, sy, 22, WHITE);
        sy += 30;
        DrawRectangleRounded({ (float)SIDEBAR_X, (float)sy, 150, 50 }, 0.3f, 6,
                             { 59, 85, 96, 255 });
        DrawText(std::to_string(score_).c_str(),
                 SIDEBAR_X + 10, sy + 13, 24, WHITE);
        sy += 70;

        DrawText("NEXT", SIDEBAR_X, sy, 22, WHITE);
        sy += 30;
        DrawRectangleRounded({ (float)SIDEBAR_X, (float)sy, 150, 120 }, 0.3f, 6,
                             { 59, 85, 96, 255 });
        nextPiece_.DrawPreview(SIDEBAR_X + 20, sy + 15);
        sy += 140;

        DrawText("CONTROLS", SIDEBAR_X, sy, 18, LIGHTGRAY);
        sy += 26;
        DrawText("<- ->  Move",   SIDEBAR_X, sy, 16, GRAY); sy += 20;
        DrawText("Up     Rotate", SIDEBAR_X, sy, 16, GRAY); sy += 20;
        DrawText("Down   Soft",   SIDEBAR_X, sy, 16, GRAY); sy += 20;
        DrawText("Space  Drop",   SIDEBAR_X, sy, 16, GRAY); sy += 20;

        // Game‑over overlay
        if (gameOver_) {
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, { 0, 0, 0, 180 });
            const char* go = "GAME OVER";
            int goW = MeasureText(go, 40);
            DrawText(go, (SCREEN_W - goW) / 2, SCREEN_H / 2 - 40, 40,
                     { 232, 18, 18, 255 });
            const char* pr = "Press ENTER to restart";
            int prW = MeasureText(pr, 20);
            DrawText(pr, (SCREEN_W - prW) / 2, SCREEN_H / 2 + 20, 20, WHITE);
        }
    }

private:
    Grid  grid_;
    Piece currentPiece_;
    Piece nextPiece_;

    std::vector<TetrominoData> tetrominoes_;
    std::vector<int> bag_;    // random bag of piece indices

    int    score_        = 0;
    bool   gameOver_     = false;
    double dropTimer_    = 0.0;
    double dropInterval_ = 0.5;

    // --- Random‑bag piece generation (7‑bag system) ---

    void FillBag()
    {
        bag_.clear();
        for (int i = 0; i < 7; ++i)
            bag_.push_back(i);
        // Fisher–Yates shuffle
        for (int i = 6; i > 0; --i) {
            int j = std::rand() % (i + 1);
            std::swap(bag_[i], bag_[j]);
        }
    }

    int PickFromBag()
    {
        if (bag_.empty()) FillBag();
        int idx = bag_.back();
        bag_.pop_back();
        return idx;
    }

    void SpawnPiece()
    {
        int idx = PickFromBag();
        currentPiece_ = Piece(&tetrominoes_[idx], 3);
    }

    void SpawnNextPiece()
    {
        int idx = PickFromBag();
        nextPiece_ = Piece(&tetrominoes_[idx], 3);
    }

    // --- Collision helper ---

    bool PieceFitsOnGrid(const std::vector<Position>& cells) const
    {
        for (auto& p : cells) {
            if (p.col < 0 || p.col >= GRID_COLS) return false;
            if (p.row >= GRID_ROWS)               return false;
            // Allow cells above the visible grid (row < 0)
            if (p.row >= 0 && grid_.GetCell(p.row, p.col) != 0) return false;
        }
        return true;
    }

    // --- Movement ---

    bool TryMoveDown()
    {
        currentPiece_.MoveDown();
        if (!PieceFitsOnGrid(currentPiece_.GetCells())) {
            currentPiece_.MoveUp();
            return false;
        }
        return true;
    }

    void TryMoveLeft()
    {
        currentPiece_.MoveLeft();
        if (!PieceFitsOnGrid(currentPiece_.GetCells()))
            currentPiece_.MoveRight();   // undo
    }

    void TryMoveRight()
    {
        currentPiece_.MoveRight();
        if (!PieceFitsOnGrid(currentPiece_.GetCells()))
            currentPiece_.MoveLeft();    // undo
    }

    void TryRotate()
    {
        currentPiece_.Rotate();
        auto cells = currentPiece_.GetCells();
        if (PieceFitsOnGrid(cells))
            return;   // rotation succeeded

        // Simple wall‑kick: try shifting left, right, then give up
        currentPiece_.MoveLeft();
        if (PieceFitsOnGrid(currentPiece_.GetCells())) return;
        currentPiece_.MoveRight();   // undo left

        currentPiece_.MoveRight();
        if (PieceFitsOnGrid(currentPiece_.GetCells())) return;
        currentPiece_.MoveLeft();    // undo right

        // For I‑piece try ±2
        currentPiece_.MoveLeft(); currentPiece_.MoveLeft();
        if (PieceFitsOnGrid(currentPiece_.GetCells())) return;
        currentPiece_.MoveRight(); currentPiece_.MoveRight();

        currentPiece_.MoveRight(); currentPiece_.MoveRight();
        if (PieceFitsOnGrid(currentPiece_.GetCells())) return;
        currentPiece_.MoveLeft(); currentPiece_.MoveLeft();

        currentPiece_.UndoRotate();   // nothing worked
    }

    void HardDrop()
    {
        while (TryMoveDown()) {}
        LockAndAdvance();
    }

    // --- Locking & line clear ---

    void LockAndAdvance()
    {
        // Write current piece into the grid
        for (auto& p : currentPiece_.GetCells())
            grid_.SetCell(p.row, p.col, currentPiece_.Id());

        // Clear lines & score
        int cleared = grid_.ClearFullRows();
        switch (cleared) {
            case 1: score_ += 100;  break;
            case 2: score_ += 300;  break;
            case 3: score_ += 500;  break;
            case 4: score_ += 800;  break;
            default: break;
        }

        // Speed up slightly every 500 points
        dropInterval_ = std::max(0.08, 0.5 - (score_ / 500) * 0.03 );

        // Advance to next piece
        currentPiece_ = nextPiece_;
        // Reset column offset for the new current piece
        currentPiece_ = Piece(&tetrominoes_[nextPiece_.Id() - 1], 3);

        SpawnNextPiece();

        // If the new piece immediately collides → game over
        if (!PieceFitsOnGrid(currentPiece_.GetCells()))
            gameOver_ = true;
    }

    // --- Ghost piece (landing preview) ---

    void DrawGhost() const
    {
        Piece ghost = currentPiece_;
        // Drop ghost until it can't go further
        while (true) {
            ghost.MoveDown();
            if (!PieceFitsOnGrid(ghost.GetCells())) {
                ghost.MoveUp();
                break;
            }
        }
        for (auto& p : ghost.GetCells()) {
            int x = GRID_OFFSET_X + p.col * CELL_SIZE;
            int y = GRID_OFFSET_Y + p.row * CELL_SIZE;
            DrawRectangle(x, y, CELL_SIZE - 1, CELL_SIZE - 1,
                          Fade(CELL_COLORS[currentPiece_.Id()], 0.30f));
        }
    }
};

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main()
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    InitWindow(SCREEN_W, SCREEN_H, "Tetris Clone");
    SetTargetFPS(60);

    Game game;

    while (!WindowShouldClose()) {
        // --- Update ---
        game.HandleInput();
        game.Update();

        // --- Draw ---
        BeginDrawing();
        ClearBackground({ 44, 44, 127, 255 });
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
