#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <set>

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

	bool IsPositionInPrintableSize(const Position& pos) const;

private:
	std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
    std::set<Size> printable_size_;
};