path = r'c:\Codex2\gpt\HazzureInternal\src\SkinEngine.h'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. GlovePath
old_gp = '"agents/models/shared/arms/'
new_gp = '"weapons/models/shared/arms/'
assert old_gp in text, 'old_gp not found'
text = text.replace(old_gp, new_gp)

# 2. ApplyGloves
old_ag = '    if(!PrecacheModelResource(path)||!SkinSetAttributes(item+kSkinDynamicAttributes,*config))return false;'
new_ag = '''    SkinWrite(local+0x1268,0xFFFFFFFFu);
    if(path)PrecacheModelResource(path);
    SkinSetAttributes(item+kSkinDynamicAttributes,*config);'''
assert old_ag in text, 'old_ag not found'
text = text.replace(old_ag, new_ag)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)

print('SkinEngine.h patched successfully!')
