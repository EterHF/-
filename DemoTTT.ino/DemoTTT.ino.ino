#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <IRLremote.h>

#define SCREEN_WIDTH 128  // OLED 宽
#define SCREEN_HEIGHT 64  // OLED 高
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define IR_RECEIVE_PIN 2  // 红外遥控管脚
CNec IRLremote;           // 红外遥控实例

char CommData[5];   // 留一点余地
int CurCommByte = 0;
bool NewCommMove = false;  

void setup() {
  Serial.begin(115200);
  delay(100);  

  // 初始化红外接收器
  IRLremote.begin(IR_RECEIVE_PIN);
  delay(100);  

  // 初始化OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 Fail");
    for(;;);
  }

  display.clearDisplay();
  delay(100);  
  drawBoard();
}

unsigned long IrCmd = 0; 
void loop() {
  if (IRLremote.available()) {
    auto data = IRLremote.read();
    IrCmd = data.command;
    if (IrCmd != 0){
      //Serial.println(IrCmd,HEX);    // 调试用
      handleIRInput(IrCmd);           // 游戏处理
      delay(100);
    }    
  }
  if (NewCommMove == true) {
    //Serial.println(CommData);
    handleCommInput();    
    NewCommMove = false;
    CurCommByte = 0;
  }
}

// ============================== 以下为串口数据部分 =====================

void serialEvent() {
  if (!Serial.available()) return;
  CommData[CurCommByte] = (char)Serial.read();    
  CurCommByte ++;
  if (CurCommByte >= 4) NewCommMove = true;
}

// ============================== 以下为游戏数据部分 =====================
char board[3][3] = {        // 目前落子情况
  {' ', ' ', ' '},
  {' ', ' ', ' '},
  {' ', ' ', ' '}
};
int currentRow = 0;         // 光标位置：行
int currentCol = 0;         // 光标位置：列
bool playerTurn = true;     // true 为轮到 X，false 为轮到 O

// ============================== 以下为游戏逻辑部分 =====================
void handleCommInput(){
  int Row = CommData[1]-'1';
  int Col = CommData[3]-'1';
  if(board[Row][Col] != ' ') return;

  board[Row][Col] = playerTurn ? 'X' : 'O';
  currentRow = Row;
  currentCol = Col;
  playerTurn = !playerTurn;
  drawBoard();
  
  if(checkWin()) {
    //display.clearDisplay();
    display.setCursor(0,0);
    display.print(!playerTurn ? "X win" : "O win");
    display.display();
  }  
}

void handleIRInput(unsigned long value) {
  bool newMove = false;
  switch(value) {
    case 0x16: // * 
      resetGame();
      break;
    case 0x18: // 上
      currentRow = (currentRow + 2) % 3;
      break;
    case 0x52: // 下
      currentRow = (currentRow + 1) % 3;
      break;
    case 0x08: // 左
      currentCol = (currentCol + 2) % 3;
      break;
    case 0x5A: // 右
      currentCol = (currentCol + 1) % 3;
      break;
    case 0x1C: // OK键
      if(board[currentRow][currentCol] == ' ') {
        board[currentRow][currentCol] = playerTurn ? 'X' : 'O';
        playerTurn = !playerTurn;
        newMove = true;
      }
      break;
  }
  drawBoard();    // 刷新屏幕（可能是移动，可能是落子）

  if (newMove == true){ 
    if(checkWin()) {
      //display.clearDisplay();
      display.setCursor(0,0);
      display.print(!playerTurn ? "X win" : "O win");
      display.display();
    }
    else{
      Serial.print("R");
      Serial.print(currentRow+1);
      Serial.print("C");
      Serial.println(currentCol+1);        
    }  
  }
}

bool checkWin() {
  // 检查行
  for(int i = 0; i < 3; i++) {
    if(board[i][0] != ' ' && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
      return true;
    }
  }
  
  // 检查列
  for(int i = 0; i < 3; i++) {
    if(board[0][i] != ' ' && board[0][i] == board[1][i] && board[1][i] == board[2][i]) {
      return true;
    }
  }
  
  // 检查对角线
  if(board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
    return true;
  }
  if(board[0][2] != ' ' && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
    return true;
  }
  
  return false;
}

void resetGame() {
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 3; j++) {
      board[i][j] = ' ';
    }
  }
  currentRow = 0;
  currentCol = 0;
  playerTurn = true;
  drawBoard();
  Serial.println("*");
}


// ============================== 以下为游戏绘制部分 =====================
void drawX(int col, int row) {
  int x = col * 20 + 37;
  int y = row * 20 + 5;
  display.drawLine(x, y, x + 14, y + 14, WHITE);
  display.drawLine(x, y + 14, x + 14, y, WHITE);
}

void drawO(int col, int row) {
  int x = col * 20 + 44;
  int y = row * 20 + 12;
  display.drawCircle(x, y, 7, WHITE);
}

void drawBoard() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // 画棋盘线  行高20；线位置：2,22,42,62；列宽20；线位置：34,54,74,94
  display.drawLine(34,  2, 34, 62, WHITE);
  display.drawLine(54,  2, 54, 62, WHITE);
  display.drawLine(74,  2, 74, 62, WHITE);
  display.drawLine(94,  2, 94, 62, WHITE);
  display.drawLine(34,  2, 94,  2, WHITE);
  display.drawLine(34, 22, 94, 22, WHITE);
  display.drawLine(34, 42, 94, 42, WHITE);
  display.drawLine(34, 62, 94, 62, WHITE);
  
  // 绘制棋子
  for(int i = 0; i < 3; i++) {
    for(int j = 0; j < 3; j++) {
      if(board[i][j] == 'X') {
        drawX(j, i);
      } else if(board[i][j] == 'O') {
        drawO(j, i);
      }
    }
  }
  
  // 绘制光标
  display.drawRect(currentCol * 20 + 36, currentRow * 20 + 4, 17, 17, WHITE);

  // 绘制当前玩家
  if (playerTurn == true) {
    display.drawLine(110, 25, 110 + 14, 25 + 14, WHITE);
    display.drawLine(110, 25 + 14, 110 + 14, 25, WHITE);
  }
  else {
    display.drawCircle(118, 32, 7, WHITE);
  }  
  display.display();
}
