static HazzureLineupAssist::Sequence g_lineupSequence;
static bool g_lineupForwardOwned;
static uintptr_t g_lineupOwnerClient;
static const char* g_lineupState = "Inativo";

static void ReleaseLineupForward(uintptr_t client, bool context = false) {
    if (g_lineupForwardOwned && g_lineupOwnerClient) {
        bool physical = client && context && (GetAsyncKeyState('W') & 0x8000) != 0;
        if (HazzureMove::Button(g_lineupOwnerClient + HazzureOffsets::Buttons::forward, "forward", physical ? 65537 : 256)) {
            g_lineupForwardOwned = false;
            g_lineupOwnerClient = 0;
        }
    }
}

static bool ProcessLineupAutomation(uintptr_t client, uintptr_t pawn, bool active, const HazzureMove::Settings& movement) {
    using namespace HazzureLineupAssist;
    HazzureGame::Pawn local{};
    Input input{};
    input.now = GetTickCount64();
    input.pawn = pawn;
    input.active = g_cfg.lineupHelper && g_cfg.lineupAutoAlign && g_cfg.jumpthrowActive;
    input.key = g_cfg.jumpthrowKey > 0 && (GetAsyncKeyState(g_cfg.jumpthrowKey) & 0x8000) != 0;
    input.context = active && HazzureGame::ReadPawn(pawn, local) && local.health > 0 && local.life == 0 &&
                    local.velocityValid && HazzureInputView::ReadLive(client, input.angles);

    uint8_t move{};
    input.context = input.context && SafeRead(pawn + HazzureMove::moveTypeOffset, &move) && move == 2;

    uintptr_t list{}, services{};
    uint32_t handle{};
    uint16_t definition{};
    int flags{};
    uint8_t pin{};

    if (input.context && SafeRead(client + HazzureOffsets::Globals::dwEntityList, &list) &&
        SafeRead(pawn + HazzureOffsets::Schema::m_pWeaponServices, &services) && services &&
        SafeRead(services + HazzureOffsets::Schema::m_hActiveWeapon, &handle)) {
        input.weapon = HazzureGame::Entity(list, handle);
    }

    if (input.weapon && HazzureVisibility::BuildMatches(client)) {
        SafeRead(input.weapon + HazzureOffsets::Schema::m_iItemDefinitionIndex, &definition);
        SafeRead(input.weapon + 0x1f23, &pin);
        SafeRead(input.weapon + 0x1f30, &input.strength);
    }

    input.definition = definition;
    input.pin = (pin == 1);
    input.position = local.origin;
    input.speed = std::hypot(local.velocity.x, local.velocity.y);
    input.ground = SafeRead(pawn + HazzureOffsets::Schema::m_fFlags, &flags) && (flags & 1) != 0;
    input.ducked = (flags & 2) != 0;
    input.eye_origin = { local.origin.x, local.origin.y, local.origin.z + (input.ducked ? 46.0f : 64.0f) };
    input.map = g_cfg.lineups.map;

    bool inProgress = g_lineupSequence.phase == Phase::Align || g_lineupSequence.phase == Phase::Run || g_lineupSequence.phase == Phase::Jump;
    float matchScore = 0.0f;
    input.selected = inProgress ? g_lineupSequence.selected : HazzureLineups::Nearest(g_cfg.lineups, definition, local.origin, input.ducked ? 1 : 0, &matchScore);

    const auto* entry = (input.selected >= 0 && input.selected < g_cfg.lineups.count) ? &g_cfg.lineups.entries[input.selected] : nullptr;
    auto output = g_lineupSequence.Step(input, entry);

    if (output.aim && !HazzureMove::SendAngles(input.angles, output.targetAngles, movement, 3.f)) {
        output = g_lineupSequence.Cancel();
        g_lineupState = "Alinhamento indisponivel";
    }

    if (output.forward) {
        if (!HazzureMove::Button(client + HazzureOffsets::Buttons::forward, "forward", 65537)) {
            output = g_lineupSequence.Cancel();
        } else {
            g_lineupForwardOwned = true;
            g_lineupOwnerClient = client;
        }
    } else {
        ReleaseLineupForward(client, input.context);
    }

    if (output.restore) ReleaseLineupForward(client, input.context);

    // Handoff sub-tick para Hook_CreateMove (CUserCmdBasePB)
    if (output.releaseAttack) {
        if (!IsGameInputActive(false) || !HazzureWorld::ready.load()) {
            g_lineupSequence.Cancel();
        } else {
            g_pendingLineup.throwMode = output.throwMode;
            g_pendingLineup.when = output.when;
            g_pendingLineup.timestamp = input.now;
            strcpy_s(g_pendingLineup.name, entry ? entry->name : "lineup");
            strcpy_s(g_pendingLineup.map, input.map);
            g_pendingLineup.dxy = entry ? HazzureLineups::HorizontalDistance(input.position, entry->position) : 0.0f;
            g_pendingLineup.armed.store(true);
        }
    }

    auto phase = g_lineupSequence.phase;
    HazzureMove::lineupBusy.store(output.releaseAttack || phase == Phase::Align || phase == Phase::Run || phase == Phase::Jump || phase == Phase::Release);

    switch (g_lineupSequence.phase) {
        case Phase::Align: g_lineupState = "Alinhando mira (convergindo)"; break;
        case Phase::Run: g_lineupState = "Executando corrida"; break;
        case Phase::Jump: g_lineupState = "Disparando jumpthrow sub-tick"; break;
        case Phase::Release: g_lineupState = "Arremesso enviado"; break;
        case Phase::Done: g_lineupState = "Arremesso concluido"; break;
        case Phase::Cancelled: g_lineupState = "Cancelado / fora de posicao ou granada incorreta"; break;
        default: g_lineupState = "Inativo"; break;
    }

    return output.jump;
}
