from openai import OpenAI

client = OpenAI(
    api_key="sk-****",
    base_url="https://api.moonshot.cn/v1"
)

model_list = client.models.list()
model_data = model_list.data

for i, model in enumerate(model_data):
    print(f"model[{i}]:", model.id)
