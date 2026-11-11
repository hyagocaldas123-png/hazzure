path = r'c:\Codex2\gpt\HazzureInternal\src\UIPanel.inl'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

old_panel = """        pos=ImGui::GetWindowPos();
        auto* draw=ImGui::GetWindowDrawList();

        // 1. Barra Lateral Esquerda"""

new_panel = """        pos=ImGui::GetWindowPos();
        auto* draw=ImGui::GetWindowDrawList();
        RenderGlassBlurBackdrop(draw, pos, size);

        // 1. Barra Lateral Esquerda"""

assert old_panel in text, 'old_panel not found'
text = text.replace(old_panel, new_panel)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)

print('UIPanel.inl patched successfully!')
