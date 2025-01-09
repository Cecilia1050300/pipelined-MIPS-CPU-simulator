#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <algorithm>
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

// 解析指令
struct Instruction {
    string opcode;
    int rs = 0, rt = 0, rd = 0, immediate = 0;
};
Instruction parseInstruction(const string& instruction) {
    Instruction instr;
    istringstream stream(instruction);
    stream >> instr.opcode;
    if (instr.opcode == "LW" || instr.opcode == "SW") {
        stream >> instr.rt >> instr.immediate >> instr.rs;
    }
    else if (instr.opcode == "BEQ") {
        stream >> instr.rs >> instr.rt >> instr.immediate;
    }
    else { // 算術指令
        stream >> instr.rd >> instr.rs >> instr.rt;
    }
    return instr;
}

// IF 階段
void IF(const string& resultFilename) {
    if (IF_stall_count > 0) {
        IF_stall_count--;
        return;
    }

    //預測未成立
    if (IF_PCSrc == 1) { //跳轉目標已決定
        PC = branchTargetAddress;
        Cyc[0] = "NOP"; //沖刷錯誤指令
        IF_PCSrc = 0;
        return;
    }

    //執行下一條指令
    if (PC >= instructions.size()) {
        Cyc[0] = "NOP";
        return;
    }

    Cyc[0] = instructions[PC];
    IF_ID.instruction = Cyc[0];
    IF_ID.pc = PC;
    PC++;
}

// ID 階段
void ID(const string& resultFilename) {
    if (IF_ID.instruction == "NOP") return;

    Instruction instr = parseInstruction(IF_ID.instruction);
    ID_EX.readData1 = registers[instr.rs];
    ID_EX.readData2 = registers[instr.rt];
    ID_EX.rs = instr.rs;
    ID_EX.rt = instr.rt;
    ID_EX.rd = instr.rd;
    ID_EX.immediate = instr.immediate;

    if (instr.opcode == "BEQ") {
        ID_EX.ALUOp = "SUB";
        ID_EX.Branch = true;

        if (registers[instr.rs] == registers[instr.rt]) {
            IF_PCSrc = 1;
            branchTargetAddress = IF_ID.pc + (instr.immediate << 2);
        }
    }
    else if (instr.opcode == "LW") {
        ID_EX.ALUOp = "ADD";
        ID_EX.MemRead = true;
        ID_EX.RegWrite = true;
    }
    else if (instr.opcode == "SW") {
        ID_EX.ALUOp = "ADD";
        ID_EX.MemWrite = true;
    }
    else if (instr.opcode == "ADD" || instr.opcode == "SUB") {
        ID_EX.ALUOp = instr.opcode;
        ID_EX.RegWrite = true;
    }

    if (instr.opcode == "BEQ" && registers[instr.rs] == registers[instr.rt]) {
        IF_PCSrc = 1;
        branchTargetAddress = IF_ID.pc + (instr.immediate << 2);
    }
}

// EX 階段
void EX(const string& resultFilename) {
    if (ID_EX.ALUOp.empty()) return;

    int ALUInput1 = ID_EX.readData1;
    int ALUInput2 = ID_EX.MemRead || ID_EX.MemWrite ? ID_EX.immediate : ID_EX.readData2;

    if (ID_EX.ALUOp == "ADD") {
        EX_MEM.ALUResult = ALUInput1 + ALUInput2;
    }
    else if (ID_EX.ALUOp == "SUB") {
        EX_MEM.ALUResult = ALUInput1 - ALUInput2;
    }

    EX_MEM.writeData = ID_EX.readData2;
    EX_MEM.rd = ID_EX.rd;
    EX_MEM.MemRead = ID_EX.MemRead;
    EX_MEM.MemWrite = ID_EX.MemWrite;
    EX_MEM.RegWrite = ID_EX.RegWrite;
}

// MEM 階段
void MEM(const string& resultFilename) {
    if (!EX_MEM.MemRead && !EX_MEM.MemWrite) return;

    if (EX_MEM.MemRead) {
        MEM_WB.readData = memory[EX_MEM.ALUResult];
    }
    if (EX_MEM.MemWrite) {
        memory[EX_MEM.ALUResult] = EX_MEM.writeData;
    }

    MEM_WB.ALUResult = EX_MEM.ALUResult;
    MEM_WB.rd = EX_MEM.rd;
    MEM_WB.RegWrite = EX_MEM.RegWrite;
}

