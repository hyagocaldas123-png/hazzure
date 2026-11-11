# Índice de tarefas — T000 a T060

> Fonte: `estado:`/`titulo:` do frontmatter de `20_Tarefas/T0xx.md` (T000–T020) e os blocos de
> `00_Gestao/Reservas.md` + `00_Gestao/Estado.md` (T021 em diante). Hash = SHA-256 da DLL daquele
> ciclo, quando o registro traz hash. **"entregue" aqui significa código + build + deploy** —
> validação em partida vive em [[VALIDACAO]], não nesta coluna.

## Como ler o estado

| Valor | Significado real |
|---|---|
| `concluida` | ciclo fechado na época; não implica runtime validado |
| `concluida_isolada` | entregue num escopo isolado, sem integração completa |
| `instalada_aguardando_validacao` | DLL instalada, falta o olho do usuário em partida |
| `pausada` / `aguardando` | aberta, sem execução |
| `entregue (build+deploy)` | T021+: compilou, foi para `C:\HazzureKernel\`, não foi necessariamente injetada |

## T000–T020 (arquivo próprio em `20_Tarefas/`)

| Tarefa | Título | Estado | Executor | Arquivo |
|---|---|---|---|---|
| T000 | Organização do Obsidian | concluida | Codex coordenador | [[T000]] |
| T001 | Busca e navegação | concluida | senior | [[T001]] |
| T002 | Perfis e salvamento | **pausada** | senior | [[T002]] |
| T003 | Inventário e miniaturas | concluida_isolada | senior | [[T003]] |
| T004 | Status dos recursos | concluida_isolada | senior | [[T004]] |
| T005 | Revisão visual e acabamento | concluida_isolada | senior | [[T005]] |
| T006 | Contrato de equipagem nativa | concluida | schema-offset-specialist | [[T006]] |
| T007 | Implementação da equipagem | **aguardando** | senior | [[T007]] |
| T008 | CRUD real e troca de mapa | **aguardando** | senior | [[T008]] |
| T009 | Integração e publicação | **aguardando** | coordenador | [[T009]] |
| T010 | Corrigir rotas e bindings da GUI | concluida_isolada | Codex senior | [[T010]] |
| T011 | Simplificação segura e fila de inventário | concluida_isolada | Codex senior | [[T011]] |
| T012 | Aimbot e Trigger (build 14194) | concluida | senior | [[T012]] |
| T013 | World ESP, efeitos, FOV e clima 14194 | instalada_aguardando_validacao | senior | [[T013]] |
| T014 | Espectadores e compilação cumulativa | concluida_isolada | senior | [[T014]] |
| T015 | Luva invisível TR | instalada_aguardando_validacao | senior | [[T015]] |
| T016 | Bhop integrado e máscara sniper | instalada_aguardando_validacao | senior | [[T016]] |
| T017 | Pendências visuais, keybinds, consolidação de runtime | instalada_aguardando_validacao | senior | [[T017]] |
| T018 | Atualização geral de RVAs/globais/assinaturas (hotfix 0x6AE91E98) | concluida_instalada | senior | [[T018]] |
| T019 | UI State Routing, SOCache Preflight, Reset AnimGraph/Viewmodel | concluida_instalada | senior | [[T019]] |
| T020 | Correção de crash no triggerbot e suavização fallback | sem frontmatter | — | [[T020]] (o arquivo traz T020 **+ T021**) |

## T021–T051 (sem arquivo próprio — narrativa em [[Historico-T021-T050]] e [[Estado]])

| Tarefa | Do que se trata | Estado real | Hash da DLL |
|---|---|---|---|
| T021 | Crash do triggerbot / aimbot "robotizado" — encadeamento inseguro no `Hook_CreateMove` | entregue | — |
| T022–T024 | **sem registro no vault** (números não usados) | — | — |
| T025 | Só citado como referência a `GSLayout/UIFrame/UIWorldDrawing/cards` | — | — |
| T026 | Reancoragem na build `0x6AEBC4DA`, botões, `dwCSGOInput` como ponteiro, aim/trigger fail-closed | concluída | `f218e3869e08` |
| T027 | Inventário nativo — vtable de tipo | concluída | `2cb146097e34` |
| T028 | Trigger pelo CUserCmd (canal de botões vivo) | concluída | `06fd46a22ca9` |
| T029 | Inventário — reaplicar equips, aposentar skinchanger legado | concluída | `117fdd2d0a0d` |
| T030 | Bhop no CUserCmd + auditoria do crash "error material" | concluída | `8f1193a1b9d3` |
| T031 | Visibilidade do aim/trigger, equip por slot, 3ª pessoa removida | concluída | `998f4bb31328` |
| T032 | Skins/equips nativos, chams, gate morto de stage 9 | concluída | `330b93fcf014` |
| T033 | Skins (attrs do item view), canal de mira, toggle de mira livre no bhop | concluída | `01d8c8347e43` |
| T034 | **sem registro no vault** | — | — |
| T035 | **Inventory changer** — view base `0x12E0`, getter `vtable[0]`, lista vazia | **validada em partida** (relato do usuário) | `0ba7c8148d89` |
| T036 | Rage aim + anti-aim, chams MemeSense, bhop, grenade helper, painel, WAV | build concluída | `d2515b498e19` |
| T037 | Bhop fora da janela de spam do motor (ponto 1 de 4) | entregue | `6b86f10f8ce8` |
| T038 / T038b / T038c / T038d | Lua de Tsukuyomi como orbe 3D com shader GPU, Sharingan legível, oclusão por trace, padrão Rinne | implementado e instalado | `7cc7d376eb7f`, `46150f32ebfb`, `755f93701d47`, `2a3597a8d321` |
| T039 | Luva no lado T + anti-aim em camadas + aim lock FOV/smooth | implementado e instalado | `e07f7bb71bd0` |
| T039b | Revert da escrita de modelo na entidade de mãos (regressão visual da T039) | entregue | `0dc32177426c` |
| T040 | Features do dossiê MemeSense (ImGui demo, modos de movement, recording/lineups) | **reservado, sem código** | — |
| T041 | Prova do loadout de luva por time + fim do flood `WndProc ownership conflict` | reservado → implementado | `c5d98721aca9` |
| T042 | SetModel + mesh mask nas mãos, toggle real do agent changer, `[GLOVESLOT]` | entregue — **hipótese falseada pela medição** | `18f088a88db2` |
| T043 | Revert da escrita nas mãos | entregue | `150227ea08e9` |
| T044 | Re-apply nativo da luva dirigido por bodygroup | entregue, runtime em aberto | `fb9a500f4f74` |
| T045 | Gate de transição do céu (mecanismo do crash de troca de mapa) | entregue, não validado | `1dad849618bf` |
| T046 | Autowall "chega / não chega" só com fraction+solid+contents | entregue, runtime indisponível na época | `6391d9f23a71` |
| T047 | Rage auto-fire 360, thirdperson para ver o anti-aim, bhop boost, fix do popup ImGui | entregue | `abe9912ed41f` |
| T048 | Timer da C4 (`dwPlantedC4` estava errado) + aviso de plantação self-only | entregue, plant nunca observado | `39763c7d186c` |
| T049 | Borda preta do scope (AWP/SSG08) via `cl_ironsight_filter_alpha` | última sessão que a carregou: 06/11 02:27 | `247eb4ba589e` |
| T050 | Os 4 técnicos que o teste derrubou: strafe, auto-fire, thirdperson, autowall honesto | build+deploy, **nunca injetado** | `92af769c6d20` |
| T051 | Vault como segundo cérebro (este arquivo) | entregue 07/11, sem código de cheat tocado | — |
| T052 | Anti-aim +89/180, thirdperson direto, Silent Aim no auto-fire, aba Movement com Air Boost | **injetado e testado** na sessão 03:21:35 (pid 23924) — os sintomas que a T053 conserta saíram daqui. O entregador não escreveu no [[Estado]]; a seção retroativa está lá | `260794154939` |
| T053 | AA aplica com alvo travado, prioridade por tick Silent↔AA, auto-fire fora do rage, restauração do AA para de girar a mira | **injetado e testado** (pid 5416, `[START]` 04:16:45) — `applied=` medido funciona (797/912), mas o usuário não viu anti-aim nem aimbot funcionando; os sintomas que a T054 ataca saíram daqui | `dbfc74bb0a82` |
| T054 | Aimbot destravado do `rageSilentAim`, probe + escrita do ângulo **no `CUserCmd`**, `aimStatus` separável do AA | build+deploy 04:37:20, **não injetado** (o pid 5416 continua no t053) | `c5d5913b9a05` |
| T055 | Timer da C4 (`gpGlobals` 0x2235108 + `NativeDamage` 0x80DD0) + aviso de plantação | entregue | `0f8688ea8271` |
| T056 | Extrapolação de posição + Safe Point + Force Baim + Trigger Air/Flash Check | entregue | `5ef4dacbce2b` |
| T057 | Pacote 1 de Features da MATRIZ_H (Trigger Flash/Air Check, Scope Borda Preta) | entregue | `0f399047d331` |
| T058 | Limpeza Movement (eliminação de sobras do Bhop/Tech/AutoBuy) + eliminação de chaves sem consumidor | entregue | `401cae7e67c0` |
| T059 | Redesign GUI MemeSense 6 Telas + Pacote Features CS2 (Ammo Bar, Auto Accept, etc.) + Tema B&W/Neve | entregue | `9c80249284a5` |
| T060 | Motor de Movimento Físico (Autobhop sem fricção + Landing Prediction 1..8 ticks + Air Strafe 89° + Movement Fix) | entregue | `70550a40f28c` |
| T060b | Fix Bhop Punição (TraceShape 14194 + buttonstate2_ edge reset + ground toggle fix) | entregue | `ed3e92230a30` |
| T061 | Bhop Simples Puro (Sem Intermitência) + Limpeza de Clutter Movimentação e Aba World | entregue | `5f90ed221381` |
| T062 | Sistema de Lineups de Granada Nativo CS2 (Sub-tick CUserCmd + Solver Balístico + Guia Visual) | entregue | `c44cdb7be41f` |
| T063 | Eliminação da Punição de Fricção do Bhop (Previsão de Aterrissagem Raycast + Borda Limpa) | entregue | `99d7f08a66a7` |
| T064 | Instrumentação CSV por Tick (bhop_telemetry.csv) + PredictContact Duck-Aware + Loop Mínimo Bhop sem Fricção + Strafe 89° | entregue | `d02998253e6f` |
| T065 | Eliminação da Oscilação no Solo + Correção Canônica do Protobuf buttons_pb em +0x38 e Desarme de Sub-tick em ar no Bhop | entregue | `b16b889430f1` |
| T066 | Remoção Nativa do Scope Overlay via Hook DrawScopeOverlay 14194 + Crosshair com Punch Compensation + AUG/SG Dot + Vinheta | entregue | `55c2dadf7884` |
| T067 | Remoção da Linha Artificial do Scope (Gap/Dot/Sombra) + Linha Normal 1px CS2 + Caller-Aware DrawScopeOverlay | entregue | `5cf195df9825` |
| T068 | Rage Fase 1: Pipeline Linear CreateMove + Reprojeção 2D RotateInputs + Anti-Aim Estável com Bhop/Strafe | entregue | `4b502639bdf6` |
| T069 | Rage Fase 2: Perfect Silent Sub-tick (Zero Snap) + Firing Crosshair Swapchain com Punch Compensation | entregue | `5938acf4a063` |
| T070 | RE do CUserCmd/CSGOUserCmdPB: ângulos no `CMsgQAngle` do `input_history_` (+0x10/+0x18/+0x1C/+0x20), probe de 512 B retirado como instrumento | análise, sem código | — |
| T071 | Desync 1ª perna: yaw do Anti-Aim no sub-tick sem tocar na câmera + reprojeção no yaw enviado (`rageDesync`) | entregue; **injetado 08:01:45 e o jogo caiu 08:02:31** sem executar nenhum tick de desync | `252028ca0e3d` |
| T072 | Forense do crash (`client.dll+0x7B0CBB` = `Clear()` da entrada de histórico) + correção do has-bit do carimbo: `0x2` SHOOTPOSITION → `0x1` VIEWANGLES | entregue, não injetado; **binário perdido** no incidente de backup do deploy t078 (correção segue no código) | `f4f6fa67eefd` (órfão) |
| T073 | Crosshair persistente (Pilar 3) + indicador visual de subtick — ciclo concorrente, sem seção própria no Estado | entregue, não injetado | `ee7a90a03aa3` |
| T074–T077 | Ciclo OPUS 4.3: chams 6 layers + climate presets + GUI + IPC + persistência (registro coletivo; o ciclo não deixou seções individuais nem backups de t074/t075/t076) | entregue, não injetado; só o t077 sobrevive em `.bak-20261109-t078` (root) | t077 `0decfd3e8c90` |
| T078 | Réplica exata da GUI "Enigma v3" (briefing GLM 5.3): shell 1280×735 + sidebar 250 + 11 rows RAGE + preview 440; zero arquivo novo; acumula o estado final de T074–T077 | entregue (build exit 0, self-test PASS), **não injetado — compilado ≠ validado** | `8d04fa0265e3` |

## Regras que atravessam todas as linhas

- Só o usuário injeta; DLL no disco ≠ DLL em memória ([[Estado]]).
- Build/self-test não comprovam runtime ([[VALIDACAO]]).
- ~~Fake lag/choke, desync, body/manual yaw e resolver de inimigo: análise, nunca implementação.~~ **Revogado pelo usuário em 2026-11-08** ("Confirma, revoga as cinco") — viraram backlog; a T071 é a primeira perna implementada.
- Gameplay complexo (silent aim, backtrack, dano de autowall, prefire) exige ordem explícita nomeando a feature. **Mesma revogação de 2026-11-08** cobre pSilent/desync/resolver/prefire/backtrack/pen-dano; continuam valendo as restrições de operação (só o usuário injeta, um build por ciclo, RE antes de offset).
- Uma reserva em [[Reservas]] antes de editar; um build por ciclo.
