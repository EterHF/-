from openai import OpenAI
client = OpenAI(
    api_key="sk-****",
    base_url="https://api.moonshot.cn/v1"
)
completion = client.chat.completions.create(
    model="moonshot-v1-auto",
    messages=[
        {"role": "user", "content": "用8个字回答：天是什么色？"},
    ],
    temperature=0.3,
    stream=False
)
answer = completion.choices[0].message
print(answer.content)
