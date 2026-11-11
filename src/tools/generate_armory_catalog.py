"""Generate bounded Armory table fragments from the installed schema."""
from pathlib import Path
import argparse,json

ROOT=Path(__file__).resolve().parents[1]
def generate(schema,output):
    data=json.loads(Path(schema).read_text(encoding='utf-8'))
    output=Path(output);parts=output/'catalogs'/'armory';parts.mkdir(parents=True,exist_ok=True)
    lines=['// IDs/names from the installed items_game schema; no remote inventory mutation.',
           '#pragma once','namespace HazzureArmory {','struct CatalogEntry {int id;const char* name;};']
    counts={}
    for source,name in [('sticker_kits','stickers'),('keychain_definitions','charms')]:
        rows=sorted((int(k),v.get('name',k)) for k,v in data[source].items() if k.isdecimal())
        counts[source]=len(rows);lines.append(f'inline constexpr CatalogEntry {name}Catalog[]={{')
        for index in range(0,len(rows),512):
            filename=f'{name}-{index//512:03d}.inc'
            text=''.join('{'+str(k)+','+json.dumps(v)+'},\n' for k,v in rows[index:index+512])
            (parts/filename).write_text(text,encoding='utf-8',newline='\n')
            lines.append(f'#include "catalogs/armory/{filename}"')
        lines.append('};')
    lines.append('}');(output/'ArmoryCatalog.h').write_text('\n'.join(lines)+'\n',encoding='utf-8',newline='\n')
    return counts

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--schema',type=Path,default=ROOT/'HazzureSkinAnalysis/game-schema/items_game.json')
    parser.add_argument('--output',type=Path,default=ROOT/'HazzureInternal/src')
    args=parser.parse_args();print(json.dumps(generate(args.schema,args.output)))
