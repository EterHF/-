#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TM1638plus.h>
#include <string.h>

#define SCREEN_WIDTH 128  // OLED 宽
#define SCREEN_HEIGHT 64  // OLED 高
#define OLED_RESET -1
#define MAX_ROUNDS 10
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TM1638plus tm(A2, A1, A0, false);

// 游戏变量
bool gameRunning = false;           // 游戏状态
float recent_rewards[4] = {0, 0, 0, 0}; // 每台老虎机最近一次收益
float total_rewards[4] = {0, 0, 0, 0};  // 每台老虎机累积收益
int selection_counts[4] = {0, 0, 0, 0}; // 每台老虎机被选择的次数
float player_total = 0;             // 玩家累计收益
float ai_total = 0;                 // AI累计收益
int currentMachine = -1;            // 玩家当前选择的老虎机
int rounds = 0;                     // 当前轮次，最大为10
static char lastButtons = 0; 

void setup() {
  Serial.begin(115200);
  delay(100);

  // 初始化TM1638
  tm.displayBegin();
  // pinMode(13, OUTPUT);

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
  char changedButtons = buttons & ~lastButtons; // 仅保留从 0->1 的按键位
  tm.setLEDs(changedButtons * 256);

  // 按下第五个按钮，开始游戏
  if ((changedButtons & 0b00010000) != 0 && !gameRunning) { // S5按钮
    resetGame();
    gameRunning = true;
    Serial.println("start");
  }

  // 按下第六个按钮，重置游戏
  if ((changedButtons & 0b00100000) != 0 && gameRunning) { // S6按钮
    resetGame();
  }

  // 玩家选择老虎机 (按钮1-4)
  if (gameRunning) {
    if ((changedButtons & 0b00000001) != 0) currentMachine = 0; // S1
    if ((changedButtons & 0b00000010) != 0) currentMachine = 1; // S2
    if ((changedButtons & 0b00000100) != 0) currentMachine = 2; // S3
    if ((changedButtons & 0b00001000) != 0) currentMachine = 3; // S4

    if (currentMachine != -1) {
      // 增加被选择次数
      selection_counts[currentMachine]++;
      Serial.print("MACHINE:");
      Serial.println(currentMachine); // 发送选择的老虎机编号到串口

      // 等待串口返回收益
      while (Serial.available() == 0);
      String data = Serial.readStringUntil('\n'); // 读取串口数据
      data.trim(); // 移除空白字符 // 数据格式: "PLAYER_REWARD:AI_TOTAL"

      float player_reward = data.substring(0, data.indexOf(":")).toFloat();
      ai_total = data.substring(data.indexOf(":") + 1).toFloat();
      player_total += player_reward;

      // 更新收益
      recent_rewards[currentMachine] = player_reward;
      total_rewards[currentMachine] += player_reward;

      // 更新轮次
      rounds++;

      // 重绘老虎机收益和玩家、AI的总收益
      drawSlotMachines();
      currentMachine = -1; // 重置选择
    }

    if(rounds >= MAX_ROUNDS){
      while (Serial.available() == 0);
      String data = Serial.readStringUntil('\n'); // 读取串口数据
      data.trim(); // 移除空白字符 // 数据格式: "Winner:AI/Player/Draw"
      
      if(data[0] == 'W'){  // 如果收到胜者数据
         String winner = data.substring(data.indexOf(":") + 1);

        // 清除部分屏幕或重置显示
        display.clearDisplay();

        // 绘制胜者信息
        display.setTextSize(1);  // 设置文字大小
        display.setTextColor(WHITE);  // 设置文字颜色
        display.setCursor(10, 10);  // 设置文字位置
        display.print("Winner:");  // 显示 "Winner:"
        display.setCursor(10, 20);
        display.print(winner);  // 显示具体胜者（如 "Player" 或 "AI")

        // 绘制双方得分
        display.setTextSize(1);  // 设置较小的文字大小
        display.setCursor(10, 40);  // 设置玩家得分位置
        display.print("Player: ");  // 显示玩家得分
        display.print(player_total, 1);  // 显示玩家得分，保留一位小数

        display.setCursor(10, 50);  // 设置 AI 得分位置
        display.print("AI: ");  // 显示 AI 得分
        display.print(ai_total, 1);  // 显示 AI 得分，保留一位小数

        // 更新显示
        display.display();
        delay(5000);  // 停留 5 秒以便玩家查看结果

        resetGame();
        return;
      }
    }
  }

  lastButtons = buttons;
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
    selection_counts[i] = 0; // 重置选择计数器
  }

  drawSlotMachines();
}

// ============================== 绘制老虎机 ==============================
void drawSlotMachines() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // 绘制老虎机框架
  display.drawRect(0, 0, 60, 25, SSD1306_WHITE);   // 老虎机1
  display.drawRect(68, 0, 60, 25, SSD1306_WHITE);  // 老虎机2
  display.drawRect(0, 26, 60, 25, SSD1306_WHITE);  // 老虎机3
  display.drawRect(68, 26, 60, 25, SSD1306_WHITE); // 老虎机4

  // 显示每台老虎机的收益和选择次数
  for (int i = 0; i < 4; i++) {
    int x_offset = (i % 2 == 0) ? 4 : 72; // X轴偏移
    int y_offset = (i < 2) ? 4 : 30;      // Y轴偏移

    // 最近一次收益
    display.setCursor(x_offset, y_offset);
    display.print("R:"); // 最近收益
    display.print(recent_rewards[i], 1);

    // 累积收益
    display.setCursor(x_offset , y_offset + 10);
    display.print("T:"); // 累积收益
    display.print(total_rewards[i], 1);

    // 显示选择次数
    display.setCursor(x_offset + 42, y_offset);
    display.print("C"); // 选择次数
    display.setCursor(x_offset + 42, y_offset + 10);
    display.print(selection_counts[i]);
  }

  // 显示玩家和AI累计收益
  display.setCursor(28, 56); // 玩家收益
  display.print("P:"); 
  display.print(player_total, 1);

  display.setCursor(78, 56); // AI收益
  display.print("AI:");
  display.print(ai_total, 1);

  // 显示当前轮次
  display.setCursor(0, 56); // 设置位置
  display.setTextSize(1);
  display.print("T:");
  display.print(rounds);

  display.display();
}
