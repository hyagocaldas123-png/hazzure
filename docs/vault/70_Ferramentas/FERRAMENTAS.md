# Ferramentas — build, deploy, injeção, logs e análise

> Manual de operação do Hazzure. Tudo aqui foi conferido nesta sessão (2026-11-07) ou está
> marcado como medição anterior. Se um número divergir do disco, o disco manda: rode o comando
> e corrija este arquivo.

---

## 1. Build do módulo interno

Comando real (Git Bash):

```
cd /c/Codex2/gpt/HazzureInternal && ./build.cmd > build_tNNN.log 2>&1; echo $?   # o log fica DENTRO de HazzureInternal\ (e assim estao todos: build_t028..t050.log, t053.log)
```

- Fallback se o `.cmd` não iniciar: `cmd //c build.cmd` (o `//` escapa a conversão de path do
  MSYS; `cmd /c` puro quebra).
- O script chama `VsDevCmd.bat -arch=amd64`, compila MinHook (`hook.c buffer.c trampoline.c
  hde64.c`), depois `cl /std:c++17 /O2 /MT /EHsc /W4 /LD src\*.cpp` → **unity build**: um `.cpp`
  puxa os `.inl`/`.h` via `#include`.
- Em seguida compila `Hazzure.Internal.Test.exe` com `-DINTERNAL_SELF_TEST` e **roda** esse exe
  (é a suíte de self-test que precisa passar para o build terminar) e compila o painel C#
  `Hazzure.Internal.Control.exe` com `csc.exe`.
