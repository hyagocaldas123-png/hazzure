# Briefing Antigravity — T055: timer da C4, aviso de plant e scope visual (2026-11-07, ~04:40)

Briefing autossuficiente: quem nunca viu este projeto deve conseguir executar só com este arquivo
mais a entrada do vault. Nada aqui foi validado em partida por nós — os números citados são medidos
em `C:\HazzureKernel\debug_log.txt`.

## Entrada e estado vigente
- Ordem de leitura: `C:/Codex2/Hazzure_Projeto/Inicio.md` → `00_Gestao/Estado.md` →
  `00_Gestao/VALIDACAO.md` → `00_Gestao/Reservas.md` (reserva obrigatória antes de editar) →
  `70_Ferramentas/FERRAMENTAS.md` (build, deploy, leitura de log).
- Código: `C:\Codex2\gpt\HazzureInternal\src` (unity build C++17/MSVC x64; `build.cmd` gera
  `build\Hazzure.Internal.dll`). Deploy em `C:\HazzureKernel\Hazzure.Internal.dll` + `modules\`.
- Build-alvo: `client.dll` TimeDateStamp `0x6AEBC4DA`, CS2 **14194**. Endereços são RVA sobre
  `0x180000000`.
- **Base atual: t054**, SHA-256 `c5d5913b9a0527d64924ca34c83b9a04a8d3a4522e7056e1856c51c352f98334`,
  2.966.528 B, deployado 04:37:20. A t054 mexeu em `InternalStartup.inl`, `InputView.h`,
  `GameplayRuntime.h`, `GSLayout.inl` (anti-aim no `CUserCmd` + diagnósticos) — **releia esses
  arquivos antes de editar**, não sobrescreva.
- Quem abre o CS2 e injeta é o usuário. Não feche/reinicie o jogo, não reinjete, não mate processo.

## Item 1 — Timer da C4 não aparece
Arquivos: `src/TacticalHUD.h:62-83` (`BombLocator::Read`), `src/TacticalHUD.h:20-61`
(`ReadBombEntity`), `src/UITacticalHud.inl:6` (`DrawBombTimerHUD`), log em `src/UIFrame.inl`.

Medido às **04:47** na sessão em memória (t053: `grep -an 'Start() finalizado' debug_log.txt | tail -1`
→ linha 1.020.647 → `tail -n +1020647`; **não** recorte por timestamp, as linhas não têm data):
**905** linhas `[C4]`, das quais **735** `dwPlantedC4=0x0` e **170 com ponteiro NÃO nulo**, em ~8 janelas
contíguas de 20–100 s (04:22:52→04:46:43) e com **9 ponteiros distintos** (`0x4620E1B4800`,
`0x461B9049800`, `0x4621189C800`, `0x460EB471000`, …) — comportamento de objeto que nasce e morre por
rodada. No arquivo inteiro: 5.585 linhas `[C4]`, **todas** na forma `timer sem bomba` — nunca existiu outra.
Ou seja: **o global `dwPlantedC4` (0x24D0B70, corrigido na T048) não é o
ponto de falha** — alguma validação depois dele rejeita. Candidatas, na ordem em que o código as
executa:
1. `Handle(list, global)` (linha 69) trata `global` como **handle** de entidade; neste build o
   global é ponteiro para a entidade `C_C4`/`C_PlantedC4` → o teste de handle falha e o caminho
   "direto" morre.
2. Deref indireto (linha 70) — só roda se `!bomb`, e lê `*global` como se fosse ponteiro de
   handle; confere com o layout real.
3. Varredura por nome (linhas 72-74) compara `DesignerName(candidate,"planted_c4")`; o nome de
   classe no CS2 é `C_PlantedC4`. Verifique o que `DesignerName` normaliza antes de assumes match.
4. Portão de tempo (linhas 79-80): `|globalVars+0x30 − localPawn+0x3b8| > 5` descarta a bomba. Se
   `+0x3b8` não for um tick/tempo compatível com `+0x30`, a bomba é rejeitada **sempre**, mesmo com
   as etapas 1-3 acertando.

Tarefa: instrumentar o `[C4]` com a etapa que falhou — `loc=global|indirect|cached|scan|none`,
`handleOk=`, `simDelta=`, `readBomb=` — e **só depois** corrigir a que os números apontarem. Não
mude comportamento sem a medição; log novo é parte da entrega.

Aceitação: uma linha de log por etapa durante uma rodada com bomba plantada + o timer na tela.
Entregar caminho da DLL, SHA-256, trecho do log e o comando de build com exit code.

## Item 2 — Mensagem de plant
- Não existe **escrita no chat** no repositório: `grep` por `chat|Chat|PrintText|CenterText|Mensagem`
  em `src/*.h`/`src/*.inl` não retorna nenhum mecanismo de chat. O que existe é o aviso em tela
  `DrawBombPlantedNotice` (`src/UITacticalHud.inl:51-70`), ligado por `bombPlantedNotify`
  (default `true`; a chave está **ABSENT** no config persistido, logo vive do default) e gated por
  `g_bombLocator.Read(...)` (`UITacticalHud.inl:75`).
- Consequência: o aviso em tela não aparece **pela mesma causa do Item 1**. Se o pedido do usuário
  for literalmente "mensagem no chat", isso é **feature nova** (escrever no painel de chat do CS2) e
  exige decisão registrada antes de codar — não implementar por conta própria.

Aceitação: aviso em tela visível num plant real, ou a medição do Item 1 mostrando por que o locator
falha; para chat, primeiro um registro em `60_Decisoes/Decisoes.md`.

## Item 3 — Scope visual (borda preta do AWP/SSG)
Arquivos: `src/ScopeOverlay.h` (RVA `0x24dbe50`, `valueOffset 0x58`, `Slot()`, `Update()`),
chamada em `src/UIFrame.inl:223`, menu em `src/cards/CardVisuals.inl:70-75`
(`noScopeOverlay`, `scopeFilterAlpha`).

Medido: a última linha `[SCOPE]` do log é **02:54:45**; nas sessões de 03:21 e 04:16 há **0** linhas
`[SCOPE]` → o toggle estava desligado, então **o recurso não rodou na sessão que o usuário julga**.
Na sessão antiga o write aconteceu e o read-back bateu (`medido=0.250 entregue=0.250 alvo=0.250
ligado=1 writes=2 slot=0x59155A39EC8`) — o valor gravado é o que `GetValue()` devolve, mas a borda
preta não mudou e `writes` parou em 2 porque `current != want` já estava satisfeito.

Suspeitas a testar (nesta ordem):
1. O slider estava em **0.25, não 0**: com 0.25 ainda filtrando, a conclusão "não funciona" pode ser
   apenas alvo errado. Teste com `0.00` e com o toggle ligado, e registre o resultado.
2. Confirmar que `0x24dbe50` é mesmo o `ConVarF32` de `cl_ironsight_filter_alpha` e que o float em
   `[global+8]+0x58` é a cópia que o **renderizador** amostra, não só o cache do `GetValue()`. O
   comentário de `ScopeOverlay.h:8-11` mistura o RVA de registro (`0xA98B2`) com o RVA usado
   (`0x24dbe50`) — vale resolver com IDA/capstone no `client.dll 0x6AEBC4DA`.
3. Se a borda não vier desse convar, dizer isso por escrito e apontar o caminho real (material de
   pós-processamento do scope). **Não** desenhar barras pretas falsas por conta própria.

Aceitação: zoom de AWP/SSG com a borda visivelmente diferente, **ou** a negativa documentada com a
medição que a sustenta.

## Regras da casa que valem aqui
- Reserva em `00_Gestao/Reservas.md` antes de editar; verificar se outro escritor mexeu no arquivo.
- Um build por ciclo, log em `HazzureInternal/build_tNNN.log`, deploy com rollback
  `*.bak-<data>-tNNN`; nunca patch/chave de licença do IDA.
- Número em log é lido de volta do estado, não literal — decisão registrada na T053.
- Fora de escopo: pSilent (rejeitado), desync/body yaw/manual yaw/fake lag/resolver (análise-only),
  mexer em offsets gerados, reinjetar ou fechar o CS2.
- Compilado ≠ validado: registrar em `00_Gestao/VALIDACAO.md` o que só passou no build.
