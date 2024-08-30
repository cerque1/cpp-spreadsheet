#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::Sheet(){
    printable_size_.insert(Size{0, 0});
}

Sheet::~Sheet() {
    sheet_.clear();
}

void Sheet::SetCell(Position pos, std::string text) {
    if(!pos.IsValid()){
        throw InvalidPositionException("");
    }

    if(!IsPositionInPrintableSize(pos)){
        if(printable_size_.begin()->rows <= pos.row){
            sheet_.resize(pos.row + 1);
        }
        if(printable_size_.begin()->cols <= pos.col){
            for(int row = 0; row < std::max(pos.row + 1, printable_size_.begin()->rows); ++row){
                sheet_[row].resize(pos.col + 1);
            }
        }
        else{
            for(int row = std::min(pos.row + 1, printable_size_.begin()->rows) - 1; row < std::max(pos.row + 1, printable_size_.begin()->rows); ++row){
                sheet_[row].resize(printable_size_.begin()->cols);
            }
        }
    }
    sheet_[pos.row][pos.col] = std::make_unique<Cell>(*this, pos, text);

    Size size = *printable_size_.begin();
    if(printable_size_.begin()->rows < pos.row + 1){
        size.rows = pos.row + 1;
    }
    if(printable_size_.begin()->cols < pos.col + 1){
        size.cols = pos.col + 1;
    }
    printable_size_.insert(size);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if(!pos.IsValid()){
        throw InvalidPositionException("");
    }
    if(!IsPositionInPrintableSize(pos) || sheet_[pos.row][pos.col] == nullptr){
        return nullptr;
    }
    return const_cast<CellInterface*>(dynamic_cast<CellInterface*>(&*sheet_[pos.row][pos.col]));
}
CellInterface* Sheet::GetCell(Position pos) {
    if(!pos.IsValid()){
        throw InvalidPositionException("");
    }
    if(!IsPositionInPrintableSize(pos) || sheet_[pos.row][pos.col] == nullptr){
        return nullptr;
    }
    return dynamic_cast<CellInterface*>(&*sheet_[pos.row][pos.col]);
}

void Sheet::ClearCell(Position pos) {
    if(!pos.IsValid()){
        throw InvalidPositionException("");
    }
    if(IsPositionInPrintableSize(pos)){
        if(sheet_[pos.row][pos.col] != nullptr){
            sheet_[pos.row][pos.col].release();
            printable_size_.erase(printable_size_.find(Size{pos.row + 1, pos.col + 1}));
        }
    }
}

Size Sheet::GetPrintableSize() const {
    if(printable_size_.empty()){
        return {0, 0};
    }
    return *printable_size_.begin();
}

void Sheet::PrintValues(std::ostream& output) const {
    if(printable_size_.empty()){
        return;
    }
    for(int row = 0; row < printable_size_.begin()->rows; ++row){
        bool is_first_col = true;
        for(int col = 0; col < printable_size_.begin()->cols; ++col){
            if(is_first_col){
                is_first_col = false;
            }
            else{
                output << "\t";
            }
            if(sheet_[row][col] != nullptr){
                output << sheet_[row][col]->GetValue();
            }   
        }
        output << "\n";
    }
}
void Sheet::PrintTexts(std::ostream& output) const {
    if(printable_size_.empty()){
        return;
    }
    for(int row = 0; row < printable_size_.begin()->rows; ++row){
        bool is_first_col = true;
        for(int col = 0; col < printable_size_.begin()->cols; ++col){
            if(is_first_col){
                is_first_col = false;
            }
            else{
                output << "\t";
            }
            if(sheet_[row][col] != nullptr){
                output << sheet_[row][col]->GetText();
            }   
        }
        output << "\n";
    }
}

bool Sheet::IsPositionInPrintableSize(const Position& pos) const{
    return printable_size_.begin()->rows > pos.row && printable_size_.begin()->cols > pos.col;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}