#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

const int NUM_REGISTERS = 32;  // �w�q�H�s���ƶq
const int MEMORY_SIZE = 32;    // �w�q�O����j�p

// instruction���c
struct Instruction {
    string op;      // ���O�ާ@�] "add", "sub", "lw", "sw", "beq"�^
    int rd, rs, rt; // �Ȧs����
    int immediate;  // �ߧY�ơ]�ϥΩ�ߧY�ƾާ@�Ϊ̤�����O�^
    int address;    // �a�}�]�O����s���^

    // ��l��
    Instruction() : op(""), rd(-1), rs(-1), rt(-1), immediate(-1), address(-1) {}

    // �a�Ѽƪ��غc��ơA�Ω��l�ƫ��O���U�Ӧ���
    Instruction(const string& op, int rd, int rs, int rt, int immediate, int address = -1)
        : op(op), rd(rd), rs(rs), rt(rt), immediate(immediate), address(address) {}
};

// �B�z��J���
// �h���r�ꭺ���ťզr��
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t");  // ���Ĥ@�ӫD�ťզr��
    size_t last = str.find_last_not_of(" \t");   // ���̫�@�ӫD�ťզr��
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
}

// ���R���O
Instruction parseInstruction(const string& line) {
    Instruction inst;
    istringstream iss(line);
    string token;

    try {
        // �ѪR "add", "sub", "lw"
        if (!(iss >> inst.op)) {
            throw runtime_error("Missing operation");
        }

        // �ھڤ��P���ާ@�ŸѪR���P�����O�榡
        if (inst.op == "lw" || inst.op == "sw") {  // Load/Store
            string rd, offset;
            if (!(iss >> rd >> offset)) {
                throw runtime_error("Invalid lw/sw format");
            }

            rd = trim(rd);
            rd.pop_back(); // �h���r��

            //�o�X�A��������
            size_t start = offset.find('(');
            size_t end = offset.find(')');

            if (start == string::npos || end == string::npos) {
                throw runtime_error("Invalid memory access format");
            }

            inst.rd = stoi(rd.substr(1)); // �����H�s���s��
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
            rs.pop_back();  // �h���r��
            rt.pop_back();  // �h���r��

            inst.rs = stoi(rs.substr(1));  // �����H�s���s��
            inst.rt = stoi(rt.substr(1));  // �����H�s���s��
        }
        else if (inst.op == "add" || inst.op == "sub") {  // ��N�B��
            string rd, rs, rt;
            if (!(iss >> rd >> rs >> rt)) {
                throw runtime_error("Invalid add/sub format");
            }

            rd = trim(rd);
            rs = trim(rs);
            rt = trim(rt);

            // �h���������r��
            if (rd.back() == ',') rd.pop_back();
            if (rs.back() == ',') rs.pop_back();
            if (rt.back() == ',') rt.pop_back();

            // �ˬd�H�s���榡�O�_���T
            if (rd[0] != '$' || rs[0] != '$' || rt[0] != '$') {
                throw runtime_error("Invalid register format: " + rd + ", " + rs + ", " + rt);
            }

            inst.rd = stoi(rd.substr(1));  // �������ഫ�H�s���s��
            inst.rs = stoi(rs.substr(1));
            inst.rt = stoi(rt.substr(1));
        }
        else if (inst.op == "nop") {  // No operation (�žާ@)
            // �L�ݳB�z
        }
        else {
            throw runtime_error("Unknown operation: " + inst.op);
        }
    }
    catch (const exception& e) {
        cerr << "Error parsing line: " << line << ". Error: " << e.what() << endl;
        inst.op = "nop";  // ��ѪR���~�ɡA�N���O�]�m�� "nop"
    }

    return inst;
}

// pipeline
struct Pipeline {
    vector<int> registers;  // �H�s���Ʋ�
    vector<int> memory;     // �O����Ʋ�
    vector<Instruction> instructions;  // �x�s���O���e��
    queue<Instruction> pipeline[5];  // �y���u�����Ӷ��q
    int pc;  // �p�ƾ�
    int cycle;  // �g������
    vector<string> a_Output;  // ��X�O��

    Pipeline() {
        registers.resize(NUM_REGISTERS, 1);  // ��l�ƱH�s��
        registers[0] = 0;  // $0 �H�s���� 0
        memory.resize(MEMORY_SIZE, 1);  // ��l�ưO����
        pc = 0;  // �]�m�{���p�ƾ��� 0
        cycle = 0;  // �]�m��l�g���� 0
    }