// WB 階段
void WB(const string& resultFilename) {
    if (!MEM_WB.RegWrite) return;

    if (MEM_WB.readData != 0) {
        registers[MEM_WB.rd] = MEM_WB.readData;
    }
    else {
        registers[MEM_WB.rd] = MEM_WB.ALUResult;
    }
}

void printPipeline(ofstream& outfile, string stages[], int cycle) {
    outfile << "Cycle " << cycle << endl;

    for (int i = 0; i < 5; ++i) {
        if (stages[i] != "NOP") {
            string stageName;
            switch (i) {
            case 0: stageName = "IF"; break;
            case 1: stageName = "ID"; break;
            case 2: stageName = "EX"; break;
            case 3: stageName = "MEM"; break;
            case 4: stageName = "WB"; break;
            }
            outfile << stages[i] << ":" << stageName;

            // 根據階段附加信號值
            if (stageName == "EX") {
                outfile << " RegDst=" << (ID_EX.RegWrite ? "1" : "0")
                    << " ALUSrc=" << (ID_EX.MemRead || ID_EX.MemWrite ? "1" : "0")
                    << " Branch=" << (ID_EX.Branch ? "1" : "0")
                    << " MemRead=" << (ID_EX.MemRead ? "1" : "0")
                    << " MemWrite=" << (ID_EX.MemWrite ? "1" : "0")
                    << " RegWrite=" << (ID_EX.RegWrite ? "1" : "0")
                    << " MemtoReg=" << (ID_EX.MemRead ? "1" : "0");
            }
            else if (stageName == "MEM") {
                outfile << " Branch=" << (EX_MEM.MemRead ? "1" : "0")
                    << " MemRead=" << (EX_MEM.MemRead ? "1" : "0")
                    << " MemWrite=" << (EX_MEM.MemWrite ? "1" : "0")
                    << " RegWrite=" << (EX_MEM.RegWrite ? "1" : "0")
                    << " MemtoReg=" << (EX_MEM.MemRead ? "1" : "0");
            }
            else if (stageName == "WB") {
                outfile << " RegWrite=" << (MEM_WB.RegWrite ? "1" : "0")
                    << " MemtoReg=" << (MEM_WB.readData != 0 ? "1" : "X");
            }
            outfile << endl;
        }
    }
    outfile << "-----------------------------\n";
}


int main() {
    string instructionFile = "test3.txt";
    string resultFile = "result444.txt";
    loadInstructions(instructionFile);

    // 初始化暫存器與記憶體
    registers[0] = 0;
    memory[8] = 10;
    memory[16] = 20;

    // 清空輸出檔案
    ofstream clearFile(resultFile, ios::trunc);
    clearFile.close();

    int totalInstructions = instructions.size();
    int totalCycles = totalInstructions + 4; // 考慮指令數 + 管線填充週期

    // 初始化管線階段狀態
    string stages[5] = { "NOP", "NOP", "NOP", "NOP", "NOP" };

    // 週期迴圈
    for (int cycle = 1; cycle <= totalCycles; ++cycle) {
        ofstream outfile(resultFile, ios::app);

        // 執行各階段函式
        WB(resultFile);
        MEM(resultFile);
        EX(resultFile);
        ID(resultFile);
        IF(resultFile);

        // 更新階段狀態
        stages[4] = stages[3]; // WB <- MEM
        stages[3] = stages[2]; // MEM <- EX
        stages[2] = stages[1]; // EX <- ID
        stages[1] = stages[0]; // ID <- IF
        stages[0] = Cyc[0];    // IF <- 新取指令

        // 輸出當前週期流水線狀態
        printPipeline(outfile, stages, cycle);

        outfile.close();
    }

    // 最終結果輸出
    ofstream outfile(resultFile, ios::app);
    outfile << "## Final Result:\n";
    outfile << "Total Cycles: " << totalCycles << "\n\n";

    outfile << "Final Register Values:\n";
    for (int i = 0; i < 32; ++i) {
        outfile << registers[i] << " ";
    }
    outfile << "\n\n";

    outfile << "Final Memory Values:\n";
    for (int i = 0; i < 32; ++i) { // 假設需要顯示前 32 個記憶體位置
        outfile << memory[i] << " ";
    }
    outfile << "\n";
    outfile.close();

    cout << "模擬完成，結果已輸出至 " << resultFile << endl;
    return 0;
}
