#include "sheet.h"

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
	if (GetCell(pos) == nullptr) {
		sheet_cells_map[pos] = std::make_unique<Cell>(*this);
	}
	sheet_cells_map.at(pos)->Set(std::move(text));

	if (pos.col >= size_.cols) { size_.cols = pos.col + 1; }
	if (pos.row >= size_.rows) { size_.rows = pos.row + 1; }
}

CellInterface* Sheet::GetCell(Position pos) {
	return CheckAndGetCell(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
	return CheckAndGetCell(pos);
}

void Sheet::ClearCell(Position pos) {
	if (GetCell(pos) != nullptr) {
		sheet_cells_map.erase(pos);

		int maxcolls = 0, maxrows = 0;
		for (const auto& [key, _] : sheet_cells_map) {
			if (key.col >= maxcolls) { maxcolls = key.col + 1; }
			if (key.row >= maxrows) { maxrows = key.row + 1; }
		}
		size_ = { maxrows, maxcolls };
	}
}

Size Sheet::GetPrintableSize() const {
	return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
	for (int row = 0; row < GetPrintableSize().rows; ++row) {
		for (int col = 0; col < GetPrintableSize().cols; ++col) {
			if (col > 0) { output << '\t'; }
			auto cell = GetCell({ row,col });
			if (cell != nullptr) {
				std::visit([&](const auto& value) { output << value; }, cell->GetValue());
			}
		}
		output << '\n';
	}
}

void Sheet::PrintTexts(std::ostream& output) const {
	for (int row = 0; row < GetPrintableSize().rows; ++row) {
		for (int col = 0; col < GetPrintableSize().cols; ++col) {
			if (col > 0) { output << '\t'; }
			auto cell = GetCell({ row,col });
			if (cell != nullptr) {
				output << cell->GetText();
			}
		}
		output << '\n';
	}
}

std::unique_ptr<SheetInterface> CreateSheet() { return std::make_unique<Sheet>(); }

CellInterface* Sheet::CheckAndGetCell(const Position& pos) const
{
	using namespace std::literals;
	if (!pos.IsValid()) {
		throw InvalidPositionException("Invalid Position Exception"s);
	}

	if (sheet_cells_map.count(pos)) { return sheet_cells_map.at(pos).get(); }
	return nullptr;
}

size_t Sheet::Position_hasher::operator()(const Position& position) const {
	size_t col = std::hash<int>()(position.col);
	size_t row = std::hash<int>()(position.row);
	return col + row * Position::MAX_ROWS;
}