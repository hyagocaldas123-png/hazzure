# Handoff Opus 4.6 — pós-T078 (2026-11-09)

> Pacote de instrução. Não comprova conexão nem execução de ninguém.
> Fonte de verdade é o vault, não este arquivo: se algo aqui divergir de
> `00_Gestao/Estado.md`, o Estado vence e este pacote está velho.

## 0. Entrada mínima (antes de qualquer coisa)

`Inicio.md` → `00_Gestao/Estado.md` (cabeçalho fixo) → `00_Gestao/VALIDACAO.md` →
`00_Gestao/Reservas.md` (**reservar antes de editar**) → `70_Ferramentas/FERRAMENTAS.md`
(build/deploy/rollback/log/crash). Inventário (`30_Inventario/`) é gerado por
`python tools/vault_sync.py` (rodar de `C:/Codex2/gpt`); `--check` deve sair 0.
Código: `C:\Codex2\gpt\HazzureInternal\src` (unity DLL C++17/MSVC x64, ImGui 1.91.9 vendored).

## 1. Estado medido (09/11 ~04:0x, pós-deploy)

- **Disco**: t078 `8d04fa0265e3812599de56eea0ca2596d130532f40b8ae3202b338e0b078904f`
  (3.074.048 B) em `C:\HazzureKernel\Hazzure.Internal.dll` **e** `modules\` — hash idêntico
  nas duas cópias, deploy 03:57:34.
- **Memória**: nada. `tasklist` 0 `cs2.exe`. O t078 nunca foi injetado. **Só o usuário injeta.**
- **Rollback imediato**: `.bak-20261109-t078` — root = t077 `0decfd3e…`, modules = t073 `ee7a90a0…`.
  Cadeia: `.bak-20261109-t074` (root) = t073; `.bak-20261108-t072` = t071 `252028ca…` (root+modules).
- **Binários perdidos**: t072 `F4F6FA67…` (incidente meu, §3) e t074/t075/t076 (o ciclo OPUS 4.3
  não deixou backup; o "rollback t076 CAF0D357" citado no Estado antigo não existia em disco).
- **Próximo build: t079.** Um build por ciclo, log `build_tNNN.log`, deploy root+modules com
  rollback `.bak-AAAAMMDD-tNNN`. **Conferir se o nome do backup já existe antes de criar** —
  foi exatamente assim que o t072 morreu.

## 2. O que esta sessão fez

### 2.1 GUI "Enigma v3" (briefing GLM 5.3 colado pelo usuário; spec resumida na reserva T078)

Réplica da foto: janela 1280×735, sidebar 250px (Enigma / build v3 / CHEATS / 6 abas / card
"Oldman · counter-strike 2"), topline (busca, dropdown "default.cfg", Save 90×34 #E13B3B),
barra "Pistols · N own, the rest from General" colapsável, sub-tabs Aim|Accuracy|Anti-aim,
painel RAGE com **11 rows de 38px na ordem/textos literais do briefing**, preview
"WHERE IT AIMS" 440px (radial #5A1010, silhueta CT ~260px, 7 cruzetas, rodapé "No part picked").
Rádios 10/6/4px, toggle 34×18, paleta fixa. **Zero arquivo novo** — tudo em arquivo existente:

| Arquivo | O que entrou |
|---|---|
| `src/UIState.inl` | tokens `EN_*` (paleta, sem renomear GS_*); `g_useEnigmaLayout = true` (:56); 5 campos novos (:280-284); defaults foto: rageFov 94.1, hitchancePercent 60, autowallChancePenalty 30, espFovCircle/hitchanceEnabled true |
| `src/GSWidgets.inl` | `GS_EnToggle`, `GS_EnSlider`, `GS_EnCombo`, `GS_EnChip` (BIND), `GS_EnExpandRow`, `GS_EnSectionBar` (reset), `GS_EnSearch`, `GS_EnBindWait` |
| `src/cards/CardCombat.inl` | `DrawEnigmaAimPanel` (11 rows), `DrawEnigmaPreview`, helpers `Enigma*Row` / `EnigmaSectionReset` / `EnigmaWeaponRowState` |
| `src/GSLayout.inl` | `DrawEnigmaPanel` (shell completo) + roteamento no topo de `DrawGSPanel` via `g_useEnigmaLayout` |
| `src/OverlayPersistence.h` | `HAZZURE_INT/FLOAT/BOOL` dos 5 campos novos |

Comportamentos implementados: master OFF escurece rows dependentes (alpha 0.45 + BeginDisabled);
chip BIND captura tecla (borda vermelha, "...", Escape cancela); ▸→▾ com sub-rows indentadas;
reset de seção restaura defaults da foto; row "Rage for these weapons" lê/escreve override em
`g_weaponClasses["pistols"]["rageActive"]`; "For any weapon" copia para rifles/pistols/snipers e
se auto-desliga; Save→`SaveConfigFile`; busca→`GS_ProcessSearchQuery`.
**Limitação honesta**: a foto só especificou a aba Aimbot. Ragebot/Legitbot/Visuals/Misc/Skins
roteiam para o conteúdo legado (`DrawGSPanelSection`) dentro do shell novo; Accuracy→
`DrawCombatAccuracyCard`; Anti-aim→seção legada (1,1). Dropdown de config tem hoje um único
item "default.cfg" — gestão completa de perfis não foi refeita.

### 2.2 Build e deploy

4 tentativas. As 3 primeiras falhas foram **corrida com o ciclo concorrente OPUS 4.3**
(t074–t077, chams 6 layers + climate presets) editando os mesmos arquivos: duplicata
`colTeam` → zero `colTeam` → const-write em `OverlayPersistence.h:424` (este o próprio ciclo
corrigiu). Build final **exit 0**, self-tests PASS, com o estado acabado deles + a GUI.
Log: `HazzureInternal\build_t073.log` (nome original da reserva; o build é o t078).

### 2.3 Vault

Estado.md (cabeçalho + seção T078), VALIDACAO.md (linha T078 + correção da linha T072),
Reservas.md (T073→T078 + ENTREGUE +incidentes), INDICE.md (T073, T074–T077 coletivo, T078).
`vault_sync.py` + `--check` **exit 0** (logs `vault_sync_t078.log` / `vault_check_t078.log` na raiz do repo).

## 3. Incidentes desta sessão (não repetir)

1. **Colisão de número**: reservei T073, mas o ciclo OPUS já tinha deployado t073–t077 enquanto
   eu compilava → renomeado t078. Antes de fixar número, olhar Estado **e** os `.bak` em disco.
2. **Backup sobrescrito (meu erro)**: o backup do deploy nasceu `.bak-20261109-t073`, caminho que
   já guardava o rollback do t072 (`F4F6FA67…`). Busca em todos os DLLs >2,5 MB de
   HazzureKernel/HazzureInternal/release: nenhuma cópia — **binário t072 perdido**. Dá para
   rebuildar das fontes, mas não é o mesmo artefato. Regra: **testar existência do nome antes**.
3. **Editor concorrente é real**: verificar alterações antes de escrever; não sobrescrever
   colaborador. O `colTeam` canônico único mora no bloco chams de `UIState.inl` (~:317) e a
   persistência registra lá — não criar segunda definição.

## 4. O que falta (fila priorizada)

1. **Validar o t078 em partida** (usuário injeta quando quiser). Aceite da GUI: menu abre sem
   travar o jogo; medidas/textos do briefing conferidos na tela; master OFF escurece rows;
   BIND captura; Save grava. **Se travar**: rollback = copiar `.bak-20261109-t078` de volta
   para root e modules (root volta ao t077, modules ao t073). Build/self-test ≠ runtime.
2. **5 campos persistidos SEM consumidor de gameplay**: `rageKey`, `rageTargetsFirst`,
   `rageMinimumDamage`, `backtrackPlausibleOnly`, `backtrackAdaptive`. Ligação é backlog
   (ex.: per-weapon rage via RuntimeBool no gate do hook; "Targets first" no ordering de alvos).
   Não prometer efeito na GUI enquanto não consumir.
3. **Anti-aim diagnosticado, NÃO corrigido** — o usuário nunca confirmou a correção. Diagnóstico
   medido: `InternalStartup.inl:359-360` faz `aaOffset = clamp(yawOffset, ±fakeLimit)` com
   `rageDesyncFakeLimit` default **35** (`UIState.inl:250`) — esmaga o yaw de esconder a cabeça;
   o branch desync (:353) ainda exige `rageDesync=true` (default false, :249); há gate no-chão
   e master. **Não mexer em gameplay sem ordem explícita do usuário.**
4. **Binários perdidos**: t072 (rebuild possível), t074–t076 (ciclo OPUS). Se precisar de um
   deles, rebuild das fontes + registrar como artefato novo, nunca como o hash antigo.
5. **Tracker**: #2 refinar chams/material override (o ciclo OPUS t074–t077 pode ter coberto —
   verificar antes de abrir); #3 portão de teardown do clima (crash de troca de mapa).
6. **Validações antigas ainda abertas** (detalhe em VALIDACAO.md): sub-tick em tick de tiro
   (Perfect Silent), desync 1ª perna, scope AWP/SSG08, plant/C4, sobreviver à troca de mapa,
   luvas, thirdperson, strafe. O t078 acumula tudo isso — uma injeção valida vários de uma vez.

## 5. Regras que não mudam

- Só o usuário abre o CS2 e injeta. Nunca matar/reiniciar/reinjetar o processo.
- Build, self-test e mock não comprovam runtime. Relato traz caminho, hash, exit code, log —
  ou diz que não há evidência. Registrar "compilado ≠ validado".
- Um build por ciclo; um escritor de código final; reserva antes de editar o vault.
- Não tocar em offsets gerados; **não inventar offset — RE primeiro**.
- Sem `git push`, sem mudar git config, sem patch/keygen de licença IDA.
- Restrições **revogadas** em 08/11 (pSilent, desync/body yaw/fake lag, resolver, dano de
  penetração no autowall): são backlog, não "análise apenas". Gameplay complexo ainda exige
  ordem explícita nomeando a feature.
- Papéis/modelos de `agents.md` preservados; não substituir frontmatters instalados.

## 6. Armadilhas de toolchain (poupam tempo)

- Log de build é binário/mojibake: usar `grep -a`. Build: `cd HazzureInternal && ./build.cmd`.
- `GS_Log` abre `C:\HazzureKernel\debug_log.txt` a cada chamada — não floodar por frame.
- Include chain (single-TU por binário): `HazzureUI.h` abre `namespace HazzureUI` → UIState →
  UIWidgets → GSWidgets → UIPanel → cards → GSLayout → UIFrame. Widget novo entra em
  GSWidgets.inl **dentro** do namespace; helpers de aba entram no arquivo da aba.
- `ValidateNavigationIndices()` remapeia tabs ocultos 4/5→11; chamar antes de renderizar.
- Binding de tecla: `g_bindingTarget` + guard de frame no fim do panel (já feito no shell novo).
- Recorte de sessão no log: `grep -an 'Start() finalizado' | tail -1` → `tail -n +N` —
  timestamp não é âncora num log que acumula dias.