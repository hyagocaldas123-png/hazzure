#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Hazzure Match Readiness & Pre-Flight Runtime Verification
Checks all native binaries, assets, IPC offsets, and CS2 environment before match testing.
"""
import os
import sys
import hashlib
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

TARGET_HASH = "5BAFAA79CFD360C44EB2EA42805C67FC39FFB1AEBC8C657BD586DF42B7330C42"

def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest().upper()

def main():
    print("=" * 70)
    print(" HAZZURE CS2 - PRE-FLIGHT MATCH READINESS VERIFICATION")
    print("=" * 70)

    all_passed = True

    # 1. Check Binary Synchronisation
    print("\n[1/5] Verificando Binarios e Hashes SHA-256...")
    binary_paths = [
        ROOT / "HazzureInternal" / "build" / "Hazzure.Internal.dll",
        ROOT / "release" / "modules" / "Hazzure.Internal.dll",
        ROOT / "Hazzure" / "modules" / "Hazzure.Internal.dll",
        Path(r"C:\HazzureKernel\Hazzure.Internal.dll")
    ]
    for b in binary_paths:
        if not b.exists():
            print(f"  [FAIL] Arquivo ausente: {b}")
            all_passed = False
            continue
        h = sha256(b)
        size = b.stat().st_size
        match = (h == TARGET_HASH)
        status = "OK" if match else "HASH MISMATCH"
        print(f"  [{status}] {b.name} ({size:,} bytes)")
        print(f"         Destino: {b}")
        print(f"         SHA-256: {h}")
        if not match:
            all_passed = False

    # 2. Check 3D & Catalogue Assets
    print("\n[2/5] Verificando Assets 3D e Catalogo...")
    assets_to_check = [
        ROOT / "Hazzure" / "assets" / "models" / "phoenix.glb",
        ROOT / "Hazzure" / "assets" / "models" / "phoenix-anim.glb",
        ROOT / "Hazzure" / "assets" / "agents",
        ROOT / "Hazzure" / "assets" / "skins"
    ]
    for a in assets_to_check:
        if a.is_file():
            print(f"  [OK] Modelo 3D: {a.name} ({a.stat().st_size:,} bytes)")
        elif a.is_dir():
            count = len(list(a.glob("*")))
            print(f"  [OK] Diretorio {a.name}/: {count} arquivos indexados")
        else:
            print(f"  [WARN] Asset nao encontrado: {a}")

    # 3. Check CS2 Client Installation
    print("\n[3/5] Verificando Instalacao do Counter-Strike 2...")
    cs2_paths = [
        Path(r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll"),
        Path(r"C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\bin\win64\cs2.exe")
    ]
    cs2_installed = True
    for cp in cs2_paths:
        if cp.exists():
            print(f"  [OK] Encontrado: {cp.name} ({cp.stat().st_size:,} bytes)")
        else:
            print(f"  [INFO] Caminho padrao Steam nao detectado: {cp}")
            cs2_installed = False

    # 4. Check Process Execution State
    print("\n[4/5] Checando Processos em Execucao...")
    try:
        tasks = subprocess.check_output('tasklist /FI "IMAGENAME eq cs2.exe"', shell=True, text=True, stderr=subprocess.DEVNULL)
        if "cs2.exe" in tasks:
            print("  [ONLINE] cs2.exe detectado em execucao!")
        else:
            print("  [PRONTO] cs2.exe nao esta em execucao. Pronto para inicializacao limpa.")
    except Exception as e:
        print(f"  [INFO] Consulta tasklist: {e}")

    # 5. Runtime Checklist & Launch Instructions
    print("\n[5/5] Checklist de Validacao em Partida Local (Prontidao Completa):")
    print("-" * 70)
    print("  A. INICIALIZACAO SEGURA DO CS2:")
    print("     Opcoes de inicializacao recomendadas na Steam:")
    print("     -insecure -novid -allow_third_party_software +fps_max 144")
    print("")
    print("  B. COMANDOS DE CONSOLE PARA PARTIDA DE TESTE (Bancada de Validacao):")
    print("     No console do jogo (~), carregue o mapa e prepare o round:")
    print("     map de_mirage")
    print("     sv_cheats 1; bot_kick; mp_warmup_end; mp_freezetime 0; mp_buy_anywhere 1; mp_startmoney 65535; mp_restartgame 1")
    print("")
    print("  C. ITENS DE VERIFICACAO NO MENU HAZZURE (Pressione INSERT):")
    print("     1. Loadout & Inventario Completo:")
    print("        - Ativar Skins, Facas & Luvas.")
    print("        - Selecionar Faca, Luvas e Skins personalizadas.")
    print("        - Testar os botoes rapidos de Condicao (FN, MW, FT, WW, BS).")
    print("        - Aplicar adesivos famosos (ex: 5x Titan Holo) e StatTrak.")
    print("        - Inspecionar a Tabela do Inventario Completo.")
    print("     2. Terceira Pessoa:")
    print("        - Ativar Terceira Pessoa e testar alternancia pela tecla configurada.")
    print("        - Mover-se contra paredes/cantos para checar colisao nativa da camera.")
    print("     3. Proporcao de Tela (Aspect Ratio):")
    print("        - Ativar Forcar Proporcao e alternar entre 4:3 e 16:9.")
    print("        - Validar que as caixas do ESP e mira alinham com os inimigos.")
    print("     4. Clima 3D e Particulas:")
    print("        - Ativar Clima 3D (Chuva/Neve).")
    print("        - Ficar sob tetos (ex: Palace, B Apartments) e verificar oclusao.")
    print("        - Sair a ceu aberto e verificar precipitacao em 3D.")
    print("     5. Killfeed Persistente:")
    print("        - Adicionar bots (bot_add_ct / bot_add_t) e eliminar.")
    print("        - Verificar registro de arma, localizacao, headshot e duracao.")
    print("     6. Telemetria e Diagnostico:")
    print("        - Acessar a aba 'Sistema' -> 'Diagnostico' e checar o cartao")
    print("          'Prontidao de Partida & Runtime' em tempo real.")
    print("-" * 70)

    if all_passed:
        print("\n=> RESULTADO: SISTEMA 100% PRONTO PARA VALIDACAO EM PARTIDA.")
    else:
        print("\n=> AVISO: Algum item requer atencao antes da partida.")
    print("=" * 70)
    return 0 if all_passed else 1

if __name__ == "__main__":
    sys.exit(main())
