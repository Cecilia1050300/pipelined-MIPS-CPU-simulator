#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <cstdlib>
#include <iomanip>
using namespace std;

// 模擬全域變數
int PC = 0; // 程式計數器
int IF_PCSrc = 0; // 分支跳轉控制信號
int branchTargetAddress = 0; // 分支目標地址
int IF_stall_count = 0; // 暫停計數器
vector<string> instructions; // 指令記憶體，存儲程式中的所有指令
vector<int> registers(32, 0); // 模擬 32 個暫存器
vector<int> memory(1024, 0); // 模擬記憶體空間

// 暫存器結構
struct IF_ID_Register {
    string instruction;
    int pc;
} IF_ID;

struct ID_EX_Register {
    int readData1 = 0;
    int readData2 = 0;
    int immediate = 0;
    int rs = 0, rt = 0, rd = 0;
    string ALUOp = "";
    bool RegWrite = false;
    bool MemRead = false;
    bool MemWrite = false;
    bool Branch = false;
    bool MemToReg = false;
}ID_EX;


struct EX_MEM_Register {
    int ALUResult;
    int writeData;
    int rd;
    bool RegWrite = false;
    bool MemRead = false;
    bool MemWrite = false;
    bool MemToReg = false;
} EX_MEM;

struct MEM_WB_Register {
    int readData;
    int ALUResult;
    int rd;
    bool RegWrite = false;
    bool MemToReg = false;
} MEM_WB;

// 讀取指令記憶體
void loadInstructions(const string& filename) {
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        instructions.push_back(line);
    }
    infile.close();
}

// 取得指令的操作碼
string getOpcode(const string& instruction) {
    istringstream stream(instruction);
    string opcode;
    stream >> opcode;
    return opcode;
}

// 將指令字串解析為結構
struct Instruction {
    string opcode;
    int rs, rt, rd, immediate;
};
Instruction parseInstruction(const string& instruction) {
    Instruction instr;
    istringstream stream(instruction);
    stream >> instr.opcode >> instr.rs >> instr.rt >> instr.rd >> instr.immediate;
    return instr;
}

// IF 階段
void IF(ofstream& outfile) {
    if (IF_PCSrc == 1) {
        PC = branchTargetAddress;
        outfile << "lw IF (branch)\n";
        IF_PCSrc = 0;
        return;
    }
    if (IF_stall_count > 0) {
        outfile << "lw IF (stalled)\n";
        IF_stall_count--;
        return;
    }
    if (PC >= instructions.size()) {
        outfile << "NOP IF\n";
        return;
    }
    IF_ID.instruction = instructions[PC];
    IF_ID.pc = PC;
    PC++;
    outfile << getOpcode(IF_ID.instruction) << " IF\n";
}

// ID 階段
void ID(ofstream& outfile) {
    if (IF_ID.instruction.empty()) {
        outfile << "NOP ID\n";
        return;
    }
    Instruction instr = parseInstruction(IF_ID.instruction);
    ID_EX.readData1 = registers[instr.rs];
    ID_EX.readData2 = registers[instr.rt];
    ID_EX.rs = instr.rs;
    ID_EX.rt = instr.rt;
    ID_EX.rd = instr.rd;
    ID_EX.immediate = instr.immediate;
    ID_EX.ALUOp = instr.opcode; // 添加ALU操作碼
    outfile << instr.opcode << " ID\n";
}

// EX 階段
void EX(ofstream& outfile) {
    if (ID_EX.ALUOp.empty()) {
        outfile << "NOP EX\n";
        return;
    }
    int ALUInput1 = ID_EX.readData1;
    int ALUInput2 = ID_EX.readData2;
    if (ID_EX.ALUOp == "ADD") {
        EX_MEM.ALUResult = ALUInput1 + ALUInput2;
    } else if (ID_EX.ALUOp == "SUB") {
        EX_MEM.ALUResult = ALUInput1 - ALUInput2;
    }
    EX_MEM.writeData = ID_EX.readData2;
    EX_MEM.rd = ID_EX.rd;
    EX_MEM.RegWrite = ID_EX.RegWrite;
    EX_MEM.MemRead = ID_EX.MemRead;
    EX_MEM.MemWrite = ID_EX.MemWrite;
    outfile << "EX RegDst=X ALUSrc=X Branch=X MemRead=" << EX_MEM.MemRead
            << " MemWrite=" << EX_MEM.MemWrite
            << " RegWrite=" << EX_MEM.RegWrite
            << " MemToReg=X\n";
}

// MEM 階段
void MEM(ofstream& outfile) {
    if (!EX_MEM.MemRead && !EX_MEM.MemWrite) {
        outfile << "NOP MEM\n";
        return;
    }
    if (EX_MEM.MemRead) {
        MEM_WB.readData = memory[EX_MEM.ALUResult];
    }
    if (EX_MEM.MemWrite) {
        memory[EX_MEM.ALUResult] = EX_MEM.writeData;
    }
    MEM_WB.ALUResult = EX_MEM.ALUResult;
    MEM_WB.rd = EX_MEM.rd;
    MEM_WB.RegWrite = EX_MEM.RegWrite;
    outfile << "MEM Branch=X MemRead=" << EX_MEM.MemRead
            << " MemWrite=" << EX_MEM.MemWrite
            << " RegWrite=" << EX_MEM.RegWrite
            << " MemToReg=X\n";
}

// WB 階段
void WB(ofstream& outfile) {
    if (!MEM_WB.RegWrite) {
        outfile << "NOP WB\n";
        return;
    }
    registers[MEM_WB.rd] = MEM_WB.readData ? MEM_WB.readData : MEM_WB.ALUResult;
    outfile << "WB RegWrite=X MemToReg=X\n";
}

int main() {
    string instructionFile = "test3.txt";
    string resultFile = "result2.txt";
    loadInstructions(instructionFile);

    ofstream outfile(resultFile, ios::trunc);

    int cycle = 1;
    while (cycle <= instructions.size() + 4) {
        outfile << "Clock Cycle " << cycle << ":\n";
        WB(outfile);
        MEM(outfile);
        EX(outfile);
        ID(outfile);
        IF(outfile);
        cycle++;
    }

    // 輸出最終結果
    outfile << "\n## Final Result:\n";
    outfile << "Total Cycles: " << cycle - 1 << "\n";
    outfile << "Final Register Values:\n";
    for (int i = 0; i < registers.size(); ++i) {
        outfile << registers[i] << " ";
    }
    outfile << "\nFinal Memory Values:\n";
    for (int i = 0; i < memory.size(); ++i) {
        outfile << memory[i] << " ";
    }
    outfile << "\n";

    outfile.close();
    cout << "模擬完成，結果已輸出至 " << resultFile << endl;
    return 0;
}

