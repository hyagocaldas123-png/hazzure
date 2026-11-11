import urllib.request
import json
import os
import sys

API_KEY = os.environ.get("KAFOX_API_KEY", "")
BASE_URL = "https://api.kafox.dev/v1"

def query_kafox(prompt: str, model: str = "deepseek-ai/DeepSeek-V4.1-Flash", system_prompt: str = "") -> str:
    if not API_KEY:
        raise RuntimeError("KAFOX_API_KEY nao definida; exportar a variavel antes de consultar a API")
    url = f"{BASE_URL}/chat/completions"
    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json",
        "User-Agent": "CYBR-MAHI/1.0"
    }
    messages = []
    if system_prompt:
        messages.append({"role": "system", "content": system_prompt})
    messages.append({"role": "user", "content": prompt})

    payload = {
        "model": model,
        "messages": messages,
        "temperature": 0.3
    }

    req = urllib.request.Request(url, data=json.dumps(payload).encode("utf-8"), headers=headers, method="POST")
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data["choices"][0]["message"]["content"]
    except Exception as e:
        return f"Error contacting Kafox API: {e}"

if __name__ == "__main__":
    p = sys.argv[1] if len(sys.argv) > 1 else "Hello"
    print(query_kafox(p))
