#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

const size_t kBoardSize = 8;
const char kKingBlack = 'X';
const char kKingWhite = 'O';
const char kCheckerWhite = 'o';
const char kCheckerBlack = 'x';
const char kWhiteSpace = ',';
const char kBlackSpace = '.';


struct Move {
    int x_start;
    int y_start;
    int x_finish;
    int y_finish;

    [[nodiscard]] bool IsLegal() const {
        if (x_start > 7 || x_finish > 7 || y_start > 7 || x_finish > 7 ||
            x_start < 0 || x_finish < 0 || y_start < 0 || y_finish < 0) {
            return false;
        }
        if (std::abs(y_finish - y_start) == std::abs(x_finish - x_start)) {
            return true;
        }
        return false;
    }

    friend std::istream& operator>>(std::istream& io, Move& move);
};

std::istream& operator>>(std::istream& io, Move& move) {
    io >> move.y_start >> move.x_start >> move.y_finish >> move.x_finish;
    move.x_start--;
    move.x_finish--;
    move.y_start--;
    move.y_finish--;
    return io;
}

class Game {
    typedef std::vector<std::vector<char>> Board;

    enum MovesCase {WhiteKing, WhiteKingCuts, WhiteChecker, WhiteCheckerCuts, BlackKing, BlackKingCuts, BlackChecker, BlackCheckerCuts, NotAllowed};

public:
    Game() : board_(kBoardSize, std::vector<char>(kBoardSize)) {
        InitBoard();
    }

    void Start() {
        GameRoutine();
    }


private:

    static Move GetBlackMoveBasic() {
        Move move{};
        move.x_start = rand() % 8;
        move.x_finish = move.x_start + rand() % 5 - 2;
        move.y_start = rand() % 8;
        move.y_finish = move.y_start - std::abs(move.x_finish - move.x_start);
        return move;
    }

    void BlackMove() {
        Move move = GetBlackMoveBasic();
        MovesCase move_case;
        while ((move_case = CheckIfBlackAllowed(move)) == NotAllowed) {
            move = GetBlackMoveBasic();
        }
        if (MakeBlackMove(move, move_case)) {
            BlackMove();
        }
    }

    void WhiteMove() {
        Move move{};
        std::cin >> move;
        MovesCase move_case;
        while ((move_case = CheckIfWhiteAllowed(move)) == NotAllowed) {
            std::cout << "Move is illegal! Make another.\n";
            std::cin >> move;
        }
        if (MakeWhiteMove(move, move_case)) {
            WhiteMove();
        }
    }


    void GameRoutine() {
        while (CheckScore()) {
            round_number_++;
            if (round_number_ % 2 == 0) {
                BlackMove();
            } else {
                PrintBoard();
                WhiteMove();
            }
        }
        Finilize();
    }

    [[nodiscard]] bool CheckScore() const {
        if (white_score_ < 12 && black_score_ < 12) {
            return true;
        }
        return false;
    }

    void InitBoard() {
        for (size_t i = 0; i < board_.size(); ++i) {
            for (size_t j = 0; j < board_.size(); ++j) {
                if ((i + j) % 2 == 0) {
                    board_[i][j] = kWhiteSpace;
                } else {
                    board_[i][j] = kBlackSpace;
                }
            }
        }
        for (int y = 0; y < 3; ++y)
            for (int x = (y + 1) % 2; x < 8; x += 2)
                board_[y][x] = kCheckerBlack;
        for (int y = 5; y < 8; ++y)
            for (int x = (y + 1) % 2; x < 8; x += 2)
                board_[y][x] = kCheckerWhite;
    }

