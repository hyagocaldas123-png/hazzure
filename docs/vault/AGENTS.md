# Regras do vault

Aplicar C:/Codex2/gpt/AGENTS.md.
Documentação/administração somente aqui; código final pertence ao senior no workspace reservado.

## Entrada (contrato desde 2026-11-07)

Este vault é a fonte única de estado. A ordem curta é [[Inicio]] → [[00_Gestao/Estado]] →
[[00_Gestao/VALIDACAO]] → [[20_Tarefas/INDICE]] → a tarefa relevante → [[00_Gestao/Reservas]]
antes de editar.

- `ESTADO_ATUAL.md` (vault e repo) são ponteiros; não escrever estado neles.
- Inventário derivado do código vive em `30_Inventario/` e é gerado por
  `python C:/Codex2/gpt/tools/vault_sync.py` (nunca à mão). `--check` é a verificação inteira:
  drift do inventário + integridade do vault (`tools/vault_lint.py`: links, encoding, seções
  duplicadas). exit 0 = os dois ok.
- Operação (build, deploy, rollback, log, IDA mudo, crash) em [[70_Ferramentas/FERRAMENTAS]].
- `10_Agentes/`, `30_Pacotes/`, `40_Checkpoints/`, `50_Publicacoes/` estão marcados SUPERSEDIDOS:
  não indexar para cada pergunta.
- Não carregar histórico, perfis ou referências inteiros.
