#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"

#include <algorithm>
#include <ranges>
namespace rs = std::ranges;
namespace vw = std::ranges::views;
#include <sstream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
#include <numeric>

#include "interpreter.hpp"

using namespace std::literals;

constexpr auto RANDOM_CASES = 1'000;

auto simple_factorial(Interpreter::Integer x) -> Interpreter::Integer {
	assert(x >= 0);
	auto result = 1;
	while (x > 0)
		result *= x--;
	return result;
}

auto small_rand() -> auto {
	std::srand(123);
	return std::rand() % 100;
}

struct Test_Input {
	using Integer = Interpreter::Integer;

	Integer input, expected;

	static auto negative_zero() -> Test_Input {
		return {-small_rand(), 0};
	}

	friend auto operator<< (std::ostream& o, const Test_Input& ti) -> std::ostream& {
		return o << "Test_Input{input=" << ti.input << ", " << "expected=" << ti.expected << '}';
	}
};


TEST_CASE ("Interpreter") {
	CAPTURE(sizeof(Interpreter::Integer));
	CAPTURE(std::numeric_limits<Interpreter::Integer>::max());

	// In/out streams
	auto cin = std::stringstream{};
	auto cout = std::stringstream{};

	auto interpreter = Interpreter{cin, cout};

	auto program = std::stringstream{};

	auto test_inputs = std::vector<Test_Input>{};

	SUBCASE ("pow2") {
		program << R"end(
										# 	Positive	|	Non-positive
			0 READ							# 	5		|	-1
			1 DUP							# 	5 5		|	-1 -1
			2 PUSH 0						# 	5 5 0		|	-1 -1 0
			3 LT		# Comparisons return 0 for true		#	5 0		|	-1  1
			4 PUSH 8						# 	5 0 8		|	-1  1 8
			5 JMPZ		# Remember the 2 values are dropped	# 	5		|	-1 
			6 POP 1							# 	skipped 	|	empty
			7 PUSH 0						# 	skipped		|	0
			8 DUP							# 	5 5		|	0 0
			9 MUL							# 	25		|	0
			10 WRITE
		)end";

		test_inputs.reserve(
			2 * RANDOM_CASES	// Random cases (positive/negatives)
			+ 1			// 0
			+ 1			// 1
		);

		test_inputs.resize(2 * RANDOM_CASES);

		// Positive
		// [0, RANDOM_CASES)
		rs::generate(vw::counted(test_inputs.begin(), RANDOM_CASES), [] {
			const auto input = 2 + small_rand();	// We deal with 1 specifically
			// Kept the `input * input` to replicate the Stacks MUL operation, there is going to be overflows for
			// big numbers but at least keep the "bug consistent". For squared numbers that are in the range int32
			// there is not problem.
			return Test_Input{input, input * input};
		});

		// Negative
		// [RANDOM_CASES, 2 * RANDOM_CASES)
		rs::generate(vw::counted(test_inputs.begin() + RANDOM_CASES, RANDOM_CASES), Test_Input::negative_zero);
		
		// Edges
		test_inputs.push_back({0, 0});
		test_inputs.push_back({1, 1});
	}

	SUBCASE ("Naive Factorial") {
		const auto naive_factorial_path = fs::current_path() / "interpreter.naive_factorial.txt";
		REQUIRE_MESSAGE(fs::exists(naive_factorial_path), naive_factorial_path);

		// Thanks: https://stackoverflow.com/questions/132358/how-to-read-file-content-into-istringstream
		auto naive_factorial = std::ifstream{naive_factorial_path};
		program << naive_factorial.rdbuf();

		test_inputs.reserve(
			2 * RANDOM_CASES	// Random cases (positive/negatives)
			+ 1			// 0! = 1
			+ 1			// 1! = 1
		);

		test_inputs.resize(2 * RANDOM_CASES);

		// Positive
		// [0, RANDOM_CASES)
		rs::generate(vw::counted(test_inputs.begin(), RANDOM_CASES), [] {
			const auto input = 2 + small_rand();	// We deal with 1 specifically
			return Test_Input{input, simple_factorial(input)};
		});

		// Negative
		// [RANDOM_CASES, 2 * RANDOM_CASES)
		rs::generate(vw::counted(test_inputs.begin() + RANDOM_CASES, RANDOM_CASES), Test_Input::negative_zero);

		// Edges
		test_inputs.push_back({0, 1});
		test_inputs.push_back({1, 1});
	}

	interpreter.prepare(program);

	// Run Interpreter
	rs::for_each(test_inputs, [&] (const auto& test_input) {
		CAPTURE(test_input);
		cin << test_input.input << ' ';
		REQUIRE(interpreter.run([&] (const auto& execution_result) {
			switch (execution_result.state) {
				case Interpreter::State::Running:
					return;

				case Interpreter::State::Done: {
					auto out = Interpreter::Integer{};
					cout >> out;
					CHECK_EQ(out, test_input.expected);
					return;
				}

				case Interpreter::State::Error:
				default:
					FAIL("Interpreter entered the State::Error");
			}
		}));
	});
}


TEST_CASE ("Interpreter Empty Program") {
	auto program = std::istringstream{""};
	auto interpreter = Interpreter{};
	REQUIRE_FALSE(interpreter.prepare(program));
	REQUIRE_FALSE(interpreter.run([] (const auto&) { FAIL("Interpreter::run() should not begin running anything"); }));
}

