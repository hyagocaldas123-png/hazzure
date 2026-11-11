# Python / release_package.py / Python 3.10+ / staged release assembly and recovery
"""Assemble verified artifacts before replacing an output directory; retain its predecessor."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
from uuid import uuid4


def sha256(path):
    digest=hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):
            digest.update(block)
    return digest.hexdigest().upper()


def assemble(files, destination, workspace, validation=None):
    workspace=Path(workspace).resolve(strict=True)
    destination=Path(destination).resolve()
    if destination==workspace or not destination.is_relative_to(workspace):
        raise ValueError('The output directory must stay inside the workspace.')
    sources={}
    for name,path in files.items():
        relative=PurePosixPath(name.replace('\\','/'))
        if (relative.is_absolute() or '..' in relative.parts or ':' in str(relative)
                or str(relative) in ('.','release-manifest.json')):
            raise ValueError('Invalid package member: '+name)
        source=Path(path).resolve(strict=True)
        if not source.is_file():
            raise ValueError('Not a regular source file: '+str(source))
        sources[str(relative)]=source
    if not sources:
        raise ValueError('An empty release cannot be published.')
    if destination.exists() and not destination.is_dir():
        raise ValueError('The output path is not a directory.')
    # Preserve the installed profile instead of replacing preferences with build defaults.
    profile=destination/'settings.json'
    if profile.is_file():
        if not isinstance(json.loads(profile.read_text(encoding='utf-8-sig')),dict):
            raise ValueError('The existing profile is not a JSON object.')
        sources['settings.json']=profile
    destination.parent.mkdir(parents=True,exist_ok=True)
    stamp=datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    suffix=stamp+'-'+uuid4().hex[:8]
    staged=destination.parent/('.'+destination.name+'.staging-'+suffix)
    previous=destination.parent/(destination.name+'.previous-'+suffix)
    # All directory moves below use these checked, absolute workspace paths.
    for checked in (staged,previous):
        if not checked.resolve().is_relative_to(workspace):
            raise ValueError('A publication path escapes the workspace.')
    staged.mkdir()
    members=[]
    for relative,source in sorted(sources.items()):
        before=source.stat();expected=sha256(source)
        target=staged/relative;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(source,target)
        after=source.stat()
        if (before.st_size,before.st_mtime_ns)!=(after.st_size,after.st_mtime_ns) or sha256(target)!=expected:
            raise OSError('Source changed or copied bytes differ: '+relative)
        members.append(dict(path=relative,size=target.stat().st_size,sha256=expected))
    manifest=dict(schema_version=1,created_at_utc=datetime.now(timezone.utc).isoformat(),
                  validation=validation or {},files=members,
                  scope='File integrity and recorded local tests; no in-game validation implied.')
    (staged/'release-manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    moved=False
    try:
        if destination.exists():
            os.replace(destination,previous);moved=True
        os.replace(staged,destination)
    except OSError:
        if moved and not destination.exists():
            os.replace(previous,destination)
        raise
    return dict(destination=str(destination),previous=str(previous) if moved else None,
                manifest=str(destination/'release-manifest.json'),files=len(members))


def suite_files(root,internal=None,runtime=None,panel=None,external=None):
    root=Path(root).resolve()
    runtime=Path(runtime) if runtime else root/'Hazzure/Hazzure.exe'
    files={
        'Hazzure.exe':runtime,'Hazzure.imgui.runtime.exe':runtime,
        'Hazzure.panel.exe':Path(panel) if panel else root/'HazzurePanel/Hazzure.panel.exe',
        'HazzureExternal.exe':Path(external) if external else root/'HazzureExternal/build/HazzureExternal.exe',
        'modules/Hazzure.Internal.dll':Path(internal) if internal else root/'HazzureInternal/build/Hazzure.Internal.dll',
    }
    for name in ('settings.json','auth-server.txt','cimgui.dll','Hazzure.Render.dll'):
        path=root/'Hazzure'/name
        if path.is_file():files[name]=path
    for path in sorted((root/'Hazzure').glob('SharpDX*.dll')):files[path.name]=path
    for folder in ('offsets','sounds'):
        base=root/'Hazzure'/folder
        if base.is_dir():
            for path in sorted(base.rglob('*')):
                if path.is_file():files[folder+'/'+path.relative_to(base).as_posix()]=path
    return files