    [[nodiscard]] MovesCase CheckIfWhiteAllowed(const Move& move) const {
        if (!move.IsLegal()) {
            return NotAllowed;
        }
        if (board_[move.x_start][move.y_start] == kCheckerWhite) {
            if (move.x_start - move.x_finish == 1 && board_[move.x_finish][move.y_finish] == kBlackSpace) {
                return WhiteChecker;
            } else if (move.x_start - move.x_finish == 2 &&
                       board_[move.x_finish][move.y_finish] == kBlackSpace &&
                    (board_[move.x_finish - 1][(move.y_finish + move.y_start) / 2] == kCheckerBlack ||
                            board_[move.x_finish - 1][(move.y_finish + move.y_start) / 2] == kKingBlack)) {
                return WhiteCheckerCuts;
            }
        } else if (board_[move.x_start][move.y_start] == kKingWhite) {
            return CheckIfKingAllowed(move, kCheckerBlack);
        }
        return NotAllowed;
    }

    bool MakeWhiteMove(const Move& move, MovesCase move_case) {
        switch (move_case) {
            case WhiteChecker : {
               board_[move.x_start][move.y_start] = kBlackSpace;
               if (move.x_finish != 7) {
                   board_[move.x_finish][move.y_finish] = kCheckerWhite;
               } else {
                   board_[move.x_finish][move.y_finish] = kKingWhite;
               }
               return false;
            }
            case WhiteCheckerCuts : {
                board_[move.x_start][move.y_start] = kBlackSpace;
                board_[(move.x_start + move.x_finish) / 2][move.y_finish - 1] = kBlackSpace;
                if (move.x_finish != 7) {
                    board_[move.x_finish][move.y_finish] = kCheckerWhite;
                } else {
                    board_[move.x_finish][move.y_finish] = kKingWhite;
                }
                white_score_++;
                return CheckIfWhiteCanCut(move.x_finish, move.y_finish);
            }
            case WhiteKingCuts : {
                white_score_++;
                board_[move.x_start][move.y_start] = kBlackSpace;
                board_[move.x_finish][move.y_finish ] = kKingWhite;
                IterateKing(move, kCheckerWhite, kCheckerBlack);
            }
            case WhiteKing : {
                board_[move.x_start][move.y_start] = kBlackSpace;
                board_[move.x_finish][move.y_finish ] = kKingWhite;
                return true;
            }
        }
        return false;
    }

    std::pair<bool, MovesCase> IterateKing(const Move& move, char checker_type, char checker_opp_type) {
        if (board_[move.x_finish][move.y_finish] != checker_type) {
            return {false, NotAllowed};
        }
        if (move.x_start < move.x_finish) {
            if (move.y_start < move.y_finish) {
                for (int i = 1; i < move.x_finish - move.x_start - 1; ++i) {
                    if (board_[move.x_start + i][move.x_finish + i] == checker_opp_type) {

                    }
                }
            }
        }
        return {true, BlackKing};
    }

