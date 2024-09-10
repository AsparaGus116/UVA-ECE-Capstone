#include <iostream>
#include <cctype>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "Format.h"
#include "Utils.h"

#define INSTRUCTION_RAM_SIZE 65536
#define DATA_RAM_SIZE 65536
#define STACK_RAM_SIZE 65536
#define NUM_REGS 16

void highlightInstr(int curPtr);
void highlightData(int curPtr);
void highlightStack(int curPtr);
void printHelpMenu();
void printState();
void printLines(std::vector<uint16_t> arr, int numLines, int start);

std::vector<uint16_t> instructions(INSTRUCTION_RAM_SIZE, 0);
std::vector<uint16_t> data(DATA_RAM_SIZE, 0);
std::vector<uint16_t> stack(STACK_RAM_SIZE, 0);

std::vector<uint16_t> regs(NUM_REGS, 0);

uint16_t pc = 0;
uint16_t rsp = 0;
uint16_t mar = 0;

bool dataUpdated = false;
bool stackUpdated = false;

std::string filename;
std::ifstream file;

int main(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++)
	{
		std::string str = argv[i];
		for (auto c : str)
		{
			str[i] = tolower(str[i]);
		}
		if (str.compare("-h") == 0 ||
			str.compare("-help") == 0 ||
			str.compare("--help") == 0
			)
		{
			printHelpMenu();
			return 0;
		}
		if (str.compare("-f") == 0)
		{
			filename = argv[i + 1];
		}

	}

	//utils::clearTerminal();

	if (filename.size() != 0) // if file was input in cmd line params
	{
		file.open(filename, std::ios::in);

		if (!file.good())
		{
			std::cout << "Invalid filename. Please enter valid filename.\n";
			filename = "";
		}
	}

	if (filename.size() == 0) // if file wasn't input in cmd line params
	{
		while (filename.size() == 0)
		{
			std::cout << "Enter input filename: \n>> ";
			std::cin >> filename;
			file.open(filename, std::ios::in);
			if (!file.good())
			{
				//utils::clearTerminal();
				filename = "";
				std::cout << "Invalid filename; please try again.\n";
			}
		}
	}

	int curPtr = 0;
	while (file.good())
	{
		std::string instr;
		file >> instr;
		if (instr.size() == 0)
			break;
		int x = utils::fromHex(instr);
		if (x > 0xffff)
		{
			std::cout << "WARNING: Instruction at address " << utils::toHex(curPtr, 4, true) << "over 0xFFFF. Defaulting to value 0x0000.\n";
		}
		instructions[curPtr] = static_cast<uint16_t>(x);
		curPtr++;
		if (curPtr >= INSTRUCTION_RAM_SIZE)
		{
			std::cout << "WARNING: Instruction RAM overflow. Truncating at " << INSTRUCTION_RAM_SIZE << " instructions.\n";
			break;
		}
	}

	while (1)
	{
		std::string menuStr = "";
		int steps = 0;
		std::cout << "Type a number to step that many times, (v) to view the CPU's state, or (q) to quit.\n>> ";
		std::cin >> menuStr;
		for (int i = 0; i < menuStr.size(); i++)
		{
			menuStr[i] = tolower(menuStr[i]);
		}
		if (menuStr.find('v') != std::string::npos)
		{
			printState();
			continue;
		}
		else if (menuStr.find('q') != std::string::npos)
		{
			std::cout << "Exiting...\n";
			return 0;
		}
		else
		{
			try
			{
				steps = std::stoi(menuStr);
			}
			catch (std::invalid_argument)
			{
				std::cout << "Invalid step number. Please try again.\n";
				continue;
			}
		}

		for (int i = 0; i < steps; i++)
		{
			uint16_t instr = instructions[pc];
			int opcode = (instr & 0xf000) >> 12;
			int rA = (instr & 0x0f00) >> 8;
			int rB = (instr & 0x00f0) >> 4;
			int rY = (instr & 0x000f);
			bool flag = false;

			switch (opcode)
			{
			case 0x0: // Register - Register
				regs[rY] = regs[rA];
				++pc;
				break;
			case 0x1: // Immediate - Register
				regs[rY] = instructions[pc + 1];
				pc += 2;
				break;
			case 0x2: // Register - Memory
				data[regs[rB]] = regs[rA];
				dataUpdated = true;
				mar = regs[rB];
				++pc;
				break;
			case 0x3: // Memory - Register
				regs[rY] = data[regs[rA]];
				dataUpdated = true;
				mar = regs[rB];
				++pc;
				break;
			case 0x4: // Bitwise OR
				regs[rY] = regs[rA] | regs[rB];
				++pc;
				break;
			case 0x5: // Bitwise NOT
				regs[rY] = ~regs[rA];
				++pc;
				break;
			case 0x6: // Bitwise AND
				regs[rY] = regs[rA] & regs[rB];
				++pc;
				break;
			case 0x7: // Bitwise XOR
				regs[rY] = regs[rA] ^ regs[rB];
				++pc;
				break;
			case 0x8: // Addition
				regs[rY] = regs[rA] + regs[rB];
				++pc;
				break;
			case 0x9: // Subtraction
				regs[rY] = regs[rA] - regs[rB];
				++pc;
				break;
			case 0xA: // Logical NOT
				regs[rY] = (regs[rA] == 0) ? 1 : 0;
				++pc;
				break;
			case 0xB: // Logical Right Shift
				regs[rY] = regs[rA] >> 1;
				++pc;
				break;
			case 0xC: // Push
				flag = (bool)((instr & 0x0080) >> 7);
				stack[rsp] = flag ? pc : regs[rA];
				stackUpdated = true;
				++rsp;
				++pc;
				break;
			case 0xD: // Pop
				flag = (bool)((instr & 0x0010) >> 7);
				stackUpdated = true;
				--rsp;
				if (flag)
				{
					pc = stack[rsp];
				}
				else
				{
					regs[rY] = stack[rsp];
				}
				++pc;
				break;
			case 0xE: // Unconditional Jump
				pc = regs[rA];
				break; // note: PC does NOT increment after jumps
			case 0xF: // Conditional Jump
				bool jump = false;

				switch (rY)
				{
				case 0:
					if (regs[rB] == 0) jump = true;
					break; // check if rB == 0
				case 1:
					if (regs[rB] != 0) jump = true;
					break; // check if rB != 0
				case 2:
					if (regs[rB] & 0x1000) jump = true;
					break; // check if rB < 0
				case 3:
					if (regs[rB] != 0 && ((regs[rB] & 0x1000) == 0)) jump = true;
					break; // check if rB > 0
				}
				if (jump)
				{
					pc = regs[rA];
				}
				else
				{
					++pc;
				}
				break;
			}
			printState();
		}
		}
		
}

