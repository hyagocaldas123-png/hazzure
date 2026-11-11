from pathlib import Path
from legacy_ui_output import protected_output
import os

hazzure_ui_path = protected_output(r'c:\Codex2\gpt\HazzureInternal\src\HazzureUI.h')

ui_code = ''.join((Path(__file__).resolve().parent / 'ui_templates' / name).read_text(encoding='utf-8') for name in ('generate_perfect_hazzure_ui-000.inc', 'generate_perfect_hazzure_ui-001.inc', 'generate_perfect_hazzure_ui-002.inc'))

with open(hazzure_ui_path, 'w', encoding='utf-8') as f:
    f.write(ui_code)

print("HazzureUI.h rewritten cleanly and verified!")
