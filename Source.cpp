#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

const int NUM_REGISTERS = 32;  // 定義寄存器數量
const int MEMORY_SIZE = 32;    // 定義記憶體大小

// instruction結構
struct Instruction {
    string op;      // 指令操作（ "add", "sub", "lw", "sw", "beq"）
    int rd, rs, rt; // 暫存器器
    int immediate;  // 立即數（使用於立即數操作或者分支指令）
    int address;    // 地址（記憶體存取）

    // 初始化
    Instruction() : op(""), rd(-1), rs(-1), rt(-1), immediate(-1), address(-1) {}

    // 帶參數的建構函數，用於初始化指令的各個成員
    Instruction(const string& op, int rd, int rs, int rt, int immediate, int address = -1)
        : op(op), rd(rd), rs(rs), rt(rt), immediate(immediate), address(address) {}
};

// 處理輸入資料
// 去除字串首尾空白字元
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t");  // 找到第一個非空白字元
    size_t last = str.find_last_not_of(" \t");   // 找到最後一個非空白字元
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
}

// 分析指令
Instruction parseInstruction(const string& line) {
    Instruction inst;
    istringstream iss(line);
    string token;

    try {
        // 解析 "add", "sub", "lw"
        if (!(iss >> inst.op)) {
            throw runtime_error("Missing operation");
        }

        // 根據不同的操作符解析不同的指令格式
        if (inst.op == "lw" || inst.op == "sw") {  // Load/Store
            string rd, offset;
            if (!(iss >> rd >> offset)) {
                throw runtime_error("Invalid lw/sw format");
            }

            rd = trim(rd);
            rd.pop_back(); // 去掉逗號

            //得出括號內的值
            size_t start = offset.find('(');
            size_t end = offset.find(')');

            if (start == string::npos || end == string::npos) {
                throw runtime_error("Invalid memory access format");
            }

            inst.rd = stoi(rd.substr(1)); // 提取寄存器編號
            inst.address = stoi(offset.substr(0, start));
            inst.rs = stoi(offset.substr(start + 2, end - start - 2));
        }
        else if (inst.op == "beq") {
            string rs, rt;
            if (!(iss >> rs >> rt >> inst.immediate)) {
                throw runtime_error("Invalid beq format");
            }

            rs = trim(rs);
            rt = trim(rt);
            rs.pop_back();  // 去掉逗號
            rt.pop_back();  // 去掉逗號

            inst.rs = stoi(rs.substr(1));  // 提取寄存器編號
            inst.rt = stoi(rt.substr(1));  // 提取寄存器編號
        }
        else if (inst.op == "add" || inst.op == "sub") {  // 算術運算
            string rd, rs, rt;
            if (!(iss >> rd >> rs >> rt)) {
                throw runtime_error("Invalid add/sub format");
            }

            rd = trim(rd);
            rs = trim(rs);
            rt = trim(rt);

            // 去除末尾的逗號
            if (rd.back() == ',') rd.pop_back();
            if (rs.back() == ',') rs.pop_back();
            if (rt.back() == ',') rt.pop_back();

            // 檢查寄存器格式是否正確
            if (rd[0] != '$' || rs[0] != '$' || rt[0] != '$') {
                throw runtime_error("Invalid register format: " + rd + ", " + rs + ", " + rt);
            }

            inst.rd = stoi(rd.substr(1));  // 提取並轉換寄存器編號
            inst.rs = stoi(rs.substr(1));
            inst.rt = stoi(rt.substr(1));
        }
        else if (inst.op == "nop") {  // No operation (空操作)
            // 無需處理
        }
        else {
            throw runtime_error("Unknown operation: " + inst.op);
        }
    }
    catch (const exception& e) {
        cerr << "Error parsing line: " << line << ". Error: " << e.what() << endl;
        inst.op = "nop";  // 當解析錯誤時，將指令設置為 "nop"
    }

    return inst;
}

// pipeline
struct Pipeline {
    vector<int> registers;  // 寄存器數組
    vector<int> memory;     // 記憶體數組
    vector<Instruction> instructions;  // 儲存指令的容器
    queue<Instruction> pipeline[5];  // 流水線的五個階段
    int pc;  // 計數器
    int cycle;  // 週期紀錄
    vector<string> a_Output;  // 輸出記錄

