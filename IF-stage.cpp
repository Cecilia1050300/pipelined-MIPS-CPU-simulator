#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
using namespace std;

// 模擬全域變數
int PC = 0; // 程式計數器
int IF_PCSrc = 0; // 分支跳轉控制信號
int branchTargetAddress = 0; // 分支目標地址
int IF_stall_count = 0; // 暫停計數器
vector<string> instructions; // 指令記憶體
vector<string> Cyc(5, "NOP"); // 管線暫存器，初始為 NOP 指令

// IF/ID 暫存器 (模擬管線的傳遞)
struct IF_ID_Register {
    string instruction;
    int pc;
} IF_ID;

// 讀取指令記憶體
void loadInstructions(const string& filename) {
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        instructions.push_back(line);
    }
    infile.close();
}

// 取得指令的操作碼（如 lw, beq, add 等）
string getOpcode(const string& instruction) {
    istringstream stream(instruction);
    string opcode;
    stream >> opcode; // 讀取指令的操作碼
    return opcode;
}

// 取指令 (IF 階段)
void IF(const string& resultFilename) {
    ofstream outfile(resultFilename, ios::app);

    // 如果有跳轉信號，清空當前階段，並根據跳轉邏輯更新 PC
    if (IF_PCSrc == 1) {
        PC = branchTargetAddress; // 更新 PC 為跳轉目標地址
        Cyc[0] = "NOP";           // 當前階段插入 NOP
        outfile << "IF : NOP (branch)" << endl;
        IF_PCSrc = 0;             // 清除跳轉信號
        return;
    }

    // 如果有暫停信號，維持當前階段內容不變
    if (IF_stall_count > 0) {
        outfile << "IF : " << getOpcode(Cyc[0]) << " (stalled)" << endl;
        IF_stall_count--;
        return;
    }

    // 如果 PC 超過指令範圍，填入 NOP
    if (PC >= instructions.size()) {
        Cyc[0] = "NOP";
        outfile << "IF : NOP" << endl; // 明確輸出 NOP 的 IF 階段
        return;
    }

    // 讀取指令記憶體中的指令
    Cyc[0] = instructions[PC];
    IF_ID.instruction = Cyc[0]; // 將指令存入 IF/ID 暫存器
    IF_ID.pc = PC;             // 將 PC 存入 IF/ID 暫存器
    PC++;                      // 更新 PC

    // 輸出取指令階段的結果到檔案
    outfile << "IF : " << getOpcode(Cyc[0]) << endl;
    outfile.close();
}

int main() {
    // 初始化指令記憶體
    string instructionFile = "test3.txt";
    string resultFile = "result2.txt";
    loadInstructions(instructionFile);

    // 每次模擬執行前清空結果檔案
    ofstream clearFile(resultFile, ios::trunc);
    clearFile.close();

    // 模擬 IF 階段執行
    for (int cycle = 0; cycle < instructions.size() + 4; ++cycle) { // 模擬額外週期以填補 NOP
        IF(resultFile);
    }

    cout << "模擬完成，結果已輸出至 " << resultFile << endl;
    return 0;
}
