/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

/* ScriptData
SDName: Dustwallow_Marsh
SD%Complete: 95
SDComment: Quest support: 11180, 558, 11126, 11142, 11174, Vendor Nat Pagle
SDCategory: Dustwallow Marsh
EndScriptData */

/* ContentData
npc_lady_jaina_proudmoore
npc_nat_pagle
npc_private_hendel
npc_cassa_crimsonwing - handled by npc_taxi
EndContentData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedEscortAI.h"
#include "ScriptedGossip.h"
#include "SpellScript.h"
#include "Player.h"
#include "WorldSession.h"

/*######
## npc_lady_jaina_proudmoore
######*/

enum LadyJaina
{
    QUEST_JAINAS_AUTOGRAPH = 558,
    SPELL_JAINAS_AUTOGRAPH = 23122
};

#define GOSSIP_ITEM_JAINA "I know this is rather silly but i have a young ward who is a bit shy and would like your autograph."

class npc_lady_jaina_proudmoore : public CreatureScript
{
public:
    npc_lady_jaina_proudmoore() : CreatureScript("npc_lady_jaina_proudmoore") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_SENDER_INFO)
        {
            player->SEND_GOSSIP_MENU(7012, creature->GetGUID());
            player->CastSpell(player, SPELL_JAINAS_AUTOGRAPH, false);
        }
        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (player->GetQuestStatus(QUEST_JAINAS_AUTOGRAPH) == QUEST_STATUS_INCOMPLETE)
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_JAINA, GOSSIP_SENDER_MAIN, GOSSIP_SENDER_INFO);

        player->SEND_GOSSIP_MENU(player->GetGossipTextId(creature), creature->GetGUID());

        return true;
    }

};

/*######
## npc_nat_pagle
######*/

enum NatPagle
{
    QUEST_NATS_MEASURING_TAPE = 8227
};

class npc_nat_pagle : public CreatureScript
{
public:
    npc_nat_pagle() : CreatureScript("npc_nat_pagle") { }

    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        player->PlayerTalkClass->ClearMenus();
        if (action == GOSSIP_ACTION_TRADE)
            player->GetSession()->SendListInventory(creature->GetGUID());

        return true;
    }

    bool OnGossipHello(Player* player, Creature* creature)
    {
        if (creature->IsQuestGiver())
            player->PrepareQuestMenu(creature->GetGUID());

        if (creature->IsVendor() && player->GetQuestRewardStatus(QUEST_NATS_MEASURING_TAPE))
        {
            player->ADD_GOSSIP_ITEM(GOSSIP_ICON_VENDOR, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
            player->SEND_GOSSIP_MENU(7640, creature->GetGUID());
        }
        else
            player->SEND_GOSSIP_MENU(7638, creature->GetGUID());

        return true;
    }

};

/*######
## npc_private_hendel
######*/

enum Hendel
{
    SAY_PROGRESS_1_TER          = 0,
    SAY_PROGRESS_2_HEN          = 1,
    SAY_PROGRESS_3_TER          = 2,
    SAY_PROGRESS_4_TER          = 3,
    EMOTE_SURRENDER             = 4,

    QUEST_MISSING_DIPLO_PT16    = 1324,
    FACTION_HOSTILE             = 168,                      //guessed, may be different

    NPC_SENTRY                  = 5184,                     //helps hendel
    NPC_JAINA                   = 4968,                     //appears once hendel gives up
    NPC_TERVOSH                 = 4967
};

/// @todo develop this further, end event not created
class npc_private_hendel : public CreatureScript
{
public:
    npc_private_hendel() : CreatureScript("npc_private_hendel") { }

    bool OnQuestAccept(Player* /*player*/, Creature* creature, const Quest* quest)
    {
        if (quest->GetQuestId() == QUEST_MISSING_DIPLO_PT16)
            creature->setFaction(FACTION_HOSTILE);

        return true;
    }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_private_hendelAI(creature);
    }

    struct npc_private_hendelAI : public ScriptedAI
    {
        npc_private_hendelAI(Creature* creature) : ScriptedAI(creature) { }

        void Reset()
        {
            me->RestoreFaction();
        }

        void AttackedBy(Unit* pAttacker)
        {
            if (me->GetVictim())
                return;

            if (me->IsFriendlyTo(pAttacker))
                return;

            AttackStart(pAttacker);
        }

        void DamageTaken(Unit* pDoneBy, uint32 &Damage, DamageEffectType, SpellSchoolMask)
        {
            if (Damage >= me->GetHealth() || me->HealthBelowPctDamaged(20, Damage))
            {
                Damage = 0;

                if (pDoneBy)
                    if (Player* player = pDoneBy->GetCharmerOrOwnerPlayerOrPlayerItself())
                        player->GroupEventHappens(QUEST_MISSING_DIPLO_PT16, me);

                Talk(EMOTE_SURRENDER);
                EnterEvadeMode();
            }
        }
    };

};

/*######
## npc_tervosh
######*/

enum Tervosh
{
    QUEST_MISSING_DIPLO_PT14    = 1265,
    SPELL_PROUDMOORE_DEFENSE    = 7120,
    SAY1                        = 0
};

class npc_archmage_tervosh : public CreatureScript
{
public:
    npc_archmage_tervosh() : CreatureScript("npc_archmage_tervosh") { }

