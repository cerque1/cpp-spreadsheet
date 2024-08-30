#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <set>
#include <optional>
#include <algorithm>

//EmptyImpl
CellInterface::Value Cell::EmptyImpl::GetValue(SheetInterface& sheet) const {
    return 0.;
}
std::string Cell::EmptyImpl::GetText() const{
    return "";
}

//TextImpl
Cell::TextImpl::TextImpl(std::string value){
    value_ = value;
}
CellInterface::Value Cell::TextImpl::GetValue(SheetInterface& sheet) const {
    return value_[0] == '\'' ? value_.substr(1) : value_;
}
std::string Cell::TextImpl::GetText() const{
    return value_;
}

//FormulaImpl
Cell::FormulaImpl::FormulaImpl(std::string value){
    try{
        formula_ = ParseFormula(value.substr(1));
    }catch(...){
        throw FormulaException("");
    }
}
CellInterface::Value Cell::FormulaImpl::GetValue(SheetInterface& sheet) const {
    value_ = formula_->Evaluate(sheet);
    if(std::holds_alternative<double>(value_.value())){
        return std::get<double>(value_.value());
    }
    return std::get<FormulaError>(value_.value());
}
std::string Cell::FormulaImpl::GetText() const{
    return "=" + formula_->GetExpression();
}

bool Cell::FormulaImpl::HasValue(){
    return value_.has_value();
}
void Cell::FormulaImpl::Invalidate() const{
    value_.reset();
}
std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const{
    return formula_->GetReferencedCells();
}

//Cell
Cell::Cell(SheetInterface& sheet, const Position& pos, std::string text) : sheet_(sheet){
    if(!pos.IsValid()){
        throw InvalidPositionException("InvalidPositionException");
    }

    Cell* past_cell = reinterpret_cast<Cell*>(sheet_.GetCell(pos));
    if(past_cell != nullptr){
        dependent_cells_ = std::move(past_cell->dependent_cells_);
        CacheInvalidation();
    }

    if(text.empty()){
        impl_ = std::make_unique<EmptyImpl>(EmptyImpl{});
        type_ = CellType::Empty;
    }
    else if(text[0] == '=' && text.size() > 1){
        try{
            impl_ = std::make_unique<FormulaImpl>(FormulaImpl(text));
        }catch(const FormulaException& exp){
            throw exp;
        }
        type_ = CellType::Formula;
        FillCellDepends(text, pos);
        CheckingCyclicDependence();
    }
    else{
        impl_ = std::make_unique<TextImpl>(TextImpl(text));
        type_ = CellType::Text;
    }
}

void Cell::CacheInvalidation() const{
    std::unordered_set<const Cell*> invalid_cells;
    CacheInvalidationRec(invalid_cells);
}

void Cell::CacheInvalidationRec(std::unordered_set<const Cell*>& invalid_cells) const{
    if(type_ != CellType::Formula){
        return;
    }

    FormulaImpl* f_impl = reinterpret_cast<FormulaImpl*>(&*impl_);
    f_impl->Invalidate();
    invalid_cells.insert(this);

    for(Cell* cell : dependent_cells_){
        if(invalid_cells.find(cell) == invalid_cells.end()){
            cell->CacheInvalidationRec(invalid_cells);
        }
    }
}

void Cell::CheckingCyclicDependence() const{
    std::unordered_set<const Cell*> verified_cells;
    CheckingCyclicDependenceRec(dependent_cells_, verified_cells);
}

void Cell::CheckingCyclicDependenceRec(const std::unordered_set<Cell*>& dependent_cells, std::unordered_set<const Cell*>& verified_cells) const{
    if(dependent_cells.find(const_cast<Cell*>(this)) != dependent_cells.end()){
        throw CircularDependencyException("");
    }
    for(Cell* cell : cell_depends_){
        if(verified_cells.find(cell) == verified_cells.end()){
            if(dependent_cells.find(cell) != dependent_cells.end()){
                throw CircularDependencyException("");
            }
            else{
                if(!cell->cell_depends_.empty()){
                    cell->CheckingCyclicDependenceRec(dependent_cells, verified_cells);
                }
                verified_cells.insert(this);
            }
        }
    }
}

bool Cell::IsReferenced() const{
    return !cell_depends_.empty();
}

std::vector<Position> Cell::GetReferencedCells() const{
    std::vector<Position> positions;
    if(type_ == CellType::Formula){
        positions = reinterpret_cast<FormulaImpl*>(&*impl_)->GetReferencedCells();
    }
    return positions;
}

void Cell::FillCellDepends(std::string text, Position pos){
    bool is_letters = false;
    bool is_num = false;
    std::string letters = "";
    std::string num = ""; 
    for(char c : text){
        if(std::isalpha(c) && !is_num){
            is_letters = true;
            letters += c;
        }
        else if(std::isdigit(c) && is_letters){
            is_num = true;
            num += c;
        }
        else{
            if((!letters.empty() && !num.empty())){
                Position new_cell_pos = Position::FromString(letters + num);
                if(pos == new_cell_pos){
                    throw CircularDependencyException("");
                }
                if(sheet_.GetCell(new_cell_pos) == nullptr){
                    sheet_.SetCell(new_cell_pos, "");
                }
                Cell* cell = reinterpret_cast<Cell*>(sheet_.GetCell(new_cell_pos));
                cell_depends_.insert(cell);
                cell->dependent_cells_.insert(this);
                letters.clear();
                num.clear();
            }
            is_letters = false;
            is_num = false;
        }
    }
    if((!letters.empty() && !num.empty())){
        Position new_cell_pos = Position::FromString(letters + num);
        if(pos == new_cell_pos){
            throw CircularDependencyException("");
        }
        if(sheet_.GetCell(new_cell_pos) == nullptr){
            sheet_.SetCell(new_cell_pos, "");
        }
        Cell* cell = reinterpret_cast<Cell*>(sheet_.GetCell(new_cell_pos));
        cell_depends_.insert(cell);
        cell->dependent_cells_.insert(this);
    }
}

Cell::~Cell() {}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>(EmptyImpl{});
}

CellInterface::Value Cell::GetValue() const {
    for(auto cell : cell_depends_){
        if(cell->type_ == CellType::Formula){
            Cell::FormulaImpl* f_cell = reinterpret_cast<Cell::FormulaImpl*>(&cell->impl_);
            if(!f_cell->HasValue()){
                cell->GetValue();
            }
        }
    }
    if(type_ == Text){
        std::string str = reinterpret_cast<Cell::TextImpl*>(&*impl_)->GetText();
        bool is_letter = false;
        for(char c : str){
            if(std::isalpha(c)){
                is_letter = true;
                break;
            }
        }
        if(is_letter == false){
            double num = std::stoi(str);
            return num;
        }
    }
    return impl_->GetValue(sheet_);
}
std::string Cell::GetText() const {
    return impl_->GetText();
}