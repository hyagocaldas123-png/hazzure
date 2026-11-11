# Mapa de lacunas — Hazzure vs mercado (gamesense / neverlose / memesense)

**Data:** 2026-11-05 (noite), revisado às 23:10 depois da auditoria da T044. **Escritor:** coordenador (documentação; nenhum código alterado *nesta* doc — o código do ciclo está na T044).
**Método corrigido:** a versão anterior deste mapa concluiu "ausente" grepando **termos do mercado** (`chams`, `autowall`, `faststop`). Isso estava errado e produziu linhas falsas, porque o nosso código usa vocabulário próprio (`chamsVisible`, `rageAutoStop`, `shotStop`, `viewmodelX`). A auditoria agora parte das **nossas 157 labels de UI** (`GS_Toggle`/`CleanToggle`) e dos campos de `g_cfg`/`OverlayPersistence`, e só então casa com o nome de mercado. Itens marcados ❌ foram re-confirmados por grep com os dois vocabulários.

**Taxonomia:** árvore de menu real do MemeSense lida em vídeo (`ANALISE_REFERENCIA_MEMESENSE.md`) — LegitBot, Aim Assist, Players, Chams, Items, Visuals, World, View, Indicators, Miscellaneous, Inventory, Grenade Helper, Configs. É leitura de menu, não copy de loja.

---

## 1. Estado por aba

| Aba | Entrega típica | Hazzure | Estado |
|---|---|---|---|
| **LegitBot** | fov, smooth, RCS, hitbox por arma, hitchance, delay de troca | `CardCombat.inl`, `CombatTargeting.inl`, `CombatAccuracy.h`, `GameplayRuntime.h` | ✅ |
| **Aim Assist / suavidade** | auto-stop, camada macia para tap | `rageAutoStop` + limiar de velocidade (`GSLayout.inl:558`), `shotStop` "Frear Antes de Disparar" (`CardCombat.inl:248`), smooth/bezier/ease | 🟠 existe auto-stop real; falta um tier separado de assist |
| **Rage pesado** | autowall, minimum damage, safe point, prefire | nenhum hit em `penetr/autowall/wallbang/safe.?point/prefire`; e a nossa própria UI declara "Mira atravess de parede nunca é permitida" (`GSLayout.inl:567`) | ❌ ausente por escolha declarada — mudar isso é decisão do usuário, não bug |
| **Players / Visuals** | box, health, nome, arma, dormindo, radar, sound, tracer, hitmarker, granadas, zonas | tudo isso **mais**: spectator list, dano flutuante 3D, setas fora de tela, impact boxes, killfeed persistente, timer da C4, radar nativo, partículas de neve, keybinds overlay | ✅ acima do mercado em QoL de informação |
| **Chams** | material override, contorno, camada dupla, gradiente | `chamsVisible`/`chamsHidden`/`chamsArms`/`chamsWeapon`/`chamsDualLayer`/`chamsGradient` + `chamsVisibleStyle`/`chamsHiddenStyle` (0–4); implementação real: hook de draw em `scenesystem+0x59FC0` com gate de prologue (`InternalStartup.inl:448-456`) e troca de `CMaterial2`/cor em `mesh+0x20`/`mesh+0x50` com backup/restore (`InternalResources.inl:455-486`), materiais por `CreateMaterial` (`csgo_unlitgeneric.vfx`) | ✅ **presente** — a linha "❌ chams ausente" desta doc estava errada e foi corrigida aqui |
| **View / viewmodel** | offsets X/Y/Z, FOV da arma, proporção forçada | `viewmodelX/Y/Z` + FOV (`CardEnvironment.inl:31-34`), aplicação/restore por `HazzureTuning` (`BasicTuning.h:52-94`), aspect forçado, camera FOV hook | 🟠 existe; falta separar **por arma** (é um set global) |
| **World / clima** | céu custom, cor sólida, night mode, clean map, no-flash, neve | `skyboxIndex`/material do céu, `Dark World (Nightmode)`, `Clean Map (Sem Sombras)`, `Limitar Alpha do Flash` (pawn+0x150c/0x1504), snow particles, vignette, Tsukuyomi | ✅ presente — e é o suspeito do crash de troca de mapa |
| **HvH — anti-aim** | jitter, spin, pitch, restore | entregue em t039, escreve só no cmd, câmera livre | 🟠 **não validado em partida** |
| **HvH — desync / fake lag / resolver** | body yaw, manual yaw, quick peek, choke, leitura de animstate | nenhum hit (`desync` só no texto que declara fora de escopo, `GSLayout.inl:586`) | ❌ ausente — **evasion: sem receita** |
| **Backtrack real** | mirar em posição rebobinada | `PoseHistory` é visual ("Historico Visual de Movimento", `CardVisuals.inl:74`); o aimbot nunca usa posição rebobinada | ❌ ausente |
| **Movement** | bhop, strafe, edge/pixel surf, jumpbug, ladder, slow walk, air jump | bhop, auto-strafe (direcional e por física), jumpbug, edgebug, pixelsurf, edgejump, fastladder, jumpthrow, preserve edge | 🟠 forte; `slowWalk`/`airJump`/`ladderDrop` = 0 hits |
| **Grenade Helper** | lineups gravados/carregados, projétil assistido | `Marcadores de Lineup` + física de zona (`GrenadePhysics.h`, `GrenadeZones.h`) | 🟠 sem record/replay persistente — é o T040 |
| **Items / Inventory** | facas, rifles, luvas, agentes, music kits, wear/seed/StatTrak, nametag | catálogo completo, StatTrak, 5 adesivos, chaveiro, **nametag em item** (`ArmoryEditor.h:124`), inventory local | 🟠 sem music kits; **visual de luva** = T044 (build novo, não validado) |
| **Miscellaneous** | auto-accept, auto-buy, watermark, clantag, stream-proof | `auto.?buy`/`purchase`/`autoAccept`/`watermark` = 0 hits; `clan` = 0 hits; `Stream-Proof Interno` existe na UI mas está dentro de `ImGui::BeginDisabled()` (`CardEnvironment.inl:43`) — placeholder, não função | ❌ ausente (stream-proof é vitrine vazia) |
| **Configs** | nuvem, import por link/hash | JSON local + hot reload | ❌ sem nuvem/share |
| **Scripts** | engine Lua/JS | 0 hits | ❌ ausente |

