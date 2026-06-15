# Pipelined MIPS CPU Simulator

這是計算機組織期末專題的 Pipelined MIPS CPU 模擬器。專案以 C++ 實作五階段 pipeline，依照課堂規格輸出每個 clock cycle 中指令所在的 stage、控制訊號，以及程式結束後的 register / memory 狀態。

## 專案目標

- 模擬五階段 MIPS pipeline：IF、ID、EX、MEM、WB
- 支援指令：`lw`、`sw`、`add`、`sub`、`beq`
- 以 forwarding 處理 data hazard
- 對無法 forwarding 的 load-use hazard 插入 stall
- 以 predict not taken 處理 control hazard
- 輸出每個 cycle 的 pipeline 狀態與控制訊號

## 專案結構

```text
.
├── src/
│   └── main.cpp              # 模擬器主程式
├── inputs/                   # 測試輸入檔
│   ├── test1.txt
│   └── ...
├── outputs/                  # 範例或執行輸出
├── archive/                  # 開發過程中的分段實作與草稿
├── Makefile                  # Linux/macOS/MinGW 建置用
├── .gitignore
└── README.md
```

## 執行環境

建議環境：

- C++17 編譯器
- Windows：MinGW-w64 / MSYS2 / WSL
- Linux/macOS：`g++` 或 `clang++`

本專案只使用 C++ 標準函式庫，不需要額外套件。

## 編譯方式

使用 `make`：

```bash
make
```

或直接使用 `g++`：

```bash
g++ -std=c++17 -Wall -Wextra -O2 src/main.cpp -o mips-pipeline
```

Windows 若輸出 `.exe`：

```powershell
g++ -std=c++17 -Wall -Wextra -O2 src\main.cpp -o mips-pipeline.exe
```

## 執行方式

```bash
./mips-pipeline --input inputs/test1.txt --output outputs/test1.txt
```

Windows PowerShell：

```powershell
.\mips-pipeline.exe --input inputs\test1.txt --output outputs\test1.txt
```

參數說明：

- `--input` 或 `-i`：指定 MIPS 組合語言輸入檔
- `--output` 或 `-o`：指定模擬結果輸出檔
- `--help` 或 `-h`：顯示使用方式

若未提供參數，程式預設讀取 `inputs/test6.txt`，並輸出到 `outputs/test6.txt`。

## 輸入格式

每行一個 MIPS 指令，例如：

```text
lw $2, 8($0)
lw $3, 16($0)
add $4, $2, $3
sw $4, 24($0)
```

支援格式：

```text
lw  $rt, offset($rs)
sw  $rt, offset($rs)
add $rd, $rs, $rt
sub $rd, $rs, $rt
beq $rs, $rt, offset
```

## 初始狀態

- Register 共 32 個
- Memory 共 32 words
- `$0 = 0`
- 其他 registers 初始值皆為 `1`
- Memory 每個 word 初始值皆為 `1`

## 輸出內容

程式會輸出：

- 每個 cycle 中指令所在 stage
- ID / EX / MEM / WB 階段的控制訊號
- Don't care 訊號以 `X` 表示
- 總 cycle 數
- 最終 register values
- 最終 memory values

輸出範例片段：

```text
Cycle 3:
lw EX RegDst=0 ALUSrc=1 Branch=0 MemRead=1 MemWrite=0 RegWrite=1 MemToReg=1
lw ID
add IF

##Final Result:
Total Cycles : 9

Final Register Values:
0 1 1 1 2 ...

Final Memory Values:
1 1 1 1 1 ...
```

## Pipeline 與 Hazard 策略

### Data Hazard

模擬器在指令進入 EX 階段前檢查 EX hazard 與 MEM hazard。若前一段 pipeline 中已有可用結果，會透過 forwarding 讓目前指令使用最新值。

### Load-Use Hazard

若目前指令需要使用前一個 `lw` 尚未從 memory 讀出的結果，forwarding 無法立即解決，因此插入 stall 等待資料可用。

### Store Forwarding

`sw` 也可能依賴前面 ALU 指令或 load 指令產生的資料，因此同樣需要 hazard 判斷與 forwarding。

### Control Hazard

`beq` 採用 predict not taken。若分支實際成立，會清除錯抓的指令並跳到正確位置繼續執行。

## 測試資料

`inputs/` 內保留課堂範例測資：

- `test1.txt`：基本 `lw` / `add` / `sw`
- `test2.txt`：load-use hazard
- `test3.txt`：`beq` 與 branch flush
- `test4.txt` - `test8.txt`：forwarding、stall、branch 混合案例

可以逐一執行：

```bash
for f in inputs/*.txt; do
  ./mips-pipeline --input "$f" --output "outputs/$(basename "$f")"
done
```

## 清除建置檔

```bash
make clean
```

或手動刪除編譯出的 `mips-pipeline` / `mips-pipeline.exe`。
