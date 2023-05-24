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


TEST_CASE ("pow2") {
	INFO("sizeof(Interpreter::Integer) == ", sizeof(Interpreter::Integer));
	INFO("std::numeric_limits<Interpreter::Integer>::max() == ", std::numeric_limits<Interpreter::Integer>::max());

	constexpr auto PROGRAM = R"end(
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

	// Generate alot of random and specific test cases
	struct Test_Case {
		using Integer = Interpreter::Integer;

		Integer input, expected;
	};
	constexpr auto RANDOM_CASES = 100'000;
	auto read_inputs = std::vector<Test_Case>{};
	read_inputs.reserve(
		RANDOM_CASES	// Random positive cases
		+ RANDOM_CASES	// Random negative cases
		+ 1		// 0
		+ 1		// 1
	);
	read_inputs.resize(2 * RANDOM_CASES);

	// Positive
	// [0, RANDOM_CASES)
	rs::generate(vw::counted(read_inputs.begin(), RANDOM_CASES),
		[] {
			const auto input = small_rand();
			// Kept the `input * input` to replicate the Stacks MUL operation, there is going to be overflows for
			// big numbers but at least keep the "bug consistent". For squared numbers that are in the range int32
			// there is not problem.
			return Test_Case{input, input * input};
		}
	);

	// Negative
	// [RANDOM_CASES, RANDOM_CASES + RANDOM_CASES)
	rs::generate(vw::counted(read_inputs.begin() + RANDOM_CASES, RANDOM_CASES),
		[] {
			return Test_Case{-std::rand(), 0};
		}
	);
	
	// Edges
	read_inputs.push_back({0, 0});
	read_inputs.push_back({1, 1});


	// Run Interpreter
	rs::for_each(read_inputs,
		[] (const auto& input_pair) {
			// Setup in/out streams
			auto cin = std::istringstream{std::to_string(input_pair.input)};
			auto cout = std::stringstream{};

			REQUIRE(Interpreter{cin, cout}
				.run(std::istringstream{PROGRAM},
					[&] (const auto& execution_result) {
						switch (execution_result.state) {
							case Interpreter::State::Running:
								return;

							case Interpreter::State::Done: {
								auto i = Interpreter::Integer{};
								cout >> i;
								CAPTURE(input_pair.input);
								CHECK_EQ(i, input_pair.expected);
								return;
							}

							case Interpreter::State::Error:
							default:
								FAIL("Interpreter entered the State::Error");
						}
					}
				)
			);
		}
	);
}


TEST_CASE ("Naive Factorial") {
	const auto naive_factorial_path = fs::current_path() / "interpreter.naive_factorial.txt";
	REQUIRE_MESSAGE(fs::exists(naive_factorial_path), naive_factorial_path);

	struct Test_Case {
		using Integer = Interpreter::Integer;

		Integer input, expected;
	};

	constexpr auto RANDOM_CASES = 100'000;
	auto inputs = std::vector<Test_Case>{};
	inputs.reserve(
		RANDOM_CASES
		+ 1	// 0 -> 1
		+ 1	// 1 -> 1
	);

	inputs.resize(RANDOM_CASES);
	rs::generate(vw::counted(inputs.begin(), RANDOM_CASES),
		[] {
			const auto input = 2 + small_rand();
			return Test_Case{input, simple_factorial(input)};
		}
	);

	inputs.push_back({0, 1});
	inputs.push_back({1, 1});

	std::cerr << inputs.size() << '\n';

	// Setup in/out streams
	auto cin = std::stringstream{};
	auto cout = std::stringstream{};

	auto interpreter = Interpreter{cin, cout};

	// Valid input values (>= 0)
	rs::for_each(inputs,
		[&] (const auto& input_pair) {
			cin << input_pair.input << ' ';	// Space for the "separator" of subsequent values
			REQUIRE(interpreter.run(std::ifstream{naive_factorial_path},
				[&] (const auto& execution_result) {
					switch (execution_result.state) {
						case Interpreter::State::Running:
							return;

						case Interpreter::State::Done: {
							auto i = Interpreter::Integer{};
							cout >> i;
							CAPTURE(input_pair.input);
							CHECK_EQ(i, input_pair.expected);
							return;
						}

						case Interpreter::State::Error:
						default:
							FAIL("Interpreter entered the State::Error: ", input_pair.input, " ", input_pair.expected);
					}
				}
			));
		}
	);

	// Invalid input values (< 0)
	for (const auto i : vw::iota(1, RANDOM_CASES)) {
		cin << -i << ' ';
		REQUIRE(interpreter.run(std::ifstream{naive_factorial_path},
			[&] (const auto& execution_result) {
				switch (execution_result.state) {
					case Interpreter::State::Running:
						return;

					case Interpreter::State::Done: {
						auto i = Interpreter::Integer{};
						cout >> i;
						CHECK_EQ(i, 0);
						return;
					}

					case Interpreter::State::Error:
					default:
						FAIL("Interpreter entered the State::Error: ", i);
				}
			}
		));
	}
}
