# CLAUDE.md — Hazzure Project Instructions

## 1. Entrada e Fonte Única de Verdade
- **Fonte canônica**: Vault Obsidian em `C:/Codex2/Hazzure_Projeto/`.
  - Sequência obrigatória: `Inicio.md` → `00_Gestao/Estado.md` → `00_Gestao/VALIDACAO.md` → `20_Tarefas/INDICE.md`.
  - Reserve em `00_Gestao/Reservas.md` antes de editar.
- **Inventário**: `30_Inventario/` gerado por `python tools/vault_sync.py` (nunca à mão). `--check` valida integridade.

## 2. Token-Efficient Engineering (Regras Ativas)
- **Seja direto e estritamente técnico**: Sem preâmbulos, resumos óbvios ou floreios. Apresente primeiro a causa raiz, depois a solução/diff.
- **Leitura cirúrgica**: Identifique arquivos e símbolos exatos via grep antes de ler. Nunca varra pastas inteiras para problemas localizados.
- **Diffs pequenos**: Prefira modificações localizadas em vez de reescrever arquivos completos.
- **Sem abstrações sem justificativa**: Preserve a arquitetura existente; não crie wrappers ou classes extras desnecessárias.
- **Não altere contratos**: Verifique todos os consumidores antes de alterar structs compartilhadas, offsets ou assinaturas de hooks.
- **Honestidade técnica**: Compilação e self-tests não comprovam runtime em jogo. Relate caminhos, comandos, exit codes e logs reais.

## 3. Skills Registradas do Projeto (`.claude/skills/`)
1. `token-efficient-engineering`: Minimização de tokens, concisão e foco técnico (ativa globalmente).
2. `cpp17-code-reviewer`: Análise de memory safety, UB, lifecycles de ponteiros e otimizações em C++17.
3. `windows-api-dll-lifecycle`: Tratamento de handles, threads, hooks MinHook/Kiero e segurança no DllMain.
4. `ipc-shared-memory`: Auditoria de layouts de memória compartilhada em `Compartilhado/ipc.h`, alinhamento e barreiras atômicas.
5. `software-architecture`: Separação de camadas (GUI vs CreateMove/Hooks vs Física), prevenindo acoplamento e hacks.

## 4. Regras Operacionais Invioláveis
- **Processo do CS2**: Somente o operador inicia o jogo e injeta a DLL. Nunca encerre, reinicie ou injete via scripts.
- **Build**: Uma compilação por ciclo (`cd HazzureInternal && build.cmd`). Deploy sincronizado com rollback em `C:\HazzureKernel`.
