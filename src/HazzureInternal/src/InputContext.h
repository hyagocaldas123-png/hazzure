#pragma once
namespace HazzureInputContext {

// BUG6 FIX: cursorKnown e cursorVisible removidos da assinatura — eram
// dead parameters suprimidos por (void). Callers atualizados.
inline const char* BlockReason(bool menu, bool capture, bool foreground, bool ownProcess) {
    if(menu)    return "menu-open";
    if(capture) return "imgui-capture";
    if(!foreground) return "no-foreground";
    if(!ownProcess) return "other-window";
    return "ready";
}

} // namespace HazzureInputContext
