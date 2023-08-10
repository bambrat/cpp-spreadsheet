#include "cell.h"
#include "sheet.h"

#include <optional>

class Cell::Impl {
public:
	virtual ~Impl() = default;
	virtual Value GetValue() const = 0;
	virtual std::string GetText() const = 0;
	virtual std::vector<Position> GetReferencedCells() const { return {}; }
	virtual void CacheInvalidate() {}
	virtual bool HaveCache() { return true; }
};

class Cell::EmptyImpl : public Impl {
public:
	Value GetValue() const override {
		return "";
	}

	std::string GetText() const override {
		return "";
	}
};

class Cell::TextImpl : public Impl {
public:
	TextImpl(std::string text) : text_(std::move(text)) {
		if (text_.empty()) throw std::logic_error("");
	}

	Value GetValue() const override {
		if (text_[0] == ESCAPE_SIGN) {
			return text_.substr(1);
		}
		return text_;
	}

	std::string GetText() const override {
		return text_;
	}

private:
	std::string text_;
};

class Cell::FormulaImpl : public Impl {
public:
	explicit FormulaImpl(std::string expression, const SheetInterface& sheet) : sheet_(sheet) {
		formula_ptr_ = ParseFormula(expression.substr(1));
	}

	Value GetValue() const override {
		if (!cache_.has_value()) { 
			cache_ = formula_ptr_->Evaluate(sheet_); 
		}
		return std::visit([](auto& arg) -> Value {return arg; }, *cache_);
	}

	std::string GetText() const override {
		return FORMULA_SIGN + formula_ptr_->GetExpression();
	}

	std::vector<Position> GetReferencedCells() const override {
		return formula_ptr_->GetReferencedCells();
	}

	void CacheInvalidate() override {
		cache_.reset();
	}

	bool HaveCache() override {
		return cache_.has_value();
	}

private:
	mutable std::optional<FormulaInterface::Value> cache_;
	std::unique_ptr<FormulaInterface> formula_ptr_;
	const SheetInterface& sheet_;
};

Cell::Cell(Sheet& sheet) :impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	if (text == GetText()) { return; }

	std::unique_ptr<Impl> impl;
	if (text.empty()) {
		impl = std::make_unique<EmptyImpl>();
	}
	else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
		CheckCircularDependency(impl->GetReferencedCells());
	}
	else {
		impl = std::make_unique<TextImpl>(std::move(text));
	}
	
	ClearDepCells();
	impl_ = std::move(impl);
	FillDepCells();
	CacheInvalidate(true);
}

void Cell::FillDepCells()
{
	for (const auto& pos : impl_->GetReferencedCells()) {
		Cell* cell = static_cast<Cell*>(sheet_.GetCell(pos));
		if (cell == nullptr) {
			sheet_.SetCell(pos, "");
			cell = static_cast<Cell*>(sheet_.GetCell(pos));
		}
		cell->dependent_cells_.insert(this);
	}
}

void Cell::ClearDepCells()
{
	for (const auto& pos : impl_->GetReferencedCells()) {
		Cell* cell = static_cast<Cell*>(sheet_.GetCell(pos));
		cell->dependent_cells_.erase(this);
	}
}

void Cell::CacheInvalidate(bool force = false) {
	if (impl_->HaveCache() || force) {
		impl_->CacheInvalidate();
		for (Cell* dependent : dependent_cells_) {
			dependent->CacheInvalidate();
		}
	}
}

void Cell::CheckCircularDependency(std::vector<Position> cells) {
	if (cells.empty()) { return; }

	std::unordered_set<const CellInterface*> refs;
	for (const auto& pos : cells) {
		refs.insert(sheet_.GetCell(pos));
	}

	std::unordered_set<const CellInterface*> checked;
	std::stack<const CellInterface*> for_check;
	for_check.push(this);

	while (!for_check.empty()) {
		auto current = for_check.top();
		for_check.pop();

		if (refs.find(current) != refs.end()) {
			throw CircularDependencyException("Circular Dependency Exception");
		}

		checked.insert(current);
		auto& ref_cells = static_cast<const Cell*>(current)->dependent_cells_;
		for (auto& check_cell : ref_cells) {
			if (checked.find(check_cell) == checked.end()) {
				for_check.push(check_cell);
			}
		}
	};
}

void Cell::Clear() {
	CacheInvalidate();
	ClearDepCells();
	impl_ = std::make_unique<EmptyImpl>();
}

bool Cell::IsUsed()
{
	return !dependent_cells_.empty();
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