<!-- NAO EDITAR A MAO (menu): gerado por tools/vault_sync.py
     Fonte da verdade = codigo-fonte + C:/HazzureKernel. Rodar: python tools/vault_sync.py -->
# Inventário do menu (gerado)

_Gerado por `python tools/vault_sync.py` a partir do código. Não edite à mão._

Como ler: **aba** = barra lateral real (`gs_mainTabs`), **seção** = cabeçalho mais próximo acima do widget (`GS_SectionHeader` / `BeginCleanCard` / `CollapsingHeader`), **campo** = o que o widget escreve — `g_cfg.<campo>` quando é campo de `UIState.inl:122 struct OverlayConfig` (aí `default` vem da declaração), ou uma variável local / `shared->…` quando o card escreve por IPC no lado do jogo (nesse caso não há default no struct), **origem** = `arquivo:linha` do widget.

A UI ativa é a do GSLayout (`g_useGSLayout = true` em `UIState.inl`; o roteador é `GSLayout.inl: DrawGSPanelSection`). Widget em `(fora do roteador)` está numa função que o GSLayout não chama — UI legada de `UIPanel.inl` ou código morto; trate como não visitável até prova em jogo.

Limites desta tabela (parser de texto, não execução): a **sub-aba** vem do roteador ou dos botões locais da função; quando um card é chamado dentro de um bloco de sub-aba de outra função (ex.: `DrawLegitbotMemeSense`) ou quando a própria função gerencia as sub-abas internamente (`DrawLocalInventory`), a sub-aba pode estar aproximada — a **aba** e o **arquivo:linha** são exatos. Label montado por concatenação ou dentro de macro pode não aparecer aqui. Rótulo oculto do ImGui (`##id`) é resolvido pelo `ImGui::TextUnformatted` correspondente na mesma linha ou até 4 linhas acima; quando não resolve, o `##id` aparece cru.

O total inclui as duas UIs do repositório: a ativa (GSLayout + cards por `DrawSettingsCardN`) e a legada (`UIPanel.inl`). Por isso o mesmo campo pode aparecer duas vezes com origens diferentes — a linha do GSLayout é a que o usuário vê; a do `UIPanel.inl` não é renderizada enquanto `g_useGSLayout = true`.

## Navegação real

`visível = não` vem de `gs_hiddenTabs[i]`: a aba não aparece na sidebar e, se for selecionada pela busca, é remapeada para outra (`GSLayout.inl:919-923 ValidateNavigationIndices`; o remapeamento atual está nas linhas 921-922). O `case` dela continua no roteador, mas não é alcançável pela sidebar.

| # | aba | visível | seções (topbar) |
|---|---|---|---|
| 0 | Legitbot | sim | Aim, Accuracy, RCS, Misc |
| 1 | Aim Assist | sim | Aim, Anti-Aim |
| 2 | Players | sim | ESP, Glow, Offscreen |
| 3 | Chams | sim | Chams, Gradient, Flags, Materials |
| 4 | Agents | não | Agents |
| 5 | Items | sim | Collection |
| 6 | Visuals | sim | ESP, World, Effects, Glow & Offscreen |
| 7 | World | sim | Skybox, Lighting, Textures, Removals |
| 8 | View | sim | Viewmodel, Camera & Third Person |
| 9 | Indicators | sim | Overlays, Typography |
| 10 | Miscellaneous | sim | General |
| 11 | Inventory | sim | Profile, Editor, Graphic Art |
| 12 | Grenade Helper | não | Lineups & Helper, Prediction, Smoke Timer, Incendiary, Tracers |
| 13 | Configs | sim | General, Profiles, Diagnostics |

## Widgets por aba e sub-aba (topbar)

### Legitbot › Aim — 33 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| General | Enable | `GS_Toggle` | `aimActive` | `true` | `GSLayout.inl:610` |
| General | Aim key | `GS_Keybind` | `aimKey` | `VK_LBUTTON` | `GSLayout.inl:611` |
| General | Draw FOV | `GS_Toggle` | `espFovCircle` | `true` | `GSLayout.inl:612` |
| General | Aim time | `GS_SliderInt` | `aimTime` | `` | `GSLayout.inl:614` |
| General | Radius type | `GS_Combo` | `radType` | `` | `GSLayout.inl:644` |
| General | Radius | `GS_SliderFloat` | `aimFov` | `4.5` | `GSLayout.inl:645` |
| General | Multiplier | `GS_SliderFloat` | `aimEaseBias` | `.35` | `GSLayout.inl:646` |
| General | Smooth type | `GS_Combo` | `smType` | `` | `GSLayout.inl:649` |
| General | Smooth pitch | `GS_SliderFloat` | `aimSmooth` | `4.5` | `GSLayout.inl:650` |
| General | Smooth yaw | `GS_SliderFloat` | `aimSmooth` | `4.5` | `GSLayout.inl:651` |
| Reacao | Atraso de Reacao | `CleanToggle` | `aimReactionDelay` | `false` | `cards/CardCombat.inl:88` |
| Reacao | Atraso Minimo | `CleanSliderFloat` | `low` | `` | `cards/CardCombat.inl:94` |
| Reacao | Atraso Maximo | `CleanSliderFloat` | `high` | `` | `cards/CardCombat.inl:96` |
| Controle de Recoil | RCS Ativo | `CleanToggle` | `rcsActive` | `true` | `cards/CardCombat.inl:112` |
| Controle de Recoil | Compensacao Vertical (Pitch) | `CleanSliderFloat` | `rcsScaleX` | `1.0` | `cards/CardCombat.inl:114` |
| Controle de Recoil | Compensacao Horizontal (Yaw) | `CleanSliderFloat` | `rcsScaleY` | `1.0` | `cards/CardCombat.inl:116` |
| Controle de Recoil | Suavizacao RCS | `CleanSliderFloat` | `rcsSmooth` | `1.` | `cards/CardCombat.inl:118` |
| Controle de Recoil | Intensidade RCS | `CleanSliderFloat` | `rcsStrength` | `.94` | `cards/CardCombat.inl:120` |
| Controle de Recoil | Balas Sem Compensacao | `CleanSliderFloat` | `bulletDelay` | `` | `cards/CardCombat.inl:124` |
| Controle de Recoil | Rampa em Balas | `CleanSliderFloat` | `rampBullets` | `` | `cards/CardCombat.inl:126` |
| Precisao & Pontos | Filtro de Hitchance no Trigger | `CleanToggle` | `hitchanceEnabled` | `true` | `cards/CardCombat.inl:204` |
| Precisao & Pontos | Chance Minima Estimada | `CleanSliderFloat` | `hitchancePercent` | `60.` | `cards/CardCombat.inl:206` |
| Precisao & Pontos | Amostras | `CleanCombo` | `quality` | `` | `cards/CardCombat.inl:212` |
| Precisao & Pontos | Multipoint | `CleanToggle` | `aimMultipoint` | `false` | `cards/CardCombat.inl:214` |
| Precisao & Pontos | Escala dos Pontos | `CleanSliderFloat` | `multipointScale` | `.5` | `cards/CardCombat.inl:216` |
| Precisao & Pontos | Preferir Corpo se Cabeca Incerta | `CleanToggle` | `aimBodyFallback` | `false` | `cards/CardCombat.inl:218` |
| Precisao & Pontos | Forcar Body Aim (Baim) | `CleanToggle` | `forceBodyAim` | `false` | `cards/CardCombat.inl:219` |
| Precisao & Pontos | Tecla Force Baim | `CleanKeybind` | `forceBodyAimKey` | `0` | `cards/CardCombat.inl:220` |
| Precisao & Pontos | Baim on Peek (Alvo em Movimento) | `CleanToggle` | `baimOnPeek` | `false` | `cards/CardCombat.inl:221` |
| Precisao & Pontos | Limiar da Cabeca | `CleanSliderFloat` | `bodyFallbackBelow` | `40.` | `cards/CardCombat.inl:223` |
| Precisao & Pontos | Safe Point | `CleanCombo` | `safePointMode` | `0` | `cards/CardCombat.inl:225` |
| Precisao & Pontos | Extrapolacao por Ping | `CleanToggle` | `aimExtrapolation` | `false` | `cards/CardCombat.inl:226` |
| Precisao & Pontos | Escala de Extrapolacao | `CleanSliderFloat` | `aimExtrapolationScale` | `1.0` | `cards/CardCombat.inl:227` |

### Legitbot › Misc — 17 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| General | Auto-Scope AWP / Scout | `GS_Toggle` | `autoScope` | `false` | `GSLayout.inl:658` |
| General | Visible Only | `GS_Toggle` | `aimVisibleOnly` | `false` | `GSLayout.inl:659` |
| General | After kill delay | `GS_SliderInt` | `aimKillDelayMs` | `0` | `GSLayout.inl:660` |
| General | Target switch delay | `GS_SliderInt` | `aimSwitchDelayMs` | `0` | `GSLayout.inl:661` |
| Hit groups | Selection | `GS_Combo` | `aimBone` | `0` | `GSLayout.inl:666` |
| Hit groups | Only body aim | `GS_ToggleWithKeybind` | `forceBodyAim` + `forceBodyAimKey` | `false` | `GSLayout.inl:667` |
| Hit groups | Prefer head aim | `GS_Toggle` | `prefHead` | `` | `GSLayout.inl:669` |
| Hit groups | Prefer body aim | `GS_Toggle` | `aimBodyFallback` | `false` | `GSLayout.inl:670` |
| Hit groups | Body aim if lethal | `GS_Toggle` | `baimOnPeek` | `false` | `GSLayout.inl:671` |
| Hit groups | Targets | `GS_Combo` | `tgtIdx` | `` | `GSLayout.inl:674` |
| Auto fire \| Auto wall | Enable | `GS_Toggle` | `autoFire` | `false` | `GSLayout.inl:678` |
| Auto fire \| Auto wall | Auto stop | `GS_Toggle` | `shotStop` | `false` | `GSLayout.inl:679` |
| Auto fire \| Auto wall | Hit chance type | `GS_Combo` | `hcIdx` | `` | `GSLayout.inl:682` |
| Auto fire \| Auto wall | Reaction time | `GS_SliderInt` | `reactMs` | `` | `GSLayout.inl:684` |
| Auto fire \| Auto wall | Delay between shots | `GS_SliderInt` | `triggerPauseMs` | `350` | `GSLayout.inl:685` |
| Auto fire \| Auto wall | Minimum hit chance | `GS_SliderFloat` | `hitchancePercent` | `60.` | `GSLayout.inl:686` |
| Auto fire \| Auto wall | Ghost time | `GS_SliderInt` | `poseHistoryMs` | `160` | `GSLayout.inl:687` |

