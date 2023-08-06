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
		if (!cache_) { cache_ = formula_ptr_->Evaluate(sheet_); }
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

private:
	mutable std::optional<FormulaInterface::Value> cache_;
	std::unique_ptr<FormulaInterface> formula_ptr_;
	const SheetInterface& sheet_;
};

Cell::Cell(Sheet& sheet) :impl_(std::make_unique<EmptyImpl>()), sheet_(sheet) {}

Cell::~Cell() {}

void Cell::Set(std::string text) {
	if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
	}
	else if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		std::unique_ptr<Impl> impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);

		CheckCircularDependency(impl->GetReferencedCells());

		impl_ = std::move(impl);

		for (const auto& pos : impl_->GetReferencedCells()) {
			Cell* cell = static_cast<Cell*>(sheet_.GetCell(pos));
			if (cell == nullptr) {
				sheet_.SetCell(pos, "");
				cell = static_cast<Cell*>(sheet_.GetCell(pos));
			}
			cell->dependent_cells_.insert(this);
		}
	}
	else {
		impl_ = std::make_unique<TextImpl>(std::move(text));
	}
	CacheInvalidate();
}

void Cell::CacheInvalidate() {
	impl_->CacheInvalidate();

	for (Cell* cell : dependent_cells_) {
		cell->CacheInvalidate();
	}
}

void Cell::CheckCircularDependency(const std::vector <Position> cells) const {
	for (const Position& position : cells) {
		auto check_cell = sheet_.GetCell(position);
		if (check_cell) {
			if (check_cell == this) {
				throw CircularDependencyException("Circular Dependency Exception [" + check_cell->GetText() + "]");
			}
			CheckCircularDependency(check_cell->GetReferencedCells());
		}
	}
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
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