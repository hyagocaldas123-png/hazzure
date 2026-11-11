# Briefing — T059: Redesign GUI MemeSense (6 Telas de Referência) + Pacote MemeSense CS2 (2026-11-07)

Briefing autossuficiente consolidando:
1. Os **recursos de engenharia do MemeSense CS2** (`visuals.gg/product/memesense-cs2`) mapeados sobre a base **t058** (`401cae7e67c075d2ac7c87ef2337e9c71b47367b4b87e942060909a768bce849`, 2.992.640 B).
2. A **reestruturação visual e estrutural da GUI** baseada nas **6 capturas oficiais do MemeSense** enviadas pelo usuário (`Inventory`, `Legitbot`, `Players`, `Chams`, `Visuals` e `World`), preservando a identidade visual **Hazzure** (`[Haz]zure` no topo) e a regra da casa: **zero chaves de vitrine sem consumidor**.

---

## Entrada e estado vigente
- Ordem de leitura: `C:/Codex2/Hazzure_Projeto/Inicio.md` → `00_Gestao/Estado.md` → `00_Gestao/VALIDACAO.md` → `00_Gestao/Reservas.md` (reserva obrigatória antes de editar) → `70_Ferramentas/FERRAMENTAS.md`.
- Código: `C:\Codex2\gpt\HazzureInternal\src` (`build.cmd` gera `build\Hazzure.Internal.dll` e executa `Hazzure.Internal.Test.exe`).
- Base atual em disco: **t058**, SHA-256 `401cae7e67c075d2ac7c87ef2337e9c71b47367b4b87e942060909a768bce849` (2.992.640 B).

---

## Parte A — Mudanças na GUI (5 Telas do MemeSense Enviadas)

### 1. Navegação Lateral (Sidebar) e Topbar Contextual (`src/GSLayout.inl:77-142, 291-428`)
Nos 5 prints de referência, a barra lateral esquerda tem **11 abas visíveis** com ícones minimalistas e sem cabeçalhos divisores (`CONTROLS / APPEARANCE / WORKSPACE`):
1. `Legitbot` (hoje chamado `Aim Assist` no índice 0)
2. `Aim Assist` (ou `Rage` no nosso índice 1)
3. `Players` (hoje oculto em `gs_hiddenTabs[2] = true` — **deve voltar a ser aba própria visível** separada de `Visuals`!)
4. `Chams` (índice 3)
5. `Items` (índice 5)
6. `Visuals` (índice 6 — focado em `Grenades` e `Miscellaneous` visual, separado de `Players`)
7. `World` (índice 7)
8. `View` (índice 8)
9. `Indicators` (índice 9)
10. `Miscellaneous` (índice 10)
11. `Inventory` (índice 11)
12. `Configs` (índice 13)

**Barra superior contextual (Header):**
- Em vez de mostrar uma barra de busca genérica em todas as abas, o topo do MemeSense muda conforme a aba ativa:
  - Em **`Legitbot` (Print 2)**: `[x] Master switch` + ícone de engrenagem + combo de escopo (`Global` / `Per weapons` / `Per group`) + combo da arma (`Desert Eagle`, `AK-47`, `AWP`, etc.) + botão `Save` à direita.
  - Em **`Chams` (Print 4)**: `[x] Master switch` + combo do alvo (`Players`, `Viewmodel / Arms`, `Weapons`) + botão `Options` + botão `Save` à direita.
  - Em **`Inventory` (Print 1)**: botões `[Settings]` e `[Create from link]` à esquerda + botão `Save` à direita.
  - Em **`Players` (Print 3)** e **`Visuals` (Print 5)**: header limpo apenas com o botão `Save` à direita.

---

### 2. Tela 1 — `Inventory` (`src/InventoryPanel.inl:18-84, 200-450`)
Referência: **Print 1 (`Inventory > Equipment > Rifles`)**.
- **Duas barras horizontais de sub-abas com sublinhado fino na aba ativa:**
  - Nível 1: `Profile` | `Equipment` | `Containers` | `Tools` | `Graphic Art` (exibir as 5 abas na barra superior do Inventory).
  - Nível 2 (quando `Equipment` ou `Graphic Art` está ativo): `Knives` | `Pistols` | `Mid-Tier` | `Rifles` | `Misc` | `Agents` | `Gloves`.
