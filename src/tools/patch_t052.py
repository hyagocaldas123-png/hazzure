import sys

# 1. AdvancedMovement.h
path = r'C:\Codex2\gpt\HazzureInternal\src\AdvancedMovement.h'
with open(path, 'r', encoding='utf-8') as f:
    text = f.read()

target = '''    if(option.strafe&&option.space&&!ground&&!advancedAction&&option.mouse.viewLockJump){

        uintptr_t list{},services{},weapon{},vdata{};uint32_t handle{};int mode{};float maximum{};

        bool haveWeapon=HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)&&

            HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pWeaponServices,services)&&services&&

            HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,handle)&&(weapon=HazzureGame::Entity(list,handle))&&

            HazzureGame::Read(weapon+0x388,vdata)&&vdata&&HazzureGame::Read(weapon+0x1a00,mode)&&mode>=0&&mode<=1&&

            HazzureGame::Read(vdata+0x748+mode*4,maximum)&&std::isfinite(maximum)&&maximum>1&&maximum<=350;

        // O wishspeed do ar e min(|input|, sv_air_max_wishspeed) = 30, nao a velocidade da
        // arma. Com 250 aqui, available=46,9 estourava o cap, projection zerava e o acos
        // devolvia 90 graus cravados em vez dos ~84 do angulo otimo, alem de inflar o ganho.
        auto choice=haveWeapon?OptimalStrafe(local.velocity,local.angles.y,option.dt,cvars.airAcceleration,cvars.airWishSpeed,cvars.airWishSpeed,1.f,0):Strafe{};

        if(choice.valid){Vec3 target=local.angles;target.y=choice.yaw;

            static uint64_t strafeLogged{};
            if(now>=strafeLogged){strafeLogged=now+1000;
                LogDebug("[STRAFE] speed=%.1f angulo=%.1f aplicado=%.2f ganho=%.2f lado=%d wish=%.0f acc=%.0f dt=%.4f",
                    std::hypot(local.velocity.x,local.velocity.y),choice.angle,choice.applied,choice.gain,choice.side,
                    cvars.airWishSpeed,cvars.airAcceleration,option.dt);}

            if(!HazzureMove::SendAngles(local.angles,target,option.mouse,6)||!WriteDirection(client,choice.side)){Stop();return;}state=3;return;}

    }'''

repl = '''    if(option.strafe&&option.space&&!ground&&!advancedAction){

        uintptr_t list{},services{},weapon{},vdata{};uint32_t handle{};int mode{};float maximum{};

        bool haveWeapon=HazzureGame::Read(client+HazzureOffsets::Globals::dwEntityList,list)&&

            HazzureGame::Read(pawn+HazzureOffsets::Schema::m_pWeaponServices,services)&&services&&

            HazzureGame::Read(services+HazzureOffsets::Schema::m_hActiveWeapon,handle)&&(weapon=HazzureGame::Entity(list,handle))&&

            HazzureGame::Read(weapon+0x388,vdata)&&vdata&&HazzureGame::Read(weapon+0x1a00,mode)&&mode>=0&&mode<=1&&

            HazzureGame::Read(vdata+0x748+mode*4,maximum)&&std::isfinite(maximum)&&maximum>1&&maximum<=350;

        // O wishspeed do ar e min(|input|, sv_air_max_wishspeed) = 30, nao a velocidade da
        // arma. Com 250 aqui, available=46,9 estourava o cap, projection zerava e o acos
        // devolvia 90 graus cravados em vez dos ~84 do angulo otimo, alem de inflar o ganho.
        auto choice=haveWeapon?OptimalStrafe(local.velocity,local.angles.y,option.dt,cvars.airAcceleration,cvars.airWishSpeed,cvars.airWishSpeed,1.f,0):Strafe{};

        if(choice.valid){
            static uint64_t strafeLogged{};
            if(now>=strafeLogged){strafeLogged=now+1000;
                LogDebug("[STRAFE] speed=%.1f angulo=%.1f aplicado=%.2f ganho=%.2f lado=%d wish=%.0f acc=%.0f dt=%.4f",
                    std::hypot(local.velocity.x,local.velocity.y),choice.angle,choice.applied,choice.gain,choice.side,
                    cvars.airWishSpeed,cvars.airAcceleration,option.dt);}

            // Air boost via direcionamento de teclas no motor (mira 100% livre como Neverlose/Gamesense)
            if(!WriteDirection(client,choice.side)){Stop();return;}state=3;return;}

    }'''

assert target in text, 'target in AdvancedMovement.h not found'
text = text.replace(target, repl, 1)
with open(path, 'w', encoding='utf-8', newline='') as f:
    f.write(text)
print('AdvancedMovement.h ok')

# 2. UIMovement.inl: include g_cfg.autoStrafe in technique.strafe
path2 = r'C:\Codex2\gpt\HazzureInternal\src\UIMovement.inl'
with open(path2, 'r', encoding='utf-8') as f:
    text2 = f.read()

target2 = 'technique.strafe=(g_cfg.autoStrafePhysics||g_cfg.bhopBoost)&&sensitivityValid;'
repl2 = 'technique.strafe=(g_cfg.autoStrafePhysics||g_cfg.bhopBoost||g_cfg.autoStrafe)&&sensitivityValid;'
assert target2 in text2, 'target2 in UIMovement.inl not found'
text2 = text2.replace(target2, repl2, 1)
with open(path2, 'w', encoding='utf-8', newline='') as f:
    f.write(text2)
print('UIMovement.inl ok')
