#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <queue>
using namespace std;

ofstream outfile;
fstream file;

stringstream ss;
int registers[32] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
int memory[32] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
queue<string> instructions;
int cycle = 1;
bool EXHazard_Rs = false;
bool EXHazard_Rt = false;
bool MEMHazard_Rs = false;
bool MEMHazard_Rt = false;
bool LoadUseHazard = false;

struct PipelineStage {
    string Op = "";        // 操作碼（指令類型，如 "lw", "add" 等）
    string Instruction;

    int Rs = 0;            // 寄存器1
    int Rt = 0;            // 寄存器2
    int Rd = 0;            // 寄存器
    int PC = 0;

    int Immediate = 0;     // 立即數
    int ALUResult = 0;     // ALU 計算結果
    int ReadData = 0;      
    int ReadData1 = 0;
    int ReadData2 = 0;

    bool RegWrite = false; // 是否寫回寄存器
    bool MemtoReg = false; // 是否從記憶體寫回寄存器
    bool MemRead = false;  // 記憶體讀取
    bool MemWrite = false; // 記憶體寫入
    bool Branch = false;   // 分支指令
    bool ALUSrc = false;   // 是否使用立即數
    bool RegDst = false;   // 註冊目的位址
    bool nop = false;      // NOP (空操作，通常用於防止衝突)
};

PipelineStage IF, ID, EX, MEM, WB;

int parseRegister(const string& reg) {
    return stoi(reg.substr(1));  
}

int parseImmediate(const string& part) {
    size_t openParen = part.find('(');
    return stoi(part.substr(0, openParen));
}

int parseBaseRegister(const string& part) {
    size_t openParen = part.find('(');
    size_t closeParen = part.find(')');
    return stoi(part.substr(part.find('$') + 1, closeParen - openParen - 2));
}

void IF_state() {
    if (!instructions.empty()) {
        IF.Instruction = instructions.front();
        instructions.pop();
    }
    else {
        IF.Instruction = ""; 
    }

    string reg1, reg2, reg3_or_offset;
    ss.str(IF.Instruction);
    ss.clear();
    ss >> IF.Op;

    if (IF.Op == "lw" || IF.Op == "sw") {
        ss >> reg1 >> reg2;
        IF.Rt = parseRegister(reg1);
        IF.Rs = parseBaseRegister(reg2);
        IF.Immediate = parseImmediate(reg2);
    }
    else if (IF.Op == "add" || IF.Op == "sub") {
        ss >> reg1 >> reg2 >> reg3_or_offset;
        IF.Rd = parseRegister(reg1);
        IF.Rs = parseRegister(reg2);
        IF.Rt = parseRegister(reg3_or_offset);
    }
    else if (IF.Op == "beq") {
        ss >> reg1 >> reg2 >> reg3_or_offset;
        IF.Rs = parseRegister(reg1);
        IF.Rt = parseRegister(reg2);
    }

    ss.str("");
    ss.clear();
}

void ID_state() {
    if (LoadUseHazard) {
        ID.nop = true;
        return; 
    }

    ID.nop = IF.nop;
    ss << IF.Instruction;
    ss >> ID.Op; 

    if (ID.Op == "lw") {
        string regPart, memoryPart;
        ss >> regPart;
        ss.ignore();
        ss >> memoryPart;

        ID.Rt = parseRegister(regPart);
        ID.Rs = parseBaseRegister(memoryPart);
        ID.Immediate = parseImmediate(memoryPart);
    }
    else if (ID.Op == "sw") {
        string regPart, memoryPart;
        ss >> regPart;
        ss.ignore();
        ss >> memoryPart;

        ID.Rt = parseBaseRegister(memoryPart);
        ID.Rs = parseRegister(regPart);
        ID.Immediate = parseImmediate(memoryPart);
    }
    else if (ID.Op == "add" || ID.Op == "sub") {
        string rd, rs, rt;
        ss >> rd >> rs >> rt;

        ID.Rd = parseRegister(rd);
        ID.Rs = parseRegister(rs);
        ID.Rt = parseRegister(rt);
    }
    else if (ID.Op == "beq") {
        string rs, rt, offset;
        ss >> rs >> rt >> offset;

        ID.Rs = parseRegister(rs);
        ID.Rt = parseRegister(rt);
        ID.Immediate = parseImmediate(offset);
    }

    ss.str("");
    ss.clear();
    IF = PipelineStage();
}

