#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    output << fe.ToString();
    return output;
}

std::ostream& operator<<(std::ostream& output, FormulaError::Category fe){
    switch (fe) {
        case FormulaError::Category::Ref:
            output << "#REF!";
            break;
        case FormulaError::Category::Value:
            output << "#VALUE!";
            break;
        case FormulaError::Category::Arithmetic:
            output << "#ARITHM!";
            break;
    }
    return output;
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)){}

    Value Evaluate(const SheetInterface& sheet) const override {
        try{
            double res = ast_.Execute(sheet);
            return res;
        }catch(FormulaError& error){
            return error;
        }
    }

    std::string GetExpression() const override {
        std::stringstream ss; 
        ast_.PrintFormula(ss);
        return ss.str();
    }

    std::vector<Position> GetReferencedCells() const{
        auto cells = ast_.GetCells();
        std::vector<Position> pos = std::vector(cells.begin(), cells.end());
        std::sort(pos.begin(), pos.end(), [](const Position& lhs, const Position& rhs){
            return lhs < rhs;
        });
        auto iter = std::unique(pos.begin(), pos.end());
        return std::vector(pos.begin(), iter);
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try{
        return std::make_unique<Formula>(std::move(expression));
    }catch(...){
        throw FormulaException("");
    }
}