    Pipeline() {
        registers.resize(NUM_REGISTERS, 1);  // 初始化寄存器
        registers[0] = 0;  // $0 寄存器為 0
        memory.resize(MEMORY_SIZE, 1);  // 初始化記憶體
        pc = 0;  // 設置程式計數器為 0
        cycle = 0;  // 設置初始週期為 0
    }

    // 載入指令
    void loadInstructions(const vector<Instruction>& insts) {
        instructions = insts;
    }


    void IF();
    bool ID();
    void EX();
    void MEM();
    void WB();
    void cyclecount(ofstream& output);
};

// 控制信號
string controlSignals(const string& op) {
    if (op == "lw")  return "RegDst=0 ALUSrc=1 Branch=0 MemRead=1 MemWrite=0 RegWrite=1 MemToReg=1";
    if (op == "sw")  return "RegDst=X ALUSrc=1 Branch=0 MemRead=0 MemWrite=1 RegWrite=0 MemToReg=X";
    if (op == "add") return "RegDst=1 ALUSrc=0 Branch=0 MemRead=0 MemWrite=0 RegWrite=1 MemToReg=0";
    if (op == "sub") return "RegDst=1 ALUSrc=0 Branch=0 MemRead=0 MemWrite=0 RegWrite=1 MemToReg=0";
    if (op == "beq") return "RegDst=X ALUSrc=0 Branch=1 MemRead=0 MemWrite=0 RegWrite=0 MemToReg=X";
    if (op == "nop") return "NOP";
    return "RegDst=X ALUSrc=X Branch=X MemRead=X MemWrite=X RegWrite=X MemToReg=X";
}

// IF 階段
void Pipeline::IF() {
    // 檢查程式計數器是否超出指令範圍
    if (pc < (int)instructions.size()) {
        Instruction inst = instructions[pc];
        pipeline[0].push(inst);           // 將指令放入 IF 階段（pipeline[0]）
        a_Output.push_back(inst.op + " IF");
        pc++;  // 預測不跳轉：抓取下一條指令
    }
}

// ID階段 & hazard偵測
bool Pipeline::ID() {
    if (pipeline[0].empty()) return false;

    Instruction inst = pipeline[0].front();
    bool needsStall = false;

    // 檢查 EX 階段
    if (!pipeline[2].empty()) {
        Instruction prevEX = pipeline[2].front();
        bool prevEXWritesRegister =
            (prevEX.op == "add" || prevEX.op == "sub" || prevEX.op == "lw");

        // 如果 EX 階段寫入的寄存器是當前指令的 rs 或 rt，則需要 stall
        if (prevEXWritesRegister && prevEX.rd != 0) {
            if (prevEX.rd == inst.rs || prevEX.rd == inst.rt) {
                if (prevEX.op == "lw") {
                    needsStall = true;  // data hazard
                }
            }
        }
    }

    // 檢查 MEM 階段
    if (!pipeline[3].empty()) {
        Instruction prevMEM = pipeline[3].front();
        bool prevMEMWritesRegister =
            (prevMEM.op == "add" || prevMEM.op == "sub" || prevMEM.op == "lw");

        // 如果 MEM 階段寫入的寄存器是當前指令的 rs 或 rt，則需要 stall
        if (prevMEMWritesRegister && prevMEM.rd != 0) {
            if (prevMEM.rd == inst.rs || prevMEM.rd == inst.rt) {
                if (prevMEM.op == "lw") {
                    needsStall = true;
                }
            }
        }
    }


    if (inst.op == "beq") {
        // 檢查 EX 階段 (pipeline[2])
        if (!pipeline[2].empty()) {
            Instruction exInst = pipeline[2].front();
            if (exInst.rd != 0 && (exInst.rd == inst.rs || exInst.rd == inst.rt)) {
                if (exInst.op == "sub" || exInst.op == "lw") {
                    needsStall = true;
                }
            }
        }

        // 檢查 MEM 階段 (pipeline[3])
        if (!pipeline[3].empty()) {
            Instruction memInst = pipeline[3].front();
            if (memInst.rd != 0 && (memInst.rd == inst.rs || memInst.rd == inst.rt)) {
                if (memInst.op == "lw") {
                    needsStall = true;
                }
            }
        }
    }

    if (needsStall) {
        Instruction bubble = { "nop", -1, -1, -1, -1 };
        pipeline[1].push(bubble);
        a_Output.push_back(inst.op + " ID");
        return true;  // 回傳 true 表示需要 stall
    }

    pipeline[0].pop();      // IF -> 移除已處理的指令
    pipeline[1].push(inst); // ID <- 放入當前指令
    a_Output.push_back(inst.op + " ID");
    return false;
}

