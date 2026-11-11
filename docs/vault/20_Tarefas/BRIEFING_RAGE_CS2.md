# Briefing Técnico: Módulo Rage — CS:2 (Source 2, Sub-tick)

> Registrado em 2026-11-08 a partir da diretriz enviada por AN0N.
> Fonte canônica de arquitetura para o subsistema de Rage, Silent Aim, Perfect Silent, Anti-Aim e Reprojeção de Movimento no CS2 (Engine Source 2).

---

## 0. Pré-requisitos e Convenções
- **Offsets e signatures**: dinâmicos por build (alvo atual: 14194 / `0x6AEBC4DA`), verificador com fail-closed estrito.
- **Estruturas de entrada**: `CCSGOInput` → `CSGOUserCmdPB` (`base + 0x38`) → `m_input_history[]` (`CSGOInputHistoryEntryPB` com `m_nButtonState`, `m_flWhen`, `m_view_angles`).
- **Princípio Mestre**: Toda escrita de mira e botões opera sobre entradas de input (`m_input_history`), **nunca** via `SetViewAngles`.

---

## 1. Ordem de Execução Obrigatória (Pipeline em Função Única)
1. **Coleta de estado**: pawn local, arma ativa, flags de movimento, tickbase.
2. **Seleção de alvo**: score composto multidimensional.
3. **Scan de hitbox / pontos**: multipoint escalonado, safe points.
4. **Autowall + dano**: penetração Source 2 com cache por `(alvo, hitbox, tick)`.
5. **Hitchance**: cone determinístico de dispersão com amostragem em 256 seeds.
6. **Solve de ângulo**: vetor angular exato para o ponto vencedor.
7. **Aplicação do ângulo (Silent / Perfect Silent)**: escrita estrita no `m_view_angles` do sub-tick.
8. **Anti-aim**: pitch fake, yaw fake, desync dentro dos limites dinâmicos do animgraph.
9. **Movement fix + Strafe + Bhop**: reprojeção dos inputs baseada no yaw real versus yaw enviado.
10. **Botões**: gravação de flags (`IN_ATTACK`, `IN_ZOOM`, `IN_DUCK`, `IN_JUMP`) nas entradas sub-tick.
11. **Fakelag / Tickbase**: choke de pacotes e orçamento de comandos (double tap / hide shots).
12. **Pós-tick**: restauração de ângulos visuais no cliente e reset de flags de frame.

---

## 2. Seleção de Alvo
- **Fórmula de Score**:
  $$\text{Score} = w_{\text{fov}} \cdot \text{FOV} + w_{\text{dist}} \cdot \text{Dist} + w_{\text{hp}} \cdot \text{HP} + w_{\text{threat}} \cdot \text{Ameaça} + \text{PrioridadeManual}$$
- **Filtros**:
  - FOV configurável (priorizar alvos próximos do centro geométrico da mira).
  - Visibilidade / Autowall pass.
  - Filtro de HP mínimo e ignorar teammates.
  - Ignorar alvos já engajados por teammates / delay de troca entre ticks para evitar oscilação de mira.
  - Trava rígida de alvo durante a rajada de disparo.

---

## 3. Hitbox, Multipoint e Safe Points
- **Hierarquia**: Head → Neck → Chest → Stomach → Pelvis → Limbs.
- **Multipoint**: geração de $N$ pontos por hitbox com escala configurável e descarte por visibilidade.
- **Safe Points**: pontos validados por múltiplos raios (trace de borda e autowall).
- **Baim Condicional (Force Body)**: queda automática para o tórax/estômago se o dano na cabeça não atingir o limiar configurado ou se o HP do alvo for menor que o dano de corpo.

---

## 4. Autowall e Dano
- **Cálculo**:
  $$\text{Dano} = \text{CalculateDamage}(\text{origem}, \text{ponto}, \text{arma}, \text{armor\_ratio}, \text{range\_mod}, \text{hitgroup\_mult}, \text{penetration})$$
- Limite de penetrações por tick para proteger frametime.
- Cache obrigatório chaveado em `(target_id, hitbox_id, tick_count)`, invalidado por deslocamento linear do alvo $> \text{threshold}$.

---

