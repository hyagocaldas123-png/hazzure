# Matriz H — o que o Hazzure tem, meia-tem e não tem

**Data:** 2026-11-08 (07:5x), base **t071** em disco (`252028ca0e3d…`, 3.016.704 B, deploy 07:52:25); **nada em memória** — `tasklist` 0 `cs2.exe` e a última sessão viva é `[START]` **05:10:48** (linha 1.111.095), que rodou o **t069** (`5938ACF4…`, deploy 02:55:40) com `shotTick=0` e `aimWriteOk=0`, ou seja o caminho do sub-tick nunca foi exercido em partida. **Escritor:** coordenador (T070 = RE do canal de ângulos, sem código; T071 = 1ª perna do desync). A linha anterior desta cabeçalho descrevia a sessão do t058 (09:24:10, 27.391 aplicações de AA via `SetViewAngles`, probe com 512 runs esgotados) — mantida aqui como histórico: foi ela que abriu a pergunta que a T070 fechou. Reserva em `00_Gestao/Reservas.md`.
**Método:** cada linha parte das **nossas próprias labels e chaves**, não de termos de mercado — `30_Inventario/MENU.md`
(454 widgets gerados do código), `30_Inventario/CONFIG_KEYS.md` (101 chaves `Runtime*` com `lida em arquivo:linha`) e
`grep` em `HazzureInternal/src`. Quando a evidência é um "0 ocorrências", o termo pesquisado está na própria linha.
Este arquivo é **mantido à mão** (não é gerado por `tools/vault_sync.py`, por isso vive em `20_Tarefas/` e não em `30_Inventario/`).
**Precedência:** quando esta matriz divergir de `20_Tarefas/MAPA_LACUNAS_MERCADO.md` (2026-11-05, base t044), manda a
linha desta doc — ela é mais nova e já incorpora T045–T071 (timer da C4 validado, auto-fire fora do rage, anti-aim no
canal do comando, scope mapeado por desmontagem, Pacotes 2 e 1 de Features nas T056/T057, limpeza de vitrines/Movement
na T058, rage fases 1/2 nas T068/T069 e a 1ª perna do desync na T071).

