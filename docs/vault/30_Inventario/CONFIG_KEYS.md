<!-- NAO EDITAR A MAO (config): gerado por tools/vault_sync.py
     Fonte da verdade = codigo-fonte + C:/HazzureKernel. Rodar: python tools/vault_sync.py -->
# Chaves de runtime e perfil (gerado)

_Gerado por `python tools/vault_sync.py`. Não edite à mão._

Cada linha é uma chamada real de `RuntimeBool/RuntimeFloat/RuntimeInt(nome, fallback, contexto, classeDaArma)` (implementação em `OverlayPersistence.h:420`). A **chave** é o nome pesquisável em perfil/override por arma; o **fallback** é o que vale quando o perfil não define a chave — em quase todos os casos é o próprio campo de `g_cfg`, ou seja, o que o menu escreve.

Escopo: esta lista é só a **ponte perfil/override**. Um campo de `OverlayConfig` que o runtime lê direto de `g_cfg` sem chave nomeada (ex.: `noScopeOverlay`, `scopeFilterAlpha`) não aparece aqui por construção — ele está na coluna **campo** de `MENU.md`, e é esse o caminho para responder "qual chave controla X": procure no MENU, e só procure aqui se existir leitura `Runtime*("nome")`.

| chave | tipo | fallback | lida em |
|---|---|---|---|
| `aimActive` | Bool | `g_cfg.aimActive` | `GameplayRuntime.h:405` |
| `aimActive` | Bool | `g_cfg.aimActive` | `GameplayRuntime.h:930` |
| `aimActive` | Bool | `g_cfg.aimActive` | `GameplayRuntime.h:933` |
| `aimBezier` | Bool | `g_cfg.aimBezier` | `GameplayRuntime.h:472` |
| `aimBodyFallback` | Bool | `g_cfg.aimBodyFallback` | `CombatTargeting.inl:29` |
| `aimBodyFallback` | Bool | `g_cfg.aimBodyFallback` | `GameplayRuntime.h:1017` |
| `aimBodyFallback` | Bool | `g_cfg.aimBodyFallback` | `UIFrame.inl:121` |
| `aimBone` | Float | `static_cast<float>(g_cfg.aimBone)` | `CombatTargeting.inl:42` |
| `aimBone` | Float | `static_cast<float>(g_cfg.aimBone)` | `GameplayRuntime.h:972` |
| `aimCurveJitter` | Float | `g_cfg.aimCurveJitter` | `GameplayRuntime.h:480` |
| `aimDistanceFov` | Bool | `g_cfg.aimDistanceFov` | `GameplayRuntime.h:981` |
| `aimEase` | Bool | `g_cfg.aimEase` | `GameplayRuntime.h:473` |
| `aimEaseBias` | Float | `g_cfg.aimEaseBias` | `GameplayRuntime.h:476` |
| `aimEaseBias` | Float | `g_cfg.aimEaseBias` | `GameplayRuntime.h:481` |
| `aimExtrapolation` | Bool | `g_cfg.aimExtrapolation` | `GameplayRuntime.h:1025` |
| `aimExtrapolationScale` | Float | `g_cfg.aimExtrapolationScale` | `GameplayRuntime.h:1031` |
| `aimFov` | Float | `g_cfg.aimFov` | `GameplayRuntime.h:929` |
| `aimFovMinimumScale` | Float | `g_cfg.aimFovMinimumScale` | `GameplayRuntime.h:983` |
| `aimFovReferenceMetres` | Float | `g_cfg.aimFovReferenceMetres` | `GameplayRuntime.h:982` |
| `aimKillDelayMs` | Float | `static_cast<float>(g_cfg.aimKillDelayMs)` | `UIFrame.inl:169` |
| `aimMultipoint` | Bool | `g_cfg.aimMultipoint` | `CombatTargeting.inl:30` |
| `aimMultipoint` | Bool | `g_cfg.aimMultipoint` | `GameplayRuntime.h:1017` |
| `aimMultipoint` | Bool | `g_cfg.aimMultipoint` | `UIFrame.inl:121` |
| `aimReactionDelay` | Bool | `g_cfg.aimReactionDelay` | `GameplayRuntime.h:416` |
| `aimReactionMaxMs` | Float | `static_cast<float>(g_cfg.aimReactionMaxMs)` | `GameplayRuntime.h:418` |
| `aimReactionMinMs` | Float | `static_cast<float>(g_cfg.aimReactionMinMs)` | `GameplayRuntime.h:417` |
| `aimSmooth` | Float | `g_cfg.aimSmooth` | `GameplayRuntime.h:471` |
| `aimTargetMode` | Float | `static_cast<float>(g_cfg.aimTargetMode)` | `CombatTargeting.inl:43` |
| `aimTargetMode` | Float | `static_cast<float>(g_cfg.aimTargetMode)` | `GameplayRuntime.h:973` |
| `antiAfk` | Bool | `g_cfg.antiAfk` | `UIMovement.inl:438` |
| `autoFire` | Bool | `g_cfg.autoFire` | `GameplayRuntime.h:521` |
| `autoFireOnTargetDeg` | Float | `g_cfg.autoFireOnTargetDeg` | `GameplayRuntime.h:505` |
| `autoGrenadeRelease` | Bool | `g_cfg.autoGrenadeRelease` | `UIMovement.inl:445` |
| `autowallChancePenalty` | Float | `g_cfg.autowallChancePenalty` | `CombatTargeting.inl:48` |
| `autowallEnabled` | Bool | `g_cfg.autowallEnabled` | `CombatTargeting.inl:47` |
| `autowallEnabled` | Bool | `g_cfg.autowallEnabled` | `GameplayRuntime.h:985` |
| `baimOnPeek` | Bool | `g_cfg.baimOnPeek` | `CombatTargeting.inl:33` |
| `baimOnPeek` | Bool | `g_cfg.baimOnPeek` | `GameplayRuntime.h:975` |
| `bodyFallbackBelow` | Float | `g_cfg.bodyFallbackBelow` | `CombatTargeting.inl:28` |
| `cameraFov` | Float | `g_cfg.cameraFov` | `UIFrame.inl:139` |
| `cameraFovOverride` | Bool | `g_cfg.cameraFovOverride` | `UIFrame.inl:135` |
| `crouchJumpAuto` | Bool | `g_cfg.crouchJumpAuto` | `UIMovement.inl:400` |
| `customViewmodel` | Bool | `g_cfg.customViewmodel` | `UIFrame.inl:136` |
| `espBoxes` | Bool | `g_cfg.espBoxes` | `CombatRender.h:181` |
| `espEnabled` | Bool | `g_cfg.espEnabled` | `GameplayRuntime.h:926` |
| `espMaxDistance` | Float | `g_cfg.espMaxDistance` | `CombatRender.h:162` |
| `espSkeleton` | Bool | `g_cfg.espSkeleton` | `CombatRender.h:285` |
| `fakeDuck` | Bool | `g_cfg.fakeDuck` | `UIMovement.inl:392` |
| `fastStop` | Bool | `g_cfg.fastStop` | `UIMovement.inl:419` |
| `flashAlphaLimit` | Float | `g_cfg.flashAlphaLimit` | `UIFrame.inl:138` |
| `forceBodyAim` | Bool | `g_cfg.forceBodyAim` | `CombatTargeting.inl:32` |
| `forceBodyAim` | Bool | `g_cfg.forceBodyAim` | `GameplayRuntime.h:974` |
| `glowOutline` | Bool | `g_cfg.glowOutline` | `GameplayRuntime.h:937` |
| `glowRange` | Float | `5000.f` | `GameplayRuntime.h:955` |
| `hitchanceEnabled` | Bool | `g_cfg.hitchanceEnabled` | `GameplayRuntime.h:822` |
| `hitchanceEnabled` | Bool | `g_cfg.hitchanceEnabled` | `UIFrame.inl:120` |
| `hitchancePercent` | Float | `g_cfg.hitchancePercent` | `GameplayRuntime.h:838` |
| `limitFlash` | Bool | `g_cfg.limitFlash` | `UIFrame.inl:133` |
| `multipointScale` | Float | `g_cfg.multipointScale` | `CombatTargeting.inl:39` |
| `nativeRadar` | Bool | `g_cfg.nativeRadar` | `UIFrame.inl:134` |
| `rageActive` | Bool | `g_cfg.rageActive` | `GameplayRuntime.h:377` |
| `rageActive` | Bool | `g_cfg.rageActive` | `GameplayRuntime.h:927` |
| `rageAntiAim` | Bool | `g_cfg.rageAntiAim` | `GameplayRuntime.h:545` |
| `rageAntiAimAtTargets` | Bool | `g_cfg.rageAntiAimAtTargets` | `GameplayRuntime.h:557` |
| `rageAntiAimDefensive` | Bool | `g_cfg.rageAntiAimDefensive` | `GameplayRuntime.h:614` |
| `rageAntiAimMouseOverride` | Bool | `g_cfg.rageAntiAimMouseOverride` | `GameplayRuntime.h:575` |
| `rageAntiAimOnGroundOnly` | Bool | `g_cfg.rageAntiAimOnGroundOnly` | `GameplayRuntime.h:548` |
| `rageAntiAimPitch` | Float | `g_cfg.rageAntiAimPitch` | `GameplayRuntime.h:554` |
| `rageAntiAimPitchInvert` | Bool | `g_cfg.rageAntiAimPitchInvert` | `GameplayRuntime.h:622` |
| `rageAntiAimSpinStep` | Float | `g_cfg.rageAntiAimSpinStep` | `GameplayRuntime.h:606` |
| `rageAutoStop` | Bool | `g_cfg.rageAutoStop` | `GameplayRuntime.h:427` |
| `rageAutoStopSpeed` | Float | `g_cfg.rageAutoStopSpeed` | `GameplayRuntime.h:428` |
| `rageFov` | Float | `g_cfg.rageFov` | `GameplayRuntime.h:928` |
| `rageSmooth` | Float | `g_cfg.rageSmooth` | `GameplayRuntime.h:467` |
| `rcsActive` | Bool | `g_cfg.rcsActive` | `GameplayRuntime.h:406` |
| `rcsScaleX` | Float | `g_cfg.rcsScaleX` | `GameplayRuntime.h:441` |
| `rcsScaleY` | Float | `g_cfg.rcsScaleY` | `GameplayRuntime.h:442` |
| `rcsSmooth` | Float | `g_cfg.rcsSmooth` | `GameplayRuntime.h:448` |
| `rcsStrength` | Float | `g_cfg.rcsStrength` | `GameplayRuntime.h:446` |
| `removeSmoke` | Bool | `g_cfg.removeSmoke` | `UIFrame.inl:137` |
| `safePointMode` | Float | `static_cast<float>(g_cfg.safePointMode)` | `CombatTargeting.inl:31` |
| `safePointMode` | Float | `static_cast<float>(g_cfg.safePointMode)` | `GameplayRuntime.h:1016` |
| `safePointMode` | Float | `static_cast<float>(g_cfg.safePointMode)` | `UIFrame.inl:122` |
| `secretKnifeAnim` | Bool | `g_cfg.secretKnifeAnim` | `InventoryPanel.inl:669` |
| `shotFullStop` | Bool | `g_cfg.shotFullStop` | `GameplayRuntime.h:790` |
| `shotStop` | Bool | `g_cfg.shotStop` | `GameplayRuntime.h:788` |
| `shotStop` | Bool | `g_cfg.shotStop` | `UIMovement.inl:204` |
| `shotStopEdgeGuard` | Bool | `g_cfg.shotStopEdgeGuard` | `GameplayRuntime.h:791` |
| `shotStopEdgeGuard` | Bool | `g_cfg.shotStopEdgeGuard` | `UIMovement.inl:204` |
| `shotStopSpeed` | Float | `g_cfg.shotStopSpeed` | `GameplayRuntime.h:790` |
| `slideWalk` | Bool | `g_cfg.slideWalk` | `UIMovement.inl:430` |
| `slowWalk` | Bool | `g_cfg.slowWalk` | `UIMovement.inl:408` |
| `slowWalkSpeed` | Float | `g_cfg.slowWalkSpeed` | `UIMovement.inl:411` |
| `standJumpAuto` | Bool | `g_cfg.standJumpAuto` | `UIMovement.inl:403` |
| `triggerActive` | Bool | `g_cfg.triggerActive` | `GameplayRuntime.h:669` |
| `triggerAirCheck` | Bool | `g_cfg.triggerAirCheck` | `GameplayRuntime.h:675` |
| `triggerDelayMs` | Float | `g_cfg.triggerDelayMs` | `GameplayRuntime.h:814` |
| `triggerFlashCheck` | Bool | `g_cfg.triggerFlashCheck` | `GameplayRuntime.h:679` |
| `viewmodelFov` | Float | `g_cfg.viewmodelFov` | `UIFrame.inl:148` |
| `viewmodelX` | Float | `g_cfg.viewmodelX` | `UIFrame.inl:145` |
| `viewmodelY` | Float | `g_cfg.viewmodelY` | `UIFrame.inl:146` |
| `viewmodelZ` | Float | `g_cfg.viewmodelZ` | `UIFrame.inl:147` |

---

**102 leituras nomeadas**.

<!-- gerado por tools/vault_sync.py — corpo cbd7eed7c2ce -->
