#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"

#include <algorithm>
#include <ranges>
namespace rs = std::ranges;
namespace vw = std::ranges::views;
#include <sstream>

#include "interpreter.hpp"

using namespace std::literals;

TEST_CASE ("pow2") {
	MESSAGE("sizeof(Interpreter::Integer) == ", sizeof(Interpreter::Integer));
	MESSAGE("std::numeric_limits<Interpreter::Integer>::max() == ", std::numeric_limits<Interpreter::Integer>::max());

	//						Examples:	Positive			|	Non-positive
	constexpr auto INPUT = R"end(
	0 READ							# 	5				|	-1
	1 DUP							# 	5 5				|	-1 -1
	2 PUSH 0						# 	5 5 0				|	-1 -1 0
	3 GT		# GT returns 0 for true			#	5 0				|	-1  1
	4 PUSH 8						# 	5 0 8				|	-1  1 8
	5 JMPZ		# Remember the 2 values are dropped	# 	5     PC == 8 (SECOND is 0)	|	-1 
	6 POP 1							# 	skipped 			|	empty
	7 PUSH 0						# 	skipped				|	0
	8 DUP							# 	5 5				|	0 0
	9 MUL							# 	25				|	0
	10 WRITE
	)end";

	// Generate alot of random and specific test cases
	struct Test_Case {
		using Integer = Interpreter::Integer;

		Integer input, expected;
	};
	constexpr auto RANDOM_CASES = 100'000;
	auto read_inputs = std::vector<Test_Case>(
		RANDOM_CASES	// Random positive cases
		+ RANDOM_CASES	// Random negative cases
		+ 1		// 0
		+ 1		// 1
	);
	std::srand(123);

	// Positive
	// [0, RANDOM_CASES)
	rs::generate(vw::counted(read_inputs.begin(), RANDOM_CASES),
		[] {
			const auto input = std::rand();
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
	
	read_inputs.push_back({0, 0});
	
	read_inputs.push_back({1, 1});


	// Run Interpreter
	rs::for_each(read_inputs,
		[] (const auto& input_pair) {
			// Setup in/out streams
			auto cin = std::istringstream{std::to_string(input_pair.input)};
			auto cout = std::ostringstream{};

			for (auto interp = Interpreter{std::istringstream{INPUT}, cin, cout};;)
			{
				switch (const auto result = interp.execute();
					result.state)
				{
					case Interpreter::State::Running:
						break;

					case Interpreter::State::Done:
						CHECK_EQ(std::stoll(cout.str()), input_pair.expected);
						return;

					case Interpreter::State::Error:
					default:
						FAIL("Interpreter entered the State::Error");
				}
			}
		}
	);
}
