p = r"C:\Codex2\gpt\HazzureInternal\src\NativeInventoryEngine.h"
with open(p, "r", encoding="utf-8", errors="ignore") as f:
    text = f.read()

target = "#pragma once\n"
if "\r\n" in text:
    target = "#pragma once\r\n"
    repl = "#pragma once\r\n#include \"DebugLogger.h\"\r\n"
else:
    repl = "#pragma once\n#include \"DebugLogger.h\"\n"

if target in text and "#include \"DebugLogger.h\"" not in text:
    text = text.replace(target, repl, 1)
    with open(p, "w", encoding="utf-8") as f:
        f.write(text)
    print("SUCCESS: Include added")
else:
    print("SKIPPED or already present")
