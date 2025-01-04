#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

// 指令結構
struct Instruction {
    string opcode;  // 操作碼 (如 R, LW, SW, BEQ)
    int rs;         // 來源暫存器1
    int rt;         // 來源暫存器2或目標暫存器
    int rd;         // 目標暫存器 (僅對 R-format指令有效)
    int immediate;  // 立即值 (僅對 I-format指令有效)
};

// ID/EX 暫存器 (用於 ID -> EX 資料傳遞)
struct ID_EX_Register {
    int readData1;            // 從暫存器讀取的資料1
    int readData2;            // 從暫存器讀取的資料2
    int immediate;            // 解碼後的立即值
    int rs, rt, rd;           // 暫存器編號
    string ALUOp;             // ALU 操作
    bool RegWrite = false;    // 是否寫回暫存器
    bool MemRead = false;     // 是否讀記憶體
    bool MemWrite = false;    // 是否寫記憶體
    bool Branch=false;        // 是否為分支指令
};

// 模擬暫存器檔案
vector<int> registers(32, 0); // 32 個暫存器

// 控制信號生成器，操作碼解析
void generateControlSignals(const string& opcode, ID_EX_Register& idExReg) {
    if (opcode == "R") { // R-format指令
        idExReg.ALUOp = "R";
        idExReg.RegWrite = true;
    }
    else if (opcode == "LW") { // Load Word
        idExReg.ALUOp = "ADD";
        idExReg.RegWrite = true;
        idExReg.MemRead = true;
    }
    else if (opcode == "SW") { // Store Word
        idExReg.ALUOp = "ADD";
        idExReg.MemWrite = true;
    }
    else if (opcode == "BEQ") { // Branch if Equal
        idExReg.ALUOp = "SUB";
        idExReg.Branch = true;
    }
}

// ID 階段模擬
void instructionDecode(const Instruction& instr, ID_EX_Register& idExReg) {
    // 從暫存器檔案讀取資料
    idExReg.readData1 = registers[instr.rs]; //解析操作數
    idExReg.readData2 = registers[instr.rt]; //解析操作數
    idExReg.rs = instr.rs;
    idExReg.rt = instr.rt;
    idExReg.rd = instr.rd;
    idExReg.immediate = instr.immediate;

    // 生成控制信號
    generateControlSignals(instr.opcode, idExReg);

    cout << "[ID] 解碼完成，控制信號與資料已寫入 ID/EX 暫存器。\n";
}



// 主程式
int main() {
    // 初始化暫存器檔案 (模擬)
    registers[8] = 5;  // $t0
    registers[9] = 10; // $t1
    registers[10] = 15; // $t2

    // 指令集合 (模擬)
    vector<Instruction> instructions = {
        {"R", 9, 10, 8, 0},       // ADD $t0, $t1, $t2
        {"LW", 9, 8, 0, 4},       // LW $t0, 4($t1)
        {"SW", 8, 9, 0, 4},       // SW $t0, 4($t1)
        {"BEQ", 8, 9, 0, 2}       // BEQ $t0, $t1, offset 2
    };

    ID_EX_Register idExReg; // ID -> EX 暫存器

    for (const auto& instr : instructions) {
        cout << "====================\n";
        cout << "處理指令：" << instr.opcode << "\n";

        // ID 階段
        instructionDecode(instr, idExReg);

        
    }

    return 0;
}