### Aim Assist › Aim — 19 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Rage Aim | Enable | `GS_Toggle` | `rageActive` | `false` | `GSLayout.inl:938` |
| Rage Aim | FOV | `GS_SliderFloat` | `rageFov` | `94.1` | `GSLayout.inl:939` |
| Rage Aim | Smooth | `GS_SliderFloat` | `rageSmooth` | `1.` | `GSLayout.inl:940` |
| Rage Aim | Auto-stop | `GS_Toggle` | `rageAutoStop` | `false` | `GSLayout.inl:941` |
| Rage Aim | Auto-stop velocidade | `GS_SliderFloat` | `rageAutoStopSpeed` | `12.` | `GSLayout.inl:942` |
| Rage Aim | Ignore Smoke | `GS_Toggle` | `rageIgnoreSmoke` | `true` | `GSLayout.inl:943` |
| Rage Aim | Silent Aim | `GS_Toggle` | `rageSilentAim` | `true` | `GSLayout.inl:944` |
| Rage Aim | Perfect Silent (Zero Snap) | `GS_Toggle` | `ragePerfectSilent` | `true` | `GSLayout.inl:945` |
| Rage Aim | Firing Crosshair (Swapchain) | `GS_Toggle` | `rageFiringCrosshair` | `true` | `GSLayout.inl:946` |
| Autowall (chega / nao chega) | Enable##autowall | `GS_Toggle` | `autowallEnabled` | `false` | `GSLayout.inl:950` |
| Autowall (chega / nao chega) | Camadas maximas | `GS_SliderInt` | `autowallMaxLayers` | `2` | `GSLayout.inl:951` |
| Autowall (chega / nao chega) | Penalidade por camada | `GS_SliderFloat` | `autowallChancePenalty` | `30.` | `GSLayout.inl:952` |
| Selecao de alvo | Osso Alvo | `GS_Combo` | `aimBone` | `0` | `GSLayout.inl:956` |
| Selecao de alvo | Selecao de Alvo | `GS_Combo` | `aimTargetMode` | `0` | `GSLayout.inl:958` |
| Selecao de alvo | Safe Point | `GS_Combo` | `safePointMode` | `0` | `GSLayout.inl:960` |
| Selecao de alvo | Force Body Aim (Baim) | `GS_ToggleWithKeybind` | `forceBodyAim` + `forceBodyAimKey` | `false` | `GSLayout.inl:961` |
| Selecao de alvo | Baim on Peek | `GS_Toggle` | `baimOnPeek` | `false` | `GSLayout.inl:962` |
| Selecao de alvo | Extrapolacao por Ping | `GS_Toggle` | `aimExtrapolation` | `false` | `GSLayout.inl:963` |
| Selecao de alvo | Escala Extrapolacao | `GS_SliderFloat` | `aimExtrapolationScale` | `1.0` | `GSLayout.inl:965` |

### Aim Assist › Anti-Aim — 23 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Anti-Aim | Enable | `GS_Toggle` | `rageAntiAim` | `false` | `GSLayout.inl:972` |
| Anti-Aim | Modo | `GS_Combo` | `rageAntiAimMode` | `1` | `GSLayout.inl:974` |
| Anti-Aim | Pitch (+89 = Chao/Guardado) | `GS_SliderFloat` | `rageAntiAimPitch` | `89.` | `GSLayout.inl:975` |
| Anti-Aim | Yaw Offset | `GS_SliderFloat` | `rageAntiAimYaw` | `180.` | `GSLayout.inl:977` |
| Anti-Aim | Jitter Min | `GS_SliderFloat` | `rageAntiAimJitterMin` | `150.` | `GSLayout.inl:979` |
| Anti-Aim | Jitter Max | `GS_SliderFloat` | `rageAntiAimJitterMax` | `210.` | `GSLayout.inl:980` |
| Anti-Aim | Spin Passo | `GS_SliderFloat` | `rageAntiAimSpinStep` | `22.` | `GSLayout.inl:982` |
| Anti-Aim | Only on ground | `GS_Toggle` | `rageAntiAimOnGroundOnly` | `true` | `GSLayout.inl:984` |
| Anti-Aim | At-Targets (Olhar Oposto ao Alvo) | `GS_Toggle` | `rageAntiAimAtTargets` | `false` | `GSLayout.inl:985` |
| Anti-Aim | Mouse Override Direcional | `GS_Toggle` | `rageAntiAimMouseOverride` | `false` | `GSLayout.inl:986` |
| Anti-Aim | Anti-Aim Defensivo (Ar/Velocidade) | `GS_Toggle` | `rageAntiAimDefensive` | `false` | `GSLayout.inl:987` |
| Anti-Aim | Pitch Invert | `GS_Toggle` | `rageAntiAimPitchInvert` | `false` | `GSLayout.inl:988` |
| Anti-Aim | Desync (AA no sub-tick, sem snap) | `GS_Toggle` | `rageDesync` | `false` | `GSLayout.inl:989` |
| Anti-Aim | Fake Limit (desync) | `GS_SliderFloat` | `rageDesyncFakeLimit` | `35.` | `GSLayout.inl:991` |
| Anti-Aim | Body Yaw (alterna lados) | `GS_Toggle` | `rageBodyYaw` | `false` | `GSLayout.inl:993` |
| Anti-Aim | Resolver | `GS_Toggle` | `rageResolver` | `false` | `GSLayout.inl:994` |
| Anti-Aim | Backtrack | `GS_Toggle` | `rageBacktrack` | `false` | `GSLayout.inl:995` |
| Anti-Aim | BT Ticks | `GS_SliderInt` | `rageBacktrackTicks` | `6` | `GSLayout.inl:997` |
| Anti-Aim | Fake Lag | `GS_Toggle` | `rageFakeLag` | `false` | `GSLayout.inl:998` |
| Anti-Aim | Choke Ticks | `GS_SliderInt` | `rageFakeLagTicks` | `4` | `GSLayout.inl:1000` |
| Anti-Aim | Fake Duck | `GS_ToggleWithKeybind` | `fakeDuck` + `fakeDuckKey` | `false` | `GSLayout.inl:1001` |
| Thirdperson (ver anti-aim) | Thirdperson | `GS_ToggleWithKeybind` | `thirdpersonEnabled` + `thirdpersonKey` | `false` | `GSLayout.inl:1025` |
| Thirdperson (ver anti-aim) | Distancia | `GS_SliderFloat` | `thirdpersonDistance` | `120.` | `GSLayout.inl:1026` |

### Players › ESP — 29 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| General | Enable | `GS_Toggle` | `espEnabled` | `true` | `GSLayout.inl:698` |
| General | Through wall | `GS_Toggle` | `espThroughWall` | `true` | `GSLayout.inl:699` |
| General | Nickname | `GS_Toggle` | `espName` | `true` | `GSLayout.inl:700` |
| General | Box | `GS_Toggle` | `espBoxes` | `true` | `GSLayout.inl:701` |
| General | Box style | `GS_Combo` | `espBoxType` | `1` | `GSLayout.inl:704` |
| General | Health bar | `GS_Toggle` | `espHealth` | `true` | `GSLayout.inl:706` |
| General | Armor bar | `GS_Toggle` | `espShowArmor` | `true` | `GSLayout.inl:708` |
| General | Armor Color | `GS_ColorRow` | `g_cfg.colArmorBar` | `` | `GSLayout.inl:710` |
| General | Armor Position | `GS_Combo` | `armorBarPos` | `0` | `GSLayout.inl:711` |
| General | Ammo bar | `GS_Toggle` | `espAmmoBar` | `true` | `GSLayout.inl:713` |
| General | Ammo Color | `GS_ColorRow` | `g_cfg.colAmmoBar` | `` | `GSLayout.inl:715` |
| General | Ammo Position | `GS_Combo` | `ammoBarPos` | `0` | `GSLayout.inl:717` |
| General | Weapon | `GS_Toggle` | `espWeapon` | `true` | `GSLayout.inl:719` |
| General | Weapon icons | `GS_Toggle` | `espWeaponIcons` | `true` | `GSLayout.inl:720` |
| General | Skeleton | `GS_Toggle` | `espSkeleton` | `true` | `GSLayout.inl:721` |
| General | Sound arrow | `GS_Toggle` | `soundEsp` | `false` | `GSLayout.inl:722` |
| General | Out of FOV arrow | `GS_Toggle` | `offScreenArrows` | `true` | `GSLayout.inl:723` |
| Flags | Bomb | `GS_Toggle` | `espFlagBomb` | `true` | `GSLayout.inl:726` |
| Flags | Health | `GS_Toggle` | `espFlagHealth` | `false` | `GSLayout.inl:727` |
| Flags | Ammo | `GS_Toggle` | `espFlagAmmo` | `false` | `GSLayout.inl:728` |
| Flags | Reload | `GS_Toggle` | `espFlagReload` | `true` | `GSLayout.inl:729` |
| Flags | Scoped | `GS_Toggle` | `espFlagScoped` | `true` | `GSLayout.inl:730` |
| Flags | Flashed | `GS_Toggle` | `espFlagFlashed` | `true` | `GSLayout.inl:731` |
| Flags | Distance | `GS_Toggle` | `espFlagDistance` | `true` | `GSLayout.inl:732` |
| Flags | Defuse kit | `GS_Toggle` | `espFlagDefuseKit` | `true` | `GSLayout.inl:733` |
| Flags | Helmet & kevlar | `GS_Toggle` | `espFlagHelmet` | `true` | `GSLayout.inl:734` |
| Flags | Defusing / rescuing | `GS_Toggle` | `espFlagDefusing` | `true` | `GSLayout.inl:735` |
| Flags | Pin pulled | `GS_Toggle` | `espFlagPinPulled` | `true` | `GSLayout.inl:736` |
| Flags | Planting | `GS_Toggle` | `espFlagPlanting` | `true` | `GSLayout.inl:737` |

