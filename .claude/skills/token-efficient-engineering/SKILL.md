---
name: token-efficient-engineering
description: >
  Token-efficient engineering and concise coding for Hazzure (C++17). Minimizes token
  consumption, reads only necessary files, avoids unnecessary explanations, abstractions,
  and full rewrites. Active across all tasks.
license: MIT
---

# Token-Efficient Engineering (Hazzure C++17)

Apply this skill across all interactions to preserve context window and maximize technical precision.

## Directives
- **Direto e estritamente técnico**: Sem introduções vazias, recapitulativos óbvios ou floreios educados. Apresente primeiro a causa raiz/problema, depois a solução/diff.
- **Sem explicações de conceitos básicos**: O operador já domina C++, Windows internals e engenharia reversa. Vá direto ao ponto específico do projeto.
- **Leitura cirúrgica de arquivos**: Antes de ler, identifique quais arquivos e linhas são estritamente necessários via grep/busca de símbolos. Nunca varra o projeto inteiro para resolver um problema localizado.
- **Diffs pequenos**: Prefira edições localizadas e cirúrgicas. Nunca reescreva arquivos inteiros nem substitua blocos funcionais existentes sem necessidade.
- **Sem redundância de documentação**: Não repita parágrafos inteiros já presentes em `Estado.md` ou `FERRAMENTAS.md`. Apenas cite referências ou o dado alterado.
- **Sem abstrações prematuras**: Não crie wrappers, templates extras, interfaces abstratas ou opções "para o futuro" que não foram pedidas.
- **Integridade de contratos**: Não altere layouts de structs compartilhadas, assinaturas de hooks ou offsets sem verificar todos os pontos consumidores.
- **Relato de alterações**: Informe objetivamente quais arquivos foram inspecionados e modificados com seus respectivos caminhos.
- **Fidelidade técnica**: Não invente testes, saídas ou logs. Compilação com exit 0 é pré-requisito, mas só a partida real valida runtime.
- **Padrão de código**: Preserve fielmente as convenções existentes (C++17, snake_case para funções/variáveis locais, PascalCase para structs/classes, unity includes em `HazzureInternal`).
