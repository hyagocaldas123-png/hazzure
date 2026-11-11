import os, stat, json, csv, hashlib, time, zipfile
from collections import defaultdict
from pathlib import Path
OUT=Path(__file__).parent/'auditoria-espaco'
OUT.mkdir(exist_ok=True)
sizes=defaultdict(int); files=[]; errors=[]; skipped=[]
def walk(root):
    try:
        entries=list(os.scandir(root))
    except OSError as e:
        errors.append([root,str(e)]); return
    for e in entries:
        try:
            s=e.stat(follow_symlinks=False)
            if e.is_symlink() or getattr(s,'st_reparse_tag',0)==0xA0000003:
                skipped.append(e.path); continue
            if e.is_dir(follow_symlinks=False): walk(e.path)
            elif e.is_file(follow_symlinks=False):
                files.append((e.path,s.st_size,getattr(s,'st_file_attributes',0)))
                p=os.path.dirname(e.path)
                while p:
                    sizes[p]+=s.st_size
                    up=os.path.dirname(p)
                    if up==p: break
                    p=up
        except OSError as ex: errors.append([e.path,str(ex)])
print('Iniciando inventario de C:',flush=True)
walk('C:\\')
files.sort(key=lambda x:x[1],reverse=True)
def csvout(name,header,rows):
    with open(OUT/name,'w',encoding='utf-8-sig',newline='') as f:
        w=csv.writer(f,delimiter=';'); w.writerow(header); w.writerows(rows)
csvout('arquivos.csv',['Caminho','Bytes','GiB','Atributos'],((p,n,round(n/2**30,4),a) for p,n,a in files))
csvout('pastas.csv',['Caminho','Bytes','GiB'],((p,n,round(n/2**30,4)) for p,n in sorted(sizes.items(),key=lambda x:-x[1])))
print('Inventario concluido: '+str(len(files))+' arquivos. Buscando duplicatas pessoais.',flush=True)
home=os.environ['USERPROFILE']
personal=[os.path.join(home,x).lower()+'\\' for x in ['Downloads','Documents','Videos','Music','Pictures','OneDrive']]+['c:\\teraboxdownload\\']
groups=defaultdict(list); candidates=[]
for p,n,a in files:
    if n and any(p.lower().startswith(r) for r in personal) and not a & (0x1000|0x40000|0x400000) and not any(x in p.lower().split('\\') for x in ['node_modules','.git','.venv','venv']):
        groups[n].append(p); candidates.append((p,n))
dups=[]
for n,paths in groups.items():
    if len(paths)<2: continue
    quick=defaultdict(list)
    for p in paths:
        try:
            with open(p,'rb') as f:
                h=hashlib.sha256(f.read(65536)); f.seek(max(0,n-65536)); h.update(f.read(65536))
            quick[h.hexdigest()].append(p)
        except OSError as e: errors.append([p,str(e)])
    for ps in quick.values():
        if len(ps)<2: continue
        full=defaultdict(list)
        for p in ps:
            try:
                with open(p,'rb') as f: digest=hashlib.file_digest(f,'sha256').hexdigest()
                full[digest].append(p)
            except OSError as e: errors.append([p,str(e)])
        for h,pp in full.items():
            if len(pp)>1: dups.append({'bytes':n,'redundant_bytes':n*(len(pp)-1),'sha256':h,'paths':pp})
dups.sort(key=lambda x:-x['redundant_bytes'])
csvout('duplicatas.csv',['Grupo','Bytes por arquivo','SHA256','Caminho'],((i,d['bytes'],d['sha256'],p) for i,d in enumerate(dups,1) for p in d['paths']))
print('Duplicatas concluídas. Verificando ZIPs pessoais de ate 500 MiB.',flush=True)
checks=[]
for p,n in candidates:
    if p.lower().endswith('.zip') and n<=500*2**20:
        try:
            with zipfile.ZipFile(p) as z:
                if sum(i.file_size for i in z.infolist())>2*2**30:
                    checks.append([p,'Nao testado: conteudo expandido >2 GiB']); continue
                bad=z.testzip()
                checks.append([p,'Falha CRC: '+bad if bad else 'OK'])
        except (OSError,zipfile.BadZipFile,RuntimeError,NotImplementedError,EOFError) as e: checks.append([p,'Falha de verificacao: '+str(e)])
csvout('verificacao-zip.csv',['Caminho','Resultado'],checks)
csvout('acesso-limitado.csv',['Caminho','Erro'],errors)
summary={'file_count':len(files),'errors':len(errors),'skipped_links':len(skipped),'top_folders':sorted(sizes.items(),key=lambda x:-x[1])[:100],'top_files':files[:60],'duplicate_groups':len(dups),'duplicate_redundant_bytes':sum(x['redundant_bytes'] for x in dups),'top_duplicates':dups[:30],'zip_checks':checks}
(OUT/'resumo.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(summary,ensure_ascii=True),flush=True)
