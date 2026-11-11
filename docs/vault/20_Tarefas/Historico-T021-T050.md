# Histórico T021 → T050 (condensado)

> Por que este arquivo existe: `20_Tarefas/` parava em `T020.md`. As tarefas T021–T050 só viviam como
> parágrafos dentro de [[Reservas]] (104 KB) e [[Estado]] (135 KB). Este é o resumo de uma tela por
> tarefa, condensado **daqueles dois arquivos** — nada foi reconstruído de memória de conversa.
>
> Legenda de confiabilidade:
> - **evidência direta** = hash, exit code de build, linha de log ou caminho citados no registro.
> - **inferido** = só o texto do registro afirma; sem binário/log correspondente localizado.
> - Narrativa completa: procure o cabeçalho `T0xx` em [[Estado]] (a seção original foi mesclada aqui
>   sem edição) ou o bloco de reserva em [[Reservas]].

---

## T021 — Crash do triggerbot / aimbot "robotizado"
- **Feito**: `Hook_CreateMove` deixou de encadear o `CUserCmd` de forma insegura; diagnóstico do crash e da mira mecânica.
- **Evidência**: inferida — o bloco de reserva não registra hash da DLL; a narrativa está em [[Estado]] (duas versões, vault e repo).
- **Devendo**: nada declarado; a correção foi reancorada de fato na T026.

## T022–T025 — não existem como tarefa
- Nenhum bloco em [[Reservas]]/[[Estado]] e nenhum arquivo. `T025` aparece uma única vez, como referência a arquivos de UI (`GSLayout.inl`, `UIFrame.inl`, `UIWorldDrawing.inl`, `cards/*`). **Não invente trabalho para esses números.**

## T026 — Reancoragem na build `0x6AEBC4DA`
- **Feito**: botões reancorados (`0x2727E30..0x27284F0`, trio `0x2240BA0/0x2240A80/0x2240B10`), `dwCSGOInput` tratado como ponteiro, bloco de materiais/recursos restaurado, gates da build em todos os perfis, aim/trigger fail-closed por visibilidade.
- **Evidência direta**: build `f218e3869e08`, `build.cmd` exit 0, suíte 10/10.
- **Importante**: é a partir daqui que todo endereço do vault é RVA sobre `client.dll` `0x6AEBC4DA` / SizeOfImage `0x29A3000` (build 14194).

## T027 — Inventário nativo: vtable de tipo
- **Feito**: identificação de entidade por vtable de tipo (RTTI), base do que virou o inventory changer.
- **Evidência direta**: `2cb146097e34` (rollback `76d88752c91e`).
- **Aberto na época**: offsets de botões — cluster sem leitores no binário.

## T028 — Trigger pelo CUserCmd
- **Feito**: trigger passou a usar o canal de botões vivo do `CUserCmd` em vez do cluster morto.
- **Evidência direta**: `06fd46a22ca9`, 10/10 PASS.

## T029 — Inventário: reaplicar equips
- **Feito**: reequipar no início e aposentar o skinchanger legado.
- **Evidência direta**: `117fdd2d0a0d`.

## T030 — Bhop no CUserCmd + auditoria do crash de material
- **Feito**: bhop no canal do comando; auditoria do crash "error material".
- **Evidência direta**: `8f1193a1b9d3`.

## T031 — Visibilidade do aim/trigger, equip por slot, 3ª pessoa removida
- **Evidência direta**: `998f4bb31328`, rollback `*.bak-20261105-t031`.

## T032 — Skins/equips nativos, chams, gate morto de stage 9
- **Evidência direta**: `330b93fcf014` (2.881.024 B).

## T033 — Skins (attrs do item view), canal de mira, mira livre no bhop
- **Feito**: skins escritas nos atributos do item view; aim/trigger no canal de mira; toggle de mira livre no bhop.
- **Evidência direta**: `01d8c8347e43` (2.882.560 B).

## T034 — não existe
- Sem bloco, sem arquivo.

