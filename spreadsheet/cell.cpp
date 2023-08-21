#include "cell.h"
#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <optional>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

Cell::Cell(Sheet& sheet) 
    :sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if ((text[0] == '=') && (text.size() > 1)) {
        /* Введена формула */ 

        /*  Если формула некорректная, бросится исключение */
        std::unique_ptr<FormulaInterface> new_formula;
        try {    
            new_formula = ParseFormula(text.substr(1, text.size() - 1));
        } catch (...) {
            throw FormulaException("");
        }

        /*  Создаем копию оригинальной ячейки и индекс изменений */
        Cell backup_copy(*this);
        std::unordered_set<Position, PositionHasher> upper_cells_added_;
        std::unordered_set<Position, PositionHasher> upper_cells_removed_;

        /*  Избавляемся от старых зависимостей, запоминаем изменения */
        for (Position p : lower_cells_) {
            sheet_.cells_.at(p).RemoveUpperNode(sheet_.cells_to_positions_.at(this));
            upper_cells_removed_.insert(p);
        }
        lower_cells_.clear();

        /*  Создаем новые зависимости, запоминаем изменения */
        std::vector<Position> new_dependencies = new_formula->GetReferencedCells();
        for (Position p : new_dependencies) {
            AddLowerNode(p);
            if (!sheet_.cells_.count(p)) {
                sheet_.SetCell(p, "");
            }
            sheet_.cells_.at(p).AddUpperNode(sheet_.cells_to_positions_.at(this));
            upper_cells_added_.insert(p);
        }

        formula_.swap(new_formula);
        type_ = CellType::FORMULA;

        /*  Проверяем новую конфигурацию на циклические зависимости*/
        std::optional<const Cell*> cyclic_chiecking_result = CheckNode();

        if (cyclic_chiecking_result.has_value()) {
            /*  Восстанавливаем всё как было */
            for (Position p : upper_cells_added_) {
                sheet_.cells_.at(p).RemoveUpperNode(sheet_.cells_to_positions_.at(this));
            }
            for (Position p : upper_cells_removed_) {
                sheet_.cells_.at(p).AddUpperNode(sheet_.cells_to_positions_.at(this));
            }
            Swap(backup_copy);

            throw CircularDependencyException("");

        } else {
            text_.clear();
            InvalidateCache();
        }
    } else {
        /*  Введено число или текст */
        if (type_ == CellType::FORMULA) {
            for (Position p : lower_cells_) {
                sheet_.cells_.at(p).RemoveUpperNode(sheet_.cells_to_positions_.at(this));
            }
            lower_cells_.clear();
        }
        if (text.empty()) {
            type_ = CellType::EMPTY;
        } else {
            type_ = CellType::TEXT;
        }
        text_ = std::move(text);
        InvalidateCache();
    }
}

void Cell::Clear() {
    text_.clear();
    formula_.reset();
    for (Position p : lower_cells_) {
        sheet_.cells_.at(p).RemoveUpperNode(sheet_.cells_to_positions_.at(this));
    }
    type_ = CellType::CLEARED;
    InvalidateCache();
}

Cell::Value Cell::GetValue() const {
    if (type_ == CellType::FORMULA) {
        if (sheet_.cache_[this].has_value()) {
            return sheet_.cache_[this].value();
        }
        FormulaInterface::Value result = formula_->Evaluate(sheet_);
        if (std::holds_alternative<double>(result)) {
            sheet_.cache_[this] = std::get<double>(result);
            return std::get<double>(result);
        } else {
            return std::get<FormulaError>(result);
        }
    } else {
        if (text_[0] == '\'') {
            return text_.substr(1, text_.size() - 1);
        } else {
            return text_;
        }
    }
}
std::string Cell::GetText() const {
    if (type_ == CellType::FORMULA) {
        return '=' + formula_->GetExpression();
    } else {
        return text_;
    }
}

std::vector<Position> Cell::GetReferencedCells() const {
    std::vector<Position> result(lower_cells_.begin(), lower_cells_.end());
    std::sort(result.begin(), result.end());
    return result;
}

bool Cell::IsReferenced() const {
    return !lower_cells_.empty();
}

bool Cell::IsEmpty() const {
    return type_ == Cell::CellType::EMPTY;
}

bool Cell::IsCleared() const {
    return type_ == Cell::CellType::CLEARED;
}

void Cell::AddLowerNode(Position cell) {
    lower_cells_.insert(cell);
}

void Cell::AddUpperNode(Position cell) {
    upper_cells_.insert(cell);
}

void Cell::RemoveLowerNode(Position cell) {
    lower_cells_.erase(cell);
}

void Cell::RemoveUpperNode(Position cell) {
    upper_cells_.erase(cell);
}

std::optional<const Cell*> Cell::CheckNode() const {    
    std::optional<const Cell*> result;
    if (lower_cells_.empty()) {
        return result;
    }
    std::unordered_set<const Cell*> cells;
    cells.insert(this);
    for (Position p : lower_cells_) {
        result = sheet_.cells_.at(p).CheckNode(cells);
        if (result.has_value()) {
            break;
        }
    }
    return result;
}

std::optional<const Cell*> Cell::CheckNode(std::unordered_set<const Cell*> cells) const {
    if (cells.count(this)) {
        return this;
    } else {
        cells.insert(this);
        std::optional<const Cell*> result;
        for (Position p : lower_cells_) {
            result = sheet_.cells_.at(p).CheckNode(cells);
            if (result.has_value()) {
                break;
            }
        }
        return result; 
    }
}

Cell::Cell(const Cell& other_cell) 
    : type_(other_cell.type_)
    , text_(other_cell.text_)
    , lower_cells_(other_cell.lower_cells_)
    , upper_cells_(other_cell.upper_cells_)
    , sheet_(other_cell.sheet_)
    { 
        if (type_ == CellType::FORMULA) {
            formula_ = ParseFormula(other_cell.formula_->GetExpression());
        }
    }

void Cell::Swap(Cell& other_cell) {
    std::swap(type_, other_cell.type_);
    std::swap(text_, other_cell.text_);
    std::swap(formula_, other_cell.formula_);
    std::swap(lower_cells_, other_cell.lower_cells_);
    std::swap(upper_cells_, other_cell.upper_cells_);
    std::swap(sheet_, other_cell.sheet_);
}

void Cell::InvalidateCache() {
    sheet_.cache_[this].reset();
    for (Position p : upper_cells_) {
         sheet_.cells_.at(p).InvalidateCache();
    }
}