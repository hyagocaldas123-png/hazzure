# Decisões vigentes

- **[2026-11-07] Uma fonte atual: `00_Gestao/Estado.md` deste vault.** Substitui a linha antiga
  ("fonte = C:/Codex2/gpt/ESTADO_ATUAL.md"), que estava falsa: havia dois estados divergindo, cada
  um com seções que o outro não tinha. Mesclados por `tools/vault_merge_estado.py`, ambos os
  `ESTADO_ATUAL.md` viraram ponteiros e os originais ficaram em `*.bak-20261107-vault-merge`.
- **[2026-11-07] O vault manda, o repo aponta.** Regras de bancada continuam em
  `C:/Codex2/gpt/agents.md`, mas estado, validação, fila, inventário e playbook vivem aqui, e a
  porta de entrada de qualquer chat novo é `Inicio.md`.
- **[2026-11-07] O que é derivável do código é gerado, não transcrito.** `tools/vault_sync.py`
  escreve `30_Inventario/{MENU,CONFIG_KEYS,OFFSETS,ARTEFATOS}.md` a partir do fonte +
  `C:/HazzureKernel`; é determinístico, marca o arquivo como "não editar à mão" e `--check` sai
  !=0 quando o vault divergiu do código. O que é julgamento humano (validação, decisão, histórico
  condensado) continua escrito à mão.
- **[2026-11-07] Verificação do vault é um comando só.** `tools/vault_sync.py --check` roda os dois
  passos — drift do inventário contra o código/deploy **e** `tools/vault_lint.py` sobre os `.md`
  (encoding, frontmatter, `[[wikilink]]` e links relativos, colunas de tabela, seções duplicadas,
  texto colado, CRLF). exit 1 em qualquer falha. Motivo: o aceite anterior respondia "o inventário
  bate" mas deixava de fora a classe de defeito que o diff de títulos não pegava (a duplicata de
  `## T026`); uma checagem que não cobre o defeito já observado não é uma checagem. `vault_lint.py`
  continua acionável sozinho, e `HAZZURE_VAULT` permite apontá-lo para outra pasta em teste.
- **[2026-11-07] Compilado ≠ validado em partida.** Toda afirmação de funcionamento referencia a
  matriz `00_Gestao/VALIDACAO.md`; build, self-test e mock não contam como evidência de runtime, e
  uma DLL no disco não é a DLL em memória (manual map, sem hot reload).
- **[2026-11-07] Número impresso em log tem que ser lido de volta, não escrito.** O campo
  `applied=1` do `[ANTIAIM]` era literal no código e virou a "prova" de anti-aim validado em
  `VALIDACAO.md`; a linha foi retirada. Regra: telemetria de aplicação (escrevi no `CUserCmd`? o motor
  aceitou?) é contada/lerada no local onde a escrita acontece (hook) e consumida por quem reporta
  (render thread/menu), nunca deduzida do pedido.
- **[2026-11-07] Auto-fire deixou de ser sub-item do rage.** É recurso próprio (`autoFire`,
  Legitbot › Misc) com critério medido — residual angular do ângulo enviado contra o ponto resolvido,
  dentro de `autoFireOnTargetDeg`. Ligar o rage não dispara nada; com o botão físico pressionado o
  canal não é tocado (o release de 40 ms do hook soltaria o clique do jogador).
- **[2026-11-07] Disputa de ângulo no `CUserCmd` é resolvida por tick, no hook.** Silêncio do
  anti-aim com rage ligado era gate (`aimLocked`), não conflito inevitável. Agora: o Silent Aim toma só
  o tick em que há disparo; rage **sem** Silent Aim com comando de mira fresco mantém o AA fora (aí a
  câmera é a mira — aplicar offset atiraria de costas); todo o resto é do anti-aim. A devolução do AA
  roda antes de qualquer escrita de mira, para não rotacionar o ângulo recém-escrito.
- **[2026-11-07] A prova do anti-aim passa a ser o `CUserCmd`, não `SetViewAngles`.** `applied=1` mede o
  estado vivo lido de volta a 0.01° — e o vivo é devolvido ao real no mesmo frame, então não diz nada sobre
  o que o servidor recebe. Enquanto o probe empírico não travar o offset de ângulos do comando (≥8
  coincidências, yaw variado, **candidato único**, fail-closed), o caminho novo não roda e o legado continua
  igual. Se `cmdOff` ficar `-1` depois de muitas rodadas, a conclusão é "o campo não está nos primeiros
  512 B" — e aí o trabalho é engenharia reversa, não gate.
- **[2026-11-07] Causa falseada se corrige na vista, não se apaga.** A "Causa 1" da T054 (portão
  `!rageSilentAim` travando o aimbot legítimo) era suportada por `aimWriteOk=325` "congelado"; o recorte
  correto da sessão mostrou 3.459 escritas com `silentApplied=0`, ou seja o portão estava aberto. O código
  continua corrigido (o defeito é real em outra configuração), mas o Estado/Reservas/VALIDACAO passam a
  dizer que **não foi isso que o usuário viu**. Recorte de sessão por número de linha do `[START]` é
  obrigatório antes de citar contagem.
