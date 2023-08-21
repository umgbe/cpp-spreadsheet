#include "sheet.h"

#include "cell.h"
#include "common.h"
#include "formula.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>

using namespace std::literals;

Sheet::~Sheet() = default;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (!cells_.count(pos)) {
        cells_.insert({pos, Cell(*this)});            
        cells_to_positions_[&cells_.at(pos)] = pos;
    }
    cells_.at(pos).Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (!cells_.count(pos) || cells_.at(pos).IsCleared()) {
        return nullptr;
    } else {
        return &cells_.at(pos);
    }
}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (!cells_.count(pos) || cells_.at(pos).IsCleared()) {
        return nullptr;
    } else {
        return &cells_.at(pos);
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }
    if (cells_.count(pos)) {
        cells_.at(pos).Clear();
    }
}

Size Sheet::GetPrintableSize() const {
    Size result;
    for (const auto& [pos, cell] : cells_) {
        if (!cell.IsEmpty() && !cell.IsCleared()) {            
            if ((pos.col + 1) > result.cols) {
                result.cols = pos.col + 1;
            }
            if ((pos.row + 1) > result.rows) {
                result.rows = pos.row + 1;
            }
        }
    }
    return result;
}

void Sheet::PrintValues(std::ostream& output) const {

    Size boundaries = GetPrintableSize();
    for (int row = 0; row < boundaries.rows; ++row) {
        for (int col = 0; col < boundaries.cols; ++col) {
            if (cells_.count({row, col})) {
                CellInterface::Value value = cells_.at({row, col}).GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                } else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                } else {
                    output << std::get<FormulaError>(value);
                }
            }
            if ((boundaries.cols - col) == 1) {
                output << '\n';
            } else {
                output << '\t';
            }
        }
    }
    
}
void Sheet::PrintTexts(std::ostream& output) const {
    
    Size boundaries = GetPrintableSize();
    for (int row = 0; row < boundaries.rows; ++row) {
        for (int col = 0; col < boundaries.cols; ++col) {
            if (cells_.count({row, col})) {
                output << cells_.at({row, col}).GetText();                
            }
            if ((boundaries.cols - col) == 1) {
                output << '\n';
            } else {
                output << '\t';
            }
        }
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}