void printState()
{
	int start = 0;
	utils::clearTerminal();
	for (int j = 0; j < NUM_REGS / 4; j++)
	{
		for (int i = 0; i < 4; i++)
		{
			int num = (4 * j) + i;
			std::cout << "r" << num << ": " << ((num < 10) ? " " : "") << utils::toHex(regs[num], 4, true) << '\t';
		}
		std::cout << '\n';
	}

	std::cout << "pc:  " << utils::toHex(pc, 4, true) << '\t';
	std::cout << "rsp: " << utils::toHex(rsp, 4, true) << '\t';
	std::cout << "\n\n";

	std::cout << "Instruction RAM: \n";
	std::cout << "       |  0x0  0x1  0x2  0x3  0x4  0x5  0x6  0x7  0x8  0x9  0xA  0xB  0xC  0xD  0xE  0xF\n";
	std::cout << "-------+--------------------------------------------------------------------------------\n";

	if (pc >= 0x0010)
	{
		int start = (pc - 16 & 0xfff0);
		std::cout << utils::toHex((start), 4, true) << " | ";
		for (int i = 0; i < 16; i++)
		{
			highlightInstr(start + i);
			std::cout << utils::toHex(instructions[start + i], 4, false) << " ";
			format::resetTextColor();
		}
		std::cout << "\n\n";
		//print first line 
	}
	start = pc & 0xfff0;
	std::cout << utils::toHex((start), 4, true) << " | ";
	for (int i = 0; i < 16; i++)
	{
		highlightInstr(start + i);

		std::cout << utils::toHex(instructions[start + i], 4, false);
		format::resetTextColor();
		std::cout << " ";
	}
	std::cout << '\n';
	//always print second line
	if (pc < 0xfff0)
	{
		int start = (pc + 16) & 0xfff0;
		std::cout << utils::toHex((start), 4, true) << " | ";
		for (int i = 0; i < 16; i++)
		{
			highlightInstr(start + i);
			std::cout << utils::toHex(instructions[start + i], 4, false) << " ";
			format::resetTextColor();
		}
		std::cout << '\n';
	}
	std::cout << "\n\n";

	std::cout << "Data RAM: \n";
	std::cout << "       |  0x0  0x1  0x2  0x3  0x4  0x5  0x6  0x7  0x8  0x9  0xA  0xB  0xC  0xD  0xE  0xF\n";
	std::cout << "-------+--------------------------------------------------------------------------------\n";

	if (pc >= 0x0010)
	{
		int start = (pc - 16 & 0xfff0);
		std::cout << utils::toHex((start), 4, true) << " | ";
		for (int i = 0; i < 16; i++)
		{
			highlightData(start + i);
			std::cout << utils::toHex(data[start + i], 4, false) << " ";
			format::resetTextColor();
		}
		std::cout << "\n\n";
		//print first line 
	}
	start = pc & 0xfff0;
	std::cout << utils::toHex((start), 4, true) << " | ";
	for (int i = 0; i < 16; i++)
	{
		highlightData(start + i);

		std::cout << utils::toHex(data[start + i], 4, false);
		format::resetTextColor();
		std::cout << " ";
	}
	std::cout << '\n';
	//always print second line
	if (pc < 0xfff0)
	{
		int start = (pc + 16) & 0xfff0;
		std::cout << utils::toHex((start), 4, true) << " | ";
		for (int i = 0; i < 16; i++)
		{
			highlightData(start + i);
			std::cout << utils::toHex(data[start + i], 4, false) << " ";
			format::resetTextColor();
		}
		std::cout << '\n';
	}
	std::cout << "\n\n";

	std::cout << "Stack RAM: \n";
	std::cout << "       |  0x0  0x1  0x2  0x3  0x4  0x5  0x6  0x7  0x8  0x9  0xA  0xB  0xC  0xD  0xE  0xF\n";
	std::cout << "-------+--------------------------------------------------------------------------------\n";

	if (pc >= 0x0010)
	{
		int start = (pc - 16 & 0xfff0);
		std::cout << utils::toHex((start), 4, true) << " | ";
		for (int i = 0; i < 16; i++)
		{
			highlightStack(start + i);
			std::cout << utils::toHex(stack[start + i], 4, false) << " ";
			format::resetTextColor();
		}
		std::cout << "\n\n";
		//print first line 
	}
	start = pc & 0xfff0;
	std::cout << utils::toHex((start), 4, true) << " | ";
	for (int i = 0; i < 16; i++)
	{
		highlightStack(start + i);

		std::cout << utils::toHex(stack[start + i], 4, false);
		format::resetTextColor();
		std::cout << " ";
	}
	std::cout << '\n';
	//always print second line
	if (pc < 0xfff0)
	{
		int start = (pc + 16) & 0xfff0;
		std::cout << utils::toHex((start), 4, true) << " | ";
		for (int i = 0; i < 16; i++)
		{
			highlightStack(start + i);
			std::cout << utils::toHex(stack[start + i], 4, false) << " ";
			format::resetTextColor();
		}
		std::cout << '\n';
	}
}

