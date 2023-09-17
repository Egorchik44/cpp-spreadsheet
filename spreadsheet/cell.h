#pragma once

#include "common.h"
#include "formula.h"

#include <set>
#include <stack>
#include <optional>
#include <functional>
#include <unordered_set>

class Sheet;

enum class CellState {
    NotVisited, 
    Visiting, 
    Visited 
};

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text, Position pos, Sheet* sheet);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;


    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;
    void ResetCache(bool force = false);

private:
    class Impl;

    bool CheckNumberInVector(const std::vector<int>& numbers, int n) const;
    bool FindPairsInCalc(const Cell* dependent, const std::set<std::pair<const Cell*, int>>& calc_, int dependent_level) const;

    bool hasCircularDependency( Cell* cell, std::set<Cell*>& visitedPos, const Position pos_const);
    bool CheckCircularDependencies( const Impl& new_impl, Position pos);

    class Impl {
    public:
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;

        virtual bool HasCache();
        virtual void ResetCache();

        virtual ~Impl() = default;
    };



    class EmptyImpl : public Impl {
    public:

        Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:

        explicit TextImpl(std::string text);
        Value GetValue() const override;
        std::string GetText() const override;

    private:

        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:

        explicit FormulaImpl(std::string text, SheetInterface& sheet);

        Value GetValue() const override;
        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        bool HasCache() override;
        void ResetCache() override;

    private:
        mutable std::optional<FormulaInterface::Value> cache_;
        std::unique_ptr<FormulaInterface> formula_ptr_;
        SheetInterface& sheet_;
    };

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;

    std::set<Cell*> calculated_cells_;
    std::unordered_set<Cell*> using_cells_;

    Position pos_;
};