Legenda: ✅ existe e faz o que o nome diz · 🟡 existe incompleto/parcial · ❌ não existe ·
⛔ **não é lacuna** — linha vermelha decidida pelo usuário (análise-only ou rejeitado), ver §4.
**A legenda ⛔ está aposentada desde 2026-11-08**: o usuário revogou as cinco linhas ("Confirma, revoga as
cinco"), então pSilent, desync/body yaw/manual yaw/fake lag/choke, resolver/prefire/backtrack e dano de
penetração voltaram a ser lacunas comuns (❌/🟡) e são tratadas como backlog, não como proibidas.

Status de runtime: ✅/🟡 nesta tabela significam **implementado**, não "validado em partida". A fonte do que foi visto
rodando é `00_Gestao/VALIDACAO.md`; o que está aqui como ✅ e ainda não foi visto em jogo está marcado em §5.

---

## 1. Mira e tiro

| Feature | H | Evidência (menu/código) |
|---|---|---|
| Aimbot legit (FOV/smooth/hitbox) | ✅ | `Aimbot Ativo`, `FOV`, `Smooth`, `Osso Alvo`, `Seleção de Alvo`, `aimMultipoint`+`multipointScale`, `aimDistanceFov` |
| Perfil de aim por arma | ✅ | `Perfis & Overrides (v3)` com `Classe Arma`+`Variável`+`Valor`; `Runtime*(nome, fallback, contexto, classeDaArma)` resolve por arma (`OverlayPersistence.h:420`) |
| RCS / recoil view | ✅ | `rcsActive`, `rcsStrength`, `rcsScaleX/Y`, `rcsSmooth`, `Suprimir Recuo Visual` |
| Triggerbot (+flash/smoke/air check) | ✅ | `Triggerbot Ativo` (`triggerActive`), `Delay Disparo` (`triggerDelayMs`), `Anti-Smoke Estrito` (`strictSmoke`), `Nao Disparar Cego (Flash Check)` (`triggerFlashCheck`) e `Nao Disparar no Ar (Air Check)` (`triggerAirCheck`) em `CardCombat.inl:146-148` e `GameplayRuntime.h:571-579` (T057) |
| AutoWall + minimum damage | 🟡 | `autowallEnabled`, `Camadas maximas`, `Penalidade por camada`, `autowallChancePenalty` — decide **alcance**, não dano; modelo de dano de penetração adiado por decisão |
| Hit chance / multipoint | ✅ | `hitchanceEnabled`, `hitchancePercent`, multipoint acima |
| Silent aim | 🟡 | `Silent Aim` existe no card Rage e escreve o ângulo resolvido só no tick de disparo. Desde a T069 o canal é o sub-tick: com `ragePerfectSilent` ligado o ângulo vai direto para o `input_history_` (`StampSubtickViewAngles`, `InternalStartup.inl:326-338`) sem tocar em `pCSGOInput` (zero snap); sem ele, ainda passa pelo `SetViewAngles`+restaurar. **Nenhum dos dois caminhos foi exercido em partida** — a sessão do t069 teve `shotTick=0`. Validação HvH pendente |
| Backtrack | ❌ | `backtrack`/`Backtrack` = **0 ocorrências no código inteiro**; `PoseHistory` é histórico visual (`CardVisuals.inl:74`), o aimbot nunca usa posição rebobinada |
| Extrapolação (prediction por ping) | ✅ | `Extrapolacao por Ping` (`aimExtrapolation`) + `Escala de Extrapolacao` (`aimExtrapolationScale`) em `CardCombat.inl:221` e `GSLayout.inl:579`; predição linear por `velocity * (ping/1000 + dt) * scale` em `GameplayRuntime.h:350` (T056) |
| Safe point / prefer+force | ✅ | `Safe Point` (`safePointMode`: Desativado / Preferir / Forçar) em `CardCombat.inl:220` e `GSLayout.inl:578`; interseção de multipoints da cápsula (`probeScale >= 0.35`, `>= 3` pontos alcançáveis + `chance >= 55%`) em `CombatTargeting.inl:27-76` (T056) |
| Force body aim / baim-on-peek | ✅ | `Forcar Body Aim (Baim)` (`forceBodyAim`, `forceBodyAimKey`), `Baim on Peek (Alvo em Movimento)` (`baimOnPeek`) e `Preferir Corpo se Cabeca Incerta` (`aimBodyFallback`) em `CardCombat.inl:218-221`, `GSLayout.inl:579-580`, `CombatTargeting.inl:30-36,48,98` e `GameplayRuntime.h:845-849` (T057) |
| Autostop / quick stop | ✅ | `rageAutoStop`, `shotStop`, `shotFullStop`, `shotStopSpeed`, `shotStopEdgeGuard` |
| Auto-fire (mira no alvo) | ✅ | `autoFire` (saiu do card Rage na T053, hoje em Legitbot › Misc) + tolerância `autoFireOnTargetDeg`, medido por `[AUTOFIRE] … residual=` |
| Silent aim "P" (pSilent) | ❌ | ⛔ revogado em 2026-11-08 ("Confirma, revoga as cinco"); continua 0 ocorrências — backlog, não proibição |

## 2. Anti-aim / evasion

| Feature | H | Evidência |
|---|---|---|
| AA defensiva / mouse override / at-targets | ✅ | `At-Targets (Olhar Oposto ao Alvo)` (`rageAntiAimAtTargets`), `Mouse Override Direcional` (`rageAntiAimMouseOverride`) e `Anti-Aim Defensivo (Ar/Velocidade)` (`rageAntiAimDefensive`) em `GSLayout.inl:600-602` e `GameplayRuntime.h:436-470` (T056) |
| Jitter / spin / pitch | ✅ | `Modo` (Estatico/Jitter/Spin), `Pitch` e `Pitch Invert` (`rageAntiAimPitchInvert`) em `GSLayout.inl:603` e `GameplayRuntime.h:471` (T056) |
| Fake lag / choke | ❌ | ⛔ revogado em 2026-11-08. `Choke` = 0 ocorrências no código inteiro — backlog, nada implementado |
| Desync / body yaw / LBY | 🟡 | ⛔ revogado em 2026-11-08. **T071 implementou a 1ª perna**: `rageDesync` + `rageDesyncFakeLimit` (0..60, padrão 35°) no card Anti-Aim (`GSLayout.inl:980-982`); em tick de AA o yaw limitado é gravado direto no `input_history_` do sub-tick (`StampSubtickViewAngles`, `InternalStartup.inl:339-354`) **sem tocar na câmera** (o `SetViewAngles` legado é pulado, `InternalStartup.inl:266`) e o WASD/strafe é reprojetado contra esse `yawSent` (`InternalStartup.inl:618`). Falta: body yaw/LBY pelo AnimGraph, limite dinâmico por estado, desync em tick de tiro — e **nada validado em partida** (a sessão t071 das 08:01 caiu antes de qualquer tick de desync; com o has-bit do carimbo corrigido, **t072 está em disco e aguarda injeção**) |
| Manual yaw + quick peek | ❌ | ⛔ revogado em 2026-11-08. `quickPeek` = 0; o manual yaw por tecla continua sem código (o offset estático do AA não é manual yaw: é relativo à câmera, não absoluto) |
| Fake duck | ✅ | `Fake Duck` (`fakeDuck` + `fakeDuckKey`) em `GSLayout.inl:604`, ciclo de oscilação em `UIMovement.inl:113` e carimbo de `IN_DUCK` no `CUserCmd` via `StampCommandButtonMasks` (`InternalStartup.inl:142`, `AdvancedMovement.h:425`) (T056) |
| Anti-aim publicado no comando do servidor | 🟡 | A linha da T054 (probe empírico de float nos primeiros 512 B) **foi fechada pela RE da T070**: os ângulos que o servidor consome vivem no `CMsgQAngle` do `input_history_` protobuf (`CCSGOInputHistoryEntryPB`: `pViewAngles=+0x18`, máscara `0x1`=VIEWANGLES — a T072 corrigiu o carimbo, que gravava `0x2`=SHOOTPOSITION; `CMsgQAngle`: has_bits `+0x10`, x/y/z `+0x18/+0x1C/+0x20`; desmontagem `0xB7C711..0xB7C763` bate byte a byte com `StampSubtickViewAngles`), e `cmdOff=-1` era o fail-closed correto do probe, não falha. A escrita real do AA no canal do comando é a da T071 (desync, acima) — compilada, **não validada em partida** |

## 3. Visual

| Feature | H | Evidência |
|---|---|---|
| Chams visível+oculto, cores, glow | ✅ | 8 widgets + `Material Visivel/Oculto`, `Contorno Brilhante (Outline Glow)`, `glowOutline`/`glowRange`; hook real de draw (`InternalStartup.inl`) |
| Layered / ragdoll / on-shot glow / skychams / water-flow | 🟡 | `Dual Layer` (`chamsDualLayer`) é real; os 3 toggles sem consumidor (`chamsRagdoll`, `chamsOnShot`, `chamsSky`) foram **removidos do menu na T058** para não haver vitrines sem consumidor. `Ragdoll`/water-flow = 0 |
| ESP (box/skeleton/flags/weapon/armor) | ✅ | boxes, esqueleto, vida, `Barra de Armadura` (`espShowArmor`, `m_ArmorValue`), nome, arma+ícones, distância, snaplines, offscreen, dormant/fade em `CardVisuals.inl:45-48` e `CombatRender.h:120-122,186-191` (T057) |
| Flags extras (reloading/planted/pin/defuse kit/immunity) | ✅ | `Flags Taticas (Reload/Kit/Pin/Imune/C4)` (`espFlagsExtras`) no ESP lateral (`C4`, `KIT`, `RELOAD`, `PIN!`, `ZOOM`, `IMMUNE`) em `CardVisuals.inl:47` e `CombatRender.h:123-135,192-204` (T057) |
| Event logs / scoreboard override / ESP animations | ✅ | `Killfeed Persistente`, `Event Logs na Tela` (`eventLogsScreen` → `RenderEventLogsHUD`), `Scoreboard Override (TAB)` (`scoreboardRevealAll` → `RenderScoreboardOverrideHUD`) em `UIFrame.inl:225-227` e `Animacao Suave do ESP` (`espSmoothAnimations`) ligado ao interpolador real `HazzureCombatRefine::Health` em `CombatRender.h:112` (T058) |
| Damage indicators / tracers / impacts / hitmarker | ✅ | `Dano Flutuante 3D`, `Bullet Tracers 3D`, `Caixas de Impacto`, `Hitmarker na Mira`, `Efeito de Eliminação` |
| Sound ESP + radar 2D custom | ✅ | `Sound ESP / Anéis 3D`, `Alertas Sonoros Radiais`, `Radar Nativo`, `In-game radar` |
| Grenade trajectory + proximity + timers | ✅ | `Trajetoria 3D Estimada`, `Grenade Prediction`, `Smoke Timer`, `Incendiary Timer`, `Granadas no ar` |
| Smoke/Molotov radius colorido | ✅ | `Áreas Estimadas de Granadas`, `Raio Individual por Granada (3D)` (`grenadeIndividualRadius`), `Raio da Smoke` (`smokeRadiusUnits`), `Raio do Molotov` (`molotovRadiusUnits`), `colSmoke` e `colMolotov` em `CardVisuals.inl:79-83` e `GrenadeZoneDrawing.inl:8-13` (T057) |
| Bomb/defuser/hostage ESP + rescue | ✅ | `Timer da C4` + `Aviso de Bomba Plantada` (**validados em partida na T055**) + `ESP Defuser / Refens / Resgate` (`espDefuserHostage`) em `CardVisuals.inl:76` e `GameplayRuntime.h:754-779` (T057) |
| Lineup helper / gravar+replay | ✅ | lineup gravado de verdade: `Tipo Gravado`, `Corrida Gravada`, `Nome`, `Validado para automacao`, `Alinhar e Arremessar por Tecla`, `Reconhecer Mapa Automaticamente` |
| Skybox, fog, nightmode, color-grade, weather, lua | 🟡 | skybox (seletor/modulação/brilho), fog (densidade+tint), night mode, `Exposição Cenário`, correção de cor completa, clima 3D e lua são reais; os 3 toggles sem consumidor (`worldFullbright`, `worldSunRotation`, `worldDofDisable`) foram **removidos do menu na T058** |
| Removals granulares | 🟡 | `Clean Map (Sem Sombras)`, `No Flash` e `Remover Clima Original` são reais (pawn `+0x150c/0x1504`, clima); os 4 toggles sem consumidor (`removeDecals`, `removeRagdolls`, `removeSky3D`, `removePostProcessing`) foram **removidos do menu na T058** |
| Scope visual (borda preta do AWP/SSG) | 🟡 | **A borda não mudou de lugar.** `noScopeOverlay` + `scopeFilterAlpha` agem sobre `cl_ironsight_filter_alpha` (`0x24DBE50`) e, pela negativa provada na T055, isso só governa **AUG/SG 553**. O que a T057 acrescentou é `Linhas Limpas do Scope Sniper` (`cleanSniperScopeLines`), que desenha uma **retícula nossa por cima** quando `m_bIsScoped==1` (`UITacticalHud.inl:296-314`) — o painel Panorama `HudScope` e o `legacy_sniper_scope.vmat` continuam intactos por baixo. Nunca medido com `noScopeOverlay=1` em sessão (`[SCOPE] … ligado=0 writes=0`) |

## 4. Movimento, view, itens, misc

| Feature | H | Evidência |
|---|---|---|
| Bhop / strafe / jumpbug / edgebug / pixel surf / fast ladder | ✅ | **Enxugado no menu na T058 por pedido explícito do usuário** (`"limpe tudo que esta de sobra ou nao precisa estar no moviment"`): card `Movimentacao` agora exibe apenas `BunnyHop Automatico` (`bhopActive`), `Auto-Strafe no Ar` (`autoStrafePhysics`, que liga o steer de ar diretamente sem exigir toggles extras), `Crouch&Jump Automatico (DuckJump)`, `Slowwalk`, `Fast Stop` e `Velocimetro Compacto` (`CardMovement.inl:7-22`, `UIMovement.inl:216`) |
| Slow walk / slide / fast stop / ramp boost / anti-AFK | 🟡 | `slowWalk`(+`slowWalkKey`/`slowWalkSpeed`), `fastStop`, `antiAfk` e `crouchJumpAuto` no menu limpo (`CardMovement.inl:13-35`) com cadeia real: `ProcessMovementMechanics` (`UIMovement.inl:391-442`, chamado por `UIFrame.inl:173`) → `PublishCmdButtonMasks` → `ReadCmdButtonMasks`/`StampCommandButtonMasks` no hook (`InternalStartup.inl:288-290`). **`ramp boost` = 0 ocorrências** |
| Crouch&Jump / Stand&Jump auto | ✅ | `Crouch&Jump Automatico (DuckJump)` (`crouchJumpAuto`) em `CardMovement.inl:13`, `UIMovement.inl:433` e `StampCommandButtonMasks` (`InternalStartup.inl:142`) (T056/T058) |
| Viewmodel offsets/FOV/aspect/thirdperson | ✅ | `Custom Viewmodel` X/Y/Z + `FOV da Arma`, `Forçar Proporção`, `Thirdperson` com distância (3P validado em partida, com `pawnPerguntado=0`) |
| Limit flash / streamproof / unload | ✅ | `Limitar Alpha do Flash` + `Flash Alpha`, `Stream-Proof Interno` e botão `Unload cheat` em `GSLayout.inl:523` acionando `HazzureState::g_unloadRequested` / `PerformProductUnload` (`StateRegistry.h:155`) |
| Skin changer (armas/facas/luvas/agentes) | ✅ | `Modulo Armory`, `Modelo da faca`, chams de braços/luvas, `Agent changer` CT/TR |
| Stickers / charms / nametag / seed / wear / StatTrak | ✅ | os 6 com widgets próprios (5 slots, chaveiro com seed/xyz, StatTrak com contador, nametag) |
| Graffiti, music kits, case opening, inspect link | ❌ | 0 fora de `catalogs/armory/stickers-*.inc` (os 384 hits de `graffiti` ali são IDs de sticker, ex. `atlanta2017_team_astr_graffiti`); `MusicKit`/`music_kit`/`case.?open` = 0; os 7 hits de `inspect` são `InspectCache` (`NativeInventoryEngine.h:229`) e comentário, não o link de inspeção; os 31 hits de "case" são `switch/case` |
| Chams por item / model changer / radio commands | ❌ | `ModelChanger`/`RadioCommand` = 0; escrever modelo na entidade das mãos já foi rejeitado pelo motor (t042/t043) |
| Clantag / name changer / avatar | ❌ | 0 |
| Auto-accept matchmaking + region picker | ❌ | 0 |
| Auto-buy / automatic grenade release | 🟡 | `Soltar Granada Automatico` (`autoGrenadeRelease`) é real — lê pino/armazenamento da granada (`weapon+0x1f23/0x1f30`) e limpa `IN_ATTACK/IN_ATTACK2` no comando (`UIMovement.inl:445-450`). A vitrine `Auto-Buy` (`autoBuyEnabled`) que só imprimia log foi **removida na T058** (`CardMovement.inl`, `UIMovement.inl:452`) |
| Hit/kill sounds custom | ✅ | `WAV de acerto/headshot/eliminação` + arquivos + `Preset de Acerto` + `Audio Procedural` |
| Configs JSON + perfis + hot-reload | ✅ | `hazzure_config.json`, `Perfis & Overrides (v3)`, `Presets & Configuração` |
| Cloud configs / import por hash | ✅ | `Sincronização nativa` + ShareCodes + `Exportar Config por Hash (SHA-256)` / `Importar Config por Hash` (`HZ-XXXXXXXXXXXXXXXX` em `C:\HazzureKernel\cloud_hashes`, `OverlaySharing.h:31-91`, `CardProfiles.inl:31-37`) (T057) |
| Lua scripting | ❌ | 0 real (os 13 hits de "Lua" são "excluída"/"atualiza") |
| Keybinds overlay / watermark (1 % low, GPU/CPU, var, choke, loss) | 🟡 | `Overlay de Atalhos`, `Telemetry HUD` com `FPS` + `1% low FPS` + `frametime (ms)` + `Ping` + `Tick` (`UIWorldDrawing.inl:371-420`, T056), `Velocímetro Compacto`, `Gráfico de Velocidade`; sem GPU/choke/loss |

## 5. As cinco linhas vermelhas — REVOGADAS em 2026-11-08

Ordem do usuário: "ignore oque eu tinha tido de restriçao, isso nao vale mais" → "Confirma, revoga as cinco"
(`60_Decisoes/Decisoes.md`). Elas voltaram a ser lacunas comuns da matriz e hoje têm este estado:

1. **Fake lag / choke** — ❌ continua sem código; virou backlog, não proibição.
2. **Desync / body yaw / manual yaw / quick peek** — 🟡 **1ª perna implementada na T071**: o yaw do AA é gravado
   no sub-tick (`rageDesync` + `rageDesyncFakeLimit`) sem tocar na câmera, com reprojeção de input no yaw enviado.
   Body yaw por `m_nBodyPitchYawTarget`/AnimGraph, manual yaw por tecla e quick peek seguem ❌.
3. **Resolver de anti-aim inimigo / prefire / backtrack** — ❌ (backtrack = 0 ocorrências no código inteiro).
4. **pSilent** — ❌; a restrição "risco de VAC" foi retirada pelo usuário, mas nada foi implementado.
5. **Dano de penetração no autowall** — ❌; a sonda atual decide **alcance** (chega/não chega), não dano restante.

O que a revogação **não** mudou: só o usuário abre o CS2 e injeta; build/self-test não comprovam runtime;
offset inventado é proibido (RE primeiro); um build por ciclo com rollback.

## 6. Correções aplicadas nesta matriz

1. **Perfil de aim por arma** — ✅, não ❌ (`Perfis & Overrides v3` + `Runtime*(…, classeDaArma)`).
2. **Backtrack** — ❌, não ✅ (ainda 0 ocorrências no código inteiro, mesmo depois da T057).
3. **Lineup record/replay** — ✅, não 🟡 (gravação com `Tipo Gravado`/`Corrida Gravada` existe).
4. **Event logs** — 🟡, não ❌ (`Killfeed Persistente`; o `Event Logs na Tela` da T057 é real, o `espSmoothAnimations` não).
5. **Silent aim** — 🟡, não ❌/✅ de uma vez (existe, escrito no tick de disparo; validação HvH pendente).
6. **Scope visual** — o `cl_ironsight_filter_alpha` que escrevemos **não** é a borda do AWP/SSG (T055, desmontagem `0x833A10`).
7. **Já superadas pela T056/T057** (estavam ❌ e hoje têm código): `Safe Point`, `Extrapolação por ping`,
   `Force body aim`/`baim on peek`, `Fake duck`, `Slowwalk`/`Fast stop`/`Slide`/`Anti-AFK`/`Crouch&Jump`/`Stand&Jump`,
   `At-Targets`/`Mouse override`/`AA defensivo`/`Pitch invert`, `Trigger air/flash check`, armadura e flags do ESP,
   event logs/scoreboard, raio individual de granadas, defuser/refens, import/export por hash e `Unload`.

## 7. Auditoria de consumidores (T056/T057/T058)

**Regra:** uma chave só existe como feature se, além de `UIState.inl` (declaração) e `OverlayPersistence.h` (save/load),
ela é lida por um terceiro arquivo de código. O teste é mecânico:

```bash
# de C:/Codex2/gpt — para cada chave nova: hits e em que arquivos aparece
for k in chamsRagdoll chamsOnShot chamsSky espSmoothAnimations worldFullbright worldSunRotation \
         worldDofDisable removeDecals removeRagdolls removeSky3D removePostProcessing autoBuyEnabled; do
  printf "%-22s %s\n" "$k" "$(grep -rln "\b$k\b" HazzureInternal/src --include=*.h --include=*.inl --include=*.cpp \
      | grep -v '\.bak-' | tr '\n' ' ')"
done
```

**Resolvido na T058 (2026-11-07 sobre o t058 `401cae7e67c0…`):**
- `espSmoothAnimations` passou a ser consumida em `CombatRender.h:112` (`if(g_cfg.espSmoothAnimations)` no interpolador de barra de vida `HazzureCombatRefine::Health`).
- As 10 chaves sem consumidor (`chamsRagdoll`, `chamsOnShot`, `chamsSky`, `worldFullbright`, `worldSunRotation`, `worldDofDisable`, `removeDecals`, `removeRagdolls`, `removeSky3D`, `removePostProcessing`) e o `autoBuyEnabled` (que era só `LogDebug("[AUTOBUY]")`) foram **removidos de `UIState.inl`, `OverlayPersistence.h`, `cards/Card*.inl`, `GSLayout.inl` e `UIMovement.inl`** → **0 chaves de vitrine sem consumidor** no código (`MENU.md`: 378 widgets; `CONFIG_KEYS.md`: 101 chaves).
- A aba Movement (`CardMovement.inl`) foi enxugada por ordem explícita do usuário (`"limpe tudo que esta de sobra ou nao precisa estar no moviment"`), mantendo apenas `BunnyHop Automatico`, `Auto-Strafe no Ar` (`autoStrafePhysics`), `Crouch&Jump Automatico (DuckJump)`, `Slowwalk`, `Fast Stop`, `Velocimetro Compacto`, `Jumpthrow por Tecla`, `Soltar Granada Automatico`, `Auto-Pistol` e `Anti-AFK Automatico`.

**Nada disso foi validado em partida como feature.** Na sessão do t058 (09:24:10, linha 1.044.541) o aimbot escreveu ângulos
(`aimWriteOk` 0→3.395, 285 `[GAMEPLAY]` com `target=1`) e o AA aplicou via `SetViewAngles` (`aaApplyOk=27.391`, `aa=publicado`
em 35 linhas), mas o probe do `CUserCmd` falhou com `cmdHits=0 cmdOff=-1` em todas as 74 linhas `[ANTIAIM]` —
ou seja: o AA e o aimbot **aplicaram** (caminho de `SetViewAngles` e escrita de mira funcionando), mas a **prova de que o ângulo chega ao comando serializado continua pendente** — é isso que a próxima etapa (RE do `CUserCmd`) tem que resolver.

## 8. Como re-verificar

```bash
# de C:/Codex2/gpt
python tools/vault_sync.py --check          # regenera MENU/CONFIG_KEYS/OFFSETS/ARTEFATOS e confere (exit 0)

# linhas ❌ desta doc — medido 2026-11-07 sobre o t057: retorna 0
grep -rniE 'backtrack|clan.?tag|music.?kit|graffiti|auto.?accept|watermark|ramp.?boost' \
     HazzureInternal/src --include=*.h --include=*.inl --include=*.cpp | grep -v '\.bak-'
```

**Armadilha de falso positivo (por isso o `--include` acima é obrigatório):** sem ele, `graffiti` casa 384 vezes em
`HazzureInternal/src/catalogs/armory/stickers-00*.inc` — são IDs de sticker (`atlanta2017_team_astr_graffiti`), não o
recurso de pichação. Mesma familia de erro: `case` (31 hits de `switch/case`) e `Lua` (13 hits de "excluída"/"atualiza").
Negativo só vale se o recorte excluir catálogos e `.bak-`.

Depois de qualquer ciclo que adicione feature: atualizar a linha correspondente, o `Data/base` no cabeçalho e o
registro em `00_Gestao/Reservas.md`. Se a mudança for de UI/offset/deploy, re-gerar `30_Inventario/` antes.
