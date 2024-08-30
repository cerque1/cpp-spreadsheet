#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <forward_list>
#include <unordered_set>
#include <optional>

class Sheet;

enum CellType{
    Empty,
    Text,
    Formula
};

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet, const Position& pos, std::string text);
    ~Cell();

    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

    void CheckingCyclicDependence() const;
    void CacheInvalidation() const;
private:
    void CheckingCyclicDependenceRec(const std::unordered_set<Cell*>& dependent_cells, std::unordered_set<const Cell*>& verified_cells) const;
    void CacheInvalidationRec(std::unordered_set<const Cell*>& invalid_cells) const;

    void FillCellDepends(std::string text, Position pos);

    class Impl{
    public:
        virtual Value GetValue(SheetInterface& sheet) const = 0;
        virtual std::string GetText() const = 0;
    };

    class EmptyImpl : public Impl{
    public:
        EmptyImpl() = default;

        Value GetValue(SheetInterface& sheet) const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl{
    public:
        TextImpl(std::string value);

        Value GetValue(SheetInterface& sheet) const override;
        std::string GetText() const override;
    private:
        std::string value_;
    };

    class FormulaImpl : public Impl{
    public:
        FormulaImpl(std::string value);

        Value GetValue(SheetInterface& sheet) const override;
        std::string GetText() const override;

        bool HasValue();
        void Invalidate() const;
        std::vector<Position> GetReferencedCells() const;
    private:
        mutable std::optional<FormulaInterface::Value> value_;
        std::unique_ptr<FormulaInterface> formula_;
    };

    CellType type_;
    //зависимые ячейки
    std::unordered_set<Cell*> dependent_cells_;
    //ячейки от которых зависит данная ячейка
    std::unordered_set<Cell*> cell_depends_;
    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_;
};