    bool OnQuestReward(Player* player, Creature* creature, const Quest* quest, uint32 /*opt*/)
    {
        if (quest->GetQuestId() == QUEST_MISSING_DIPLO_PT14)
        {
            creature->CastSpell(player, SPELL_PROUDMOORE_DEFENSE);
            creature->AI()->Talk(SAY1);
        }
        return true;
    }

   CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_archmage_tervoshAI(creature);
    }

    struct npc_archmage_tervoshAI : public ScriptedAI
    {
        npc_archmage_tervoshAI(Creature* creature) : ScriptedAI(creature) { }
    };
};

/*######
## npc_zelfrax
######*/

Position const MovePosition = {-2967.030f, -3872.1799f, 35.620f, 0.0f};

enum Zelfrax
{
    SAY_ZELFRAX1     = 0,
    SAY_ZELFRAX2     = 1
};

class npc_zelfrax : public CreatureScript
{
public:
    npc_zelfrax() : CreatureScript("npc_zelfrax") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_zelfraxAI(creature);
    }

    struct npc_zelfraxAI : public ScriptedAI
    {
        npc_zelfraxAI(Creature* creature) : ScriptedAI(creature)
        {
            MoveToDock();
        }

        void AttackStart(Unit* who)
        {
            if (!who)
                return;

            if (me->Attack(who, true))
            {
                me->SetInCombatWith(who);
                who->SetInCombatWith(me);

                if (IsCombatMovementAllowed())
                    me->GetMotionMaster()->MoveChase(who);
            }
        }

        void MovementInform(uint32 Type, uint32 /*Id*/)
        {
            if (Type != POINT_MOTION_TYPE)
                return;

            me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation());
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IMMUNE_TO_PC);
            SetCombatMovement(true);

            if (me->IsInCombat())
                if (Unit* unit = me->GetVictim())
                    me->GetMotionMaster()->MoveChase(unit);
        }

        void MoveToDock()
        {
            SetCombatMovement(false);
            me->GetMotionMaster()->MovePoint(0, MovePosition);
            Talk(SAY_ZELFRAX1);
            Talk(SAY_ZELFRAX2);
        }

        void UpdateAI(uint32 /*Diff*/)
        {
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

};

enum SpellScripts
{
    SPELL_OOZE_ZAP              = 42489,
    SPELL_OOZE_ZAP_CHANNEL_END  = 42485,
    SPELL_OOZE_CHANNEL_CREDIT   = 42486,
    SPELL_ENERGIZED             = 42492,
};

class spell_ooze_zap : public SpellScriptLoader
{
    public:
        spell_ooze_zap() : SpellScriptLoader("spell_ooze_zap") { }

        class spell_ooze_zap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ooze_zap_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_OOZE_ZAP))
                    return false;
                return true;
            }

            SpellCastResult CheckRequirement()
            {
                if (!GetCaster()->HasAura(GetSpellInfo()->Effects[EFFECT_1].CalcValue()))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW; // This is actually correct

                if (!GetExplTargetUnit())
                    return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (GetHitUnit())
                    GetCaster()->CastSpell(GetHitUnit(), uint32(GetEffectValue()), true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_ooze_zap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_ooze_zap_SpellScript::CheckRequirement);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ooze_zap_SpellScript();
        }
};

class spell_ooze_zap_channel_end : public SpellScriptLoader
{
    public:
        spell_ooze_zap_channel_end() : SpellScriptLoader("spell_ooze_zap_channel_end") { }

        class spell_ooze_zap_channel_end_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_ooze_zap_channel_end_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_OOZE_ZAP_CHANNEL_END))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Player* player = GetCaster()->ToPlayer())
                    player->CastSpell(player, SPELL_OOZE_CHANNEL_CREDIT, true);
                Unit::Kill(GetHitUnit(), GetHitUnit());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_ooze_zap_channel_end_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_ooze_zap_channel_end_SpellScript();
        }
};

class spell_energize_aoe : public SpellScriptLoader
{
    public:
        spell_energize_aoe() : SpellScriptLoader("spell_energize_aoe") { }

        class spell_energize_aoe_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_energize_aoe_SpellScript);

            bool Validate(SpellInfo const* /*spellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ENERGIZED))
                    return false;
                return true;
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    if ((*itr)->GetTypeId() == TYPEID_PLAYER && (*itr)->ToPlayer()->GetQuestStatus(GetSpellInfo()->Effects[EFFECT_1].CalcValue()) == QUEST_STATUS_INCOMPLETE)
                        ++itr;
                    else
                        targets.erase(itr++);
                }
                targets.push_back(GetCaster());
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetCaster()->CastSpell(GetCaster(), uint32(GetEffectValue()), true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_energize_aoe_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_energize_aoe_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENTRY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_energize_aoe_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_energize_aoe_SpellScript();
        }
};

void AddSC_dustwallow_marsh()
{
    new npc_lady_jaina_proudmoore();
    new npc_nat_pagle();
    new npc_private_hendel();
    new npc_archmage_tervosh();
    new npc_zelfrax();
    new spell_ooze_zap();
    new spell_ooze_zap_channel_end();
    new spell_energize_aoe();
}
