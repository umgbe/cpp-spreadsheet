#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include <map>


class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


private:

    struct PositionHasher {
        size_t operator()(const Position& p) const {
            return p.col + p.row * 37;
        }
    };

    friend class Cell;

    std::unordered_map<Position, Cell, PositionHasher> cells_;
    std::unordered_map<const Cell*, Position> cells_to_positions_;
    std::unordered_map<const Cell*, std::optional<double>> cache_;

};