- Artefatos (todos em `HazzureInternal\build\`): `Hazzure.Internal.dll`,
  `Hazzure.Internal.Test.exe`, `Hazzure.Internal.Control.exe`.
- Estado medido do último build: `Hazzure.Internal.dll` 2.957.312 bytes,
  SHA-256 `92af769c…` (t050), gravado 2026-11-06 03:05.

**Build ≠ runtime.** Exit 0 e self-test verde provam compilação e invariantes offline; não
provam nada dentro do jogo. Nunca escreva "funciona" com base no build.

Suite completa do repositório (offsets + internal + panel + external + loader + empacotamento):

```
cd /c/Codex2/gpt && ./build-all.cmd          # roda tools/build_all_release.py
```

Ele executa, nesta ordem: `tools/run_offset_dumper.cmd` → `HazzureInternal/build.cmd` →
`HazzurePanel` (build + selftest) → `build-external.cmd` + `test-external.cmd` →
`Hazzure/build.ps1 -OutputName Hazzure.exe -SkipRenderer`, e staging em `release/` com
manifesto. `--package-only` empacota sem compilar.

## 2. Offsets

- Header usado pelo build: `C:\Codex2\gpt\Compartilhado\offsets_generated.h` (gerado por
  `python tools/offset_dumper.py`, alvo `client.dll` `TimeDateStamp=0x6AEBC4DA`,
  `SizeOfImage=0x29A3000`, CS2 build 14194). Hoje: 73 offsets, 9 RVAs assinados em
  `src/SkinAddresses.h`.
- `build.cmd` **não** regenera o header. Depois de mexer no dumper, rode
  `tools/run_offset_dumper.cmd`.
- Só alguns globais são sig-scanned; o resto vem de `DEFAULT_GLOBALS` no próprio dumper — um
  global pode estar **silenciosamente errado** para o build vivo (foi o caso de `dwPlantedC4`
  até 2026-11-06). Para corrigir um valor, corrija os **quatro portadores**:
  `Compartilhado/offsets_generated.h`, `Hazzure/offsets/offsets.json`,
  `Hazzure/payload/offsets/offsets.json` e `DEFAULT_GLOBALS` em `tools/offset_dumper.py`.
- Todos os valores são RVAs relativos ao handle do módulo; a base muda a cada boot.

## 3. Deploy e rollback

Deploy = copiar o DLL compilado para o diretório do kernel:

| destino | papel |
|---|---|
| `C:\HazzureKernel\Hazzure.Internal.dll` | o arquivo que o loader injeta |
| `C:\HazzureKernel\modules\Hazzure.Internal.dll` | segunda cópia usada pelo caminho de módulos |

Antes de sobrescrever, preserve o anterior com sufixo descritivo — a convenção em uso é
`Hazzure.Internal.dll.bak-AAAAMMDD-tNNN` (ex.: `.bak-20261105-t038d`). Hoje há 60 artefatos
`Hazzure.Internal.dll*` entre a raiz e `modules\` (lista completa e hashes em
`30_Inventario/ARTEFATOS.md`, gerada por script). Existem também
`C:\HazzureKernel\backup\<yyyymmdd-hhmm>\` (vistos: `20261105-0631`, `20261105-0747`).

Fontes versionadas por cópia ao lado do arquivo: `*.bak-<data>-<tarefa>`
(ex.: `src/cards/CardEnvironment.inl.bak-20261105-t031`). O repositório **não é git** —
rollback é sempre por essas cópias, nunca por `git checkout`.

## 4. Injeção — quem faz

**Só o usuário abre o CS2 e injeta.** O agente não mata, reinicia, re-injeta nem "testa" o
jogo sem ordem explícita para aquela instância. Manual map não tem hot reload: depois de
build+deploy, o CS2 aberto continua executando o módulo antigo; o código novo só existe na
próxima injeção.

### Como saber qual build está EM MEMÓRIA agora

Cruze o log com o disco — é o único método que funciona:

1. último marcador de sessão no log: `grep -an "\[START\] Start() finalizado" debug_log.txt | tail -1`
2. mtime e SHA-256 do DLL deployado: `sha256sum /c/HazzureKernel/Hazzure.Internal.dll`
3. sessão iniciada **depois** do deploy roda o artefato novo; a última sessão iniciada **antes**
   roda o build anterior.

Não infira identidade de build por linha de feature: muitas tags são *change-gated* e ficam
mudas em sessão estável.

Medição de 2026-11-07 01:3x (log parado em 93.688.092 bytes, última linha 2026-11-06 03:20:13,
`tasklist` sem `cs2.exe`): o último `[START] Start() finalizado` é 02:27:20 e o DLL t050 foi gravado
03:06 → **a última sessão que rodou era anterior ao deploy do t050** (ela carregou o t049,
`247eb4ba…`), e hoje **nenhum build do projeto está em memória**. Prova cruzada: as tags exclusivas
do t050 (`[STRAFE]`, `[AUTOFIRE]`, `[3P]`) aparecem **0 vezes no arquivo inteiro**.
Repetir estes três comandos antes de citar "o que está rodando" — a resposta é por sessão, não
permanente.

## 5. Log

`C:\HazzureKernel\debug_log.txt` cresce ~40 MB/dia. **Nunca** leia o arquivo inteiro nem o
mantenha aberto: `LogDebug` faz `fopen_s(path,"a")` + `fclose` por chamada, então qualquer handle
segurado (`tail -F`, Monitor segurando) faz **todas** as escritas do módulo falharem em silêncio
— isso já simulou duas injeções "falhas" numa sessão.

- Leia por offset: `stat -c %s`, depois `tail -c +<offset>` em aberturas curtas.
- Timestamps do log não têm data; ordene por número de linha, não pelo relógio.
- **Recorte de sessão (receita, medida errada duas vezes em 07/11):**
  `N=$(grep -an "\[START\] Start() finalizado" debug_log.txt | tail -1 | cut -d: -f1); tail -n +$N debug_log.txt > /tmp/sessao.txt`
  e conte em `/tmp/sessao.txt`. `awk '/04:16:45/,0'` **não serve** — o padrão casa dentro de outras
  linhas e o alcance recomeça, então a "sessão" passa a incluir horas de sessões antigas (foi assim que
  saíram 470 amostras de C4 no lugar de 170 e 154 `[3P]` no lugar de 13). `grep -abn` também engana: o
  primeiro campo que aparece é o número da linha, não o offset de bytes.
- **Antes de escrever "0 ocorrências" ou "N ocorrências", diga o recorte.** A mesma tag tem contagens
  radicalmente diferentes no arquivo inteiro e na sessão em memória.
- Ausência de linha ≠ subsistema morto (muitos diagnósticos só disparam quando um campo muda).
  Liveness do canal compartilhado: `Local\HazzureInternal.<pid>`, magic `0x485A4931`.

- **Leituras do desync (T071) prontas:** `python tools/log_desync.py` recorta a sessão atual sozinho
  (procura o último `[START] Start() finalizado` por offset de byte, lê em blocos de 4 MB e **fecha o
  arquivo** — sem handle segurado) e imprime só as linhas `[DESYNC]`, com o último `[AIMTRIG]` e um
  veredito. Opções: `--session 2|all`, `--tail N`, `--csv arquivo`, `--watch SEG` (reabre a cada SEG
  s; é o substituto correto do `tail -F`). Ele distingue as cinco leituras: sem linha (build anterior ao t071 não está
  em memória), `cfg=0` (desligado no config), `cfg=1` com `aplicado=0` em tudo (o
  `StampSubtickViewAngles` não escreveu nas entries — canal da T070/T072 falseado), `aplicado=1` com
  `delta ~0` (offset do AA zerado ou cortado pelo fake limit → não é desync) e `ok` congelado com
  duas amostras ou mais (escreveu uma vez e parou). Medido 08/11: no log real acerta o recorte
  (byte 110.215.305 = sessão 05:10:48, t069 → "SEM linhas"; byte 110.252.531 = 08:01:45, t071 →
  `aplicado=0 em todas`, que é o estado correto daquelas sessões). **Teste offline:**
  `python tools/test_log_desync.py` roda 39 checagens sem jogo e sem log real — as cinco decisões,
  recorte de sessão com `BLOCO` encolhido a 37 bytes (marcador atravessando fronteira), CRLF,
  `--session last|N|all`, `--tail`, `--csv`, linha malformada (aviso, não exceção) e o `--watch`
  incluindo a prova de que ele **não segura handle** (renomear o log enquanto o watcher dorme tem de
  dar certo). Saída medida: 39 PASS, 0 falhas, exit 0.

### Watcher

`C:/Codex2/gpt/tools/log_watch_t049.sh` (v5) — polling curto, nunca handle segurado; filtra só
eventos acionáveis:

```
grep -aE '\[STRAFE\]|\[AUTOFIRE\]|\[3P\]|\[AUTOWALL\]|\[START\]|\[CLIMA\] Transicao|\[C4\]|\[BOMB|\[GLOVEBG\] ok=0|\[ERROR|\[ERRO|EXCEPTION|falhou|Falhou|nao confirmado|Unverified|Fail=[1-9A-Fa-f]|RestoreFail=[1-9]|\[ANTIAIM\].*applied=0|\[SCOPE\].*writes=0'
```

e descarta o benigno (`timer sem bomba` = healthy sem C4 plantada; `[ANTIAIM] applied=1`;
`[SCOPE] filtro` com tudo zero). Deduplica por chave normalizada persistida em arquivo
(janela de 60 s) — estado awk **não** sobrevive entre lotes.

Armadilhas já pagas, não repague:
- filtrar por **nome de campo** (`writeFail`) pega o heartbeat com `=0` e inunda; filtre por
  **valor não-zero** (`Fail=[1-9A-Fa-f]`).
- normalizar sinal: `gsub(/[0-9]+/,"#")` deixa `-89.00` e `89.00` como chaves distintas; use
  `gsub(/-?[0-9]+(\.[0-9]+)?/,"#")`.
- gawk 5.4 não tem `flush()` → use `fflush(sf)`, senão o arquivo de vistos nunca é escrito.
- **Dry-run do pipeline contra a cauda real do log antes de subir o Monitor.** Meta: ~3 chaves
  por 400 KB. Hoje o dry-run dos últimos 400 KB fecha com 0 linhas — silencioso por estar certo.

Tags que só aparecem sob condição (silêncio não é falha): `[SCOPE]` exige "Sem Borda do Scope"
ligado; `[AUTOWALL] sample=` exige rage travado em alvo; `[C4] timer sem bomba` é a linha
saudável de "ainda não plantaram".

## 6. Config e perfis

- `C:\HazzureKernel\hazzure_config.json` — medido: 279 chaves, mtime 2026-11-06 00:58.
- Chave ausente **não** é bug: chave nova só entra no arquivo no primeiro shutdown *gratuito*;
  até lá vale o default do código (`UIState.inl:122 struct OverlayConfig`, inventário em
  `30_Inventario/MENU.md`). Sessão que crasha ou é morta não escreve nada.
- Editar o JSON com o CS2 aberto: o módulo sobrescreve no shutdown. Não serve como canal de
  teste.
- Chaves nomeadas de runtime/per-arma (`RuntimeBool/Float/Int`) em
  `30_Inventario/CONFIG_KEYS.md` — 79 leituras medidas.

## 7. IDA e o que fazer quando ele não responde

O conector MCP registrado como `ida` (server stdio `assist.exe --mcp`) só atende quando o IDA
está aberto **com o banco carregado e o widget Assist aberto**. Medido nesta sessão:
`list_databases` não existe como ferramenta e `search_strings` retornou "invocation did not
complete" → **IDA indisponível**. política: tente uma vez, registre o limite e siga com os
fallbacks; não trave a tarefa esperando o IDA.

- Banco de análise: `C:\Codex2\ida_work\client.dll.i64` (client.dll `0x6AEBC4DA`,
  `SizeOfImage=0x29A3000`). Image base `0x180000000` ⇒ **VA = base + RVA**; hexa dígito a menos
  vira "no function at address".
- Binário do jogo (não é o de `game/bin/win64`):
  `C:\Program Files (x86)\Steam\steamapps\common\Counter-Strike Global Offensive\game\csgo\bin\win64\client.dll`
  — conferido nesta sessão: `TimeDateStamp=0x6AEBC4DA`, `SizeOfImage=0x29A3000`, igual ao alvo.

### Fallbacks offline que já responderam perguntas reais (`C:\Codex2\gpt\tools\`)

| script | o que responde |
|---|---|
| `faultfn.py <rva> <back> <fwd>` | capstone de qualquer janela do `client.dll` instalado; `faultfn.py callers <rva>` varre `.text` por sites `E8` |
| `scratch_region_strings.py <start> <len>` | strings impressíveis da região (cluster + caminho `cstrike15\*.cpp` nomeia a TU dona) |
| `scratch_disasm_res.py <start> <len>` | desmonta e resolve operandos rip-relative para o que vive no alvo |
| `scratch_global_xref.py <rva> [-d]` | xrefs de dados (mov/lea/movss/cmp) apontando para um RVA |
| `scratch_scope_xref.py <kw>` / `scratch_scope_probe.py <kw>` | busca de string + vizinhos + xrefs / varredura de VPK |
| `crash_rva_summary.py` | histograma `(TimeDateStamp, faultRVA)` de todos os minidumps |
| `crash_stack_ownership.py` | tenta atribuir o stack ao nosso mapa (ver §8: capped) |
| `offset_dumper.py` | regenera `offsets_generated.h` por byte-signature |

Outras fontes quando o IDA está morto: dump de schema `work\dump_14194\client_dll.hpp` (listas
**e contagem** de campos por classe — contagem 0 encerra "essa entidade tem o campo X") e a
referência vendida `HazzureInternal\references\Andromeda-CS2-Base` (ex.:
`CInventoryChanger.cpp` para a sequência de chamadas de um changer que funciona).

## 8. Forense de crash (minidumps)

- Os `.mdmp` estão em `game\bin\win64\cs2_<yyyymmdd>_<hhmmss>_<n>_accessviolation.mdmp` — **não**
  na pasta de `client.dll` (que é `game\csgo\bin\win64`, §2/§7). Corrigido 08/11 ~08:20: a linha
  anterior dizia `game\csgo\bin\win64` e media 0 dumps; medição nova: 52 `.mdmp` em `game\bin\win64`
  (o mais recente `cs2_2026_1108_080231_0_accessviolation.mdmp`), 0 na pasta do `csgo`.
  Os dois scripts (`tools/crash_rva_summary.py`, `tools/crash_stack_ownership.py`) já apontam para
  o caminho certo; `tools/faultfn.py` usa o caminho do `client.dll` (também certo).
  Quando o usuário diz "o jogo fechou", procure dump **no mesmo segundo** da última linha do log
  — vários "fechamentos" eram crash.
- O que o dump + disassemblagem responde (provado 08/11): a RVA do fault → `python
  tools/crash_rva_summary.py AAAAMMDD` → depois `python tools/faultfn.py <rva> <back> <fwd>` disassembla
  o entorno no `client.dll` **em disco** (confira antes se o `TimeDateStamp` do arquivo bate com o
  `tds` dos dumps — 08/11: `0x6AEBC4DA` nos dois). Isso identificou o `Clear()` de
  `CCSGOInputHistoryEntryPB` em `client.dll+0x7B0C80` e, com ele, o nosso próprio bug de has-bit
  (T072). Continua sem stack legível: a disassemblagem diz *o que* caiu, não *quem* chamou.
- O dump dá: código da exceção, endereço do fault, direção/alvo do AV, todo o CONTEXT e a lista
  de módulos com TimeDateStamp por módulo (isso bastou para mostrar uma banda de fault recorrente
  em três builds do client.dll). Parâmetros da exceção ficam em `record+32`.
- O dump **não** dá: stack legível no RSP capturado nem faixa de memória contendo nosso mapa
  (a fingerprint `[CLIMA]` nunca aparece). Ou seja, causalidade offline é capped — não dá para
  provar do dump se o nosso código está na cadeia do fault. O discriminador decisivo é A/B em
  jogo (injetado vs não injetado). Não gaste mais tempo tentando walk de stack.
- Defeitos conhecidos: `crashmap.py` trata o VA do registro de exceção como offset de arquivo
  (parâmetros sem sentido — use `regs.py`); constantes do capstone são `capstone.CS_OP_MEM` e
  `capstone.x86.X86_REG_RIP`.
- Scripts descartáveis: escreva com a ferramenta de escrita, **não** por heredoc no Git Bash
  (heredoc longo é truncado em silêncio). Scripts que varrem binário precisam de patterns em
  bytes (`rb'...'`). Promova o que presta para `C:/Codex2/gpt/tools/` — o diretório de scratch
  é limpo.

## 9. Conteúdo do jogo e assets (para trabalho futuro com modelos)

- Conteúdo shipped está **dentro de `game\csgo\pak01_*.vpk`** (chunked: `pak01_000.vpk`, …; não
  existe `pak01_dir.vpk` nesta instalação). "Esse dado vem no jogo?" se responde pelos VPKs, não
  por `ls`. O parse da árvore v2 ainda não foi resolvido; o que funciona é ler o arquivo e varrer
  runs imprimíveis (`re.finditer(rb'[ -~]{4,})`) filtrando por palavra-chave.
- DLLs de engine em `game\bin\win64\` (`materialsystem2.dll`, …). `resourcecompiler.exe` e
  `resourcecompiler.dll` **existem** aí (conferido). `vphysics.dll` não existe nesta instalação.
- Addons de comunidade já montados em `game\csgo_community_addons\` (cs_shelter, de_boulder,
  de_debris, de_eldorado, de_fachwerk, de_poseidon) e `content\csgo_addons\` existe — é o
  caminho provável para conteúdo custom (modelos de agente/mão), ainda não validado.
- ConVar: registro estático só tem xref do próprio registration + thunk de destruidor; "não achei
  leitores" **não** falsifica a convar (o valor é lido pelo acessor genérico). Float runtime em
  `[global+8]+0x58`; leia com `HazzureCvars::Value` e só escreva depois de ler valor plausível.

## 10. Verificadores e geradores do vault

| comando | o que faz |
|---|---|
| `python tools/vault_sync.py` | gera `30_Inventario/{MENU,CONFIG_KEYS,OFFSETS,ARTEFATOS}.md` a partir do código + `C:\HazzureKernel`. Medido (após a T053): 355 widgets, 77 linhas de chamada `Runtime*` (= 63 chaves distintas), 73 offsets, 9 RVAs; rodar 2× produz corpo byte a byte idêntico |
| `python tools/vault_sync.py --check` | **verificação do vault inteira num comando**, exit 1 se algo divergiu. Dois passos: (1) compara só o corpo dos 4 arquivos de inventário com o código/deploy — ignora o rodapé; (2) roda o `vault_lint` abaixo. Hoje: os dois ok, exit 0 |
| `python tools/vault_lint.py` | integridade do vault em si, read-only, 53 `.md`: encoding/mojibake, frontmatter fechado, `[[wikilink]]` e links relativos resolvendo, colunas de tabela, seções duplicadas, texto colado, CRLF misturado. exit 0 = "sem problemas" (é a resposta de "está tudo certo no Obsidian?"). Também chamável como `vault_lint.run(diretorio) -> (achados, n_arquivos)`, que é como o `--check` o usa; `HAZZURE_VAULT` aponta para outro vault nos testes |
| `python tools/verify_artifacts.py` | checagem read-only de procedência dos artefatos (sem rebuild, sem carregar binário) |
| `python tools/log_desync.py` | filtra `[DESYNC]` da sessão em memória e dá o veredito do desync (T071): recorte por offset do último `[START]`, sem segurar handle; `--session N\|all`, `--tail`, `--csv`, `--watch SEG`. **Teste offline:**
  `python tools/test_log_desync.py` (39 checagens, sem jogo). Ver também o §5 Log |
| `python tools/validate_match_readiness.py` (`test-match-readiness.cmd`) | pré-voo: binários nativos, assets, offsets de IPC e ambiente CS2 |
| `python tools/build_all_release.py` (`build-all.cmd`) | suite completa + staging em `release/` |

O inventário é **derivável**: ninguém edita `30_Inventario/*.md` à mão; mudou o código, roda o
gerador. O que não é derivável (estado, decisões, validação em partida) vive em `00_Gestao/`.

## 11. Git do vault e entrada de outros agentes

- **O vault é git desde 2026-11-07** (primeiro commit `b46ee8a`). `C:\Codex2\gpt` **não é** git:
  rollback de código continua sendo `*.bak-<yyyymmdd>-tNNN` e `C:\HazzureKernel\backup\`, nunca
  `git checkout`. Não confundir os dois.
- Commitar documentação do vault depois de mexer nela: `git -C C:/Codex2/Hazzure_Projeto add -A`
  + `git -C ... commit`. `.gitignore` só deixa de fora `.obsidian/workspace.json` (muda a cada
  sessão) e `.trash/`; os `.bak-` **são** versionados porque são o rollback.
- **Identidade do commit**: o git global não tem `user.name`/`user.email`, então os commits deste
  ciclo usaram override por comando (`-c user.name=... -c user.email=...`). Se você quiser a sua
  assinatura nos próximos, rode uma vez
  `git config --global user.name "seu nome" && git config --global user.email "seu email"`.
- **Antigravity / OpenCode / CLI**: o que eles carregam é o arquivo de regras do **folder aberto no
  workspace**. Para que caiam no contrato sem eu repetir a instrução, existem dois ponteiros novos
  (não sobrescrevem nada): `C:\Codex2\gpt\GEMINI.md` (5 passos até o `Inicio.md`) e
  `C:\Codex2\Hazzure_Projeto\GEMINI.md`. Manter `gpt` no workspace continua obrigatório — é onde
  estão o código, o `build.cmd` e os `tools/*.py`; o vault entra como **segunda** pasta, não como
  substituta. O `~/.gemini/GEMINI.md` global (persona/regras suas) não foi tocado.

## 12. Limites deste arquivo

- Nada aqui substitui prova em partida; comportamento de jogo só é válido com o usuário tendo
  injetado aquele build e testado.
- Números de contagem (widgets, chaves, offsets, artefatos, dumps) são fotografias: re-gerar com
  os comandos acima antes de citar em relatório.
- Se uma seção aqui contradisser `00_Gestao/Estado.md` sobre o que está em memória, o Estado
  manda — ele é atualizado por sessão; este arquivo descreve mecanismo, não é snapshot de estado.
