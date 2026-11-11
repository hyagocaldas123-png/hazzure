# Mapa de módulos — refatoração de 26/10/2026

`HazzureUI.h` e `Internal.cpp` são os pontos de composição. Os `.inl` mantêm a unidade de compilação e a ordem originais: não são headers autossuficientes e não devem ser compilados separadamente nem incluídos de outro `.cpp`. Isso preserva uma única instância de cada estado estático por unidade original.

## UI — incluídos dentro de namespace HazzureUI

| Arquivo | Responsabilidade |
| --- | --- |
| UIState.inl | Paleta, tipos, leituras auxiliares, configuração e estado da UI. |
| UIAudio.inl | Conversão de cores, síntese/reprodução PCM e hitmarker. |
| UIWorldDrawing.inl | Punch, neve, telemetria, fundo, trajetória de granadas e spread. |
| UIMovement.inl | Velocímetro, publicação de movimento e pulso de jump no Present. |
| UIPlayerGeometry.inl | Geometria de caixas e inclusão do GameplayRuntime existente. |
| UITacticalHud.inl | Apresentação de C4 e espectadores. |
| UIWidgets.inl | Cards, sliders, toggles, combos, cores e keybind. |
| UIProfiles.inl | Registro de variáveis, perfis, persistência/sharing e presets. |
| UIPanel.inl | Navegação, integração de Skins e composição das abas do painel. |
| UIFrame.inl | Consumo de eventos, atalhos e RenderFrame. |

Preservar a ordem dos includes no agregador. Bibliotecas padrão e headers autossuficientes ficam antes do namespace. `GameplayRuntime.h`, `CombatRender.h`, `OverlayPersistence.h`, `OverlaySharing.h` e `SkinSelections.h` conservam seus contratos contextuais anteriores. Não adicionar um segundo namespace HazzureUI dentro dos fragmentos.

## Módulo interno — mesmo escopo global e unidade Internal.cpp

| Arquivo | Responsabilidade |
| --- | --- |
| InternalPresentation.inl | Estado e callbacks da apresentação já existentes. |
| InternalResources.inl | Acesso auxiliar, integração de clima/skin/áudio e recursos de materiais. |
| InternalDrawing.inl | Cores, desenho e restauração do processamento existente. |
| InternalStartup.inl | Inicialização, registro e ativação dos callbacks. |
| InternalTests.inl | Autotestes do ramo INTERNAL_SELF_TEST. |

`DllMain` e a seleção dos ramos normal/teste permanecem em `Internal.cpp`. Não criar novos `.cpp` que também incluam essas implementações: `build.cmd` já compila todos os `src/*.cpp`.

Após a refatoração, `CameraFov.h` passou a resolver o FOV efetivo na saída do getter de câmera validado para o PE instalado. `BasicTuning.h` mantém flash/radar/viewmodel e a configuração compartilhada; não escreve mais os campos de rede do FOV. O callback tipado é composto em `InternalResources.inl` e registrado em `InternalStartup.inl`. `DrawCameraControls` em `UIPanel.inl` apresenta o controle e a telemetria recente. Verificações e limites estão em `work/camera-fov-fix-20261026/RESULTADO.md`.

## Dados gerados

- `ArmoryCatalog.h`: tipos/tabelas públicas; dados em `catalogs/armory/*.inc`, até 512 entradas por arquivo. Gerador mantido em `tools/generate_armory_catalog.py`; a entrada antiga encaminha para ele.
- Catálogo do painel: `HazzurePanel/src/skin_catalog.generated.h` e `catalogs/skins/*.inc`. `HazzurePanel/export-catalog.ps1` gera o mesmo formato e aceita `-OutputDirectory` para validação isolada.
- Catálogo C#: `Hazzure/SchemaSkinCatalog.cs` compõe métodos dos `SchemaSkinCatalog.Part*.cs`. Fonte preservada em `Hazzure/catalog-data/schema-skins-*.json`; gerador em `tools/generate_schema_skin_catalog.py`.

Os `.inc` de catálogos são apenas entradas de inicializadores: não adicionar namespace, includes de sistema ou declarações neles.

## Runtime C#

`Hazzure/Program.cs` contém a entrada do programa. `PlatformNative.cs`, `ProcessMemory.cs`, `OverlayGeometry.cs` e `OverlayWindow.cs` preservam as classes anteriores, no mesmo namespace e com os mesmos corpos. `Hazzure/build.ps1` inclui os arquivos novos explicitamente e as partes do catálogo. Nenhuma função de acesso ao jogo foi alterada nesta extração.

## Evidência

`work/refactor-haiku-20261026/RESULTADO.md` contém builds, verificações e estado da DLL instalada. O verificador `verify_refactor.py` confirma expansão textual, entradas de catálogos, regeneração, imagens e limite de linhas. Backups da extração e mapa estão nessa pasta. Os arquivos grandes de bibliotecas, runtime Python distribuído, referências e históricos não foram refatorados.