### Agents › Agents — 3 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Agent Changer | Ativar Agent Changer | `CleanToggle` | `agEnabled` | `` | `cards/CardCosmetics.inl:17` |
| Agent Changer | Agente Contra-Terrorista (CT) | `CleanCombo` | `ctIdx` | `` | `cards/CardCosmetics.inl:25` |
| Agent Changer | Agente Terrorista (TR) | `CleanCombo` | `tIdx` | `` | `cards/CardCosmetics.inl:33` |

### Visuals › ESP — 12 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Grenades | Grenade prediction | `GS_Toggle` | `showGrenadeTrajectory` | `true` | `GSLayout.inl:795` |
| Grenades | Trajectory Color | `GS_ColorRow` | `g_cfg.colTrajectory` | `` | `GSLayout.inl:796` |
| Grenades | Collision Color | `GS_ColorRow` | `g_cfg.colTrajectoryCollision` | `` | `GSLayout.inl:797` |
| Grenades | Smoke grenade timer | `GS_Toggle` | `grenadeZones` | `false` | `GSLayout.inl:798` |
| Grenades | Smoke Color | `GS_ColorRow` | `g_cfg.colSmoke` | `` | `GSLayout.inl:799` |
| Grenades | Incendiary grenade timer | `GS_Toggle` | `grenadeZones` | `false` | `GSLayout.inl:800` |
| Grenades | Incendiary Color | `GS_ColorRow` | `g_cfg.colMolotov` | `` | `GSLayout.inl:801` |
| Grenades | Grenade tracers | `GS_Toggle` | `grenadeProjectiles` | `false` | `GSLayout.inl:802` |
| Miscellaneous | Bomb timer | `GS_Toggle` | `showBombTimer` | `true` | `GSLayout.inl:805` |
| Miscellaneous | Spectator list | `GS_Toggle` | `showSpectators` | `false` | `GSLayout.inl:806` |
| Miscellaneous | Hit marker | `GS_Toggle` | `showHitmarker` | `true` | `GSLayout.inl:807` |
| Miscellaneous | Textured radar | `GS_Toggle` | `nativeRadar` | `false` | `GSLayout.inl:808` |

### World › Skybox — 14 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Textures | Skybox | `GS_Toggle` | `skyActive` | `false` | `GSLayout.inl:819` |
| Textures | Skybox Color | `GS_ColorRow` | `g_cfg.colSky` | `` | `GSLayout.inl:820` |
| Textures | Lightning | `GS_Toggle` | `darkWorld` | `false` | `GSLayout.inl:821` |
| Textures | World textures | `GS_Toggle` | `cleanMap` | `false` | `GSLayout.inl:822` |
| Textures | World Fog Color | `GS_ColorRow` | `g_cfg.colFog` | `` | `GSLayout.inl:823` |
| Textures | Exposure | `GS_SliderFloat` | `worldBrightness` | `0.25` | `GSLayout.inl:824` |
| Extra | Force sniper crosshair | `GS_Toggle` | `forceSniperCrosshair` | `false` | `GSLayout.inl:835` |
| Extra | Penetration crosshair | `GS_Toggle` | `penetrationCrosshair` | `false` | `GSLayout.inl:836` |
| Extra | Flash scale | `GS_SliderFloat` | `flashAlphaLimit` | `.25` | `GSLayout.inl:837` |
| Removals | Shadows | `GS_Toggle` | `cleanMap` | `false` | `GSLayout.inl:842` |
| Removals | Zoom hud | `GS_Toggle` | `noScopeOverlay` | `false` | `GSLayout.inl:843` |
| Removals | Zoom spread | `GS_Toggle` | `showSpreadCircle` | `false` | `GSLayout.inl:844` |
| Removals | Visual recoil | `GS_Toggle` | `noVisualRecoil` | `false` | `GSLayout.inl:845` |
| Removals | Weather original | `GS_Toggle` | `removeOriginalWeather` | `false` | `GSLayout.inl:846` |

### View › Camera & Third Person — 2 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Camera (World FOV) | Alterar FOV da Camera | `GS_Toggle` | `cameraFovOverride` | `false` | `GSLayout.inl:1071` |
| Camera (World FOV) | FOV Geral (Camera) | `GS_SliderFloat` | `cameraFov` | `105.` | `GSLayout.inl:1072` |

### View › Viewmodel — 7 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Viewmodel Offset & FOV | Custom Viewmodel | `GS_Toggle` | `customViewmodel` | `false` | `GSLayout.inl:1054` |
| Viewmodel Offset & FOV | X (Eixo Horizontal) | `GS_SliderFloat` | `viewmodelX` | `1.` | `GSLayout.inl:1055` |
| Viewmodel Offset & FOV | Y (Profundidade) | `GS_SliderFloat` | `viewmodelY` | `1.` | `GSLayout.inl:1056` |
| Viewmodel Offset & FOV | Z (Altura) | `GS_SliderFloat` | `viewmodelZ` | `-1.` | `GSLayout.inl:1057` |
| Viewmodel Offset & FOV | FOV da Arma | `GS_SliderFloat` | `viewmodelFov` | `68.` | `GSLayout.inl:1058` |
| Proporcao de Tela | Forcar Proporcao (Aspect) | `GS_Toggle` | `aspectRatioOverride` | `false` | `GSLayout.inl:1061` |
| Proporcao de Tela | Proporcao | `GS_SliderFloat` | `aspectRatio` | `1.3333` | `GSLayout.inl:1062` |

### Indicators › Overlays — 2 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Overlays | Telemetry HUD | `GS_Toggle` | `showTelemetryHUD` | `true` | `GSLayout.inl:1085` |
| Overlays | Keybinds Overlay | `GS_Toggle` | `showKeybindsOverlay` | `false` | `GSLayout.inl:1086` |

### Indicators › Typography — 2 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Font Size | UI Font | `GS_SliderFloat` | `uiFontSize` | `14.` | `GSLayout.inl:1091` |
| Font Size | ESP Font | `GS_SliderFloat` | `espFontSize` | `13.` | `GSLayout.inl:1092` |

