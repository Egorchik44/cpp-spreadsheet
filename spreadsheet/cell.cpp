#include "cell.h"
#include "sheet.h"

#include <set>
#include <stack>
#include <string>
#include <cassert>
#include <iostream>
#include <optional>



Cell::Cell(Sheet& sheet) : impl_(std::make_unique<EmptyImpl>()),
                           sheet_(sheet) {}
Cell::~Cell() = default;

void Cell::Set(std::string text, Position pos, Sheet* sheet) { 
    std::unique_ptr<Impl> impl;
    pos_ = pos;

    std::forward_list<Position> set_formula_cells;

    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    } else if (text.size() >= 2 && text[0] == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
        auto new_iterim_formula = impl->GetText();

        std::string text_copy = text;

        if (text_copy.size() > 0 && text_copy[0] == FORMULA_SIGN) {
            text_copy = text_copy.substr(1); 

            Position pos_link = Position::FromString(text_copy);
            if (pos_link.IsValid() && !sheet->GetCell(pos_link)  ) {
                sheet->SetCell(pos_link, "");
            }
        }

    } else {
        impl= std::make_unique<TextImpl>(std::move(text));
    }

    if (CheckCircularDependencies(*impl,  pos)) {

        throw CircularDependencyException("Circular dependence is found");
    }

    auto second_st = this->pos_.ToString();

    impl_ = std::move(impl);

    for (Cell* used : using_cells_) {
        auto third_st = used->pos_.ToString();
        used->calculated_cells_.erase(this);
    }

    using_cells_.clear();

    auto iterim_st = impl_->GetText();
    for (const auto& pos : impl_->GetReferencedCells()) {
        auto four_st = pos_.ToString();
        Cell* used = sheet_.GetConcreteCell(pos);
        if (!used ) {
            sheet_.SetCell(pos, "");
            used  = sheet_.GetConcreteCell(pos);
        }
        auto five_st = used->pos_.ToString();
        using_cells_.insert(used );
        used ->calculated_cells_.insert(this);
    }

    ResetCache(true);
}


bool Cell::CheckNumberInVector(const std::vector<int>& numbers, int n) const {
    while (true) {
        if (n < 10) {
            return false;
        }

        int lastDigit = n % 10;
        n = (n - lastDigit) / 10;
        if (std::find(numbers.begin(), numbers.end(), n) != numbers.end()) {
            return true;
        }
    }
    return false;
}

bool Cell::FindPairsInCalc(const Cell* dependent, const std::set<std::pair<const Cell*, int>>& calc_, int dependent_level) const {
    std::vector<int> matchingPairs;
    for (const auto& pair : calc_) {
        if (pair.first->pos_ == dependent->pos_) {
            matchingPairs.push_back(pair.second);
        }
    }
    return CheckNumberInVector(matchingPairs, dependent_level);
}

bool Cell::hasCircularDependency( Cell* cell, std::set<Cell*>& visitedPos, const Position pos_const) {
    for (auto dependentPos : cell->GetReferencedCells()) {
        Cell* ref_cell = sheet_.GetConcreteCell(dependentPos);
        
        if (pos_const == dependentPos){
            return  true;
        }

        if (visitedPos.find(ref_cell) == visitedPos.end() ) {
            visitedPos.insert(ref_cell);
            if (hasCircularDependency(ref_cell, visitedPos,  pos_const))
                return true;
        }

    }
    return false;
}

bool Cell::CheckCircularDependencies( const Impl& new_impl, Position pos) {
    const Position pos_const = pos;
    const auto& cells = new_impl.GetReferencedCells();
    if (!cells.empty()) {
        for (const auto& position : cells) {
            if (position == pos) {
                throw CircularDependencyException("circular dependency detected");
            }
        }
    }
    int i = 0;
    for (const auto& position : cells) {
        Cell* ref_cell = sheet_.GetConcreteCell(position);
        std::set<Cell*> visitedPos;
        visitedPos.insert(ref_cell);
       
        if (hasCircularDependency(ref_cell, visitedPos , pos_const)){
            return true;
        }
        i = i+1;
    }
    return false;
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !calculated_cells_.empty();
}

void Cell::ResetCache(bool force ) {
    if (impl_->HasCache() || force) {
        impl_->ResetCache();
        for (Cell* dependent : calculated_cells_) {
            dependent->ResetCache();
        }
    }
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
    return {};
}

bool Cell::Impl::HasCache() {
    return true;
}

void Cell::Impl::ResetCache() {}

Cell::Value Cell::EmptyImpl::GetValue() const {
    return "";
}
std::string Cell::EmptyImpl::GetText() const {
    return "";
}

Cell::TextImpl::TextImpl(std::string text) : text_(std::move(text)) {}

Cell::Value Cell::TextImpl::GetValue() const {

    if (text_.empty()) {
        throw FormulaException("It is empty, not text!");

    } else if (text_.at(0) == ESCAPE_SIGN) {
        return text_.substr(1);

    } else {
        return text_;
    }
}

std::string Cell::TextImpl::GetText() const {
    return text_;
}

Cell::FormulaImpl::FormulaImpl(std::string text, SheetInterface& sheet) : formula_ptr_(ParseFormula(text.substr(1)))
        , sheet_(sheet) 
{}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_) {
        cache_ = formula_ptr_->Evaluate(sheet_);
    }
    return std::visit([](auto& helper){return Value(helper);}, *cache_);
}

std::string Cell::FormulaImpl::GetText() const {
    return FORMULA_SIGN + formula_ptr_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_ptr_->GetReferencedCells();
}

bool Cell::FormulaImpl::HasCache() {
    return cache_.has_value();
}

void Cell::FormulaImpl::ResetCache() {
    cache_.reset();
}