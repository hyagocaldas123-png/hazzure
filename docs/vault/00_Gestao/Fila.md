# Fila atual

> Fonte do que está pendente é `00_Gestao/Estado.md` → seção **Pendências abertas**. Esta página é
> só a fila ordenada; se divergir do Estado, o Estado manda. Atualizada 2026-11-07.

## Ordem de trabalho (2026-11-07, 04:5x)

1. **Reinjetar o t058 (`401cae7e…`)** — só o usuário abre o CS2 e injeta; a sessão viva é o **t056** (`[START]` 06:28:09,
   linha 1.042.490). O que ler depois, na sessão nova (recorte por número de linha do `[START]`, não por timestamp):
   - **ligar o Anti-Aim** com alguém vivo, arma na mão e pitch em **+89**: na sessão do t056 isso não aconteceu
     (`[ANTIAIM]` = 0 linhas, `aa=desligado` em 235/235 `[GAMEPLAY]`, `cmdOff=-1`), então `cmdOff=-1` **não é medida**;
   - `[AIMTRIG] … cmdOff=0x…` travado e `[ANTIAIM] … cmdRead=` com o pitch do slider = o ângulo falso no comando;
     se continuar `-1` com AA ligado e probe alto, o campo de ângulos **não** está nos primeiros 512 B → próxima etapa é RE;
   - `[GAMEPLAY] aim=<motivo> aa=<estado>` — `aim=anti-aim` não pode mais aparecer;
   - `Safe Point` / `Extrapolação por Ping` / `Force baim` / `Trigger air-flash check` e o movimento novo
     (`slowwalk`/`fast stop`/`fake duck`, via `[MOVEMENT] … stamp=` e máscara de botões).
2. **Concluído na T058 (limpeza da aba Movement + remoção das vitrines)** — `espSmoothAnimations` conectado em `CombatRender.h:112`, as 10 chaves sem consumidor (`chamsRagdoll`, `chamsOnShot`, `chamsSky`, `worldFullbright`, `worldSunRotation`, `worldDofDisable`, `removeDecals`, `removeRagdolls`, `removeSky3D`, `removePostProcessing`) e o `autoBuyEnabled` removidos do menu/persistência, e a aba Movement enxugada das sobras do Bhop/Tech (`MENU.md`: 378 widgets). Evidência em `20_Tarefas/MATRIZ_H.md` §7.



3. **Briefing do Antigravity concluído** (T055/T055b/T056/T057): timer da C4 **validado em partida**, aviso de plant
   idem, scope com negativa arquitetural provada (`0x24DBE50` = AUG/SG 553; AWP/SSG vem de `HudScope` +
   `legacy_sniper_scope.vmat`) — a borda do AWP continua lá, o que foi feito é retícula por cima.
4. **Do t052/t050, o que continua sem partida**: `[STRAFE]` (0 ocorrências na sessão em memória),
   sobreviver à troca de mapa (t045/clima), luvas (t044). Thirdperson **funciona** (relato do usuário,
   07/11) — o que falta é um instrumento honesto, pois funcionou com `pawnPerguntado=0`.
4. **Agent changer**: `enabled=1` com `.vmdl` escolhido nesta sessão; falta o relato visual de que trocou.
5. **Tracker aberto**: #2 refinar chams/material override · #3 portão de teardown no clima.
6. **Inventário do vault**: re-gerar `30_Inventario/` com `python tools/vault_sync.py` depois de
   qualquer mudança de UI/offset/deploy; `--check` precisa sair 0.

## Reservado, sem código

- Modelo de **dano de penetração** (autowall hoje decide só alcance) — tarefa separada, exige ordem
  explícita.
- Handoff **MemeSense** (T040): reservado, nada implementado.
- Modelos de personagem/mãos custom ("anime"): o usuário enviará o modelo; por enquanto só
  entendimento. **Escrever modelo na entidade das mãos já foi rejeitado pelo motor** (t042/t043).

## Backlog que virou implementável (revogação de 2026-11-08)

Fake lag/choke, desync, body yaw/manual yaw, pSilent, silent aim, backtrack, prefire, resolver de
inimigo e dano de penetração. O usuário revogou as cinco linhas vermelhas ("Confirma, revoga as
cinco") — deixaram de ser "análise, nunca implementação". Status por item, medido no código:

- **Crash `0xC0000005` em `client.dll+0x7B0CBB`** — ✅ forense feita (T072): é o `Clear()` da entrada de histórico escrevendo em `NULL+0x18`; o nosso carimbo marcava o bit errado (`0x2` SHOOTPOSITION em vez de `0x1` VIEWANGLES). Corrigido no t072 (`f4f6fa67…`), ainda sem injeção — e honesto: nas duas sessões que caíram (t069 05:11, t071 08:02) o carimbo não rodou, então a queda delas continua sem causa atribuída.
- **Desync** — 🟡 1ª perna entregue na T071 (`rageDesync`, yaw do AA no sub-tick com fake limit,
  câmera intacta, input reprojetado). Falta body yaw/LBY, limite dinâmico por estado, fake lag.
- **Backtrack** — ❌ 0 ocorrências no código inteiro.
- **Fake lag/choke** — ❌ `Choke` = 0 ocorrências.
- **pSilent** — ❌.
- **Dano de penetração** — ❌ (a sonda de autowall decide alcance, não dano; ver a memória de
  dados de penetração do CS2 e `60_Decisoes/Decisoes.md`).

Aceite de qualquer perna nova continua sendo partida + linha lida de volta do estado, não build.

---

## Fila de 2026-10-30 — SUPERSEDIDA

> SUPERSEDIDO em 2026-11-06 — ver `00_Gestao/Estado.md`. Mantido abaixo como estava, sem apagar: os
> itens T008/T002 foram absorvidos pelo histórico (`20_Tarefas/Historico-T021-T050.md`) e o caminho
> de estado citado não é mais a fonte (era `C:/Codex2/gpt/ESTADO_ATUAL.md`, hoje um ponteiro).

T010/T011 concluídas na candidata; provas/estado em C:/Codex2/gpt/ESTADO_ATUAL.md.
1. Revisar/aplicar overlay preparado em destino quiescente, quando autorizado/compatível.
2. T008: teste real manual de inventário/equipagem/mapa por He.
3. T002: perfis e salvamento completos, ainda pausada.
Antes de editar: [[00_Gestao/Reservas]]. Um escritor por vez; sem despacho automático.
