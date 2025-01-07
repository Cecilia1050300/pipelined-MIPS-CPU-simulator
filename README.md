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
## 3.forwarding與data hazard相關設計想法:
因為有四種指令可能造成data hazard，分別為
a.lw:lw要在MEM階段將值存入，且在WB階段才能使用所以在case
b.beq: beq指令在ID階段就會判斷了
c.add/sub:會在EX階段執行，使用forwarding技術可以在MEM階段使用。
d.sw:會在ex階段使用到前面指令的結果。




