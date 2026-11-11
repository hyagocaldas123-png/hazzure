# Hazzure — porta de entrada

Cheat internal para CS2 (Source 2): DLL C++17 mapeada manualmente no `client.dll`, UI ImGui,
D3D11, MinHook. Referência técnica e estética: **MemeSense**. Documento mestre de especificação:
[[DIRETRIZ_CHEAT_CS2]].

Este vault é a **fonte única de estado** do projeto desde 2026-11-07. Um chat novo deve sair
funcional lendo só esta página e o que ela aponta — sem histórico de conversa.

## Leitura mínima (nesta ordem)

1. [[00_Gestao/Estado]] — **a** fonte de estado. Cabeçalho fixo no topo responde as quatro
   perguntas que abrem qualquer sessão: qual DLL está em disco, qual está em memória, qual é o
   alvo de engenharia (`client.dll` TimeDateStamp `0x6AEBC4DA`, SizeOfImage `0x29A3000`, CS2
   build 14194), e quais são as pendências / riscos / regras. Debaixo do cabeçalho, uma seção
   `## T0nn` por tarefa, da mais nova para a mais antiga.
2. [[00_Gestao/VALIDACAO]] — matriz do que **compila** vs o que foi **validado em partida**.
   Leia antes de afirmar que qualquer feature funciona.
3. [[20_Tarefas/INDICE]] — T001..T050 em uma linha cada (status, hash do binário, link).
   [[20_Tarefas/Historico-T021-T050]] condensa as tarefas que não têm arquivo próprio.
4. [[00_Gestao/Fila]] — ordem de trabalho de hoje. [[00_Gestao/Protocolo]] — como executar uma
   tarefa. [[00_Gestao/Reservas]] — reservar antes de editar (obrigatório).
5. Se a pergunta é "o que existe no menu / qual chave controla quê / quais offsets":
   `30_Inventario/` — [[30_Inventario/MENU]], [[30_Inventario/CONFIG_KEYS]],
   [[30_Inventario/OFFSETS]], [[30_Inventario/ARTEFATOS]]. **Gerado por script, não editar à mão**
   (`python tools/vault_sync.py`). Para conferir o vault como um todo — drift do inventário **e**
   integridade interna (links, encoding, seções duplicadas) — um comando só:
   `python tools/vault_sync.py --check` (exit 0 = os dois ok; hoje está 0).
6. Se a pergunta é "o que falta no nosso cheat / qual é o estado de cada feature":
   [[20_Tarefas/MATRIZ_H]] — matriz linha por linha (✅/🟡/❌; o ⛔ das linhas vermelhas foi
   aposentado em 2026-11-08, ver §5) com a evidência de cada avaliação
   (`MENU.md`, `CONFIG_KEYS.md`, grep).
   Para o comparativo por aba de menu contra o mercado, [[20_Tarefas/MAPA_LACUNAS_MERCADO]].
7. Se a pergunta é operacional (buildar, deployar, rollback, ler log, descobrir o que está em
   memória, IDA mudo, forense de crash): [[70_Ferramentas/FERRAMENTAS]].

## O que NÃO precisa ler para trabalhar

- `ESTADO_ATUAL.md` (raiz do vault e do repo): viraram ponteiros; o conteúdo está em
  `00_Gestao/Estado.md`. Os originais pré-mescla estão em `ESTADO_ATUAL.md.bak-20261107-vault-merge`.
- `10_Agentes/`, `30_Pacotes/`, `40_Checkpoints/`, `50_Publicacoes/`: coordenação de 29–30/10
  sobre T002–T011, marcada **SUPERSEDIDA**. Não descreve a entrega atual.
- `90_Anexos/`, `80_Modelos/`, `ANALISE_REFERENCIA_*.md`: referência, só sob dúvida concreta.
- Histórico integral arquivado fora do vault: `C:/Codex2/Hazzure_Docs_Backup_20261029`.
- O repo de código inteiro: os números derivados do código já estão em `30_Inventario/`.

## Regras que não mudam (repetidas do Estado para o caso de você só ler esta página)

- **Só o usuário abre o CS2 e injeta.** Manual map não tem hot reload: DLL no disco ≠ DLL em
  memória; depois de redeploy é preciso reinjetar para o código novo rodar.
- **Build, self-test e mock não provam runtime.** Relato traz caminho, SHA-256, exit code e linha
  de log — ou declara que não há evidência.
- Feature de gameplay complexa (silent aim, backtrack, dano de penetração, prefire) só com ordem
  explícita nomeando a feature. **Revogado em 2026-11-08** pelo usuário ("Confirma, revoga as cinco"):
  fake lag/choke, desync, body/manual yaw, pSilent, resolver/prefire/backtrack e dano de penetração
  voltaram a ser backlog implementável — a T071 (desync no sub-tick) é a primeira.
- Reserva em [[00_Gestao/Reservas]] antes de editar; um build por ciclo; um escritor de código final.
- O repo `C:\Codex2\gpt` **não é git**: rollback é arquivo `.bak-AAAAMMDD-tNNN` /
  `C:\HazzureKernel\backup\<yyyymmdd-hhmm>\`, nunca `git checkout`.
- Alvo de engenharia é o build 14194 (`client.dll` `0x6AEBC4DA`). Todo endereço aqui é RVA sobre a
  base `0x180000000`; em jogo a base é a do módulo carregado.
- Assets: catálogo de skins/miniaturas em `C:\HazzureKernel\assets\skins` — **2.148 PNG** medidos
  em 2026-11-07. "Instalado" não significa "ativo em jogo": ver VALIDACAO.

## Respostas rápidas

| Pergunta | Onde |
|---|---|
| Qual DLL está em memória agora? | Estado › "O que está em memória AGORA" + método em FERRAMENTAS §4 |
| Como buildar e deployar? | FERRAMENTAS §1 e §3 |
| Quem injeta? | O usuário (sempre) |
| O que funciona em partida? | VALIDACAO |
| O que tem no menu? | `30_Inventario/MENU.md` (354 widgets, atribuídos por aba real) |
| Qual chave de config controla isto? | `30_Inventario/CONFIG_KEYS.md` + coluna "campo" do MENU |
| Offset / RVA de algo? | `30_Inventario/OFFSETS.md` |
| Rollback de uma tarefa? | `30_Inventario/ARTEFATOS.md` e o bloco da tarefa em Reservas |
| Por que um log não aparece? | FERRAMENTAS §5 (tags gated) e §1 (build em memória) |
| IDA/Ghidra mudos — e agora? | FERRAMENTAS §7 tabela de fallbacks offline |
| O vault está íntegro (links, codificação, duplicatas)? | `python tools/vault_sync.py --check`, que já roda o `tools/vault_lint.py` |