## 2. Prioridades que não são feature

1. **Crash em troca de mapa** — 4/5 dumps em `client.dll+0x230DAF` (céu/exposure do motor), 2/2 sessões morrendo logo depois de `[CLIMA] ... 0 controller(s)`. A/B com clima desligado ainda sem resposta do usuário.
2. **Anti-aim t039 não validado** em partida.
3. **T044 (deste ciclo)** — dirigir o re-apply nativo da luva (`SetBodyGroup` client+0x964BB0 + `UpdateBodyGroupChoice` client+0x955FF0, `m_bDisallowSOC=false`, burst de 3 frames). Build `fb9a500f…`, **não está em memória**.

## 3. Impacto ÷ esforço (só o que ainda é verdade)

1. fechar o crash (pré-requisito de teste longo)
2. validar anti-aim e o resultado da T044 em partida
3. viewmodel **por arma** (a máquina já existe em `HazzureTuning`, é separar o set)
4. chams: resta refinar o que já existe (por arma/por time), não construir
5. music kits + presets de loadout no inventory
6. T040 record/replay de lineups
7. QoL: auto-accept, auto-buy, watermark (todos do zero)
8. autowall/minimum damage/safe point: o custo real **caiu** depois da medição de 2026-11-06 — o registro do trace já traz o ponteiro de superfície em `+0x00` e o `contents` em `+0x14`, então não existe arquivo `surfaceproperties.txt`/`.vphys` para parsear (ver §3.1). "chega ou não chega" é multi-trace com o que já temos; "quanto dano sobra" depende de confirmar em runtime os dois floats atrás de `+0x00`. E ainda **contradiz a regra que a nossa UI declara hoje** — precisa ordem explícita
9. cloud/share de config

### 3.1 Registro do trace, medido no binário instalado (2026-11-06, sem IDA e sem jogo aberto)

Feito com `C:/Codex2/gpt/tools/faultfn.py` sobre `game\csgo\bin\win64\client.dll` (`TimeDateStamp 0x6AEBC4DA`, `SizeOfImage 0x29A3000`). É o que responde "quanto custa autowall" hoje:

