import datetime

doc_path = r'c:\Codex2\gpt\Documentacao_Hazzure\HAZZURE_DOCUMENTACAO_COMPLETA.md'

with open(doc_path, 'r', encoding='utf-8') as f:
    content = f.read()

new_section = r'''
<a id="entrega-20261024-1745"></a>

## Atualização de Produção — 24/10/2026 (17:45) — Overlay D3D11 Interno Completo, Correção Crítica de Input/Mouse e Sanagem de Falha de Memória

Registro canônico da revisão, correção e entrega do módulo interno de renderização DirectX 11 (`HazzureInternal`), correção de input com liberação do cursor e implementação das mecânicas ativas de jogo.

---

### 1. Diagnóstico do Crash e Sanagem de Falha de Memória (CDB / WinDbg Post-Mortem)

- **Arquivo de Despejo Analisado:** `C:\Users\tsgka\AppData\Local\CrashDumps\cs2.exe.24424.dmp` (gerado em 24/10/2026).
- **Causa Raiz da Falha:** `Access Violation c0000005` em `client.dll + 0x19B4886` (`client!ExtractModuleMetadata+0xd81b26`).
  - *Mecanismo do Erro:* Na tentativa de forçar a inicialização do D3D11 ignorando discrepâncias de build, a função `Start()` continuou executando hooks em RVAs antigas da Valve. A variável `arenaCtor` foi apontada para `client.dll + 0x19B4860`. No momento em que o callback de desenho de chams disparou `Prepare()` -> `MakeMaterial()`, essa rotina chamou `arenaCtor(arena, 0)`, que na Build 14192 correspondia a outra rotina interna não relacionada, tentando escrever no registrador inválido `[rdi]` (`0x0000025b7a0a5c20`).
- **Solução Definitiva de Isolamento:**
  1. Todos os hooks do motor (`scenesystem.dll`, `client.dll`) e o construtor KV3 (`arenaCtor`) foram estritamente condicionados à validação de módulo (`modulesMatched`).
  2. Caso o jogo receba atualização ou os hashes variem, `arenaCtor` é anulado (`nullptr`) e os chams entram em modo passthrough transparente via SEH (`__try / __except`), garantindo que o módulo D3D11 continue ativo e o jogo nunca sofra crash.

---

### 2. Arquitetura de Hooking do DirectX 11 (VMT Shadow Hook Puro)

- **Eliminação de Recursão e Stack Overflow (`0xC00000FD`):**
  - O uso inicial de `kiero::bind(8)` em conjunto com VMT Hook causava loop recursivo infinito, pois a VTable apontava para o trampolim do MinHook e este voltava para `hkPresent`.
  - **Arquitetura de Transição One-Shot:** No primeiro frame de execução (`hkPresent`), a biblioteca `kiero` é desvinculada imediatamente com `kiero::unbind(8)`, `MH_DisableHook` e `MH_RemoveHook`, restaurando o prólogo original limpo da `dxgi.dll`.
  - Em seguida, a instância ativa da `IDXGISwapChain` recebe a classe `VMTShadowHook`, que aloca uma VTable sombra na heap e sobrescreve o índice 8 com `hkPresent`. Os frames subsequentes executam 100% via VMT sem qualquer modificação no código executável de `dxgi.dll`.

---

### 3. Correção Crítica de Input do Mouse e WndProc no CS2

1. **Liberação de Confinamento do Cursor (`ClipCursor`):**
   - *Problema:* O CS2 aprisiona o cursor em um retângulo de 1x1 pixel no centro da tela via raw input. Ao abrir o menu (`INSERT`), o mouse ficava travado no centro sem conseguir clicar nas abas; ao fechar, o cursor desenhado permanecia visível sobre a mira.
   - *Solução:* 
     - No `hkPresent`, quando `g_bShowMenu == true`, é chamada a API nativa `ClipCursor(NULL)` para soltar o cursor do confinamento do jogo e forçado `io.MouseDrawCursor = true`. As coordenadas do cliente são atualizadas com `GetCursorPos` + `ScreenToClient(g_hWindow, &mousePt)` antes da chamada de `ImGui_ImplWin32_NewFrame()`.
     - Quando `g_bShowMenu == false`, `io.MouseDrawCursor = false` é garantido.
2. **Filtragem Condicional no `HookedWndProc`:**
   - As mensagens do Windows são entregues primeiro ao `ImGui_ImplWin32_WndProcHandler`.
   - Mensagens de mouse (`WM_LBUTTONDOWN`, `WM_MOUSEMOVE`, etc.) só são bloqueadas para o CS2 se `g_bShowMenu && io.WantCaptureMouse`.
   - Mensagens de teclado (`WM_KEYDOWN`, `WM_CHAR`, etc.) só são bloqueadas se `g_bShowMenu && io.WantCaptureKeyboard`.
   - Mensagens de sistema operacional (`WM_PAINT`, `WM_NCCALCSIZE`, redimensionamento e ativação) nunca são interceptadas, eliminando deadlocks e congelamentos de janela.

---

### 4. Isolamento Completo dos HUDs (`kHudFlags` e Proteção Nula)

- **Problema Anterior:** As janelas flutuantes de HUD (C4, Espectadores e Velocímetro) estavam sobrepostas, sem fundo e capturavam cliques do mouse destinados ao jogo ou ao menu.
- **Correção Aplicada:**
  - Definida constante canônica de flags:
    ```cpp
    static const ImGuiWindowFlags kHudFlags = 
        ImGuiWindowFlags_NoInputs | 
        ImGuiWindowFlags_NoBackground | 
        ImGuiWindowFlags_NoDecoration | 
        ImGuiWindowFlags_AlwaysAutoResize;
    ```
  - Posicionamento em regiões independentes e sem colisão na tela:
    - **Espectadores:** Canto superior direito (`screenW - 220, 25`).
    - **Temporizador C4:** Topo centralizado (`screenW * 0.5 - 100, 25`).
    - **Velocímetro:** Centro inferior (`screenW * 0.5 - 85, screenH - 120`).
  - **Validação de Ponteiros:** Nenhum HUD é renderizado se os ponteiros de memória (`localPawn`, `client`) forem nulos ou ilegíveis.

---

### 5. Resolução da Varredura de Entidades CS2 (Chunk Traversal)

- **Correção do ESP Invisível:** A leitura legada por handle de controle (`controller + 0x7E4`) não resolvia mais no Source 2 (Build 14192).
- **Implementação Verificada:** Adotada a iteração por blocos idêntica ao utilitário externo:
  - `dwEntityList + 0x10` -> Array de 64 chunks -> Iteração de até 512 slots (stride `0x70`) -> Leitura da `identity` -> Ponteiro do `pawn` (`identity + 0x0`).
  - Deduplicação por array de pawns já processados.
  - Leitura segura de vida (`m_iHealth = 0x344`), equipe (`m_iTeamNum = 0x3C8`), posição (`m_vecAbsOrigin = 0x1264`) e nós de ossos (`pGameSceneNode + 0x1F0 + 0x80`).

---

### 6. Mecânicas e Recursos Implementados (Partes 1, 2, 3 e 4)

| Módulo | Recurso | Detalhes Técnicos |
|---|---|---|
| **Parte 1: Balística** | **Off-Screen Arrows** | Setas triangulares na borda da tela projetadas trigonometricamente apontando inimigos fora do FOV, com medidor de distância em metros (`18m`, `32m`). |
| | **Trajetória de Granadas** | Simulação física de 35 passos discretos considerando velocidade de arremesso (1050 u/s), gravidade (800 u/s²) e arco parabólico com marcador visual de aterrissagem. |
| | **Spread & Recoil Circle** | Círculo tático dinâmico no centro da tela indicando a dispersão da arma ativa. |
| **Parte 2: D3D11 / Estética** | **Fundo Vinheta Escurecido** | Efeito translúcido suave aplicado em tela cheia ao abrir o menu para contraste e foco visual no tema Obsidian P&B com partículas de neve. |
| | **Abas Globais Corrigidas** | Variável `g_currentTab` promovida para o escopo global, permitindo alternância instantânea entre as 7 abas do painel. |
| **Parte 3: Áudio DSP** | **Sintetizador Procedural em Memória** | Geração dinâmica de ondas PCM em 44.1 kHz 16-bit mono sem dependência de arquivos `.wav` no disco. Sons executados assincronamente via `PlaySoundA` (`SND_MEMORY \| SND_ASYNC`):<br>- *Headshot:* Tom agudo harmônico e metálico (1950 Hz -> 1350 Hz).<br>- *Bodyshot:* Impacto acústico encorpado (480 Hz -> 220 Hz). |
| | **Beep Dinâmico da C4** | Pulso sonoro tático que acelera gradualmente de 1 bip/s até bips contínuos conforme a contagem regressiva se aproxima de zero. |
| | **Botões de Teste** | Teste imediato dos efeitos acústicos integrado na aba de Áudio. |
| **Parte 4: Movimentação** | **Speedometer & Gráfico** | Badge flutuante (`SPEED: XXX u/s`) com gráfico histórico dos últimos 60 quadros (verde para ganho de velocidade em strafe/bhop, vermelho para frenagem). |
| | **BunnyHop Funcional** | Verificação direta de `m_fFlags & 1` (`FL_ONGROUND`) no pawn local e emissão de ciclos de salto enquanto o espaço for mantido. |
| | **Aimbot Suave** | Rastreamento dinâmico em direção ao osso de referência do alvo mais próximo do centro dentro do FOV, com interpolação via `mouse_event` dividida pelo fator `Smooth`. |
| | **Auto-Pistol & Jumpthrow** | Disparo semi-automático contínuo ao segurar botão esquerdo e tecla dedicada (`X`) para salto e soltura sincronizada de granada. |

*(A Parte 5 de telemetria/exportação em base64 foi explicitamente excluída por solicitação do usuário).*

---

### 7. Validação e Binários Vigentes

- **Compilação da DLL (`HazzureInternal.vcxproj`):** 0 erros, MSVC x64 C++17.
- **Pipeline Completo (`build-all.cmd`):** 100% dos testes unitários e de integração aprovados (autoteste de skin, IPC, matemática de gradiente, trampolins x64 e empacotamento).
- **Binários Atualizados e Prontos:**
  - DLL do Injetor Kernel: `C:\HazzureKernel\Hazzure.Internal.dll`
  - Release Oficial: `C:\Codex2\gpt\release\modules\Hazzure.Internal.dll`
- **Controle de Interface:** Tecla padrão `INSERT` (reconfigurável via hotkey binder na aba Perfis).
'''

with open(doc_path, 'a', encoding='utf-8') as f:
    f.write(new_section)

print("HAZZURE_DOCUMENTACAO_COMPLETA.md updated successfully with the 24/10/2026 milestone!")
