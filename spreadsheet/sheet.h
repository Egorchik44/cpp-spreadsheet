#pragma once

#include "cell.h"
#include "common.h"

#include <vector>
#include <functional>

using Table = std::vector<std::vector<std::unique_ptr<Cell>>>;

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void EnsurePositionIsValid(Position pos) const;

    void SetCell(Position pos, std::string text) override;

    CellInterface* GetCell(Position pos) override;
    const CellInterface* GetCell(Position pos) const override;
    Cell* GetConcreteCell(Position pos);
    const Cell* GetConcreteCell(Position pos) const;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;


private:

    Table cells_;

};