### Miscellaneous › General — 38 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Interface & Radar | Menu Snow Particles | `GS_Toggle` | `menuSnow` | `true` | `GSLayout.inl:859` |
| Interface & Radar | Snow Color | `GS_ColorRow` | `g_cfg.colSnow` | `` | `GSLayout.inl:860` |
| Interface & Radar | Panel Accent Color | `GS_ColorRow` | `g_cfg.colMenuAccent` | `` | `GSLayout.inl:861` |
| Interface & Radar | Panel Background Color | `GS_ColorRow` | `g_cfg.colMenuBg` | `` | `GSLayout.inl:862` |
| Interface & Radar | Auto-accept matchmaking | `GS_Toggle` | `autoAcceptMatch` | `false` | `GSLayout.inl:891` |
| Interface & Radar | In-game radar | `GS_Toggle` | `nativeRadar` | `false` | `GSLayout.inl:892` |
| Interface & Radar | Preserve kill-feed | `GS_Toggle` | `killfeedEnabled` | `true` | `GSLayout.inl:893` |
| Audio DSP | Audio Procedural | `CleanToggle` | `proceduralAudio` | `true` | `cards/CardEnvironment.inl:185` |
| Audio DSP | Som de Acerto | `CleanToggle` | `hitSound` | `true` | `cards/CardEnvironment.inl:186` |
| Audio DSP | Preset de Acerto | `CleanCombo` | `hitSoundPreset` | `0` | `cards/CardEnvironment.inl:187` |
| Audio DSP | Som de Eliminacao | `CleanToggle` | `killSound` | `true` | `cards/CardEnvironment.inl:189` |
| Audio DSP | Hitmarker na Mira | `CleanToggle` | `showHitmarker` | `true` | `cards/CardEnvironment.inl:190` |
| Audio DSP | Beep Acustico da C4 | `CleanToggle` | `bombBeepAudio` | `false` | `cards/CardEnvironment.inl:191` |
| Audio DSP | Volume | `CleanSliderFloat` | `soundVolume` | `0.8` | `cards/CardEnvironment.inl:192` |
| Audio DSP | WAV de acerto | `CleanToggle` | `customHitSound` | `false` | `cards/CardEnvironment.inl:203` |
| Audio DSP | Arquivo de acerto | `ImGui InputText` | `g_cfg.hitSoundFile` | `` | `cards/CardEnvironment.inl:207` |
| Audio DSP | WAV de headshot | `CleanToggle` | `customHeadshotSound` | `false` | `cards/CardEnvironment.inl:209` |
| Audio DSP | Arquivo de headshot | `ImGui InputText` | `g_cfg.headshotSoundFile` | `` | `cards/CardEnvironment.inl:213` |
| Audio DSP | WAV de eliminacao | `CleanToggle` | `customKillSound` | `false` | `cards/CardEnvironment.inl:215` |
| Audio DSP | Arquivo de eliminacao | `ImGui InputText` | `g_cfg.killSoundFile` | `` | `cards/CardEnvironment.inl:219` |
| Movimentacao | BunnyHop Automatico | `CleanToggle` | `bhopActive` | `true` | `cards/CardMovement.inl:9` |
| Movimentacao | Modo | `CleanCombo` | `bhopMode` | `0` | `cards/CardMovement.inl:12` |
| Movimentacao | Tecla Bhop | `CleanKeybind` | `bhopKey` | `VK_SPACE` | `cards/CardMovement.inl:13` |
| Movimentacao | Strafe Gain % | `CleanSliderFloat` | `v` | `` | `cards/CardMovement.inl:14` |
| Movimentacao | Angulo Step | `CleanSliderFloat` | `bhopAngleStep` | `89.0` | `cards/CardMovement.inl:15` |
| Movimentacao | Humanize (ms) | `CleanSliderFloat` | `v` | `` | `cards/CardMovement.inl:16` |
| Movimentacao | Hold Ticks | `CleanSliderFloat` | `v` | `` | `cards/CardMovement.inl:17` |
| Movimentacao | Auto Edge-Jump | `CleanToggle` | `bhopAutoEdgeJump` | `false` | `cards/CardMovement.inl:18` |
| Movimentacao | Auto JumpBug | `CleanToggle` | `bhopAutoJumpBug` | `false` | `cards/CardMovement.inl:19` |
| Movimentacao | Prestrafe Align | `CleanToggle` | `bhopPrestrafeAlign` | `false` | `cards/CardMovement.inl:20` |
| Movimentacao | Preservar Speed no Solo | `CleanToggle` | `bhopPreserveOnLand` | `true` | `cards/CardMovement.inl:21` |
| Movimentacao | Apenas em Movimento | `CleanToggle` | `bhopOnlyMoving` | `true` | `cards/CardMovement.inl:22` |
| Movimentacao | Velocidade Minima | `CleanSliderFloat` | `bhopMinSpeedGate` | `25.0` | `cards/CardMovement.inl:23` |
| Automacao | Jumpthrow por Tecla | `CleanToggle` | `jumpthrowActive` | `true` | `cards/CardMovement.inl:33` |
| Automacao | Tecla Jumpthrow | `CleanKeybind` | `jumpthrowKey` | `'X'` | `cards/CardMovement.inl:35` |
| Automacao | Soltar Granada Automatico (Release) | `CleanToggle` | `autoGrenadeRelease` | `false` | `cards/CardMovement.inl:37` |
| Automacao | Auto-Pistol | `CleanToggle` | `autoPistol` | `false` | `cards/CardMovement.inl:38` |
| Automacao | Anti-AFK Automatico | `CleanToggle` | `antiAfk` | `false` | `cards/CardMovement.inl:39` |

### Inventory › Profile — 9 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Agentes | Agent changer | `GS_Toggle` | `agentEnabled` | `false` | `InventoryPanel.inl:385` |
| Agentes | ##agentCTInline | `ImGui Combo` | `agentCT` | `0` | `InventoryPanel.inl:402` |
| Agentes | ##agentTRInline | `ImGui Combo` | `agentT` | `0` | `InventoryPanel.inl:410` |
| Agentes | Agente CT | `GS_Combo` | `agentCT` | `0` | `InventoryPanel.inl:415` |
| Agentes | Agente TR | `GS_Combo` | `agentT` | `0` | `InventoryPanel.inl:416` |
| (sem secao) | Sincronizacao nativa | `CleanToggle` | `active` | `` | `InventoryPanel.inl:434` |
| (sem secao) | Sincronizacao nativa | `GS_Toggle` | `active` | `` | `InventoryPanel.inl:440` |
| (sem secao) | Modelo da faca | `GS_Combo` | `selectedKnifeIdx` | `` | `InventoryPanel.inl:668` |
| (sem secao) | Animações Secretas | `ImGui Checkbox` | `secretAnim` | `` | `InventoryPanel.inl:670` |

### Grenade Helper › Incendiary — 2 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Incendiary Timer | Fire Glow | `GS_Toggle` | `fireGlowEnabled` | `false` | `GSLayout.inl:1126` |
| Incendiary Timer | Fire Color | `GS_ColorRow` | `g_cfg.fireGlowColor` | `` | `GSLayout.inl:1127` |

### Grenade Helper › Lineups & Helper — 13 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Grenade Helper CS:2 | Marcadores de Lineup | `CleanToggle` | `lineupHelper` | `false` | `LineupPanel.inl:6` |
| Grenade Helper CS:2 | Reconhecer Mapa Automaticamente | `CleanToggle` | `lineupAutoMap` | `true` | `LineupPanel.inl:7` |
| Grenade Helper CS:2 | Alinhar e Arremessar por Tecla | `CleanToggle` | `lineupAutoAlign` | `false` | `LineupPanel.inl:8` |
| Grenade Helper CS:2 | Mapa selecionado | `ImGui InputText` | `g_cfg.lineups.map` | `` | `LineupPanel.inl:13` |
| Grenade Helper CS:2 | Modalidade de Arremesso | `CleanCombo` | `throwMode` | `` | `LineupPanel.inl:24` |
| Grenade Helper CS:2 | Tipo de Trajetoria | `CleanCombo` | `arcKind` | `` | `LineupPanel.inl:27` |
| Grenade Helper CS:2 | Stance Exigida | `CleanCombo` | `stance` | `` | `LineupPanel.inl:30` |
| Grenade Helper CS:2 | Duracao da Corrida | `CleanSliderFloat` | `runMs` | `` | `LineupPanel.inl:33` |
| Grenade Helper CS:2 | Nome do Lineup | `ImGui InputText` | `label` | `` | `LineupPanel.inl:38` |
| Grenade Helper CS:2 | Tipo | `CleanCombo` | `entry.throwMode` | `` | `LineupPanel.inl:113` |
| Grenade Helper CS:2 | Arco | `CleanCombo` | `entry.arcKind` | `` | `LineupPanel.inl:114` |
| Grenade Helper CS:2 | Stance | `CleanCombo` | `entry.stance` | `` | `LineupPanel.inl:115` |
| Grenade Helper CS:2 | Validado para execucao automatica | `ImGui Checkbox` | `entry.validated` | `` | `LineupPanel.inl:116` |

### Grenade Helper › Prediction — 3 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Grenade Prediction | Enabled | `GS_Toggle` | `showGrenadeTrajectory` | `true` | `GSLayout.inl:1112` |
| Grenade Prediction | Trajectory Glow | `GS_Toggle` | `trajectoryGlow` | `false` | `GSLayout.inl:1113` |
| Grenade Prediction | Width | `GS_SliderFloat` | `trajectoryWidth` | `1.8` | `GSLayout.inl:1114` |

### Grenade Helper › Smoke Timer — 3 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Smoke Timer | Enabled | `GS_Toggle` | `strictSmoke` | `false` | `GSLayout.inl:1119` |
| Smoke Timer | Smoke Glow | `GS_Toggle` | `smokeGlowEnabled` | `false` | `GSLayout.inl:1120` |
| Smoke Timer | Smoke Color | `GS_ColorRow` | `g_cfg.smokeGlowColor` | `` | `GSLayout.inl:1121` |

### Grenade Helper › Tracers — 3 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Grenade Tracers | Enabled | `GS_Toggle` | `tracerGlow` | `true` | `GSLayout.inl:1132` |
| Grenade Tracers | Width | `GS_SliderFloat` | `tracerWidth` | `1.5` | `GSLayout.inl:1133` |
| Grenade Tracers | Color | `GS_ColorRow` | `g_cfg.colTracer` | `` | `GSLayout.inl:1134` |

### Configs › General — 5 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Perfis & Overrides (v3) | Alvo | `ImGui Combo` | `s_targetType` | `` | `cards/CardProfiles.inl:69` |
| Perfis & Overrides (v3) | Contexto | `ImGui Combo` | `s_subIndex` | `` | `cards/CardProfiles.inl:70` |
| Perfis & Overrides (v3) | Classe Arma | `ImGui Combo` | `s_subIndex` | `` | `cards/CardProfiles.inl:71` |
| Perfis & Overrides (v3) | Variavel | `ImGui Combo` | `s_varIndex` | `` | `cards/CardProfiles.inl:73` |
| Perfis & Overrides (v3) | Valor | `ImGui InputText` | `s_overrideVal` | `` | `cards/CardProfiles.inl:74` |

### Configs › Profiles — 4 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| Presets & Configuracao | Tecla do Menu | `CleanKeybind` | `menuKey` | `VK_INSERT` | `cards/CardProfiles.inl:9` |
| Presets & Configuracao | Efeito Glass Blur (Translucido) | `CleanToggle` | `glassBlur` | `true` | `cards/CardProfiles.inl:15` |
| Presets & Configuracao | HUD Telemetria (FPS/Ping/Tick) | `CleanToggle` | `showTelemetryHUD` | `true` | `cards/CardProfiles.inl:16` |
| Presets & Configuracao | Overlay de Atalhos | `CleanToggle` | `showKeybindsOverlay` | `false` | `cards/CardProfiles.inl:30` |

### (fora do roteador) — 258 widgets

