#pragma once

#include <vector>
#include <optional>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <iomanip>
#include <unordered_map>

#include "stack.hpp"


// Not alot of error handling will be done to keep the code cleaner
//
// Usage:
//
// auto interpreter = Interpreter{some_input_stream, some_output_stream};
//
// if (not interpreter.prepare(some_program_input_stream)) {
// 	// Handle error
// }
//
// if (not interpreter.run(
// 	[] (auto&& execution_result) {
// 		switch (execution_result.state) {
// 			// ...
// 		}
// 	}))
// {
// 	// Handle error
// }
//
struct Interpreter {
	using Integer = Stack::Integer;

	struct Instruction {
		using Argument = std::optional<Integer>;

		std::string name;
		Argument arg;

		friend auto operator<< (std::ostream& o, const Instruction& i) -> std::ostream& {
			o << std::left << std::setw(5) << i.name << ' ';
			if (i.arg.has_value())
				return o << std::right << std::setw(5) << *i.arg;
			else
				return o << "     ";
		}
	};
	using Instructions = std::vector<Instruction>;
	
	using PC = Instructions::const_iterator;

	// Hashing
	using String_Hasher = std::hash<std::string>;
	using Hash_Result = size_t;

	using Interpreter_Mutator = std::function<void(Interpreter&)>;

	enum class State {
		Running, Error, Done
	};


private:
	Instructions instructions;
	PC pc;
	Stack stack;
	State state;

// Streams
private:
	std::istream& cin;
	std::ostream& cout;


public:
	// Setup input/output streams
	Interpreter(std::istream& in = std::cin, std::ostream& out = std::cout)
		: cin{in}
		, cout{out}
	{}

	// Prepare program
	auto prepare(std::istream& program) -> bool {
		// Reset program
		instructions.clear();
		stack.clear();

		{ // Read program and prepare interpreter
			auto i = 0;
			auto line = std::string{};
			while (std::getline(program, line)) {
				trim(line);
				if (line.empty())
					continue;
				else {
					auto line_ss = std::istringstream{std::move(line)};

					auto line_i = Integer{};
					auto instr_name = std::string{};
					auto instr_arg = Instruction::Argument{};

					line_ss
						>> line_i
						>> instr_name
						;

					assert(i++ == line_i and "Expect ascending order of instructions");
					assert(instruction_map.contains(string_hasher(instr_name)) and "Command not expected");

					if (not line_ss.eof()) {	// Instruction argument exists
						auto arg = Integer{};
						line_ss >> arg;
						instr_arg = arg;
					}

					instructions.emplace_back(std::move(instr_name), std::move(instr_arg));
				}
			}
		}

		if (instructions.empty()) {
			std::cerr << "Error: prepare: Failed to read any instructions\n";
			return false;
		}
		else
			return true;
	}

	// Run program
	struct Execution_Result {
		std::optional<Integer> top;
		State state;
	};
	auto run(std::function<void(Execution_Result&&)>&& callback) -> bool {
		if (instructions.empty()) {
			std::cerr << "Error: run: No program has been prepared\n";
			return false;
		}
		else {
			pc = instructions.cbegin();
			state = State::Running;
			while (state == State::Running) {
				callback(execute());
			}
			return true;
		}
	}

// Prepare and Execute
private:

	// Should not throw since we ensure hashes exist in constructor
	auto execute() noexcept -> Execution_Result {
		if (pc == instructions.end()) {
			state = State::Done;
			return { std::nullopt, state };
		}
		else {
			const auto prev_pc = pc;
#ifdef INTERPRETER_REPORT_EXECUTION
			report_pc(prev_pc);
#endif
			const auto& func = instruction_map.at(string_hasher(pc->name));
			func(*this);
			// If noone changed the pc then simply increment.
			// This is abit hacky... a simple solution could be to hide any +-1 in a function call
			// eg: auto current_instruction() { return *(pc - 1); }
			// But that is just begging for "off by one" problems both in the code and in the mind...
			if (prev_pc == pc)
				++pc;
			return { stack.top(), state };
		}
	}

// Hashing and Instruction->Interpreter_Mutator mapping 
private:
	static constexpr auto string_hasher = String_Hasher{};
	static inline const auto instruction_map = std::unordered_map<Hash_Result, Interpreter_Mutator>{
		{ string_hasher("READ"),
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: READ: arguments are not expected\n";
				
				if (auto i = Integer{};
					interpreter.cin >> i)
				{
					interpreter.stack.push(i);
					interpreter.state = State::Running;
				}
				else
				{
					std::cerr << "\tError: READ: could not read integer from stdin\n";
					interpreter.state = State::Error;
				}
			}
		},