- **Grid de Cards Quadrados (4 colunas):**
  - O **1º card da grade (canto superior esquerdo)** é um card escuro com um círculo central e o símbolo **`+`** grande — clicar nele abre o seletor/catálogo para adicionar nova skin na categoria atual (`Rifles`, `Knives`, `Gloves`, etc.).
  - Os demais cards da grade exibem:
    - Fundo escuro (`#141418`) com a **miniatura PNG da arma/skin** centralizada (`C:\HazzureKernel\assets\skins\`).
    - Nome da skin em **negrito branco** no canto inferior esquerdo (ex.: `Luxe Trim`, `Turbo Peek`, `Asiimov`, `Winter Forest`, `Mecha Industries`, `Vulcan`).
    - **Barra inferior de raridade (3 px)** ocupando toda a largura inferior do card: Vermelho Covert (`#EB4B4B`), Rosa Classified (`#D32CE6`), Roxo Restricted (`#8847FF`), Azul Mil-Spec (`#4B69FF`).

---

### 3. Tela 2 — `Legitbot` (`src/GSLayout.inl:444-505`)
Referência: **Print 2 (`Legitbot — General | Hit groups`)**.
- **Coluna Esquerda (`General`):**
  - `[x] Enable` (com botão de opções `≡` à direita)
  - `[ ] Aim key` + badge de tecla + dropdown `Always` / `On key` / `Toggle`
  - `[ ] Draw FOV`
  - Slider `Aim time` (`disabled` no 0 ms, até `1000 ms`)
  - **Sub-tabs internas em linha (`Aim` | `Accuracy` | `RCS` | `Misc`) com sublinhado:**
    - Na sub-aba `Aim`:
      - `Radius type`: combo (`Static`, `Dynamic / Distance`)
      - `Radius`: slider em graus (`0.100°` – `25.000°`, formato `%.3f°`)
      - `Multiplier`: slider (`1.00` – `10.00`)
      - `Smooth type`: combo (`Static`, `Dynamic`)
      - `Smooth pitch`: slider (`1.000` – `25.000`)
      - `Smooth yaw`: slider (`1.000` – `25.000`)
- **Coluna Direita (`Hit groups`):**
  - `Selection`: combo multi-seleção (`Head, Neck, Chest, Pelvis`)
  - `[ ] Only body aim` + keybind (`forceBodyAim` / `forceBodyAimKey`)
  - `[x] Prefer head aim`
  - `[ ] Prefer body aim` (`aimBodyFallback`)
  - `[ ] Body aim if lethal` (`baimOnPeek`)
  - `Targets`: combo (`Players`)
  - Sub-header lado a lado: **`Auto fire`** | **`Auto wall`**
    - `[ ] Enable` + keybind (`autoFire` / `triggerActive`)
    - `[ ] Auto stop` (`shotStop`)
    - `Hit chance type`: combo (`Strict`, `Balanced`)
    - `Reaction time`: slider (`disabled` no 0 ms, até `500 ms`)
    - `Delay between shots`: slider (`disabled` no 0 ms, até `1000 ms`)
    - `Minimum hit chance`: slider (`0%` – `100%`, ex.: `75%`)
    - `Ghost time`: slider (`disabled` ou `poseHistoryMs`)

---

### 4. Tela 3 — `Players` (`src/GSLayout.inl` case 2 + `src/cards/CardVisuals.inl`)
Referência: **Print 3 (`Players — General | Flags`)**.
- Desocultar `Players` na sidebar (`gs_hiddenTabs[2] = false`) e organizar em **2 colunas limpas sem cards pesados**, apenas com o título centralizado `General` (esquerda) e `Flags` (direita):
- **Coluna Esquerda (`General`):**
  - `[x] Enable` + keybind (`espEnabled`)
  - `[x] Through wall` (visível através da parede / oclusão)
  - `[x] Nickname` + botão `≡` (`espName`)
  - `[x] Box` + botão `≡` (`espBoxes`)
  - `[x] Health bar` + botão `≡` (`espHealth`)
  - `[ ] Armor bar` + seletor de cor + dropdown de posição (`Left`, `Right`, `Top`, `Bottom`) (`espShowArmor`)
  - `[ ] Ammo bar` + seletor de cor + dropdown de posição (`Bottom`, `Top`, `Left`, `Right`) (**nova feature — ver Parte B.2**)
  - `[x] Weapon` + botão `≡` (`espWeapon` + `espWeaponIcons`)
  - `[ ] Skeleton` + botão `≡` (`espSkeleton`)
  - `[ ] Sound arrow` + botão `≡` (`soundRadar` / `soundEsp`)
  - `[x] Out of FOV arrow` + botão `≡` (`offScreenArrows`)
- **Coluna Direita (`Flags`) — cada flag com toggle individual:**
  - `[ ] Bomb` (`C4`)
  - `[ ] Health` (valor numérico de HP)
  - `[ ] Ammo` (texto `[clip/max]`)
  - `[x] Reload` (`m_bInReload`)
  - `[ ] Scoped` (`m_bIsScoped`)
  - `[x] Flashed` (`m_flFlashOverlayAlpha > 0.2f`)
  - `[ ] Distance` (`espDistance`)
  - `[ ] Defuse kit` (`m_bPawnHasDefuser`)
  - `[ ] Helmet & kevlar` (`HK` / `K`)
  - `[x] Defusing / rescuing` (`m_bIsDefusing` / `m_bIsGrabbingHostage`)
  - `[ ] Pin pulled` (`m_bPinPulled`)
  - `[ ] Planting` (`m_bIsPlantingViaUse` / `C_C4::m_bStartedArming`)

---

### 5. Tela 4 — `Chams` (`src/GSLayout.inl:633-648`)
Referência: **Print 4 (`Chams — Enemy team | Local team`)**.
- Em vez de sub-abas separadas (`Chams / Gradient / Flags / Materials`), exibir **2 colunas lado a lado** na mesma tela:
  - **Coluna Esquerda (`Enemy team`):**
    - `[ ] Gun game immunity`
    - Subtítulo `Customizations`
    - `[x] Glow` + seletor de cor (`glowOutline` + `colEnemy`)
    - `[x] Visible` + seletor de cor (`colVisible`) + dropdown de material (`Textured`, `Flat`, `Metallic`, `Wireframe`, `Crystal`) (`chamsVisible` + `chamsVisibleStyle`)
    - `[x] Invisible` + seletor de cor (`colHidden`) + dropdown de material (`Textured`, `Flat`, `Metallic`, `Wireframe`, `Crystal`) (`chamsHidden` + `chamsHiddenStyle`)
  - **Coluna Direita (`Local team` / `Viewmodel` conforme seletor do topo):**
    - `[ ] Gun game immunity`
    - Subtítulo `Customizations`
    - `[ ] Glow` + seletor de cor (`colTeam`)
    - `[ ] Visible` + seletor de cor + dropdown de material (`Textured` / `Flat`)
    - `[ ] Invisible` + seletor de cor + dropdown de material (`Textured` / `Flat`)

---

### 6. Tela 5 — `Visuals` (`src/GSLayout.inl:660-690`)
Referência: **Print 5 (`Visuals — Grenades | Miscellaneous`)**.
- Com `Players` na aba 2, a aba `Visuals` fica enxuta em **2 colunas lado a lado**:
  - **Coluna Esquerda (`Grenades`):**
    - `[x] Grenade prediction` + 2 seletores de cor (`colTrajectory`, `colTrajectoryCollision`) (`showGrenadeTrajectory`)
    - `[x] Smoke grenade timer` + seletor de cor (`colSmoke`) (`grenadeZones`)
    - `[x] Incendiary grenade timer` + seletor de cor (`colMolotov`) (`grenadeZones`)
    - `[x] Grenade tracers` + botão `≡` (`grenadeProjectiles`)
  - **Coluna Direita (`Miscellaneous`):**
    - `[x] Bomb timer` (`showBombTimer`)
    - `[x] Spectator list` (`showSpectators`)
    - `[x] Hit marker` + seletor de cor (`showHitmarker`)
    - `[ ] Textured radar` + botão `≡` (`minimapEnabled` / `nativeRadar`)

---

### 7. Tela 6 — `World` (`src/GSLayout.inl:691-743`, `src/Climate.h`, `src/BasicTuning.h`)
Referência: **Print 6 (`World — Textures + Extra | Removals`)**.
- Em vez de 4 sub-abas separadas (`Skybox / Lighting / Textures / Removals`), unificar a aba `World` em **2 colunas lado a lado** na mesma tela (sem topbar de sub-abas):
  - **Coluna Esquerda — `Textures`:**
    - `[x] Skybox` + seletor de cor (`skyActive` + `colSky`, ex.: preto `#000000` ou roxo sólido)
    - `[x] Lightning` + botão `≡` (`darkWorld` + popup/expansão para `skyboxIndex`, `tsukuyomiTheme`, `vignetteEnabled`, `snowParticles`, `weather3d`)
    - `[x] World textures` + seletor de cor (`colFog` / modulação de cor do cenário)
    - Slider `Exposure` (`worldBrightness`, `0.05` – `2.00`, ex.: `1.651` — alimentando `ExposureValues` em `Climate.h`)
  - **Coluna Esquerda — `Extra`:**
    - `[x] Force sniper crosshair` (**nova feature real**: desenha mira central quando segurando AWP/SSG 08/SCAR-20/G3SG1 sem zoom, `m_bIsScoped == 0`)
    - `[ ] Penetration crosshair` + botão `≡` (**nova feature real**: usa o nosso `HazzureAutowall::Evaluate` do centro da tela para desenhar um indicador na mira mudando de cor quando a parede na frente é varável!)
    - Slider `Flash scale` (`0.00` – `1.00`, ex.: `0.67` → mapeado diretamente para `flashAlphaLimit` + `limitFlash` em `BasicTuning.h`)
  - **Coluna Direita — `Removals` (somente itens com consumidor real em runtime):**
    - `[ ] Shadows` (`cleanMap` → distância de sombras no pawn)
    - `[ ] Zoom hud` (`noScopeOverlay` + `cleanSniperScopeLines`)
    - `[ ] Zoom spread` (`showSpreadCircle`)
    - `[ ] Visual recoil` (`noVisualRecoil` em `BasicTuning.h`)
    - `[ ] Weather original` (`removeOriginalWeather` em `Climate.h`)
    - *(Nota de auditoria: itens como `Blood`, `Bullet holes`, `3D skybox`, `Smoke grenades`, `First-person legs` e `Team intro` só entram na coluna `Removals` se tiverem consumidor real verificado no `client.dll` 14194; caso contrário ficam de fora para respeitar a regra de zero vitrines da T058).*

---

## Parte B — Features de Engenharia do MemeSense CS2 (`visuals.gg`)

### 1. Viewmodel & Aimbot Per-Weapon direto no Topo (`src/BasicTuning.h`, `src/GSLayout.inl`)
- Fazer `BasicTuning.h` resolver `viewmodelX`, `viewmodelY`, `viewmodelZ` e `viewmodelFov` via `RuntimeFloat(..., weaponClass)` para que Faca, Pistola, Rifle e Sniper possam ter offsets de viewmodel independentes.
- Conectar o seletor `Per weapons` / `Desert Eagle` do header da aba `Legitbot` diretamente ao mapa `g_weaponClasses` / `g_contexts` de `OverlayPersistence.h`.

### 2. `Ammo Bar` + Flags Individuais no ESP (`src/CombatRender.h`, `src/DynamicSchema.h`)
- Resolver `m_iClip1` (`C_BasePlayerWeapon::m_iClip1`) em `DynamicSchema.h` com fallback seguro.
- Em `CaptureCombatGhost` (`CombatRender.h`), ler `m_iClip1` da arma ativa do jogador e desenhar a `Ammo bar` azul (`IM_COL32(80, 165, 255, 255)`) na posição escolhida (`Bottom` por padrão), além de separar os bits de flags individuais da coluna `Flags` da aba `Players` (`Flashed`, `Defusing/Rescuing`, `Planting`, `Ammo`, `Helmet & Kevlar`).

### 3. Supressão Real do `HudScope` / `legacy_sniper_scope.vmat` nas Snipers (`src/ScopeOverlay.h`, `src/UITacticalHud.inl`)
- Complementar as `cleanSniperScopeLines` com a supressão da borda preta das snipers (`AWP`, `SSG 08`, `SCAR-20`, `G3SG1`) atuando sobre o efeito de scope sniper mapeado no `client.dll` (`0x6AEBC4DA`).

### 4. Sincronização 1-Clique de `Solid Sky Color` + `Fog Tint` (`src/cards/CardEnvironment.inl`)
- Adicionar opção de sincronizar automaticamente a cor e densidade da neblina (`colFog`) com a cor do céu (`colSky`), reproduzindo o visual de atmosfera uniforme do MemeSense.

### 5. `Force Sniper Crosshair` & `Penetration Crosshair` (da Tela 6 `World > Extra`)
- **`Force sniper crosshair` (`forceSniperCrosshair`):** quando a arma ativa do jogador local é sniper (`def == 9` AWP, `40` SSG 08, `38` SCAR-20, `11` G3SG1) e `m_bIsScoped == 0`, desenha a retícula central fina na tela (`UITacticalHud.inl`).
- **`Penetration crosshair` (`penetrationCrosshair`):** lança 1 raio central curto pelo nosso `HazzureAutowall::Evaluate` (`Autowall.h`) na direção da câmera e pinta o ponto central da mira de **verde** quando a parede/obstáculo à frente permite penetração (`layers >= 1 && reachable`) ou **vermelho** quando bloqueado.

### 6. `Auto-Accept Matchmaking` (`src/UIMovement.inl` / `src/UIFrame.inl`)
- Toggle `Auto-Accept Matchmaking` na aba `Miscellaneous` para confirmar partida encontrada automaticamente quando na fila.

---

## Critérios de Aceitação
1. Reserva prévia em `00_Gestao/Reservas.md` e backups `.bak-20261107-t059`.
2. Build único via `build.cmd > build_t059.log 2>&1` com **ExitCode = 0**, **0** `error C` e **10/10 PASS** em `Hazzure.Internal.Test.exe`.
3. Deploy sincronizado em `C:\HazzureKernel\Hazzure.Internal.dll` e `C:\HazzureKernel\modules\Hazzure.Internal.dll` com rollback preservado.
4. `python C:/Codex2/gpt/tools/vault_sync.py --check` retornando **exit 0**.