## T035 — **Inventory changer** (a feature validada em partida)
- **Feito**: view base em `0x12E0`, getter por `vtable[0]`, correção da lista vazia.
- **Evidência direta**: `0ba7c8148d89` (2.885.632 B), 10/10 PASS **e** validação em partida confirmada pelo usuário ("funcionou o inventory changer") após reinício + reinjeção.
- **Por que importa**: é o único item do tracker com confirmação humana explícita, junto de aimbot e triggerbot.

## T036 — Rage aim + anti-aim, chams, bhop, grenade helper, painel, WAV
- **Evidência direta**: `d2515b498e19` (2.900.480 B).
- **Limite registrado**: silent aim/backtrack/resolver ficaram fora — exigem ordem explícita.

## T037 — Bhop fora da janela de spam do motor
- **Feito**: ponto 1 de 4 — bhop sem cair no filtro de spam do engine.
- **Evidência direta**: `6b86f10f8ce8` (2.900.480 B), rollback T036.

## T038 / T038b / T038c / T038d — Lua de Tsukuyomi (orbe 3D com shader GPU)
- **Feito**: orbe 3D real em vez de disco 2D; Sharingan legível corrigido (a face visível apontava para `-dir`, o padrão vivia no hemisfério de trás); oclusão por trace nativo; céu carmesim e brasas; padrão Rinne medido na referência e convertido em raio polar.
- **Evidência direta**: `7cc7d376eb7f`, `46150f32ebfb`, `755f93701d47`, `2a3597a8d321`.
- **Regra que nasceu aqui**: nenhum trace a partir da thread de apresentação; não tocar em `C_EnvSky`/`MakeSkyMaterial` nem no mask de trace `0x1C1003`.

## T039 — Luva no lado T + anti-aim em camadas
- **Feito**: identidade da luva passou a incluir a entidade de mãos (handle `pawn+0x1DA8`, RTTI `.?AVC_CS2HudModelArms@@`); anti-aim em camadas; aim lock por FOV + smooth.
- **Evidência direta**: `e07f7bb71bd0`.
- **Consequência**: a escrita de modelo nas mãos introduzida aqui causou regressão visual → T039b.

## T039b — Revert da escrita de modelo nas mãos
- **Evidência direta**: `0dc32177426c`.

## T040 — Dossiê MemeSense (spec, não dump)
- **Estado**: **reservado, sem código**. ImGui demo, modos de movement (Crouch&Jump / Stand&Jump / auto), recording/lineups de granada.
- **Decisão registrada**: skinchanger paralelo não entra — o inventory changer da T035 já cobre.
- **Referência**: `C:\Codex2\gpt\memesense_handoff\HANDOFF_QODER.md` (binário ofuscado; implementar do zero, nunca copiar bytes).

## T041 — Prova do loadout de luva por time + fim do flood de `WndProc ownership conflict`
- **Evidência direta**: `c5d98721aca9`.

## T042 — SetModel + mesh mask nas mãos (a que falseou a própria hipótese)
- **Feito**: luva com mesh group mask nas mãos + toggle real do agent changer + `[GLOVESLOT]` consertado.
- **Evidência direta**: `18f088a88db2` (2.942.464 B).
- **Resultado medido**: escrever modelo na entidade de mãos fez o motor devolver mask `0xFFFFFFFFFFFFFFFE` no frame seguinte e sumir com a geometria das mãos ("faca flutuando"). **Hipótese falseada** — revertida na T043 e documentada em `SkinEngine.h:711-714`.

## T043 — Revert da escrita nas mãos
- **Evidência direta**: `150227ea08e9` (2.941.440 B). Registro diz que no momento do deploy o processo ainda rodava t042.

## T044 — Re-apply nativo da luva dirigido por bodygroup
- **Feito**: chamar o re-apply de bodygroup do motor — o elo que faltava desde a T039.
- **Evidência direta**: `fb9a500f4f74` (2.942.464 B).
- **Em aberto**: o efeito visual em partida. O log atual mostra `[GLOVEBG] ok=1 changed=184`, ou seja a aplicação acontece; se aparece na tela é leitura do usuário.

