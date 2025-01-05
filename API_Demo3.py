from openai import OpenAI
client = OpenAI(
    api_key="sk-****",
    base_url="https://api.moonshot.cn/v1",
)
history = [
    {"role": "system","content":"你是个聪明玩家。"}
]

def chat(query, history):
    history.append({
        "role": "user","content": query
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
