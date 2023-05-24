#pragma once

#include <iostream>
#include <vector>
#include <optional>
#include <functional>
#include <algorithm>
#include <ranges>
namespace rs = std::ranges;
namespace vw = std::ranges::views;
#include <cassert>

struct Stack {
	using Integer = int32_t;
	using Vector = std::vector<Integer>;

private:
	Vector stack;

public:
	auto push(const Integer i) -> void {
		stack.push_back(i);
	}

	auto pop_top() -> std::optional<Integer> {
		if (not stack.empty()) {
			return pop_back();
		}
		else
			return std::nullopt;
	}

	auto pop_n(const size_t n) -> std::optional<Vector> {
		assert(n > 0);

		if (has_at_least(n)) {
			const auto stack_start = stack.cend() - n;

			auto vec = Vector{stack_start, stack.cend()};	// Copy n ints from stack
			stack.erase(stack_start, stack.cend());		// Erase them from stack
			
			return vec;
		}
		else
			return std::nullopt;
	}

	auto dup() -> bool {
		if (not stack.empty()) {
			stack.push_back(stack.back());
			return true;
		}
		else
			return false;
	}

	// Operations on top 2 values
	auto mul() -> bool { return pop_2_push_op(std::multiplies{}	); }
	auto add() -> bool { return pop_2_push_op(std::plus{}		); }
	auto sub() -> bool { return pop_2_push_op(std::minus{}		); }
	auto gt () -> bool { return pop_2_push_op(std::greater{}	); }
	auto lt () -> bool { return pop_2_push_op(std::less{}		); }
	auto eq () -> bool { return pop_2_push_op(std::equal_to{}	); }

	auto rot(const size_t n) -> bool {
		assert(n > 0 and "Might as well have a NOOP instruction");

		if (has_at_least(n)) {
			// See https://en.cppreference.com/w/cpp/algorithm/ranges/rotate
			// TL;DR for a single right rotation we want the last element (middle)
			// to become the first.
			const auto end = stack.end();
			rs::rotate(end - n, end - 1, end);
			return true;
		}
		else
			return false;
	}

	auto top() const -> std::optional<Integer> {
		if (not stack.empty())
			return stack.back();
		else
			return std::nullopt;
	}

	auto has_at_least(const size_t n) const -> bool {
		return stack.size() >= n;
	}

private:
	// Operation called as op(TOP, SECOND)
	auto pop_2_push_op(const std::function<Integer(Integer, Integer)>& op) -> bool {
		if (has_at_least(2)) {
			// Argument resolution unspecified so we need to "pin" the values
			const auto top = pop_back();
			const auto second = pop_back();
			stack.push_back(op(top, second));
			return true;
		}
		else
			return false;
	}

	// Does NOT check if stack is empty
	auto pop_back() -> Integer {
		const auto back = stack.back();
		stack.pop_back();
		return back;
	}

public:
	friend auto operator<< (std::ostream& o, const Stack& s) -> std::ostream& {
		o << "[ ";
		for (const auto& i : s.stack) {
			o << i << ' ';
		}
		return o << ']';
	}
};