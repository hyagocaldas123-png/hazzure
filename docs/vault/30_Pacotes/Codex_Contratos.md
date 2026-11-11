# Pacote de análise — Codex

> **SUPERSEDIDO em 2026-11-07 — ver `00_Gestao/Estado.md`.** Documento de coordenação de
> 29–30/10/2026: fala de T002/T008/T010/T011 e de artefatos que não são mais a entrega vigente.
> Nada foi apagado: o texto abaixo é histórico. Para o estado atual leia `00_Gestao/Estado.md`,
> para a fila `00_Gestao/Fila.md`, para o que existe no menu `30_Inventario/MENU.md`, para como
> buildar/deployar/ler log `70_Ferramentas/FERRAMENTAS.md`.


Papel: `schema-offset-specialist`, leitura/busca apenas. Modelo/esforço existentes do projeto; no máximo um subagente. Não editar, executar o jogo, fazer chamada nativa ou ampliar permissão.

Objetivo: [[20_Tarefas/T006]]. Cliente analisado: SHA-256 `08a741030c1e569d8b854c5886a87fcafc21d0d0e2f05dfd380e8a351171501d`. Base de contratos: `work/opencode-integration-20261028/analysis/NATIVE_CONTRACT.md`. Investigação atual: `work/gui-workflow-20261029/analysis`.

MCP encontrou um match da assinatura antiga: offset de arquivo `0x83b290`, correspondente ao RVA `0x83be90`, wrapper EquipItemInLoadout. Foram salvos corpos e alvos: setter local `0x8445c0`, getter de view `0x83dc60`, leitura de ID `0x83dc20` e notify `0x117e980`. Essas observações não validam equipagem. O setter chama `0xf92b00`; o notify chega a `0x117e500`, cujo papel/fila precisa ser demonstrado.

Cuidado: entradas de unwind fragmentam as funções; o primeiro extent de `.pdata` não cobre necessariamente o corpo completo. Distinguir side effects locais, callbacks e envio/fila do coordenador do jogo antes de propor implementação.

Entrega delimitada com prova, ABI/ownership, limites e dúvidas. Senior só implementa após contrato e revisão correspondente. Mocks não provam o cliente real.
