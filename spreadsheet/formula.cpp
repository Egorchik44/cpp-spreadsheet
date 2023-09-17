#include "formula.h"

#include "FormulaAST.h"

#include <cctype>
#include <cassert>
#include <sstream>
#include <algorithm>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}


class Formula : public FormulaInterface {
public:

    explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {}
    catch (...) {
        throw FormulaException("formula is syntactically incorrect");
    }

    Value Evaluate(const SheetInterface& sheet) const {
        try {
            std::function<double(Position)> params = [&sheet](const Position pos) -> double {
                if (pos.IsValid()) {
                    const auto* cell = sheet.GetCell(pos);
                    if (cell) {
                        const auto& cellValue = cell->GetValue();
                        if (std::holds_alternative<double>(cellValue)) {
                            return std::get<double>(cellValue);
                        }
                        else if (std::holds_alternative<std::string>(cellValue)) {
                            const auto& strValue = std::get<std::string>(cellValue);
                            if (!strValue.empty()) {
                                std::istringstream input(strValue);
                                double digit = 0.0;
                                if (input >> digit && input.eof()) {
                                    return digit;
                                }
                                else {
                                    throw FormulaError(FormulaError::Category::Value);
                                }
                            }
                            else {
                                return 0.0;
                            }
                        }
                        else {
                            throw FormulaError(std::get<FormulaError>(cellValue));
                        }
                    }
                }
                else {
                    throw FormulaError(FormulaError::Category::Ref);
                }
                return 0.0;
            };

            return ast_.Execute(params);
        }
        catch (const FormulaError& evaluate_error) {
            return evaluate_error;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::set<Position> cells;
        for (const auto& cell : ast_.GetCells()) {

            if (cell.IsValid()) {
                cells.insert(cell);
            }
            else {
                continue;
            }
        }
        return std::vector<Position>(cells.cbegin(), cells.cend());
    }

private:
    FormulaAST ast_;
};


std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}