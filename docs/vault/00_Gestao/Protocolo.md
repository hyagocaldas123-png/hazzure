# Protocolo

> Atualizado 2026-11-07. O passo 1 mudou: a porta de entrada é `Inicio.md` e a fonte de estado é
> `00_Gestao/Estado.md` (não mais `ESTADO_ATUAL.md`, que virou ponteiro).

1. Ler `Inicio.md` (ordem de leitura mínima) → `00_Gestao/Estado.md` (build em disco, build em
   memória, pendências, riscos, regras) → a tarefa relevante; conferir reserva.
2. Reservar arquivos para um executor/senior; preservar colaboradores.
3. Aplicar a menor mudança coerente; testar somente o afetado ou dúvida nova.
4. Entregar caminhos, comandos/exit codes, evidências/hash e limitações; build não é runtime.
5. Atualizar tarefa/estado e liberar reserva. Histórico vai para arquivo, não se acumula nas entradas.
Não há despacho automático ou bloqueio de arquivos pelo Obsidian. Sem controle do jogo enquanto a restrição de He vigorar.
