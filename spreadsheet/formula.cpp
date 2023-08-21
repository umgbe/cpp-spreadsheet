#include "formula.h"

#include "FormulaAST.h"
#include "common.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <forward_list>
#include <sstream>
#include <vector>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) 
    : ast_(ParseFormulaAST(expression)) {        
    }

    Value Evaluate(const SheetInterface& sheet) const override {        
        try {
            return ast_.Execute(sheet);
        } catch (FormulaError& fe) {
            return fe;
        }
    }

    std::string GetExpression() const override {
        std::stringstream result;
        ast_.PrintFormula(result);
        return result.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        const std::forward_list<Position>& cells = ast_.GetCells();
        std::vector<Position> result(cells.begin(), cells.end());
        std::sort(result.begin(), result.end());
        auto last = std::unique(result.begin(), result.end());
        result.erase(last, result.end());
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {        
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("");
    }
}