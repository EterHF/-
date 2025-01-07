#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1638plus.h>

#define SCREEN_WIDTH 128  // OLED 宽
#define SCREEN_HEIGHT 64  // OLED 高
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TM1638plus tm(A2, A1, A0, false);

// 游戏变量
bool gameRunning = false;           // 游戏状态
float recent_rewards[4] = {0, 0, 0, 0}; // 每台老虎机最近一次收益
float total_rewards[4] = {0, 0, 0, 0};  // 每台老虎机累积收益
float player_total = 0;             // 玩家累计收益
float ai_total = 0;                 // AI累计收益
int currentMachine = -1;            // 玩家当前选择的老虎机
int rounds = 0;                     // 当前轮次，最大为10

void setup() {
  Serial.begin(115200);
  delay(100);

  // 初始化TM1638
  tm.displayBegin();
  pinMode(13, OUTPUT);

  // 初始化OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 Fail");
    for (;;);
  }

  display.clearDisplay();
  delay(100);
  drawSlotMachines();
}

void loop() {
  // 读取TM1638按键
  char buttons = tm.readButtons();

  // 按下第五个按钮，开始游戏
  if ((buttons & 0b00001000) != 0 && !gameRunning) { // S5按钮
    gameRunning = true;
    Serial.println("start");
    resetGame();
  }

  // 按下第六个按钮，重置游戏
  if ((buttons & 0b00000100) != 0 && gameRunning) { // S6按钮
    resetGame();
  }

  // 玩家选择老虎机 (按钮1-4)
  if (gameRunning) {
    if ((buttons & 0b10000000) != 0) currentMachine = 0; // S1
    if ((buttons & 0b01000000) != 0) currentMachine = 1; // S2
    if ((buttons & 0b00100000) != 0) currentMachine = 2; // S3
    if ((buttons & 0b00010000) != 0) currentMachine = 3; // S4

    if (currentMachine != -1) {
      Serial.print("MACHINE:");
      Serial.println(currentMachine); // 发送选择的老虎机编号到串口

      // 等待串口返回收益
      while (Serial.available() == 0);
      String data = Serial.readString().trim(); // 数据格式: "PLAYER_REWARD:AI_REWARD"
      
      if(data[0] == 'W'){  // 如果收到胜者数据
        String winner = data.substring(data.indexOf(":") + 1);
        
        // 清除部分屏幕或重置显示
        display.clearDisplay();

        // 绘制胜者信息
        display.setTextSize(2);  // 设置文字大小
        display.setTextColor(WHITE);  // 设置文字颜色
        display.setCursor(10, 25);  // 设置文字位置（屏幕中心）
        display.print("Winner:");  // 显示 "Winner:"
        display.setCursor(10, 45);
        display.print(winner);  // 显示具体胜者（如 "Player" 或 "AI"）

        display.display();  // 更新显示
        delay(1000);
        resetGame();
        return;
      }

      float player_reward = data.substring(0, data.indexOf(":")).toFloat();
      float ai_reward = data.substring(data.indexOf(":") + 1).toFloat();
      player_total += player_reward;
      ai_total += ai_reward;

      // 更新收益
      recent_rewards[currentMachine] = player_reward;
      total_rewards[currentMachine] += player_reward;

      // 更新轮次
      rounds++;

      // 重绘老虎机收益和玩家、AI的总收益
      drawSlotMachines();
      currentMachine = -1; // 重置选择
    }
  }
}

// ============================== 游戏状态重置 ==============================
void resetGame() {
  Serial.println("reset");
  gameRunning = false;
  currentMachine = -1;
  player_total = 0;
  ai_total = 0;
  rounds = 0;  // 重置轮次

  // 重置收益数据
  for (int i = 0; i < 4; i++) {
    recent_rewards[i] = 0;
    total_rewards[i] = 0;
  }

  drawSlotMachines();
}

// ============================== 绘制老虎机 ==============================
void drawSlotMachines() {
  display.clearDisplay();

  // 绘制老虎机框架
  display.drawRect(0, 0, 60, 30, SSD1306_WHITE);   // 老虎机1
  display.drawRect(68, 0, 60, 30, SSD1306_WHITE);  // 老虎机2
  display.drawRect(0, 34, 60, 30, SSD1306_WHITE);  // 老虎机3
  display.drawRect(68, 34, 60, 30, SSD1306_WHITE); // 老虎机4

  // 显示每台老虎机的收益
  for (int i = 0; i < 4; i++) {
    int x_offset = (i % 2 == 0) ? 2 : 70; // X轴偏移
    int y_offset = (i < 2) ? 2 : 36;      // Y轴偏移

    // 最近一次收益
    display.setCursor(x_offset, y_offset);
    display.print("R:"); // 最近收益
    display.print(recent_rewards[i]);

    // 累积收益
    display.setCursor(x_offset, y_offset + 10);
    display.print("T:"); // 累积收益
    display.print(total_rewards[i]);
  }

  // 显示玩家和AI累计收益
  display.setCursor(0, 54); // 玩家收益
  display.print("P:"); 
  display.print(player_total);

  display.setCursor(68, 54); // AI收益
  display.print("AI:");
  display.print(ai_total);

  // 显示当前轮次
  display.setCursor(110, 10); // 设置位置
  display.setTextSize(1);
  display.print("Round: ");
  display.print(rounds);

  display.display();
}