from openai import OpenAI
from time import sleep

port = "COM3"
ser = Serial(port, 115200, timeout=0)

client = OpenAI(
    api_key="sk-****",
    base_url="https://api.moonshot.cn/v1"
)

SystemPrompt = [
    {"role": "system",
     "content": "你是井字棋游戏的一个玩家，后手下棋。游戏规则是：3*3 的棋盘里，双方轮流落子，不可在重复位置落子。对方每次会以'RxCy'的格式来落子，表示在第x行第y列落子，其中x和y的范围都是1、2、3。而你在思考之后，也以相同的格式来落子。直到游戏结束。"}
]

history = SystemPrompt

def chat(query, history):
    history.append({
        "role": "user", "content": query
    })
    completion = client.chat.completions.create(
        model="moonshot-v1-auto",
        messages= history,
        temperature=0.3,
        stream=False
    )

    answer = completion.choices[0].message.content

    history.append({
        "role": "assistant","content": answer
    })
    return answer


while True:

    # 检查是否有数据可读
    if ser.in_waiting > 0:

        # 读取一行数据
        line = ser.readline().decode('utf-8')
        if line[0] == '*':
            history = SystemPrompt
            print("reset")
        else:
            print("Player: ",line)

            # 问 AI 大模型，获得返回值
            result = chat(line, history)
            print("AI:     ",result)

            # 并将 AI 的回复转发到串口
            ser.write(str(result).encode())

    sleep(0.1)