## 5. Hitchance Determinístico
- Amostragem em 256 seeds sobre a penalidade de precisão da arma (`m_fAccuracyPenalty`):
```cpp
int hits = 0;
for (int seed = 0; seed < 256; ++seed) {
    Vec3 dir = ApplySpread(angle, ConeFromPenalty(penalty), seed);
    if (TraceLine(eyePos, eyePos + dir * dist, mask).hit_target)
        hits++;
}
float hc = (hits / 256.0f) * 100.0f;
```
- Disparo bloqueado se $\text{hc} < \text{hc}_{\text{min}}$.

---

## 6 & 7. Silent Aim e Perfect Silent
- **Silent Aim**:
  - Ângulo gravado na entrada de input (`m_input_history[i].m_view_angles`).
  - Câmera do jogador restaurada no final do tick.
- **Perfect Silent**:
  - Nunca escreve `m_angEyeAngles` no cliente.
  - Ângulo existe **exclusivamente** no pacote sub-tick enviado ao servidor.
  - Zero snap ou micro-jitter de tela.
  - **Exigência mandatória**: Crosshair de tiro próprio no swapchain (indicador visual do ponto de impacto real) compensado pelo recuo (`aimPunchAngle`).

---

## 8. Anti-Aim & Desync
- **Base Yaw**: offset relativo ao yaw real (Static, Jitter, Random, At-Targets, Spin).
- **Pitch**: Up, Down (-89°), Zero, Fake Up.
- **Desync**: empurrado até o limite dinâmico exposto pelo AnimGraph (velocidade e postura móvel), evitando quebrar limites estáticos.
- **Freestanding / Edge**: trace lateral de paredes para ocultar o head atrás de coberturas sólidas.
- **Fake Duck**: manipulação de `IN_DUCK` nas entradas sub-tick para alternar hitbox sem perder campo de visão.

---

## 9. Fakelag e Tickbase
- **Choke adaptativo**: controle de envio com base em aceleração e visibilidade de inimigos.
- **Double Tap**: avanço/deslocamento de tickbase para permitir 2 disparos no mesmo tick lógico.
- **Regra de Coexistência**: Reserva de orçamento de comandos entre Double Tap e Bhop sub-tick para evitar descarte silencioso de pacotes pelo servidor (`sv_maxusrcmdprocessticks`).

---

## 10. Bhop com Anti-Aim (Reprojeção Obrigatória de Inputs)
- **Problema de Causa Raiz**: O servidor projeta `forwardmove` e `sidemove` contra o `m_view_angles` do comando enviado. Se o Anti-Aim envia $Yaw_{\text{fake}} = Yaw_{\text{real}} + 180^\circ$, o WASD move o jogador de costas ou gira em falso (spin em órbita).
- **Solução Matemática (Reprojeção)**:
```cpp
float deltaYaw = DEG2RAD(yaw_enviado - yaw_real);
float forward = inputs.forwardmove;
float side    = inputs.sidemove;

inputs.forwardmove = forward * cos(deltaYaw) - side * sin(deltaYaw);
inputs.sidemove    = forward * sin(deltaYaw) + side * cos(deltaYaw);
```
- Cálculo do strafe no ar feito **estritamente no yaw real**; a matriz de rotação é aplicada no final, antes da serialização protobuf.

---

## 11. Render de Apoio
- Crosshair de disparo real com compensação de recuo:
  $$d = \text{AngleToScreenDelta}(\text{aimPunchAngle}, \text{FOV}, W, H)$$
  $$\text{Posição} = \text{Centro} + d$$
- Indicadores em tela: Hitchance atual, Dano calculado, Alvo travado.

---

## 12. Checklist de Validação
- [ ] Pipeline segue a ordem do item 1 sem desvios.
- [ ] Alvo travado estritamente durante rajada de tiro.
- [ ] Hitchance com amostragem determinística de 256 seeds respeitado.
- [ ] Autowall cacheado sem quebra de frametime (5 alvos simultâneos).
- [ ] Perfect Silent opera com zero snap ou jitter de câmera no cliente.
- [ ] Crosshair de tiro perfeitamente alinhado com o impacto real em 100+ unidades.
- [ ] Anti-Aim em Static, Jitter e Spin mantém trajetória perfeitamente reta no WASD.
- [ ] Desync acompanha dinamicamente limites de pose (parado, andando, agachado).
- [ ] Bhop coordenado com Anti-Aim sem perda de velocidade ($\Delta \text{speed} \ge 0$).
- [ ] Orçamento de comandos entre Double Tap e Bhop sincronizado.
