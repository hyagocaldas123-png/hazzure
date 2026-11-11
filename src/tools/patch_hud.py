path = r'c:\Codex2\gpt\HazzureInternal\src\UITacticalHud.inl'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

old_draw = """static void DrawSpectatorsHUD(const HazzureTactical::Spectators& specs,int screenW,int screenH) {
    if(!specs.count||screenW<160||screenH<80)return;"""

new_draw = """static void DrawSpectatorsHUD(const HazzureTactical::Spectators& specs,int screenW,int screenH) {
    HazzureTactical::Spectators effective = specs;
    if(!effective.count && g_bShowMenu && g_cfg.showSpectators) {
        effective.count = 1;
        strcpy_s(effective.entries[0].name, "Preview_Espectador");
        effective.entries[0].mode = 4;
    }
    if(!effective.count||screenW<160||screenH<80)return;
    const auto& currentSpecs = effective;"""

assert old_draw in text, 'old_draw not found'
text = text.replace(old_draw, new_draw)

text = text.replace('specs.count', 'currentSpecs.count')
text = text.replace('specs.entries', 'currentSpecs.entries')

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)

print('UITacticalHud.inl patched successfully!')
