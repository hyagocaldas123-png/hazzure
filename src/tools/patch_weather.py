path = r'c:\Codex2\gpt\HazzureInternal\src\WorldWeather.h'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

old_loop = '        for(size_t processed=0;processed<6&&queries<12;++processed){'
new_loop = '        for(size_t processed=0;processed<32&&queries<64;++processed){'
assert old_loop in text, 'old_loop not found'
text = text.replace(old_loop, new_loop)

old_sky = """            if(!p.sky){
                auto top=p.point;top.z+=1800;
                if(!HazzureGame::Finite(top)){p.active=false;continue;}
                ++queries;
                if(!query||!query(context,p.point,top,result)||result.solid||result.fraction<.999f){p.active=false;continue;}
                p.sky=true;
            }"""
new_sky = """            if(!p.sky){
                auto top=p.point;top.z+=600;
                if(!HazzureGame::Finite(top)){p.active=false;continue;}
                ++queries;
                if(query&&query(context,p.point,top,result)&&result.solid&&result.fraction<.05f){p.active=false;continue;}
                p.sky=true;
            }"""
assert old_sky in text, 'old_sky not found'
text = text.replace(old_sky, new_sky)

old_vis = 'p.visibleUntil=now+80;'
new_vis = 'p.visibleUntil=now+350;'
assert old_vis in text, 'old_vis not found'
text = text.replace(old_vis, new_vis)

old_snap = 'if(!at||now<at||now-at>100)values={};'
new_snap = 'if(!at||now<at||now-at>500)values={};'
assert old_snap in text, 'old_snap not found'
text = text.replace(old_snap, new_snap)

with open(path, 'w', encoding='utf-8') as f:
    f.write(text)

print('WorldWeather.h patched successfully!')