void highlightInstr(int curPtr)
{
	if (curPtr == pc)
	{
		format::setTextColorNB(Color::CYAN, false);
		format::setTextColorNB(Color::BLACK, true);

	}
	else if (curPtr == pc + 1 && ((instructions[pc] & 0xf000) >> 12) == 1)
	{
		format::setTextColorNB(Color::BLUE, false);
}
	else if (curPtr == pc + 1 && ((instructions[pc] & 0xf000) >> 12) != 1)
	{
		format::setTextColor(Color::MAGENTA, false);
	}
	else if (curPtr == pc + 2 && ((instructions[pc] & 0xf000) >> 12) == 1)
	{
		format::setTextColor(Color::MAGENTA, false);
	}
}

void highlightData(int curPtr)
{
	if (dataUpdated && curPtr == mar)
	{
		dataUpdated = false;
		format::setTextColorNB(Color::WHITE, false);
		format::setTextColorNB(Color::BLACK, true);
	}
}

void highlightStack(int curPtr)
{
	if (stackUpdated && curPtr == rsp)
	{
		stackUpdated = false;
		format::setTextColorNB(Color::WHITE, false);
		format::setTextColorNB(Color::BLACK, true);
	}
}

void printHelpMenu()
{
#if defined(__linux__)
	std::cout << "SYNTAX: ./rom_sim [-f input.txt]\n";
#elif _WIN32
	std::cout << "SYNTAX: rom_sim.exe [-f input.txt]\n";
#endif
	std::cout << "-f: Specify input file\n";
	//std::cout << "--dbg: enable step-through mode\n";
	std::cout << "Specify -h or --help for this menu.\n\n";
}