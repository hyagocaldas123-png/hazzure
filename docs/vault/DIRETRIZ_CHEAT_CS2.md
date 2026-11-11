# DIRETRIZ E ESPECIFICAÇÃO DO PROJETO HAZZURE — CHEAT INTERNAL PARA CS2 (SOURCE 2)

> **PROPÓSITO EXPLÍCITO**: O Hazzure é um **Cheat Internal de alta performance para Counter-Strike 2 (Source 2)**.
> Não é ferramenta acadêmica, não é projeto amador e não deve operar com travas cosméticas ("safety locks", simulações teóricas sem efeito em jogo ou abas duplicadas). O padrão de referência técnico e estético é o **MemeSense** / cheats tier-1 do mercado.

---

## 1. VISÃO E OBJETIVOS DO PRODUTO

O software opera injetado nativamente no processo do CS2 (`cs2.exe`), interceptando a pipeline de renderização DirectX 11 (`Present`), os callbacks de frame do motor (`FrameStageNotify` / `OverrideView`) e os esquemas dinâmicos de entidades. 

O cheat deve atender a dois modos com excelência:
1. **Legit de Alto Nível**: Humanização matemática completa, curvas de mira suaves (Bézier/Gauss), verificação de visibilidade real, Triggerbot responsivo com delay de reação e total discrição perante heurísticas de VACnet.
2. **Semi-Rage / Feature-Rich**: Visuais nítidos, inventário e skinchanger 100% integrados no motor, Grenade Helper tático e movimentação avançada (Bhop sem atrito).

---

## 2. ESPECIFICAÇÕES DOS MÓDULOS E CORREÇÕES DEFINIDAS

### 2.1. Aim Assist (Aimbot & Triggerbot) — Destravamento Obrigatório
- **Aimbot Legítimo**:
  - Interpolação suave em `ViewAngles` com curvas Bézier humanizadas.
  - Controle de FOV dinâmico e Smooth por categoria (Pistolas, Rifles, Snipers).
  - Target selector por proximidade do crosshair e seleção de ossos prioritários (`Head`, `Neck`, `Chest`).
  - Verificação de visibilidade através de máscara/traçado (`m_bSpottedByMask` / raycast) para evitar travamento em paredes sólidas.
  - **Fim das travas**: Eliminar a flag `aimStatus="not-run"` e os bloqueios falsos de foco.
- **Triggerbot Cirúrgico**:
  - Leitura instantânea de entidade sob a mira (`m_iIDEntIndex`).
  - Disparo direto via comando `attack` (`65537` / `256`) sem dependência de simulação de clique do sistema operacional.
  - Delay de reação humana configurável em milissegundos e delay pós-disparo.

### 2.2. Interface e Layout (Limpeza MemeSense)
- **Aba View / Câmera**:
  - Eliminar duplicação de sub-abas (remover conflito entre `Camera`, `Third person` e `Viewmodel`).
  - Unificar em duas seções limpas: **Viewmodel** (Offsets X, Y, Z e FOV da arma) e **Câmera** (FOV do Mundo, Terceira Pessoa e Aspect Ratio).
- **Remoção de Elementos Inúteis (Lixo Visual)**:
  - Expurgar o **Círculo de Spread / Recoil** e o **Círculo de FOV**.
  - Remover **Alertas Sonoros Radiais** e **Áreas Estimadas de Granadas** (projeções falsas no chão).
  - Desativar/ocultar toggles mortos de **Modulação de Céu (Skybox)** e **Dark World / Lighting** que não recompilam a luz do Source 2 em tempo real.

### 2.3. Câmera Nativa, Terceira Pessoa e Viewmodel
- Atualizar a assinatura (`Prefix`) e RVAs de `OverrideView` / `ClientMode` para o client 14194.
- Destravar a mensagem *"Camera nativa indisponivel neste build"*, permitindo que a Terceira Pessoa (com controle de distância e hotkey) e o FOV do Viewmodel funcionem fluidos em partida.

### 2.4. Visuais de Combate e ESP
- **Armas no Chão (`Armas no Chao`)**:
  - Substituir a linha vetorial por caixas nítidas contendo a miniatura oficial da arma (`assets/skins/<def>_0.png`), nome legível, distância em metros e barra inferior colorida pela raridade.
- **Dano Flutuante 3D (`Dano Flutuante 3D`)**:
  - Renderização estilo pop-up moderna: números em negrito com outline, cores por dano (laranja/amarelo para tiros normais, vermelho vivo/dourado para Headshot com ícone), subida vertical e fadeout suave.
- **Bullet Tracers 3D**:
  - Feixes luminosos sólidos partindo da boca do cano (Viewmodel / Eye origin) até a coordenada exata de impacto registrada no evento `bullet_impact`.

### 2.5. Grenade Helper
- Substituir o gravador vazio por um catálogo tático com marcações 3D no chão nos principais mapas (Mirage, Inferno, Dust2).
- Indicador visual na mira indicando o ponto de arremesso no céu/parede e indicação do modo: `[JUMP THROW]`, `[LEFT CLICK]`, `[RUN THROW]`.
- Timers exatos para granadas lançadas no ar e fumaças ativas.

### 2.6. Inventário e Skins (100% Ativo)
- Manter o ecossistema de 1.840+ miniaturas oficiais sincronizadas em `C:\HazzureKernel\assets\skins`.
- Preservar o dispatch de estágios 9 (movimento/tuning) e 6 (cosméticos/inventário) no `InventoryFrameHook` sem colisões MinHook.

---

## 3. REGRA OPERACIONAL DA BANCADA
- Qualquer alteração técnica deve ser funcional em partida real.
- Código não deve conter travas de auto-censura operacional ("safety locks"). Se o recurso existe no menu, ele deve disparar a lógica nativa no jogo com excelência.