    bool MakeBlackMove(const Move& move, MovesCase move_case) {
        switch (move_case) {
            case BlackChecker : {
                board_[move.x_start][move.y_start] = kBlackSpace;
                if (move.x_finish != 0) {
                    board_[move.x_finish][move.y_finish] = kCheckerBlack;
                } else {
                    board_[move.x_finish][move.y_finish] = kKingBlack;
                }
                return false;
            }
            case BlackCheckerCuts : {
                board_[move.x_start][move.y_start] = kBlackSpace;
                board_[(move.x_start + move.x_finish) / 2][move.y_finish + 1] = kBlackSpace;
                if (move.x_finish != 0) {
                    board_[move.x_finish][move.y_finish] = kCheckerBlack;
                } else {
                    board_[move.x_finish][move.y_finish] = kKingBlack;
                }
                black_score_++;
                return CheckIfBlackCanCut(move.x_finish, move.y_finish);
            }
            case WhiteKing : {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool CheckIfWhiteCanCut(int x, int y) const {
        if (x > 2) {
            if (y < 6) {
                if (board_[x + 1][y - 1] == kCheckerBlack && board_[x + 2][y - 2] == kBlackSpace) {
                    return true;
                }
            }
            if (y > 2) {
                if (board_[x - 1][y - 1] == kCheckerBlack && board_[x - 2][y - 2] == kBlackSpace) {
                    return true;
                }
            }
        }
        return false;
    }

    [[nodiscard]] bool CheckIfBlackCanCut(int x, int y) const {
        if (x < 6) {
            if (y < 6) {
                if (board_[x + 1][y + 1] == kCheckerWhite && board_[x + 2][y + 2] == kBlackSpace) {
                    return true;
                }
            }
            if (y > 2) {
                if (board_[x - 1][y + 1] == kCheckerWhite && board_[x - 2][y + 2] == kBlackSpace) {
                    return true;
                }
            }
        }
        return false;
    }

    void PrintBoard() {
        int counter = 1;
        for (auto & i : board_) {
            std::cout << counter << ' ';
            counter++;
            for (auto& j : i) {
                std::cout << j << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << "  ";
        for (size_t i = 0; i < kBoardSize; ++i) {
            std::cout << i + 1 << ' ';
        }
        std::cout << std::endl;
    }

    [[nodiscard]] MovesCase CheckIfBlackAllowed(const Move& move) const {
        if (!move.IsLegal()) {
            return NotAllowed;
        }
        if (board_[move.x_start][move.y_start] == kCheckerBlack) {
            if (move.x_finish - move.x_start == 1 && board_[move.x_finish][move.y_finish] == kBlackSpace) {
                return BlackChecker;
            } else if (move.x_finish - move.x_start == 2 &&
                       board_[move.x_finish][move.y_finish] == kBlackSpace &&
                    (board_[(move.x_finish + move.x_start) / 2][move.y_finish - 1] == kCheckerWhite ||
                            board_[(move.x_finish + move.x_start) / 2][move.y_finish - 1] == kKingWhite)) {
                return BlackCheckerCuts;
            }
        } else if (board_[move.x_start][move.y_start] == kKingBlack) {
            return CheckIfKingAllowed(move, kCheckerWhite);
        }
        return NotAllowed;
    }

    [[nodiscard]] MovesCase CheckIfKingAllowed(const Move& move, char checker_type) const {

    }

    bool CheckIfKingCanCut(int x, int y, char checker_color) const {
        int x1 = x + 1;
        int y1 = y + 1;
        while (x1 < 6 && y1 < 6) {
            if (board_[x1][y1] == checker_color && board_[x1 + 1][y1 + 1] == kBlackSpace) {
                return true;
            }
            if (board_[x1][y1] == checker_color && board_[x1 + 1][y1 + 1] != kBlackSpace) {
                break;
            }
            x1++;
            y1++;
        }
        x1 = x + 1;
        y1 = y - 1;
        while (x1 < 6 && y1 > 1) {
            if (board_[x1][y1] == checker_color && board_[x1 + 1][y1 - 1] == kBlackSpace) {
                return true;
            }
            if (board_[x1][y1] == checker_color && board_[x1 + 1][y1 - 1] != kBlackSpace) {
                break;
            }
            x1++;
            y1--;
        }
        x1 = x - 1;
        y1 = y - 1;
        while (x1 > 1 && y1 > 1) {
            if (board_[x1][y1] == checker_color && board_[x1 - 1][y1 - 1] == kBlackSpace) {
                return true;
            }
            if (board_[x1][y1] == checker_color && board_[x1 - 1][y1 - 1] != kBlackSpace) {
                break;
            }
            x1--;
            y1--;
        }
        x1 = x - 1;
        y1 = y + 1;
        while (x1 > 1 && y1 > 1) {
            if (board_[x1][y1] == checker_color && board_[x1 - 1][y1 + 1] == kBlackSpace) {
                return true;
            }
            if (board_[x1][y1] == checker_color && board_[x1 - 1][y1 + 1] != kBlackSpace) {
                break;
            }
            x1--;
            y1++;
        }
        return false;
    }

    void Finilize() {
        if (black_score_ == 12) {
            std::cout << "BLACK WIN!\n";
        } else {
            std::cout << "WHITE WIN!\n";
        }

        std::cout << "The game lasted " << round_number_ << " rounds!\n";
    }

private:
    Board board_;
    size_t round_number_ = 0;
    size_t black_score_ = 0;
    size_t white_score_ = 0;
};



int main() {

    Game game;
    game.Start();

    return 0;
}
