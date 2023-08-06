#pragma once

#include "common.h"
#include "formula.h"

class Sheet;

class Cell : public CellInterface {
public:
	Cell(Sheet& sheet);
	~Cell();

	void Set(std::string text);
	void Clear();

	Value GetValue() const override;
	std::string GetText() const override;
	std::vector<Position> GetReferencedCells() const override;

private:

	class Impl;
	class EmptyImpl;
	class TextImpl;
	class FormulaImpl;

	void CheckCircularDependency(const std::vector <Position> cells) const;
	void CacheInvalidate();

	std::unique_ptr<Impl> impl_;
	Sheet& sheet_;

	std::unordered_set<Cell*> dependent_cells_;
};