#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
using namespace std;

// 模擬全域變數
int PC = 0; // 程式計數器
int IF_PCSrc = 0; // 分支跳轉控制信號
int branchTargetAddress = 0; // 分支目標地址
int IF_stall_count = 0; // 暫停計數器
vector<string> instructions; // 指令記憶體
vector<int> registers(32, 0); // 模擬 32 個暫存器
vector<string> Cyc(5, "NOP"); // 管線暫存器，初始為 NOP 指令
vector<int> memory(1024, 0); // 模擬記憶體空間

// 暫存器結構
struct IF_ID_Register {
    string instruction;
    int pc;
} IF_ID;

struct ID_EX_Register {
    int readData1;
    int readData2;
    int immediate;
    int rs, rt, rd;
    string ALUOp;
    bool RegWrite = false;
    bool MemRead = false;
    bool MemWrite = false;
    bool Branch = false;
} ID_EX;

struct EX_MEM_Register {
    int ALUResult;
    int writeData;
    int rd;
    bool RegWrite = false;
    bool MemRead = false;
    bool MemWrite = false;
} EX_MEM;

struct MEM_WB_Register {
    int readData;
    int ALUResult;
    int rd;
    bool RegWrite = false;
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
void IF(const string& resultFilename) {
    ofstream outfile(resultFilename, ios::app);
    if (IF_PCSrc == 1) {
        PC = branchTargetAddress;
        Cyc[0] = "NOP";
        outfile << "IF : NOP (branch)" << endl;
        IF_PCSrc = 0;
        return;
    }
    if (IF_stall_count > 0) {
        outfile << "IF : " << getOpcode(Cyc[0]) << " (stalled)" << endl;
        IF_stall_count--;
        return;
    }
    if (PC >= instructions.size()) {
        Cyc[0] = "NOP";
        outfile << "IF : NOP" << endl;
        return;
    }
    Cyc[0] = instructions[PC];
    IF_ID.instruction = Cyc[0];
    IF_ID.pc = PC;
    PC++;
    outfile << "IF : " << getOpcode(Cyc[0]) << endl;
    outfile.close();
}

// 生成控制信號
void generateControlSignals(const string& opcode, ID_EX_Register& idExReg) {
    if (opcode == "R") {
        idExReg.ALUOp = "R";
        idExReg.RegWrite = true;
    }
    else if (opcode == "LW") {
        idExReg.ALUOp = "ADD";
        idExReg.RegWrite = true;
        idExReg.MemRead = true;
    }
    else if (opcode == "SW") {
        idExReg.ALUOp = "ADD";
        idExReg.MemWrite = true;
    }
    else if (opcode == "BEQ") {
        idExReg.ALUOp = "SUB";
        idExReg.Branch = true;
    }
}

// ID 階段
void ID(const string& resultFilename) {
    ofstream outfile(resultFilename, ios::app);
    if (IF_ID.instruction == "NOP") {
        outfile << "ID : NOP" << endl;
        return;
    }
    Instruction instr = parseInstruction(IF_ID.instruction);
    ID_EX.readData1 = registers[instr.rs];
    ID_EX.readData2 = registers[instr.rt];
    ID_EX.rs = instr.rs;
    ID_EX.rt = instr.rt;
    ID_EX.rd = instr.rd;
    ID_EX.immediate = instr.immediate;
    generateControlSignals(instr.opcode, ID_EX);
    outfile << "ID : opcode=" << instr.opcode << endl;
    outfile.close();
}

// EX 階段
void EX(const string& resultFilename) {
    ofstream outfile(resultFilename, ios::app);
    if (ID_EX.ALUOp.empty()) {
        outfile << "EX : NOP" << endl;
        return;
    }
    int ALUInput1 = ID_EX.readData1;
    int ALUInput2 = ID_EX.readData2;

    //predict not taken
    if (ID_EX.ALUOp == "SUB" && ID_EX.Branch) { // 處理 BEQ 指令
        if (ALUInput1 == ALUInput2) { // 分支條件成立
            IF_PCSrc = 1;
            branchTargetAddress = IF_ID.pc + (ID_EX.immediate << 2); // 計算目標地址
        }
    }
    else if (ID_EX.ALUOp == "ADD") {
        EX_MEM.ALUResult = ALUInput1 + ALUInput2;
    }
    else if (ID_EX.ALUOp == "SUB") {
        EX_MEM.ALUResult = ALUInput1 - ALUInput2;
    }
    EX_MEM.writeData = ID_EX.readData2;
    EX_MEM.rd = ID_EX.rd;
    EX_MEM.RegWrite = ID_EX.RegWrite;
    EX_MEM.MemRead = ID_EX.MemRead;
    EX_MEM.MemWrite = ID_EX.MemWrite;
    outfile << "EX : ALUResult=" << EX_MEM.ALUResult << endl;
    outfile.close();
}

// MEM 階段
void MEM(const string& resultFilename) {
    ofstream outfile(resultFilename, ios::app);
    if (!EX_MEM.MemRead && !EX_MEM.MemWrite) {
        outfile << "MEM : NOP" << endl;
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
    outfile << "MEM : " << (EX_MEM.MemRead ? "Read" : "Write") << endl;
    outfile.close();
}

// WB 階段
void WB(const string& resultFilename) {
    ofstream outfile(resultFilename, ios::app);
    if (!MEM_WB.RegWrite) {
        outfile << "WB : NOP" << endl;
        return;
    }
    registers[MEM_WB.rd] = MEM_WB.readData ? MEM_WB.readData : MEM_WB.ALUResult;
    outfile << "WB : RegWrite to R" << MEM_WB.rd << endl;
    outfile.close();
}

int main() {
    string instructionFile = "test3.txt";
    string resultFile = "result2.txt";
    loadInstructions(instructionFile);

    ofstream clearFile(resultFile, ios::trunc);
    clearFile.close();

    for (int cycle = 0; cycle < instructions.size() + 4; ++cycle) {
        WB(resultFile);
        MEM(resultFile);
        EX(resultFile);
        ID(resultFile);
        IF(resultFile);
    }

    cout << "模擬完成，結果已輸出至 " << resultFile << endl;
    return 0;
}