- **[2026-11-07] Handoff de feature é briefing auto-suficiente, não mensagem de conversa.** C4 timer, aviso
  de plant e scope visual saíram do meu ciclo para o Antigravity por
  `30_Pacotes/Antigravity_T055_C4_Plant_Scope.md`: ordem de entrada, arquivos e linhas, medição de cada item,
  base binária vigente com aviso para re-ler os arquivos da T054 antes de editar, e as regras da casa.
- **[2026-11-07] T055 — `dwGlobalVars` gerado vs `gpGlobals` real no Build 14194 (`0x6AEBC4DA`).** O fallback
  `dwGlobalVars = 0x22320E8` de `offsets_generated.h` aponta no Build 14194 para uma vtable em `.rdata`
  (`client + 0x1AD8DA8`) onde `+0x30` vale `0.0f`, rejeitando 100% das bombas no portão `|curtime - simTime| > 5s`.
  Sem tocar em `offsets_generated.h`, `TacticalHUD.h` lê `curtime` do `gpGlobals` real do Build 14194
  (`client + 0x2235108`, provado em `0x75E860`/`0xD17B92` com 2.321 xrefs) quando `BuildMatches(client)` é
  verdadeiro, mantendo `dwGlobalVars` como fallback para os testes sintéticos.
- **[2026-11-07] T055 — Aviso de plant é card em tela (`DrawBombPlantedNotice`); escrita em chat exige decisão.**
  Não há mecanismo de escrita no painel de chat do CS2 no repositório. O aviso de plant existente é
  `DrawBombPlantedNotice` (`UITacticalHud.inl:51-86`, `bombPlantedNotify`, default `true`), que falhava pelo
  mesmo `BombLocator::Read` do timer da C4. Escrever no chat da partida fica fora de escopo até ordem explícita.
- **[2026-11-07] T055 — `cl_ironsight_filter_alpha` (`0x24DBE50`) governa AUG/SG553 (`C_IronSightController`), não a borda preta de AWP/SSG08.**
  A desmontagem de `client.dll` `0x6AEBC4DA` confirma que `0x24DBE50` (registrado em `0xA9830`/`0xA98B2`) tem seu
  float em `[*(0x24DBE50+8)+0x58]` amostrado diretamente pelo renderizador em `0x833D23..0x833D2A` dentro de
  `C_IronSightController::RenderScopeEffect` (`0x833A10..0x834E7C`, filtrado por `IsInIronSight` `0x831710`) para
  o alpha de `scope_filter.vmat`. Já a borda preta e retícula dos snipers (**AWP, SSG 08, SCAR-20, G3SG1**) vêm
  do painel Panorama `CCSGO_HudScope` (`HudScope`) e do material de pós-processamento
  `materials/effects/legacy_sniper_scope.vmat` (`0x11D0940` → `0x25FC608`), governados por `m_bIsScoped` (`+0x2848`).
  Não desenhamos barras pretas falsas; a negativa arquitetural fica registrada com os RVAs exatos.
- **[2026-11-07] Nenhuma entrega termina sem seção no `00_Gestao/Estado.md`.** A T052 mudou 6 arquivos
  de código, buildou, deployou e foi injetada sem existir no Estado, e o cabeçalho acabou afirmando
  build-em-memória e hash errados ao mesmo tempo.
- Fake lag/choke, desync, body yaw/manual yaw, silent aim, backtrack, prefire, dano de penetração e
  resolver de inimigo permanecem **análise-only** até ordem explícita nomeando a feature.
- Notas de tarefa só objetivo/aceitação.
- Um executor técnico e um senior escritor por vez; reserva não cria bloqueio real.
- Preservar modelos e permissões; leitor não contorna restrição por shell/MCP.
- Evidências congeladas e bibliotecas não são reescritas por limpeza documental.
- Config/IDs/cache/ownership preservados; não reativar skinchanger legado.
- Histórico integral em C:/Codex2/Hazzure_Docs_Backup_20261029, apenas sob demanda.
- Próxima liderança discutida: Antigravity; não foi iniciada automaticamente.
- **[2026-11-08] Revogadas as cinco linhas vermelhas de implementação** (ordem explícita do usuário:
  "Confirma, revoga as cinco"). pSilent, desync/body yaw/manual yaw/fake lag/choke, resolver de
  anti-aim inimigo/prefire/backtrack e dano de penetração no autowall deixam de ser "análise-only"
  ou "rejeitado" e passam a ser features em potencial, começando por pSilent + desync sobre o
  alicerce do comando. A matriz `20_Tarefas/MATRIZ_H.md` e a fila precisam refletir isso (os ⛔
  viram lacunas reais). A restrição de operação (só o usuário abre/injeta o CS2) não mudou.
- **[2026-11-08] T070 — o probe de 512 B do CUserCmd está retirado como instrumento de entrega.**
  A RE fechou que os ângulos NÃO são float fixo nos primeiros 512 B do comando; vivem no protobuf
  sub-tick (`CSGOUserCmdPB.m_input_history_[]` → `CSGOInputHistoryEntryPB.m_view_angles` =
  `CMsgQAngle` com has_bits `+0x10`, x/y/z `+0x18/+0x1C/+0x20`, confirmado na desmontagem
  `0xB7C711..0xB7C763`). O `cmdOff=-1 cmdHits=13` era o comportamento correto do fail-closed
  (offfset variável por entrada), não um gate a abrir. A prova de entrega do ângulo ao servidor
  passa a ser o log de escrita do `StampSubtickViewAngles` + validação em partida da T069.