    // ���J���O
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

// ����H��
string controlSignals(const string& op) {
    if (op == "lw")  return "RegDst=0 ALUSrc=1 Branch=0 MemRead=1 MemWrite=0 RegWrite=1 MemToReg=1";
    if (op == "sw")  return "RegDst=X ALUSrc=1 Branch=0 MemRead=0 MemWrite=1 RegWrite=0 MemToReg=X";
    if (op == "add") return "RegDst=1 ALUSrc=0 Branch=0 MemRead=0 MemWrite=0 RegWrite=1 MemToReg=0";
    if (op == "sub") return "RegDst=1 ALUSrc=0 Branch=0 MemRead=0 MemWrite=0 RegWrite=1 MemToReg=0";
    if (op == "beq") return "RegDst=X ALUSrc=0 Branch=1 MemRead=0 MemWrite=0 RegWrite=0 MemToReg=X";
    if (op == "nop") return "NOP";
    return "RegDst=X ALUSrc=X Branch=X MemRead=X MemWrite=X RegWrite=X MemToReg=X";
}

// IF ���q
void Pipeline::IF() {
    // �ˬd�{���p�ƾ��O�_�W�X���O�d��
    if (pc < (int)instructions.size()) {
        Instruction inst = instructions[pc];
        pipeline[0].push(inst);           // �N���O��J IF ���q�]pipeline[0]�^
        a_Output.push_back(inst.op + " IF");
        pc++;  // �w��������G����U�@�����O
    }
}

// ID���q & hazard����
bool Pipeline::ID() {
    if (pipeline[0].empty()) return false;

    Instruction inst = pipeline[0].front();
    bool needsStall = false;

    // �ˬd EX ���q
    if (!pipeline[2].empty()) {
        Instruction prevEX = pipeline[2].front();
        bool prevEXWritesRegister =
            (prevEX.op == "add" || prevEX.op == "sub" || prevEX.op == "lw");

        // �p�G EX ���q�g�J���H�s���O��e���O�� rs �� rt�A�h�ݭn stall
        if (prevEXWritesRegister && prevEX.rd != 0) {
            if (prevEX.rd == inst.rs || prevEX.rd == inst.rt) {
                if (prevEX.op == "lw") {
                    needsStall = true;  // data hazard
                }
            }
        }
    }

    // �ˬd MEM ���q
    if (!pipeline[3].empty()) {
        Instruction prevMEM = pipeline[3].front();
        bool prevMEMWritesRegister =
            (prevMEM.op == "add" || prevMEM.op == "sub" || prevMEM.op == "lw");

        // �p�G MEM ���q�g�J���H�s���O��e���O�� rs �� rt�A�h�ݭn stall
        if (prevMEMWritesRegister && prevMEM.rd != 0) {
            if (prevMEM.rd == inst.rs || prevMEM.rd == inst.rt) {
                if (prevMEM.op == "lw") {
                    needsStall = true;
                }
            }
        }
    }


    if (inst.op == "beq") {
        // �ˬd EX ���q (pipeline[2])
        if (!pipeline[2].empty()) {
            Instruction exInst = pipeline[2].front();
            if (exInst.rd != 0 && (exInst.rd == inst.rs || exInst.rd == inst.rt)) {
                if (exInst.op == "sub" || exInst.op == "lw") {
                    needsStall = true;
                }
            }
        }

        // �ˬd MEM ���q (pipeline[3])
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
        return true;  // �^�� true ��ܻݭn stall
    }

    pipeline[0].pop();      // IF -> �����w�B�z�����O
    pipeline[1].push(inst); // ID <- ��J��e���O
    a_Output.push_back(inst.op + " ID");
    return false;
}

// EX ���q
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
            // ����Q����A�վ�{���p�ƾ�
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

// MEM ���q
void Pipeline::MEM() {
    if (pipeline[2].empty()) return;

    Instruction inst = pipeline[2].front();
    pipeline[2].pop();

    if (inst.op == "nop") {
        pipeline[3].push(inst);
        return;
    }

    if (inst.op == "lw") {
        // �O����Ū��
        registers[inst.rd] = memory[inst.address / 4];
    }
    else if (inst.op == "sw") {
        // �O����g�J
        memory[inst.address / 4] = registers[inst.rs];
    }

    pipeline[3].push(inst);
    a_Output.push_back(inst.op + " MEM " + controlSignals(inst.op));
}

// WB ���q
void Pipeline::WB() {
    if (pipeline[3].empty()) return;

    Instruction inst = pipeline[3].front();
    pipeline[3].pop();

    if (inst.op == "nop") {
        return;
    }

    a_Output.push_back(inst.op + " WB " + controlSignals(inst.op));
}

// �g���p��ÿ�X���G
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

    ifstream input("test6.txt");  // ���J���O�ɮ�
    ofstream output("test6output.txt");  // ��X���G

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

    // �����������{������
    while (p.pc < (int)instructions.size() ||
        !p.pipeline[0].empty() ||
        !p.pipeline[1].empty() ||
        !p.pipeline[2].empty() ||
        !p.pipeline[3].empty())
    {
        p.cyclecount(output);
    }

    // ��X�̲׵��G
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









