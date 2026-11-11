# 3. Fix UIFrame.inl
p3 = r'c:\Codex2\gpt\HazzureInternal\src\UIFrame.inl'
with open(p3, 'r', encoding='utf-8') as f: c3 = f.read()
old3 = '    bool menuOpen=pShowMenu&&*pShowMenu;'
new3 = '    bool menuOpen=pShowMenu&&*pShowMenu;\n    g_menuOpen = menuOpen;'
assert old3 in c3, 'old3 not found'
c3 = c3.replace(old3, new3)
with open(p3, 'w', encoding='utf-8') as f: f.write(c3)
print('3. UIFrame.inl fixed')

# 4. Fix UITacticalHud.inl
p4 = r'c:\Codex2\gpt\HazzureInternal\src\UITacticalHud.inl'
with open(p4, 'r', encoding='utf-8') as f: c4 = f.read()
old4 = 'if(!effective.count && g_bShowMenu && g_cfg.showSpectators) {'
new4 = 'if(!effective.count && g_menuOpen && g_cfg.showSpectators) {'
assert old4 in c4, 'old4 not found'
c4 = c4.replace(old4, new4)
with open(p4, 'w', encoding='utf-8') as f: f.write(c4)
print('4. UITacticalHud.inl fixed')