		{ string_hasher("WRITE"),
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: WRITE: arguments are not expected\n";

				if (const auto top = interpreter.stack.pop_top();
					top.has_value())
				{
					interpreter.cout << *top << ' ';
				}
				else
					interpreter.cout << "null" << ' ';

				interpreter.state = State::Running;
			} 
		},

		{ string_hasher("DUP"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: DUP: arguments are not expected\n";

				if (not interpreter.stack.dup()) {
					std::cerr << "\tError: DUP: failed, stack is empty\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
					// Binary Operations
		{ string_hasher("MUL"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: MUL: arguments are not expected\n";


				if (not interpreter.stack.mul()) {
					std::cerr << "\tError: MUL: failed, stack does not have 2 ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
		{ string_hasher("ADD"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: ADD: arguments are not expected\n";

				if (not interpreter.stack.add()) {
					std::cerr << "\tError: ADD: failed, stack does not have 2 ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
		{ string_hasher("SUB"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: SUB: arguments are not expected\n";

				if (not interpreter.stack.sub()) {
					std::cerr << "\tError: SUB: failed, stack does not have 2 ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
		{ string_hasher("GT"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: GT: arguments are not expected\n";

				if (not interpreter.stack.gt()) {
					std::cerr << "\tError: GT: failed, stack does not have 2 ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
		{ string_hasher("LT"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: LT: arguments are not expected\n";

				if (not interpreter.stack.lt()) {
					std::cerr << "\tError: LT: failed, stack does not have 2 ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
		{ string_hasher("EQ"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: EQ: arguments are not expected\n";

				if (not interpreter.stack.eq()) {
					std::cerr << "\tError: EQ: failed, stack does not have 2 ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},

		{ string_hasher("JMPZ"),
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: JMPZ: arguments are not expected\n";


				if (auto& stack = interpreter.stack;
					not stack.has_at_least(2))
				{
					std::cerr << "\tError: JMPZ: stack does not have at least 2 int\n";
					interpreter.state = State::Error;
				}
				else {

					if (const auto top = *stack.pop_top(), second = *stack.pop_top();
						second == 0)
					{
						if (0 <= top and static_cast<size_t>(top) < interpreter.instructions.size()) {
							interpreter.pc = interpreter.instructions.cbegin() + top;
							interpreter.state = State::Running;
						}
						else {
							std::cerr
								<< "\tError: JMPZ: requested jump to " << top
								<< " is past end of program "
								<< (interpreter.instructions.size() - 1) << '\n';
							interpreter.state = State::Error;
						}
					}
				}
			} 
		},

		{ string_hasher("PUSH"),
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (not instr.arg.has_value()) {
					std::cerr << "\tError: PUSH: arguments expected\n";
					interpreter.state = State::Error;
				}
				else {
					interpreter.stack.push(*instr.arg);
					interpreter.state = State::Running;
				}
			} 
		},
		{ string_hasher("POP"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (not instr.arg.has_value()) {
					std::cerr << "\tError: POP: arguments expected\n";
					interpreter.state = State::Error;
				}
				else if (not interpreter.stack.pop_n(*instr.arg)) {
					std::cerr << "\tError: POP: stack does not have at least " << *instr.arg << " ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			}
		},

		{ string_hasher("ROT"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (not instr.arg.has_value()) {
					std::cerr << "\tError: arguments expected\n";
					interpreter.state = State::Error;
				}
				else if (not interpreter.stack.rot(*instr.arg)) {
					std::cerr << "\tError: stack does not have at least " << *instr.arg << " ints\n";
					interpreter.state = State::Error;
				}
				else
					interpreter.state = State::Running;
			} 
		},
	};

	// Thanks: https://_stackoverflow.com/questions/216823/how-to-trim-an-stdstring
	static auto trim(std::string& l) -> void {
		// # comment trim
		l.erase(std::find_if(l.begin(), l.end(), [](const auto c) { return c == '#'; }),
			l.end()
		);
		// left trim
		l.erase(l.begin(),
			std::find_if(l.begin(), l.end(), [](const auto c) { return not std::isspace(c); })
		);
		// right trim
		l.erase(std::find_if(l.rbegin(), l.rend(), [](const auto c) { return not std::isspace(c); }).base(),
			l.end()
		);
	}

	auto report_pc(const PC& pc) const -> void {
		std::cerr
			<< "Executing: "
			<< std::right << std::setw(3) << std::distance(instructions.cbegin(), pc) << ' '
			<< *pc << '\t'
			<< stack
			<< '\n'
			;
	}

public:
	friend auto operator<< (std::ostream& o, const Interpreter& interp) -> std::ostream& {
		o << "Interpreter:\n";

		// Instructions
		for (auto i = interp.instructions.cbegin(); i != interp.instructions.end() ; ++i)
			o
				<< (i == interp.pc ? "-> " : "   ")
				<< std::right << std::setw(3) << std::distance(interp.instructions.cbegin(), i) << ' '
				<< *i << '\n';

		if (interp.pc == interp.instructions.end())
			o << "->";

		return o << '\n' << interp.stack;
	}
};

