# pipelined-MIPS-CPU-simulator
## 1.環境設置: 
### 系統需求
#### windows11
### 環境配置
#### IDE:vscode
#### 編譯器:MinGW
##
## 2.編譯與執行步驟:
### a.編譯指令： 提供具體的編譯或安裝指令
#### 參考https://hackmd.io/@liaojason2/vscodecppwindows
### b.執行指令： 提供如何運行模擬器的具體指令，並說明如何提供輸入檔案
與chat GPT聊天，下次會改研究Dockerfile
因為我本人的vscode一直不讓我改成使用MinGW
所以需要開啟終端機，打指令g++ -o 輸出檔名 檔名.cpp，換行./檔名
### c.範例執行： 提供一個範例指令來執行程式，並示範如何使用測試資料


##
## 抱歉main branch超級亂
因為一開始沒有很會使用github，所以不知道要先放在自己的branch，然後全部東西都塞在main、、、

## 3.語言選擇
### 使用C++語言製作此專案
a. 上網搜尋後發現python是寫大多數模擬器專案的最佳語言選擇，開發效率高、且處理輸入.json檔案方便。  
b. 選擇C++的原因  
&nbsp;&nbsp;&nbsp;&nbsp;1.但由於組員們普遍對python不熟悉  
&nbsp;&nbsp;&nbsp;&nbsp;2.不太會需要使用到python外掛套件  
&nbsp;&nbsp;&nbsp;&nbsp;3.效能需求:如果模擬器執行的測試指令數量較多，或者 Pipeline 階段的模擬較複雜，C++的高效能能確保執行速度不成問題  
&nbsp;&nbsp;&nbsp;&nbsp;4.豐富的資料結構(STL標準模板庫):可以用 queue模擬每個 Pipeline 階段的執行指令，且靈活度高。  
&nbsp;&nbsp;&nbsp;&nbsp;5.C++的語言特性更接近硬體邏輯  
##
## 4.forwarding與data hazard相關設計想法:
因為有四種指令可能造成data hazard，分別為
a.lw:lw要在MEM階段將值存入，且在WB階段才能使用所以在case
b.beq: beq指令在ID階段就會判斷了
c.add/sub:會在EX階段執行，使用forwarding技術可以在MEM階段使用。
d.sw:會在ex階段使用到前面指令的結果。