void EX_state() {
    EX.nop = ID.nop;
    EX.Op = ID.Op;

    int forwardRsValue = registers[ID.Rs];
    int forwardRtValue = registers[ID.Rt];

    if (EXHazard_Rs) forwardRsValue = EX.ALUResult;
    else if (MEMHazard_Rs) forwardRsValue = MEM.ALUResult;

    if (EXHazard_Rt) forwardRtValue = EX.ALUResult;
    else if (MEMHazard_Rt) forwardRtValue = MEM.ALUResult;

    if (EX.RegWrite && EX.Rd != 0 && EX.Rd == ID.Rs) {
        ID.ReadData1 = EX.ALUResult; 
    }
    if (MEM.RegWrite && MEM.Rd != 0 && MEM.Rd == ID.Rs) {
        ID.ReadData1 = MEM.ReadData; 
    }

    if (EX.Op == "beq") {
        if (forwardRsValue == forwardRtValue) {
            IF.PC += EX.Immediate; 
            IF = PipelineStage();
            ID = PipelineStage();
        }
    }

    if (EX.Op == "lw") {
        EX.Rs = ID.Rs;
        EX.Rt = ID.Rt;
        EX.Immediate = ID.Immediate;
        EX.RegDst = 0; EX.ALUSrc = 1; EX.MemtoReg = 1; EX.RegWrite = 1; EX.MemRead = 1; EX.MemWrite = 0; EX.Branch = 0;
        EX.ALUResult = forwardRsValue + EX.Immediate / 4;
    }
    else if (EX.Op == "sw") {
        EX.Rs = ID.Rs;
        EX.Rt = ID.Rt;
        EX.Immediate = ID.Immediate;
        EX.RegDst = 0; EX.ALUSrc = 1; EX.MemtoReg = 0; EX.RegWrite = 0; EX.MemRead = 0; EX.MemWrite = 1; EX.Branch = 0;
        EX.ALUResult = forwardRsValue + EX.Immediate / 4;
    }
    else if (EX.Op == "add" || EX.Op == "sub") {
        EX.Rd = ID.Rd;
        EX.Rs = ID.Rs;
        EX.Rt = ID.Rt;
        EX.RegDst = 1; EX.ALUSrc = 0; EX.MemtoReg = 0; EX.RegWrite = 1; EX.MemRead = 0; EX.MemWrite = 0; EX.Branch = 0;

        if (EX.Op == "add") {
            EX.ALUResult = forwardRsValue + forwardRtValue;
        }
        else if (EX.Op == "sub") {
            EX.ALUResult = forwardRsValue - forwardRtValue;
        }
    }
    ID = PipelineStage();
}

void MEM_state() {
    MEM.nop = EX.nop;
    MEM.Op = EX.Op;
    MEM.Rs = EX.Rs;
    MEM.Rt = EX.Rt;
    MEM.Rd = EX.Rd;
    MEM.ALUResult = EX.ALUResult;
    MEM.RegDst = EX.RegDst;
    MEM.MemtoReg = EX.MemtoReg;
    MEM.RegWrite = EX.RegWrite;
    MEM.MemRead = EX.MemRead;
    MEM.MemWrite = EX.MemWrite;
    MEM.Branch = EX.Branch;


    if (MEM.MemRead) {
        MEM.ReadData = memory[MEM.ALUResult];
    }
    else if (MEM.MemWrite) {
        memory[MEM.ALUResult] = registers[MEM.Rs];
    }
    EX = PipelineStage();
}

void WB_state() {
    WB.nop = MEM.nop;
    WB.Op = MEM.Op;
    WB.Rs = MEM.Rs;
    WB.Rt = MEM.Rt;
    WB.Rd = MEM.Rd;
    WB.RegDst = MEM.RegDst;
    WB.RegWrite = MEM.RegWrite;
    WB.MemtoReg = MEM.MemtoReg;

    if (WB.RegWrite) {
        if (WB.MemtoReg) {
            registers[WB.Rt] = MEM.ReadData;
        }
        else {
            registers[WB.Rd] = MEM.ALUResult;
        }
    }
    MEM = PipelineStage();
}

void detectEXHazard() { // Should Forwarding
    EXHazard_Rs = false;
    EXHazard_Rt = false;

    if (EX.RegWrite == true && EX.Rd != 0) {
        if (EX.Rd == ID.Rs) {
            EXHazard_Rs = true;
        }
        if (EX.Rd == ID.Rt) {
            EXHazard_Rt = true;
        }
    }
}

void detectMEMHazard() { // Should Forwarding
    MEMHazard_Rs = false;
    MEMHazard_Rt = false;
    if (MEM.RegWrite == true && MEM.Rd != 0) {
        if (MEM.Rd == ID.Rs && !EXHazard_Rs) {
            MEMHazard_Rs = true;
        }
        if (MEM.Rd == ID.Rt && !EXHazard_Rt) {
            MEMHazard_Rt = true;
        }

    }
}