## T045 — Gate de transição do céu (crash de troca de mapa)
- **Feito**: mecanismo do crash identificado no nosso próprio código (banda histórica `client+0x230daf`), gate de transição.
- **Evidência direta**: `1dad849618bf`.
- **Em aberto**: sobreviver a uma troca de mapa real ainda não foi observado.

## T046 — Autowall "chega / não chega"
- **Feito**: veredito só com fraction/solid/contents, sem fórmula de penetração inventada.
- **Evidência direta**: `6391d9f23a71`; runtime indisponível no ciclo.
- **Prova posterior** (sessão t049): `[AUTOWALL] sample=6 verdict=4 layers=1 rays=4 contents=0x00000282` e o veredito gateando escolha de ponto em `CombatTargeting.inl:47`.
- **Limitação assumida**: decide **alcance**, não dano. Texto de honestidade na UI entregue na T050.

## T047 — Rage auto-fire 360, thirdperson, bhop boost, fix do popup ImGui
- **Evidência direta**: `abe9912ed41f`.
- **Foi isso que o teste em partida derrubou** → T050.

## T048 — Timer da C4 + aviso de plantação self-only
- **Feito**: `dwPlantedC4` corrigido (`0x24D0B70`), identificação robusta da C4 no `TacticalHUD.h`, notificação "bomba plantada" só para mim, diagnóstico de falha no log.
- **Evidência direta**: `39763c7d186c`.
- **Em aberto**: nunca houve plant em partida para testar — o log repete `[C4] timer sem bomba: dwPlantedC4=0x0`.

## T049 — Borda preta do scope (AWP / SSG08)
- **Feito**: suprimir o overlay via convar `cl_ironsight_filter_alpha` (`client+0x24DBE50`, float em `[global+8]+0x58`); zoom/FOV/precisão continuam nativos.
- **Evidência direta**: `247eb4ba589e` e, em partida, `[SCOPE] filtro: medido=1.000 entregue=0.000 alvo=0.000 ligado=1 writes=1 slot=0x2823B9A9EC8` → depois `medido=0.000` estabilizado. Leitura, validação de layout, escrita e valor retido pelo motor.
- **Em aberto**: se a moldura sumiu visualmente é o olho do usuário. Na sessão em que isto foi
  medido (06/11 02:27, pid 27864) **esta era a DLL em memória**; ao fim dela o log já mostrava
  `alvo=1.000 medido=1.000 writes=90`, ou seja o slider tinha voltado para 1 — o caso controle, não
  um contra-exemplo. Hoje (07/11) nada está em memória: ver Estado › "O que está em memória AGORA".

## T050 — Os 4 técnicos que o teste em partida derrubou
- **Feito**: modelo de air-strafe corrigido (`wishspeed` do motor nos dois slots → ângulo ótimo 84–86° em vez de 90° cravado) + gate de ar aceitando o pulo que é nosso; auto-fire solto do rage master; thirdperson escondendo `C_CS2HudModelArms`/`C_CS2HudModelWeapon` com contadores `[3P]`; texto de honestidade do autowall no painel.
- **Evidência direta**: `92af769c6d20` (2.957.312 B), 10/10 PASS, rollback `247eb4ba…` em `Hazzure.Internal.dll.bak-20261106-t050`.
- **Diagnóstico que justificou cada item**: `space=0 ground=0` em 1349 frames aéreos contra `space=1` em 134 (o steer quase nunca rodava); `triggerStamp` congelado em 12805 desde 02:33 (auto-fire nunca disparou); `NativeCamera.h` só deslocava a origem da câmera — nada escondia o viewmodel.
- **Status**: **não injetado**. Tudo que a T050 afirma é código compilado, não comportamento observado.

## T051 — Vault como segundo cérebro
- Este arquivo, [[INDICE]], [[Estado]], [[VALIDACAO]], `30_Inventario/` e `70_Ferramentas/`. Sem mudança de código do cheat.