| seção | rótulo | widget | campo | default | origem |
|---|---|---|---|---|---|
| (sem secao) | Modulo Armory | `CleanCombo` | `g_armoryPage` | `` | `ArmoryEditor.h:8` |
| (sem secao) | Cinco Adesivos Ativos | `CleanToggle` | `draft.stickers` | `` | `ArmoryEditor.h:11` |
| (sem secao) | Slot | `CleanCombo` | `index` | `` | `ArmoryEditor.h:14` |
| (sem secao) | Adesivos Famosos | `CleanCombo` | `popularStickerIdx` | `` | `ArmoryEditor.h:37` |
| (sem secao) | ID do Adesivo | `ImGui InputInt` | `s.id` | `` | `ArmoryEditor.h:42` |
| (sem secao) | Posicao X | `CleanSliderFloat` | `s.x` | `` | `ArmoryEditor.h:55` |
| (sem secao) | Posicao Y | `CleanSliderFloat` | `s.y` | `` | `ArmoryEditor.h:56` |
| (sem secao) | Rotacao | `CleanSliderFloat` | `s.rotation` | `` | `ArmoryEditor.h:57` |
| (sem secao) | Raspagem (Wear) | `CleanSliderFloat` | `s.wear` | `` | `ArmoryEditor.h:59` |
| (sem secao) | Escala | `CleanSliderFloat` | `s.scale` | `` | `ArmoryEditor.h:67` |
| (sem secao) | Chaveiro (Keychain) | `CleanToggle` | `draft.charm` | `` | `ArmoryEditor.h:71` |
| (sem secao) | Chaveiros Famosos | `CleanCombo` | `popularCharmIdx` | `` | `ArmoryEditor.h:88` |
| (sem secao) | ID do Chaveiro | `ImGui InputInt` | `draft.charmId` | `` | `ArmoryEditor.h:93` |
| (sem secao) | Seed do Chaveiro | `ImGui InputInt` | `draft.charmSeed` | `` | `ArmoryEditor.h:98` |
| (sem secao) | Chaveiro X | `CleanSliderFloat` | `draft.charmX` | `` | `ArmoryEditor.h:103` |
| (sem secao) | Chaveiro Y | `CleanSliderFloat` | `draft.charmY` | `` | `ArmoryEditor.h:104` |
| (sem secao) | Chaveiro Z | `CleanSliderFloat` | `draft.charmZ` | `` | `ArmoryEditor.h:105` |
| (sem secao) | StatTrak Ativo | `CleanToggle` | `draft.stat` | `` | `ArmoryEditor.h:109` |
| (sem secao) | Contador de Abates | `ImGui InputInt` | `draft.counter` | `` | `ArmoryEditor.h:112` |
| (sem secao) | Nametag Personalizada | `CleanToggle` | `draft.nametag` | `` | `ArmoryEditor.h:124` |
| (sem secao) | Nome da Arma | `ImGui InputText` | `draft.name` | `` | `ArmoryEditor.h:127` |
| (sem secao) | Master switch | `ImGui Checkbox` | `aimActive` | `true` | `GSLayout.inl:392` |
| (sem secao) | ##legitScope | `ImGui Combo` | `s_legitScope` | `` | `GSLayout.inl:399` |
| (sem secao) | ##legitWeapon | `ImGui Combo` | `s_legitWeapon` | `` | `GSLayout.inl:404` |
| (sem secao) | Master switch | `ImGui Checkbox` | `chamsVisible` | `true` | `GSLayout.inl:407` |
| (sem secao) | ##chamsTarget | `ImGui Combo` | `s_chamsTarget` | `` | `GSLayout.inl:412` |
| (sem secao) | Neve Caindo (Snow Particles) | `ImGui Checkbox` | `menuSnow` | `true` | `GSLayout.inl:437` |
| (sem secao) | Preset do Tema: | `ImGui Combo` | `s_hdrThemePreset` | `` | `GSLayout.inl:444` |
| (sem secao) | Preset da Neve: | `ImGui Combo` | `s_hdrSnowPreset` | `` | `GSLayout.inl:472` |
| Enemy team | Gun game immunity | `GS_Toggle` | `espFlagsExtras` | `true` | `GSLayout.inl:749` |
| Enemy team | Glow | `GS_Toggle` | `glowOutline` | `true` | `GSLayout.inl:752` |
| Enemy team | Glow Color | `GS_ColorRow` | `g_cfg.colEnemy` | `` | `GSLayout.inl:753` |
| Enemy team | Visible | `GS_Toggle` | `chamsVisible` | `true` | `GSLayout.inl:754` |
| Enemy team | Visible Color | `GS_ColorRow` | `g_cfg.colVisible` | `` | `GSLayout.inl:755` |
| Enemy team | Visible Style | `GS_Combo` | `chamsVisibleStyle` | `0` | `GSLayout.inl:756` |
| Enemy team | Invisible | `GS_Toggle` | `chamsHidden` | `false` | `GSLayout.inl:757` |
| Enemy team | Invisible Color | `GS_ColorRow` | `g_cfg.colHidden` | `` | `GSLayout.inl:758` |
| Enemy team | Invisible Style | `GS_Combo` | `chamsHiddenStyle` | `2` | `GSLayout.inl:759` |
| Enemy team | History (Rastro) | `GS_Toggle` | `chamsHistory` | `false` | `GSLayout.inl:760` |
| Enemy team | History Color | `GS_ColorRow` | `g_cfg.colHistory` | `` | `GSLayout.inl:761` |
| Enemy team | On-Shot Flash | `GS_Toggle` | `chamsOnShot` | `false` | `GSLayout.inl:762` |
| Enemy team | On-Shot Color | `GS_ColorRow` | `g_cfg.colOnShot` | `` | `GSLayout.inl:763` |
| Enemy team | Glow (Contorno) | `GS_Toggle` | `chamsGlow` | `false` | `GSLayout.inl:764` |
| Local team | Gun game immunity##local | `GS_Toggle` | `espFlagsExtras` | `true` | `GSLayout.inl:767` |
| Local team | Glow##team | `GS_Toggle` | `s_teamGlow` | `` | `GSLayout.inl:771` |
| Local team | Team Glow Color | `GS_ColorRow` | `g_cfg.colTeam` | `` | `GSLayout.inl:772` |
| Local team | Visible##team | `GS_Toggle` | `chamsArms` | `false` | `GSLayout.inl:773` |
| Local team | Team Visible Color | `GS_ColorRow` | `g_cfg.colTeam` | `` | `GSLayout.inl:774` |
| Local team | Team Style | `GS_Combo` | `s_teamMat` | `` | `GSLayout.inl:777` |
| Local team | Invisible##team | `GS_Toggle` | `chamsWeapon` | `false` | `GSLayout.inl:778` |
| Local team | Team Invisible Color | `GS_ColorRow` | `g_cfg.colTeam` | `` | `GSLayout.inl:779` |
| Local team | Invisible Style##team | `GS_Combo` | `s_teamMat` | `` | `GSLayout.inl:780` |
| Local team | Local Player##local | `GS_Toggle` | `chamsLocal` | `false` | `GSLayout.inl:781` |
| Local team | Local Color | `GS_ColorRow` | `g_cfg.colLocal` | `` | `GSLayout.inl:782` |
| Local team | Aliados##team | `GS_Toggle` | `chamsTeam` | `false` | `GSLayout.inl:783` |
| Local team | Team Color | `GS_ColorRow` | `g_cfg.colTeam` | `` | `GSLayout.inl:784` |
| (sem secao) | ##enValue | `ImGui SliderFloat` | `v` | `` | `GSWidgets.inl:954` |
| Item selecionado | Tipo | `GS_Combo` | `type` | `` | `InventoryPanel.inl:223` |
| Item selecionado | Modelo | `GS_Combo` | `weapon` | `` | `InventoryPanel.inl:240` |
| Item selecionado | Modelo | `GS_Combo` | `knife` | `` | `InventoryPanel.inl:243` |
| Item selecionado | Modelo | `GS_Combo` | `glove` | `` | `InventoryPanel.inl:246` |
| Item selecionado | Pintura | `GS_Combo` | `selectedPaint` | `` | `InventoryPanel.inl:289` |
| Item selecionado | ##variantName | `ImGui InputText` | `item.label` | `` | `InventoryPanel.inl:296` |
| Item selecionado | Wear | `GS_SliderFloat` | `item.skin.wear` | `` | `InventoryPanel.inl:299` |
| Item selecionado | Seed | `ImGui InputInt` | `item.skin.seed` | `` | `InventoryPanel.inl:300` |
| (sem secao) | Auto-Rotacionar | `CleanToggle` | `state.rotate` | `` | `UIEquipmentPreviews.inl:73` |
| (sem secao) | Rotacao Horizontal | `CleanSliderFloat` | `state.yaw` | `` | `UIEquipmentPreviews.inl:74` |
| (sem secao) | Zoom da Previa | `CleanSliderFloat` | `state.zoom` | `` | `UIEquipmentPreviews.inl:75` |
| (sem secao) | Animar Modelo da Previa | `CleanToggle` | `state.animate` | `` | `UIEquipmentPreviews.inl:77` |
| (sem secao) | Clip Real da Previa | `CleanCombo` | `state.clip` | `` | `UIEquipmentPreviews.inl:80` |
| Previa do Item | Visualizar | `CleanCombo` | `type` | `` | `UIEquipmentPreviews.inl:92` |
| Previa do Item | Visualizacao 3D do modelo base | `CleanToggle` | `preview3d` | `` | `UIEquipmentPreviews.inl:95` |
| Previa do Agente | Equipe da Previa | `CleanCombo` | `g_previewAgentTeam` | `` | `UIEquipmentPreviews.inl:111` |
| Previa do Agente | Visualizacao 3D do Agente | `CleanToggle` | `g_agentPreview3D` | `` | `UIEquipmentPreviews.inl:115` |
| (sem secao) | Ocultar mira personalizada ao usar scope | `ImGui Checkbox` | `hideCustomCrosshairScoped` | `false` | `UIPanel.inl:100` |
| Aimbot | Aimbot Ativo | `CleanToggle` | `aimActive` | `true` | `cards/CardCombat.inl:17` |
| Aimbot | Somente Alvos Visiveis | `CleanToggle` | `aimVisibleOnly` | `false` | `cards/CardCombat.inl:19` |
| Aimbot | Auto-Scope AWP / Scout | `CleanToggle` | `autoScope` | `false` | `cards/CardCombat.inl:21` |
| Aimbot | Pausa apos eliminacao (ms) | `ImGui SliderInt` | `aimKillDelayMs` | `0` | `cards/CardCombat.inl:22` |
| Aimbot | Pausa ao trocar alvo (ms) | `ImGui SliderInt` | `aimSwitchDelayMs` | `0` | `cards/CardCombat.inl:23` |
| Aimbot | Campo de Visao (FOV) | `CleanSliderFloat` | `aimFov` | `4.5` | `cards/CardCombat.inl:26` |
| Aimbot | Suavizacao (Smooth) | `CleanSliderFloat` | `aimSmooth` | `4.5` | `cards/CardCombat.inl:28` |
| Aimbot | Aceleracao / Desaceleracao | `CleanToggle` | `aimEase` | `true` | `cards/CardCombat.inl:30` |
| Aimbot | Pico de Velocidade | `CleanSliderFloat` | `aimEaseBias` | `.35` | `cards/CardCombat.inl:32` |
| Aimbot | Osso Alvo | `CleanCombo` | `aimBone` | `0` | `cards/CardCombat.inl:36` |
| Aimbot | Selecao de Alvo | `CleanCombo` | `aimTargetMode` | `0` | `cards/CardCombat.inl:40` |
| Aimbot | Tecla de Mira | `CleanKeybind` | `aimKey` | `VK_LBUTTON` | `cards/CardCombat.inl:44` |
| Trajetoria & Alcance | Curva Bezier | `CleanToggle` | `aimBezier` | `false` | `cards/CardCombat.inl:64` |
| Trajetoria & Alcance | Variacao da Curva | `CleanSliderFloat` | `aimCurveJitter` | `.025` | `cards/CardCombat.inl:66` |
| Trajetoria & Alcance | FOV por Distancia | `CleanToggle` | `aimDistanceFov` | `false` | `cards/CardCombat.inl:68` |
| Trajetoria & Alcance | Distancia de Referencia | `CleanSliderFloat` | `aimFovReferenceMetres` | `20.` | `cards/CardCombat.inl:72` |
| Trajetoria & Alcance | Escala Minima | `CleanSliderFloat` | `aimFovMinimumScale` | `.25` | `cards/CardCombat.inl:74` |
| Triggerbot | Triggerbot Ativo | `CleanToggle` | `triggerActive` | `false` | `cards/CardCombat.inl:140` |
| Triggerbot | Pausa apos eliminacao (ms) | `ImGui SliderInt` | `aimKillDelayMs` | `0` | `cards/CardCombat.inl:142` |
| Triggerbot | Pausa ao trocar alvo (ms) | `ImGui SliderInt` | `aimSwitchDelayMs` | `0` | `cards/CardCombat.inl:143` |
| Triggerbot | Anti-Smoke Estrito | `CleanToggle` | `strictSmoke` | `false` | `cards/CardCombat.inl:146` |
| Triggerbot | Nao Disparar Cego (Flash Check) | `CleanToggle` | `triggerFlashCheck` | `false` | `cards/CardCombat.inl:147` |
| Triggerbot | Nao Disparar no Ar (Air Check) | `CleanToggle` | `triggerAirCheck` | `false` | `cards/CardCombat.inl:148` |
| Triggerbot | Tecla Triggerbot | `CleanKeybind` | `triggerKey` | `VK_MENU` | `cards/CardCombat.inl:156` |
| Triggerbot | Modo de Delay | `CleanCombo` | `mode` | `` | `cards/CardCombat.inl:168` |
| Triggerbot | Delay Disparo | `CleanSliderFloat` | `triggerDelayMs` | `65.0` | `cards/CardCombat.inl:180` |
| Triggerbot | Pausa Apos Disparo | `CleanSliderFloat` | `pause` | `` | `cards/CardCombat.inl:186` |
| Frenagem do Trigger | Frear Antes de Disparar | `CleanToggle` | `shotStop` | `false` | `cards/CardCombat.inl:257` |
| Frenagem do Trigger | Esperar Repouso Estavel | `CleanToggle` | `shotFullStop` | `false` | `cards/CardCombat.inl:259` |
| Frenagem do Trigger | Preservar Apoio na Borda | `CleanToggle` | `shotStopEdgeGuard` | `true` | `cards/CardCombat.inl:261` |
| Frenagem do Trigger | Velocidade Limite | `CleanSliderFloat` | `shotStopSpeed` | `75.` | `cards/CardCombat.inl:263` |
| Camera & Viewmodel | Alterar FOV da Camera | `CleanToggle` | `cameraFovOverride` | `false` | `cards/CardEnvironment.inl:11` |
| Camera & Viewmodel | FOV Geral (Camera) | `CleanSliderFloat` | `cameraFov` | `105.` | `cards/CardEnvironment.inl:12` |
| Camera & Viewmodel | Forcar Proporcao de Tela | `CleanToggle` | `aspectRatioOverride` | `false` | `cards/CardEnvironment.inl:20` |
| Camera & Viewmodel | Proporcao de Tela | `CleanSliderFloat` | `aspectRatio` | `1.3333` | `cards/CardEnvironment.inl:22` |
| Camera & Viewmodel | Viewmodel Personalizado | `CleanToggle` | `customViewmodel` | `false` | `cards/CardEnvironment.inl:29` |
| Camera & Viewmodel | Arma X | `CleanSliderFloat` | `viewmodelX` | `1.` | `cards/CardEnvironment.inl:31` |
| Camera & Viewmodel | Arma Y | `CleanSliderFloat` | `viewmodelY` | `1.` | `cards/CardEnvironment.inl:32` |
| Camera & Viewmodel | Arma Z | `CleanSliderFloat` | `viewmodelZ` | `-1.` | `cards/CardEnvironment.inl:33` |
| Camera & Viewmodel | FOV da Arma | `CleanSliderFloat` | `viewmodelFov` | `68.` | `cards/CardEnvironment.inl:34` |
| Misc / Estabilidade | Suprimir Recuo Visual | `CleanToggle` | `noVisualRecoil` | `false` | `cards/CardEnvironment.inl:41` |
| Misc / Estabilidade | Stream-Proof Interno | `CleanToggle` | `streamProof` | `false` | `cards/CardEnvironment.inl:43` |
| Particulas & Otimizacao | Particulas de Neve na Tela | `CleanToggle` | `snowParticles` | `false` | `cards/CardEnvironment.inl:50` |
| Particulas & Otimizacao | Clean Map (Sem Sombras) | `CleanToggle` | `cleanMap` | `false` | `cards/CardEnvironment.inl:52` |
| Particulas & Otimizacao | Remover Clima Original | `CleanToggle` | `removeOriginalWeather` | `false` | `cards/CardEnvironment.inl:54` |
| Particulas & Otimizacao | Cor da Fumaca | `CleanColorRow` | `g_cfg.colSmoke` | `` | `cards/CardEnvironment.inl:63` |
| Particulas & Otimizacao | Cor do Molotov / Fogo | `CleanColorRow` | `g_cfg.colMolotov` | `` | `cards/CardEnvironment.inl:64` |
| Particulas & Otimizacao | Cor do Disparo / Muzzle | `CleanColorRow` | `g_cfg.colMuzzle` | `` | `cards/CardEnvironment.inl:65` |
| Clima 3D no Mundo | Clima 3D Ativo | `CleanToggle` | `weather3d` | `false` | `cards/CardEnvironment.inl:72` |
| Clima 3D no Mundo | Tipo de Precipitacao | `CleanCombo` | `weatherKind` | `0` | `cards/CardEnvironment.inl:75` |
| Clima 3D no Mundo | Particulas Ativas | `CleanSliderFloat` | `countF` | `` | `cards/CardEnvironment.inl:77` |
| Clima 3D no Mundo | Velocidade da Queda | `CleanSliderFloat` | `weatherSpeed` | `550.` | `cards/CardEnvironment.inl:78` |
| Clima 3D no Mundo | Vento X | `CleanSliderFloat` | `weatherWindX` | `18.` | `cards/CardEnvironment.inl:79` |
| Clima 3D no Mundo | Vento Y | `CleanSliderFloat` | `weatherWindY` | `5.` | `cards/CardEnvironment.inl:80` |
| Skybox & Atmosfera | Modo Noturno Completo | `CleanToggle` | `nightMode` | `false` | `cards/CardEnvironment.inl:89` |
| Skybox & Atmosfera | Seletor de Skybox | `CleanCombo` | `skyIdx` | `` | `cards/CardEnvironment.inl:113` |
| Skybox & Atmosfera | Modulacao de Ceu Ativa | `CleanToggle` | `skyOn` | `` | `cards/CardEnvironment.inl:119` |
| Skybox & Atmosfera | Brilho do Ceu | `CleanSliderFloat` | `shared->skyBrightness` | `` | `cards/CardEnvironment.inl:122` |
| Skybox & Atmosfera | Cenario Escuro (Night Mode) | `CleanToggle` | `darkOn` | `` | `cards/CardEnvironment.inl:125` |
| Skybox & Atmosfera | Exposicao Cenario | `CleanSliderFloat` | `shared->worldBrightness` | `` | `cards/CardEnvironment.inl:129` |
| Skybox & Atmosfera | Densidade (0 = Original) | `CleanSliderFloat` | `fogDensity` | `0.0` | `cards/CardEnvironment.inl:132` |
| Skybox & Atmosfera | Cor da Nevoa (Fog Tint) | `CleanColorRow` | `g_cfg.colFog` | `` | `cards/CardEnvironment.inl:136` |
| Climate Preset | Climate Ativo | `CleanToggle` | `climateActive` | `false` | `cards/CardEnvironment.inl:146` |
| Climate Preset | Preset | `CleanCombo` | `climatePreset` | `0` | `cards/CardEnvironment.inl:149` |
| Climate Preset | Hue Shift | `CleanSliderFloat` | `climateHueShift` | `320.` | `cards/CardEnvironment.inl:151` |
| Climate Preset | Saturacao Boost | `CleanSliderFloat` | `climateSatBoost` | `40.` | `cards/CardEnvironment.inl:152` |
| Climate Preset | Valor Boost | `CleanSliderFloat` | `climateValBoost` | `8.` | `cards/CardEnvironment.inl:153` |
| Climate Preset | Bloom | `CleanSliderFloat` | `climateBloom` | `0.` | `cards/CardEnvironment.inl:155` |
| Climate Preset | Sombras | `CleanSliderFloat` | `climateShadowStr` | `1.` | `cards/CardEnvironment.inl:156` |
| Climate Preset | Tonemap | `CleanCombo` | `climateTonemap` | `0` | `cards/CardEnvironment.inl:158` |
| Pos-Processamento | Correcao de Cor da Cena | `CleanToggle` | `sceneColorEnabled` | `false` | `cards/CardEnvironment.inl:165` |
| Pos-Processamento | Saturacao | `CleanSliderFloat` | `sceneSaturation` | `1.` | `cards/CardEnvironment.inl:167` |
| Pos-Processamento | Contraste | `CleanSliderFloat` | `sceneContrast` | `1.` | `cards/CardEnvironment.inl:168` |
| Pos-Processamento | Temperatura | `CleanSliderFloat` | `sceneTemperature` | `0.` | `cards/CardEnvironment.inl:169` |
| Pos-Processamento | Tint Verde / Magenta | `CleanSliderFloat` | `sceneTint` | `0.` | `cards/CardEnvironment.inl:170` |
| Pos-Processamento | Curva de Tons | `CleanCombo` | `sceneColorCurve` | `0` | `cards/CardEnvironment.inl:172` |
| ESP Jogadores | ESP Ativo (Master) | `CleanToggle` | `espOn` | `` | `cards/CardVisuals.inl:15` |
| ESP Jogadores | Caixas Dinamicas por Osso | `CleanToggle` | `boxOn` | `` | `cards/CardVisuals.inl:24` |
| ESP Jogadores | Estilo da Caixa | `CleanCombo` | `espBoxType` | `1` | `cards/CardVisuals.inl:34` |
| ESP Jogadores | Esqueleto 3D Validado | `CleanToggle` | `skelOn` | `` | `cards/CardVisuals.inl:37` |
| ESP Jogadores | Barra de Vida | `CleanToggle` | `espHealth` | `true` | `cards/CardVisuals.inl:45` |
| ESP Jogadores | Barra de Armadura | `CleanToggle` | `espShowArmor` | `true` | `cards/CardVisuals.inl:46` |
| ESP Jogadores | Flags Taticas (Reload/Kit/Pin/Imune/C4) | `CleanToggle` | `espFlagsExtras` | `true` | `cards/CardVisuals.inl:47` |
| ESP Jogadores | Animacao Suave do ESP (Fade/Barras) | `CleanToggle` | `espSmoothAnimations` | `true` | `cards/CardVisuals.inl:48` |
| ESP Jogadores | Nome do Jogador | `CleanToggle` | `espName` | `true` | `cards/CardVisuals.inl:49` |
| ESP Jogadores | Arma Equipada | `CleanToggle` | `espWeapon` | `true` | `cards/CardVisuals.inl:50` |
| ESP Jogadores | Icones de Armas | `CleanToggle` | `espWeaponIcons` | `true` | `cards/CardVisuals.inl:51` |
| ESP Jogadores | Distancia em Metros | `CleanToggle` | `espDistance` | `true` | `cards/CardVisuals.inl:52` |
| ESP Jogadores | Alcance ESP | `CleanSliderFloat` | `espMaxDistance` | `8000.` | `cards/CardVisuals.inl:53` |
| ESP Jogadores | Linhas de Snap | `CleanToggle` | `espSnaplines` | `false` | `cards/CardVisuals.inl:54` |
| ESP Jogadores | Setas Fora de Tela | `CleanToggle` | `offScreenArrows` | `true` | `cards/CardVisuals.inl:55` |
| ESP Jogadores | Chams Visivel (Rapido) | `CleanToggle` | `chamsQuick` | `` | `cards/CardVisuals.inl:59` |
| Mundo & Cores | Circulo de FOV | `CleanToggle` | `espFovCircle` | `true` | `cards/CardVisuals.inl:69` |
| Mundo & Cores | Trajetoria 3D Estimada | `CleanToggle` | `showGrenadeTrajectory` | `true` | `cards/CardVisuals.inl:70` |
| Mundo & Cores | Circulo de Spread / Recoil | `CleanToggle` | `showSpreadCircle` | `false` | `cards/CardVisuals.inl:71` |
| Mundo & Cores | Timer da C4 | `CleanToggle` | `showBombTimer` | `true` | `cards/CardVisuals.inl:72` |
| Mundo & Cores | Aviso de Bomba Plantada | `CleanToggle` | `bombPlantedNotify` | `true` | `cards/CardVisuals.inl:73` |
| Mundo & Cores | Sem Borda do Scope (AWP/SSG) | `CleanToggle` | `noScopeOverlay` | `false` | `cards/CardVisuals.inl:74` |
| Mundo & Cores | ESP Defuser / Refens / Resgate | `CleanToggle` | `espDefuserHostage` | `true` | `cards/CardVisuals.inl:75` |
| Mundo & Cores | Event Logs na Tela (Dano/Kills/C4) | `CleanToggle` | `eventLogsScreen` | `true` | `cards/CardVisuals.inl:76` |
| Mundo & Cores | Scoreboard Override (Tabela Tatica TAB) | `CleanToggle` | `scoreboardRevealAll` | `false` | `cards/CardVisuals.inl:77` |
| Mundo & Cores | Raio Individual por Granada (3D) | `CleanToggle` | `grenadeIndividualRadius` | `true` | `cards/CardVisuals.inl:78` |
| Mundo & Cores | Raio da Smoke | `CleanSliderFloat` | `smokeRadiusUnits` | `144.` | `cards/CardVisuals.inl:80` |
| Mundo & Cores | Raio do Molotov | `CleanSliderFloat` | `molotovRadiusUnits` | `150.` | `cards/CardVisuals.inl:81` |
| Mundo & Cores | Linha Normal do Scope (Preta 1px) | `CleanToggle` | `cleanSniperScopeLines` | `true` | `cards/CardVisuals.inl:84` |
| Mundo & Cores | Compensar Recuo no Crosshair | `CleanToggle` | `scopePunchCompensation` | `true` | `cards/CardVisuals.inl:85` |
| Mundo & Cores | Dot Customizado AUG/SG553 | `CleanToggle` | `scopeCustomDotAug` | `true` | `cards/CardVisuals.inl:86` |
| Mundo & Cores | Ponto Vermelho Central | `CleanToggle` | `scopeCrosshairDot` | `false` | `cards/CardVisuals.inl:87` |
| Mundo & Cores | Vinheta Radial de Zoom (Nivel C) | `CleanToggle` | `scopeVignette` | `false` | `cards/CardVisuals.inl:88` |
| Mundo & Cores | Intensidade da Vinheta | `CleanSliderFloat` | `scopeVignetteAlpha` | `0.35` | `cards/CardVisuals.inl:90` |
| Mundo & Cores | Lista de Espectadores | `CleanToggle` | `showSpectators` | `false` | `cards/CardVisuals.inl:96` |
| Mundo & Cores | Sound ESP / Aneis 3D | `CleanToggle` | `soundEsp` | `false` | `cards/CardVisuals.inl:97` |
| Mundo & Cores | Alertas Sonoros Radiais | `CleanToggle` | `soundRadar` | `false` | `cards/CardVisuals.inl:98` |
| Mundo & Cores | Areas Estimadas de Granadas | `CleanToggle` | `grenadeZones` | `false` | `cards/CardVisuals.inl:99` |
| Mundo & Cores | Historico Visual de Movimento | `CleanToggle` | `poseHistory` | `false` | `cards/CardVisuals.inl:100` |
| Mundo & Cores | Janela do Historico | `CleanSliderFloat` | `window` | `` | `cards/CardVisuals.inl:103` |
| Mundo & Cores | Dano Flutuante 3D | `CleanToggle` | `worldDamage` | `false` | `cards/CardVisuals.inl:105` |
| Mundo & Cores | Bullet Tracers 3D | `CleanToggle` | `bulletTracers` | `false` | `cards/CardVisuals.inl:106` |
| Mundo & Cores | Caixas de Impacto | `CleanToggle` | `impactBoxes` | `false` | `cards/CardVisuals.inl:107` |
| Mundo & Cores | Efeito de Eliminacao | `CleanToggle` | `killEffects` | `false` | `cards/CardVisuals.inl:108` |
| Mundo & Cores | Preencher Areas de Fogo | `CleanToggle` | `grenadeAreaFill` | `false` | `cards/CardVisuals.inl:110` |
| Mundo & Cores | Brilho nas Bordas | `CleanToggle` | `grenadeAreaGlow` | `false` | `cards/CardVisuals.inl:111` |
| Mundo & Cores | Opacidade das Areas | `CleanSliderFloat` | `grenadeAreaOpacity` | `.18` | `cards/CardVisuals.inl:112` |
| Mundo & Cores | Espessura das Areas | `CleanSliderFloat` | `grenadeAreaWidth` | `1.5` | `cards/CardVisuals.inl:113` |
| Mundo & Cores | Brilho da Trajetoria | `CleanToggle` | `trajectoryGlow` | `false` | `cards/CardVisuals.inl:114` |
| Mundo & Cores | Cintilacao Suave | `CleanToggle` | `trajectoryShimmer` | `false` | `cards/CardVisuals.inl:115` |
| Mundo & Cores | Espessura da Trajetoria | `CleanSliderFloat` | `trajectoryWidth` | `1.8` | `cards/CardVisuals.inl:116` |
| Mundo & Cores | Pontos de Colisao | `CleanSliderFloat` | `trajectoryPointSize` | `3.4` | `cards/CardVisuals.inl:117` |
| Mundo & Cores | Opacidade da Trajetoria | `CleanSliderFloat` | `trajectoryOpacity` | `.88` | `cards/CardVisuals.inl:118` |
| Mundo & Cores | Cor da Trajetoria | `CleanColorRow` | `g_cfg.colTrajectory` | `` | `cards/CardVisuals.inl:119` |
| Mundo & Cores | Cor das Colisoes | `CleanColorRow` | `g_cfg.colTrajectoryCollision` | `` | `cards/CardVisuals.inl:120` |
| Mundo & Cores | Cor dos Tracers | `CleanColorRow` | `g_cfg.colTracer` | `` | `cards/CardVisuals.inl:124` |
| Mundo & Cores | Espessura dos Tracers | `CleanSliderFloat` | `tracerWidth` | `1.5` | `cards/CardVisuals.inl:125` |
| Mundo & Cores | Duracao dos Tracers | `CleanSliderFloat` | `tracerDuration` | `1.` | `cards/CardVisuals.inl:126` |
| Mundo & Cores | Brilho dos Tracers | `CleanToggle` | `tracerGlow` | `true` | `cards/CardVisuals.inl:127` |
| Mundo & Cores | Cor dos Impactos | `CleanColorRow` | `g_cfg.colImpact` | `` | `cards/CardVisuals.inl:128` |
| Mundo & Cores | Tamanho dos Impactos | `CleanSliderFloat` | `impactSize` | `2.` | `cards/CardVisuals.inl:129` |
| Mundo & Cores | Espessura dos Impactos | `CleanSliderFloat` | `impactWidth` | `1.` | `cards/CardVisuals.inl:130` |
| Mundo & Cores | Duracao dos Impactos | `CleanSliderFloat` | `impactDuration` | `1.` | `cards/CardVisuals.inl:131` |
| Mundo & Cores | Opacidade dos Impactos | `CleanSliderFloat` | `impactOpacity` | `1.` | `cards/CardVisuals.inl:132` |
| Mundo & Cores | Brilho dos Impactos | `CleanToggle` | `impactGlow` | `false` | `cards/CardVisuals.inl:133` |
| Mundo & Cores | Preencher Impactos | `CleanToggle` | `impactFill` | `false` | `cards/CardVisuals.inl:134` |
| Mundo & Cores | Faiscas nos Impactos | `CleanToggle` | `impactSparks` | `false` | `cards/CardVisuals.inl:135` |
| Mundo & Cores | Dormant ESP / Fade | `CleanToggle` | `dormantFade` | `false` | `cards/CardVisuals.inl:138` |
| Mundo & Cores | Tempo de Fade | `CleanSliderFloat` | `dormantSeconds` | `2.5` | `cards/CardVisuals.inl:139` |
| Mundo & Cores | Cor Inimigos | `CleanColorRow` | `g_cfg.colEnemy` | `` | `cards/CardVisuals.inl:141` |
| Mundo & Cores | Cor Aliados | `CleanColorRow` | `g_cfg.colTeam` | `` | `cards/CardVisuals.inl:142` |
| Mundo & Cores | Cor Esqueleto | `CleanColorRow` | `g_cfg.colSkeleton` | `` | `cards/CardVisuals.inl:143` |
| Mundo & Cores | Cor Snaplines | `CleanColorRow` | `g_cfg.colSnaplines` | `` | `cards/CardVisuals.inl:144` |
| Killfeed Persistente | Ativar Killfeed Persistente | `CleanToggle` | `killfeedEnabled` | `true` | `cards/CardVisuals.inl:150` |
| Killfeed Persistente | Apenas Eliminacoes Locais | `CleanToggle` | `killfeedLocalOnly` | `false` | `cards/CardVisuals.inl:152` |
| Killfeed Persistente | Duracao na Tela | `CleanSliderFloat` | `killfeedDuration` | `15.` | `cards/CardVisuals.inl:153` |
| Killfeed Persistente | Linhas Maximas | `CleanSliderFloat` | `linesF` | `` | `cards/CardVisuals.inl:155` |
| Chams de Jogadores | Chams Visivel | `CleanToggle` | `vis` | `` | `cards/CardVisuals.inl:169` |
| Chams de Jogadores | Cor Visivel | `CleanColorRow` | `cVis` | `` | `cards/CardVisuals.inl:174` |
| Chams de Jogadores | Chams Oculto (Parede) | `CleanToggle` | `hid` | `` | `cards/CardVisuals.inl:178` |
| Chams de Jogadores | Cor Oculto | `CleanColorRow` | `cHid` | `` | `cards/CardVisuals.inl:183` |
| Chams de Jogadores | Chams Bracos / Luvas | `CleanToggle` | `arms` | `` | `cards/CardVisuals.inl:187` |
| Chams de Jogadores | Cor Bracos | `CleanColorRow` | `cArms` | `` | `cards/CardVisuals.inl:192` |
| Chams de Jogadores | History (Rastro) | `CleanToggle` | `chamsHistory` | `false` | `cards/CardVisuals.inl:196` |
| Chams de Jogadores | Cor History | `CleanColorRow` | `g_cfg.colHistory` | `` | `cards/CardVisuals.inl:197` |
| Chams de Jogadores | On-Shot (Kill Flash) | `CleanToggle` | `chamsOnShot` | `false` | `cards/CardVisuals.inl:198` |
| Chams de Jogadores | Cor On-Shot | `CleanColorRow` | `g_cfg.colOnShot` | `` | `cards/CardVisuals.inl:199` |
| Chams de Jogadores | Local Player | `CleanToggle` | `chamsLocal` | `false` | `cards/CardVisuals.inl:200` |
| Chams de Jogadores | Cor Local | `CleanColorRow` | `g_cfg.colLocal` | `` | `cards/CardVisuals.inl:201` |
| Chams de Jogadores | Aliados (Team) | `CleanToggle` | `chamsTeam` | `false` | `cards/CardVisuals.inl:202` |
| Chams de Jogadores | Cor Team | `CleanColorRow` | `g_cfg.colTeam` | `` | `cards/CardVisuals.inl:203` |
| Chams de Jogadores | Glow (Contorno Luminoso) | `CleanToggle` | `chamsGlow` | `false` | `cards/CardVisuals.inl:204` |
| Chams de Jogadores | Escala do Glow | `CleanSliderFloat` | `chamsGlowScale` | `1.5` | `cards/CardVisuals.inl:205` |
| Chams de Jogadores | Chams Arma 1P (Viewmodel) | `CleanToggle` | `chamsWeapon` | `false` | `cards/CardVisuals.inl:208` |
| Chams de Jogadores | XRay Arma Inimiga (Z-Ignore) | `CleanToggle` | `weaponXray` | `false` | `cards/CardVisuals.inl:209` |
| Chams de Jogadores | Props Fosco (Objetos do Mapa) | `CleanToggle` | `propChams` | `false` | `cards/CardVisuals.inl:210` |
| Chams de Jogadores | Apenas Inimigos | `CleanToggle` | `tf` | `` | `cards/CardVisuals.inl:212` |
| Gradiente Hermite | Ativar Gradiente Vertical | `CleanToggle` | `grad` | `` | `cards/CardVisuals.inl:221` |
| Gradiente Hermite | Cor Base do Gradiente | `CleanColorRow` | `cGrad` | `` | `cards/CardVisuals.inl:225` |
| Estilo de Material & Outline | Material Visivel | `CleanCombo` | `chamsVisibleStyle` | `0` | `cards/CardVisuals.inl:241` |
| Estilo de Material & Outline | Material Oculto (Parede) | `CleanCombo` | `chamsHiddenStyle` | `2` | `cards/CardVisuals.inl:244` |
| Estilo de Material & Outline | Efeito Dupla Camada (Overlay Wireframe) | `CleanToggle` | `chamsDualLayer` | `false` | `cards/CardVisuals.inl:248` |
| Estilo de Material & Outline | Contorno Brilhante (Outline Glow) | `CleanToggle` | `glowOutline` | `true` | `cards/CardVisuals.inl:249` |
| Radar, Flash & Smoke | Radar Nativo | `CleanToggle` | `nativeRadar` | `false` | `cards/CardVisuals.inl:255` |
| Radar, Flash & Smoke | Limitar Alpha do Flash | `CleanToggle` | `limitFlash` | `false` | `cards/CardVisuals.inl:256` |
| Radar, Flash & Smoke | Alpha Maximo | `CleanSliderFloat` | `flashAlphaLimit` | `.25` | `cards/CardVisuals.inl:257` |
| Radar, Flash & Smoke | Remover Fumaca (Smoke) | `CleanToggle` | `removeSmoke` | `false` | `cards/CardVisuals.inl:258` |

---

**501 widgets** em 125 arquivos lidos (`.bak-*` excluídos).

<!-- gerado por tools/vault_sync.py — corpo a4374cf270b7 -->