void detectLoadUseHazard() { // Should Stall (unavoidable)
    if (EX.Op == "lw" && (EX.Rt == ID.Rs || EX.Rt == ID.Rt)) {
        LoadUseHazard = true;
    }
    else if ((MEM.Op == "lw" && ID.Op == "beq") && (MEM.Rd == EX.Rs || MEM.Rd == EX.Rt)) {
        LoadUseHazard = true;
    }
    else if ((EX.Op == "add" || EX.Op == "sub") && ID.Op == "beq" && (EX.Rd == ID.Rs || EX.Rd == ID.Rt)) {
        LoadUseHazard = true;
    }
    else {
        LoadUseHazard = false;
    }

}

void printState() {
    outfile << endl << "Cycle " << cycle << ":" << endl;
    if (WB.Op != "") {
        outfile << WB.Op << " WB ";
        if (WB.Op == "sw" || WB.Op == "beq") {
            outfile << "RegWrite=" << WB.RegWrite << " MemToReg=X" << endl;
        }
        else {
            outfile << "RegWrite=" << WB.RegWrite << " MemToReg=" << WB.MemtoReg << endl;
        }
    }
    if (MEM.Op != "") {
        outfile << MEM.Op << " MEM ";
        if (MEM.Op == "sw") {
            outfile << "Branch=" << MEM.Branch << " MemRead=" << MEM.MemRead << " MemWrite=" << MEM.MemWrite << " RegWrite=" << MEM.RegWrite << " MemToReg= X" << endl;
        }
        else if (MEM.Op == "beq") {
            outfile << "Branch=1" << " MemRead=" << MEM.MemRead << " MemWrite=" << MEM.MemWrite << " RegWrite=" << MEM.RegWrite << " MemToReg= X" << endl;
        }
        else {
            outfile << "Branch=" << MEM.Branch << " MemRead=" << MEM.MemRead << " MemWrite=" << MEM.MemWrite << " RegWrite=" << MEM.RegWrite << " MemToReg=" << MEM.MemtoReg << endl;
        }
    }
    if (EX.Op != "") {
        outfile << EX.Op << " EX ";
        if (EX.Op == "sw") {
            outfile << "RegDst=X" << " ALUSrc=" << EX.ALUSrc << " Branch=" << EX.Branch << " MemRead=" << EX.MemRead << " MemWrite=" << EX.MemWrite << " RegWrite=" << EX.RegWrite << " MemToReg=X" << endl;
        }
        else if (EX.Op == "beq") {
            outfile << "RegDst=X" << " ALUSrc=" << EX.ALUSrc << " Branch=1" << " MemRead=" << EX.MemRead << " MemWrite=" << EX.MemWrite << " RegWrite=" << EX.RegWrite << " MemToReg=X" << endl;
        }
        else {
            outfile << "RegDst=" << EX.RegDst << " ALUSrc=" << EX.ALUSrc << " Branch=" << EX.Branch << " MemRead=" << EX.MemRead << " MemWrite=" << EX.MemWrite << " RegWrite=" << EX.RegWrite << " MemToReg=" << EX.MemtoReg << endl;

        }

    }
    if (ID.Op != "") {

        outfile << ID.Op << " ID " << endl;

    }
    if (IF.Instruction != "" && !IF.nop) {
        string IFInstruction;
        ss << IF.Instruction;
        ss >> IFInstruction;
        outfile << IFInstruction << " IF " << endl;
        ss.str("");
        ss.clear();
    }
}

int main() {
    outfile.open("output.txt");
    if (!outfile) {
        cerr << "Error opening output file!" << endl;
        return 1;
    }

    file.open("test6.txt");
    if (!file) {
        throw "Can't open file";
    }

    string instruction;
    while (getline(file, instruction)) {
        instructions.push(instruction);
    }
    file.close();

    while (true) {
        detectLoadUseHazard();
        detectEXHazard();
        detectMEMHazard();
        WB_state();
        MEM_state();
        // 如果沒有 Load-Use Hazard 才執行
        if (!LoadUseHazard) { 
            EX_state();
            ID_state();
            IF_state();
        }

        if (IF.Instruction == "" && ID.Op == "" && EX.Op == "" && MEM.Op == "" && WB.Op == "") {
            break;
        }
        printState();
        cycle++;
    }
    outfile << endl << "##Final Result:";
    outfile << endl << "Total Cycles : " << cycle - 1 << endl;

    outfile << endl << "Final Register Values:" << endl;
    for (int i = 0; i < 32; i++) {
        outfile << registers[i] << " ";
    }

    outfile << endl << "Final Memory Values:" << endl;
    for (int i = 0; i < 32; i++) {
        outfile << memory[i] << " ";
    }

    return 0;
}
