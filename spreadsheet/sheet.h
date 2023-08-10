#pragma once

#include "cell.h"
#include "common.h"

class Sheet : public SheetInterface {
public:
	~Sheet();

	void SetCell(Position pos, std::string text) override;
	CellInterface* GetCell(Position pos) override;
	const CellInterface* GetCell(Position pos) const override;
	void ClearCell(Position pos) override;
	Size GetPrintableSize() const override;

	void PrintValues(std::ostream& output) const override;
	void PrintTexts(std::ostream& output) const override;

private:
	CellInterface* CheckAndGetCell(Position& pos) const;
	void PrintCell(std::ostream& output, std::function<void(const CellInterface*)> prnt) const;

	struct Position_hasher {
		size_t operator() (const Position& position) const;
	};

	Size size_;
	std::unordered_map<Position, std::unique_ptr<Cell>, Position_hasher> sheet_cells_map;
};