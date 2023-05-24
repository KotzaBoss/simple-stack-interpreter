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
struct Interpreter {
	using Integer = Stack::Integer;

	struct Instruction {
		using Argument = std::optional<Integer>;

		std::string name;
		Argument arg;

		friend auto operator<< (std::ostream& o, const Instruction& i) -> std::ostream& {
			o << std::left << std::setw(5) << i.name << ' ';
			if (i.arg.has_value())
				o << std::right << std::setw(5) << *i.arg;
			return o;
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
	Stack _stack;
	State state;

// Streams
private:
	std::istream& cin;
	std::ostream& cout;


public:
	// Read the input stream to setup the instruction container, state and pc
	Interpreter(std::istream&& program, std::istream& in = std::cin, std::ostream& out = std::cout)
		: state{State::Running}
		, cin{in}
		, cout{out}
	{
		auto i = 0;
		auto line = std::string{};
		while (std::getline(std::move(program), line)) {
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

		pc = instructions.cbegin();
	}

	struct Execution_Result {
		std::optional<Integer> top;
		State state;
	};
	// Should not throw since we ensure hashes exist in constructor
	auto execute() noexcept -> Execution_Result {
		if (pc == instructions.end())
			return { std::nullopt, State::Done };
		else {
			const auto prev_pc = pc;
			const auto& func = instruction_map.at(string_hasher(pc->name));
			func(*this);
			// If noone changed the pc then simply increment.
			// This is abit hacky... a simple solution could be to hide any +-1 in a function call
			// eg: auto current_instruction() { return *(pc - 1); }
			// But that is just begging for "off by one" problems both in the code and in the mind...
			if (prev_pc == pc)
				++pc;
			return { _stack.top(), state };
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
					std::cerr << "\tWarning: arguments are not expected\n";
				
				if (auto i = Integer{};
					interpreter.cin >> i)
				{
					interpreter._stack.push(i);
					interpreter.state = State::Running;
				}
				else
				{
					std::cerr << "\tError: could not read integer from stdin\n";
					interpreter.state = State::Error;
				}
			}
		},

		{ string_hasher("WRITE"),
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: arguments are not expected\n";

				if (const auto top = interpreter._stack.pop_top();
					top.has_value())
				{
					interpreter.cout << *top << '\n';
				}
				else
					interpreter.cout << "null" << '\n';

				interpreter.state = State::Running;
			} 
		},

		{ string_hasher("DUP"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (instr.arg.has_value())
					std::cerr << "\tWarning: arguments are not expected\n";

				if (not interpreter._stack.dup()) {
					std::cerr << "\tError: failed to duplicate, stack is empty\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";


				if (not interpreter._stack.mul()) {
					std::cerr << "\tError: failed to mul, stack does not have 2 ints\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";

				if (not interpreter._stack.add()) {
					std::cerr << "\tError: failed to add, stack does not have 2 ints\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";

				if (not interpreter._stack.sub()) {
					std::cerr << "\tError: failed to sub, stack does not have 2 ints\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";

				if (not interpreter._stack.gt()) {
					std::cerr << "\tError: failed to gt, stack does not have 2 ints\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";

				if (not interpreter._stack.lt()) {
					std::cerr << "\tError: failed to lt, stack does not have 2 ints\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";

				if (not interpreter._stack.eq()) {
					std::cerr << "\tError: failed to eq, stack does not have 2 ints\n";
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
					std::cerr << "\tWarning: arguments are not expected\n";


				if (auto& stack = interpreter._stack;
					not stack.has_at_least(2))
				{
					std::cerr << "\tError: stack does not have at least 2 int\n";
					interpreter.state = State::Error;
				}
				else {
					if (const auto top = stack.pop_top(), second = stack.pop_top();
						*second == 0)
					{
						interpreter.pc = interpreter.instructions.cbegin() + *top;
					}

					interpreter.state = State::Running;
				}
			} 
		},

		{ string_hasher("PUSH"),
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (not instr.arg.has_value()) {
					std::cerr << "\tError: arguments expected\n";
					interpreter.state = State::Error;
				}
				else {
					interpreter._stack.push(*instr.arg);
					interpreter.state = State::Running;
				}
			} 
		},
		{ string_hasher("POP"),	
			[] (Interpreter& interpreter)
			{
				const auto& instr = *interpreter.pc;

				if (not instr.arg.has_value()) {
					std::cerr << "\tError: arguments expected\n";
					interpreter.state = State::Error;
				}
				else if (not interpreter._stack.pop_n(*instr.arg)) {
					std::cerr << "\tError: stack does not have at least " << *instr.arg << " ints\n";
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
				else if (not interpreter._stack.rot(*instr.arg)) {
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

		return o << '\n' << interp._stack;
	}
};