// EX 階段
void Pipeline::EX() {
    if (pipeline[1].empty()) return;

    Instruction inst = pipeline[1].front();
    pipeline[1].pop();

    if (inst.op == "nop") {
        pipeline[2].push(inst);
        return;
    }

    if (inst.op == "beq") {
        int rsValue = registers[inst.rs];
        int rtValue = registers[inst.rt];

        if (rsValue == rtValue) {
            // 分支被執行，調整程式計數器
            pc = pc + inst.immediate - 1;
            while (!pipeline[0].empty()) {
                pipeline[0].pop();
            }
        }
    }
    else if (inst.op == "add") {
        registers[inst.rd] = registers[inst.rs] + registers[inst.rt];
    }
    else if (inst.op == "sub") {
        registers[inst.rd] = registers[inst.rs] - registers[inst.rt];
    }

    pipeline[2].push(inst);
    a_Output.push_back(inst.op + " EX " + controlSignals(inst.op));
}

// MEM 階段
void Pipeline::MEM() {
    if (pipeline[2].empty()) return;

    Instruction inst = pipeline[2].front();
    pipeline[2].pop();

    if (inst.op == "nop") {
        pipeline[3].push(inst);
        return;
    }

    if (inst.op == "lw") {
        // 記憶體讀取
        registers[inst.rd] = memory[inst.address / 4];
    }
    else if (inst.op == "sw") {
        // 記憶體寫入
        memory[inst.address / 4] = registers[inst.rs];
    }

    pipeline[3].push(inst);
    a_Output.push_back(inst.op + " MEM " + controlSignals(inst.op));
}

// WB 階段
void Pipeline::WB() {
    if (pipeline[3].empty()) return;

    Instruction inst = pipeline[3].front();
    pipeline[3].pop();

    if (inst.op == "nop") {
        return;
    }

    a_Output.push_back(inst.op + " WB " + controlSignals(inst.op));
}

// 週期計算並輸出結果
void Pipeline::cyclecount(ofstream& output) {
    a_Output.clear();

    WB();
    MEM();
    EX();

    bool stall = ID();
    if (!stall) {
        IF();
    }

    output << "Cycle " << ++cycle << ":\n";
    for (const auto& out : a_Output) {
        output << out << endl;
    }
    output << endl;
}

int main() {
    Pipeline p;

    ifstream input("test6.txt");  // 載入指令檔案
    ofstream output("test6output.txt");  // 輸出結果

    if (!input.is_open() || !output.is_open()) {
        cerr << "Error opening input or output file." << endl;
        return 1;
    }

    vector<Instruction> instructions;
    string line;
    while (getline(input, line)) {
        instructions.push_back(parseInstruction(line));
    }

    p.loadInstructions(instructions);

    // 持續模擬直到程式結束
    while (p.pc < (int)instructions.size() ||
        !p.pipeline[0].empty() ||
        !p.pipeline[1].empty() ||
        !p.pipeline[2].empty() ||
        !p.pipeline[3].empty())
    {
        p.cyclecount(output);
    }

    // 輸出最終結果
    output << "## Final Result:" << endl;
    output << "Total Cycles: " << p.cycle << endl;

    output << endl << "Final Register Values:" << endl;
    for (int i = 0; i < NUM_REGISTERS; i++) {
        output << p.registers[i] << " ";
    }
    output << endl;

    output << endl << "Final Memory Values:" << endl;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        output << p.memory[i] << " ";
    }
    output << endl;

    input.close();
    output.close();
    return 0;
}









