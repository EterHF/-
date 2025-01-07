from openai import OpenAI
from time import sleep
from serial import Serial
import random

# 初始化串口
port = "COM5"
ser = Serial(port, 115200, timeout=0)

# 老虎机奖励分布（平均值和方差）
machines = [
    {"mean": 5.0, "variance": 1.0},  # 老虎机1
    {"mean": 1.0, "variance": 2.0},  # 老虎机2
    {"mean": 7.0, "variance": 0.5},  # 老虎机3
    {"mean": 3.5, "variance": 1.5},  # 老虎机4
]
# AI模型初始化
client = OpenAI(
    api_key="sk-tJ2Mke6PbW5tIqwMXBI1hxCbPoXEYiTV0TiROy0IasSFIbgD",
    base_url="https://api.moonshot.cn/v1"
)

# Prompt模板
hint_prompt = (
    "你是多臂老虎机游戏的玩家之一，与人类玩家对战。游戏规则是："
    "共有四台老虎机，每台老虎机有不同的奖励分布。在每轮游戏中，你可以选择一个老虎机进行操作并获得收益，"
    "在游戏结束后，累计收益更高的一方获胜。\n"
    "我会告诉你你的历史选择以及收益，你需要据此做出当前选择。 回复'Mx'格式表示你的选择，其中x可以是1，2，3，4，"
    "比如'M1'表示你本轮选择第一台老虎机。你仅需告诉你的选择，以'Mx'的格式。\n"
)
start_prompt = "你的历史选择：\n第0轮：游戏开始\n"

# AI模型调用函数
def chat(query):
    completion = client.chat.completions.create(
        model="moonshot-v1-auto",
        messages=[{"role": "user", "content": query}],
        temperature=0.3,
        stream=False,
    )
    answer = completion.choices[0].message.content.strip()
    return answer

def get_reward(machine_index):
    """
    使用高斯分布生成老虎机奖励。
    根据给定的均值和方差生成奖励值。
    """
    mean = machines[machine_index]["mean"]
    variance = machines[machine_index]["variance"]
    return max(0, random.gauss(mean, variance))  # 保证奖励不为负数

# 游戏主逻辑
def game(T=10):
    rounds = 0
    payoff = start_prompt
    player_total = 0
    ai_total = 0

    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode("utf-8").strip()

            if line == "reset" or line == "start":
                print("Game reset")
                rounds = 0
                payoff = start_prompt
                player_total = 0
                ai_total = 0
            elif line.startswith("MACHINE:"):
                rounds += 1
                player_choice = int(line.split(":")[1])
                player_reward = get_reward(player_choice)
                player_total += player_reward

                ai_choice = int(chat(hint_prompt + payoff)[1]) - 1
                ai_reward = get_reward(ai_choice)
                ai_total += ai_reward

                payoff += f"第{rounds}轮，你选择了第{ai_choice + 1}台老虎机，收益为{ai_reward:.2f}\n"

                response = f"{player_reward:.2f}:{ai_total:.2f}\n"
                ser.write(response.encode("utf-8"))

                print(f"Sent -> Player Reward: {player_reward:.2f}, Total: {player_total:.2f}, AI Reward: {ai_reward:.2f}, Total: {ai_total:.2f}")

        if rounds >= T:
            winner = "Player" if player_total > ai_total else "AI" if ai_total > player_total else "Draw"
            print(f"Winner: {winner}")
            ser.write(f"Winner:{winner}\n".encode("utf-8"))
            # break
            
        sleep(0.1)

# 启动游戏
if __name__ == "__main__":
    game()