- `TraceShape` é **`client+0xA1B8D0`** — a RVA gateada em `Visibility.h::SelectTraceProfile`; o prefixo de 20 bytes confere **só** ali (`0xA1B750` é um thunk sem relação, `0xA1AE30` não bate neste build).
- O wrapper põe o resultado em `r14`, delega o preenchimento a **`client+0x9D4830`** e depois **o próprio motor lê `[r14+0xAC]` com `comiss` e `cmp byte ptr [r14+0xBB],0`** — ou seja, `fraction@0xAC` e `solid@0xBB` que usamos estão certos neste build, por prova do motor, não por suposição.
- Maior offset escrito em `0x9D4830`: `[rbx+0xB0]` (qword) e o byte `[rbx+0xBB]` → o registro **é 0xC0 aqui**, não 0x108 como os static_asserts de `ags-sensum`/`ags-asphyxia-cs2`. Nosso buffer `Trace{0xC0}` **não estoura a pilha** — não "consertar" isso.
- Campos dentro do nosso antigo `pad[0x94]`: `+0x0C`/`+0x10` dois ints (um recebe `-1`), `+0x14` **contents** (testado contra `0x40000000`), `+0x18`/`+0x20`/`+0x28` qwords, `+0x30`/`+0x40` dois blocos de 16 B, `+0x50..0x77` bloco de 40 B copiado direto do registro de colisão da física, `+0x78` origem do raio, `+0x84` destino, `+0x90` normal, `+0x9C` posição do hit, `+0xB4` int, `+0xBC` flag.
- `trace+0x00` é o **ponteiro de superfície** (vem de `[registro_física+0x28]`). Duas bases independentes o tipam como `{pad 0x8; float penetrationModifier@0x8; float damageModifier@0xC; pad 0x4; int material@0x14}` — essas duas linhas são **alegação das bases, não medida nossa**; custa um único leitura em runtime para confirmar.

Consequência honesta: "a bala chega" já é montável com o que está provado acima (`fraction`/`solid`/`contents@0x14`, multi-trace). "quanto dano sobra do outro lado" depende daquela confirmação em runtime — e nenhuma das duas exige extrair tabela de arquivo, porque não há `surfaceproperties.txt` nem `.vphys` no install.

## 4. Limite declarado

Fake lag / choke, desync / body yaw / manual yaw e resolver de anti-aim inimigo **não recebem implementação** — a função deles é vencer anticheat; param na análise. Autowall/safe point/backtrack/chams/QoL são código de jogo e mesmo assim só entram com ordem explícita nomeando a feature, reserva antes de escrever, um escritor e um build por ciclo.

## 5. Erros de método registrados (para não repetir)

- ❗ minha versão anterior dizia "chams ausente", "fast stop ausente", "viewmodel offset ausente", "clantag/nametag ausente", "stream-proof ausente". Quatro estavam errados e um é verdadeiramente placeholder desabilitado. Causa: grep só do lado do vocabulário de mercado.
- ❗ a linha 8 desta mesma doc afirmava que os dados de penetração vinham de `surfaceproperties.txt`/`.vphys` do jogo. Falso: nenhum dos dois arquivos existe no install, e o `pak01_dir.vpk` só carrega as propriedades compiladas dentro dos artefatos `.vmat`. Hoje o caminho é o ponteiro de superfície do próprio trace (§3.1).
- Claims falsificados de lista delegada: `PoseHistory` **não** é backtrack; `HazzureBrake` é freio de trigger; os hits de `silent`/`choke`/`Lua` eram substrings de sticker.
- Regra de busca: enumerar primeiro pelo nosso lado (labels de UI + campos de config), depois casar com o nome de mercado; e `src/catalogs/armory/*.inc` contamina qualquer grep de feature.

## 6. Qualidade da evidência

Nenhum menu vivo de cheat comercial foi enumerado. `gamesense.vip` (fóruns `id=1262`, `id=1380`) e o wiki ficam atrás de challenge JS — só snippets de busca. `memesense.net`, `neverlose.cc`, `realdudesinc.net` são copy de vendedor (as linhas "3 ms", "no wave bans", "FaceIt safe mode" não são fato nem meta). Referências de código usadas como base real: `references/Andromeda-CS2-Base` e `references/Aspasia` (esta segunda também vende o swap de material em `Classes/Classes.h:299-306`, `Features/Visuals/Visual.cpp:85`). Nada aqui foi testado em partida.
