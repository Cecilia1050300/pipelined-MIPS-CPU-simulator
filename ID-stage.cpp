#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

// ���O���c
struct Instruction {
    string opcode;  // �ާ@�X (�p R, LW, SW, BEQ)
    int rs;         // �ӷ��Ȧs��1
    int rt;         // �ӷ��Ȧs��2�ΥؼмȦs��
    int rd;         // �ؼмȦs�� (�ȹ� R-format���O����)
    int immediate;  // �ߧY�� (�ȹ� I-format���O����)
};

// ID/EX �Ȧs�� (�Ω� ID -> EX ��ƶǻ�)
struct ID_EX_Register {
    int readData1;            // �q�Ȧs��Ū�������1
    int readData2;            // �q�Ȧs��Ū�������2
    int immediate;            // �ѽX�᪺�ߧY��
    int rs, rt, rd;           // �Ȧs���s��
    string ALUOp;             // ALU �ާ@
    bool RegWrite = false;    // �O�_�g�^�Ȧs��
    bool MemRead = false;     // �O�_Ū�O����
    bool MemWrite = false;    // �O�_�g�O����
    bool Branch=false;        // �O�_��������O
};

// �����Ȧs���ɮ�
vector<int> registers(32, 0); // 32 �ӼȦs��

// ����H���ͦ����A�ާ@�X�ѪR
void generateControlSignals(const string& opcode, ID_EX_Register& idExReg) {
    if (opcode == "R") { // R-format���O
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

// ID ���q����
void instructionDecode(const Instruction& instr, ID_EX_Register& idExReg) {
    // �q�Ȧs���ɮ�Ū�����
    idExReg.readData1 = registers[instr.rs]; //�ѪR�ާ@��
    idExReg.readData2 = registers[instr.rt]; //�ѪR�ާ@��
    idExReg.rs = instr.rs;
    idExReg.rt = instr.rt;
    idExReg.rd = instr.rd;
    idExReg.immediate = instr.immediate;

    // �ͦ�����H��
    generateControlSignals(instr.opcode, idExReg);

    cout << "[ID] �ѽX�����A����H���P��Ƥw�g�J ID/EX �Ȧs���C\n";
}



// �D�{��
int main() {
    // ��l�ƼȦs���ɮ� (����)
    registers[8] = 5;  // $t0
    registers[9] = 10; // $t1
    registers[10] = 15; // $t2

    // ���O���X (����)
    vector<Instruction> instructions = {
        {"R", 9, 10, 8, 0},       // ADD $t0, $t1, $t2
        {"LW", 9, 8, 0, 4},       // LW $t0, 4($t1)
        {"SW", 8, 9, 0, 4},       // SW $t0, 4($t1)
        {"BEQ", 8, 9, 0, 2}       // BEQ $t0, $t1, offset 2
    };

    ID_EX_Register idExReg; // ID -> EX �Ȧs��

    for (const auto& instr : instructions) {
        cout << "====================\n";
        cout << "�B�z���O�G" << instr.opcode << "\n";

        // ID ���q
        instructionDecode(instr, idExReg);

        
    }

    return 0;
}
