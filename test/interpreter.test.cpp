#include <sstream>
#include <source_location>

#include "interpreter.hpp"

							// Positive example			|	Non-positive example
constexpr auto INPUT = R"end(0 READ			# 	5				|	-1 (or 0)
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

auto main() -> int {
	auto interp = Interpreter{std::istringstream{INPUT}};

	while (true) {
		std::cerr << "\n===============\n" << interp << '\n';
		switch (const auto result = interp.execute();
			result.state)
		{
			case Interpreter::State::Running:
				break;

			case Interpreter::State::Done:
				std::cerr << "Done\n";
				return EXIT_SUCCESS;
		
			default:
				assert(false and "Unexpected interpreter state");
			case Interpreter::State::Error:
				return EXIT_FAILURE;
		}
	}
}
