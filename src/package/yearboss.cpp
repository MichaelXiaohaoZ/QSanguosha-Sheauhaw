#include "yearboss.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "roomscene.h"
#include "special3v3.h"

class YearRuishou : public TriggerSkill
{
public:
    YearRuishou() : TriggerSkill("yearruishou")
    {
        events << DamageForseen;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        bool whether = false;
        if (!damage.from)
            return false;
        foreach (ServerPlayer *p, room->getOtherPlayers(damage.from, false))
            if (p->getKingdom() == damage.from->getKingdom())
                whether = true;
        if (whether && damage.to->getKingdom() != damage.from->getKingdom())
        {
            if (damage.from->hasSkill(objectName()))
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(damage.from, objectName());
                LogMessage rslog;
                rslog.type = "#ruishoulog";
                rslog.from = damage.from;
                rslog.arg = QString::number(damage.damage);
                room->sendLog(rslog);
                return true;
            }
            if (damage.to->hasSkill(objectName()))
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(damage.to, objectName());
                LogMessage rslog;
                rslog.type = "#ruishoulog";
                rslog.from = damage.to;
                rslog.arg = QString::number(damage.damage);
                room->sendLog(rslog);
                return true;
            }
        }
        return false;
    }
};

YearZishuCard::YearZishuCard()
{

}

bool YearZishuCard::targetFixed() const
{
    return true;
}

void YearZishuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    while (true)
    {
        QList<ServerPlayer *> can_targets;
        foreach (ServerPlayer *target, room->getOtherPlayers(source, false))
            if (canUseFromOther(room, source, target))
                can_targets << target;
        if (!can_targets.isEmpty())
        {
            ServerPlayer *targetc =
                    room->askForPlayerChosen(source, can_targets, "yearzishu", QString(), true, true);
            if (targetc)
            {
                int card_id = room->askForCardChosen(source, targetc, "h", "yearzishu");
                CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, source->objectName());
                room->obtainCard(source, Sanguosha->getCard(card_id), reason, false);
                room->setPlayerFlag(source, "YearZishuUsed");
                continue;
            }
        }
        break;
    }

}

bool YearZishuCard::canUseFromOther(Room *room, ServerPlayer *source, ServerPlayer *target) const
{
    return target != NULL && source->getHandcardNum() < target->getHandcardNum() && !target->isKongcheng();
}

class YearZishu : public ZeroCardViewAsSkill
{
public:
    YearZishu() : ZeroCardViewAsSkill("yearzishu")
    {
        inherit_skills << objectName();
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasFlag("YearZishuUsed"))
            return false;
        foreach (const Player *p, player->getSiblings())
            if (p->getHandcardNum() > player->getHandcardNum() && !p->isKongcheng())
                return true;
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &) const
    {
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *) const
    {
        return false;
    }

    virtual const Card *viewAs() const
    {
        return new YearZishuCard;
    }
};

class YearChouniu : public TriggerSkill
{
public:
    YearChouniu() : TriggerSkill("yearchouniu")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
        inherit_skills << objectName();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() != Player::Finish)
            return false;
        bool min = true;
        foreach (ServerPlayer *p, room->getAlivePlayers())
            if (p->getHp() < player->getHp())
            {
                min = false;
                break;
            }
        if (min)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->recover(player, RecoverStruct(player));
        }
        return false;
    }
};

class ChouNiuHpSkill : public TriggerSkill
{
public:
    ChouNiuHpSkill() : TriggerSkill("#chouniuhp")
    {
        events << TurnStart;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return room->getTag("FirstRound").toBool();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        foreach (ServerPlayer *p, room->getAllPlayers())
            if (p->hasSkill(objectName()))
                room->loseHp(p, 4);
        return false;
    }
};

YearYinhuCard::YearYinhuCard()
{

}

void YearYinhuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->setPlayerFlag(source, "YinhuUsed" + Sanguosha->getCard(getEffectiveId())->getType());
    foreach (ServerPlayer *target, targets)
        room->damage(DamageStruct("yearyinhu", source, target));
}

class YearYinhuVS : public OneCardViewAsSkill
{
public:
    YearYinhuVS() : OneCardViewAsSkill("yearyinhu")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("YinhuDying");
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return !Self->isJilei(to_select) && !Self->hasFlag("YinhuUsed"+to_select->getType());
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YearYinhuCard *card = new YearYinhuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class YearYinhu : public TriggerSkill
{
public:
    YearYinhu() : TriggerSkill("yearyinhu")
    {
        events << EnterDying;
        view_as_skill = new YearYinhuVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "yearyinhu")
        {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "YinhuDying");
            }
        }
        return false;
    }
};

class YearMaotu : public ProhibitSkill
{
public:
    YearMaotu() : ProhibitSkill("yearmaotu")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
    {
        if (!to->hasSkill(objectName())) return false;
        if (card->isKindOf("SkillCard")) return false;

        bool samek = false;
        foreach (const Player *other, to->getSiblings())
            if (other->getKingdom() == to->getKingdom())
                samek = true;
        if (samek && from->getKingdom() != to->getKingdom() &&
                from->getHp() >= to->getHp() && to->getRole() != from->getRole())
            return true;

        return false;
    }
};

YearChenlongCard::YearChenlongCard()
{

}

void YearChenlongCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@yearchenlong");
    room->setPlayerMark(card_use.from, "chenlongusing", 1);
    room->loseHp(card_use.from, user_string.toInt());
    room->setPlayerMark(card_use.from, "chenlongusing", 0);
}

void YearChenlongCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    //room->setPlayerMark(source, "#qimou", user_string.toInt())
    foreach (ServerPlayer *target, targets)
        room->damage(DamageStruct(getSkillName(), source, target, user_string.toInt()));
}

class YearChenlongVS : public ZeroCardViewAsSkill
{
public:
    YearChenlongVS() : ZeroCardViewAsSkill("yearchenlong")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@yearchenlong") > 0 && player->getHp() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &) const
    {
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *) const
    {
        return false;
    }

    virtual const Card *viewAs() const
    {
        QString user_string = Self->tag["yearchenlong"].toString();
        if (user_string.isEmpty()) return NULL;
        YearChenlongCard *skill_card = new YearChenlongCard;
        skill_card->setUserString(user_string);
        skill_card->setSkillName("yearchenlong");
        return skill_card;
    }
};

class YearChenlong : public TriggerSkill
{
public:
    YearChenlong() : TriggerSkill("yearchenlong")
    {
        events << EnterDying;
        frequency = Limited;
        limit_mark = "@yearchenlong";
        view_as_skill = new YearChenlongVS;
        inherit_skills << objectName();
    }

    QString getSelectBox() const
    {
        QStringList hp_num;
        for (int i = 1; i <= Self->getHp() && i <= 5; i++)
            hp_num << QString::number(i);
        return hp_num.join("+");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMark("chenlongusing"))
        {
            room->recover(player, RecoverStruct(player, NULL, 1 - player->getHp()));
            room->loseMaxHp(player, player->getMaxHp() - 1);
            room->setPlayerMark(player, "chenlongusing", 0);
        }
        return false;
    }
};

class YearChuancheng : public TriggerSkill
{
public:
    YearChuancheng() : TriggerSkill("yearchuancheng")
    {
        events << BeforeGameOverJudge;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->hasSkill(this);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (player->hasSkill(objectName()) && death.who == player && death.damage && death.damage->from)
        {
            foreach (const Skill* askill, death.damage->from->getSkillList(false, false))
                if (death.damage->from->getMark("chuanchenged" + askill->objectName()))
                {
                    room->detachSkillFromPlayer(death.damage->from, askill->objectName());
                    room->setPlayerMark(death.damage->from, "chuanchenged" + askill->objectName(), 0);
                }
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), death.damage->from->objectName());
            foreach (const Skill* askill, death.damage->to->getSkillList(false, false))
                if (!death.damage->from->getMark("chuanchenged" + askill->objectName()) && !askill->getInheritSkill().empty())
                    foreach (QString inherit_skill, askill->getInheritSkill())
                    {
                        room->acquireSkill(death.damage->from, inherit_skill);
                        room->addPlayerMark(death.damage->from, "chuanchenged" + inherit_skill);
                    }
        }
        return false;
    }
};

class YearSishe : public TriggerSkill
{
public:
    YearSishe() : TriggerSkill("yearsishe")
    {
        events << Damaged;
        frequency = Frequent;
        inherit_skills << objectName();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && damage.from != player && room->askForSkillInvoke(player, objectName()))
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->damage(DamageStruct(objectName(), player, damage.from, damage.damage));
        }
        return false;
    }
};

class YearWuma : public TriggerSkill
{
public:
    YearWuma() : TriggerSkill("yearwuma")
    {
        events << EventPhaseSkipping << TargetConfirmed;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseSkipping)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            return true;
        }
        else
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("TrickCard") && use.to.contains(player))
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                player->drawCards(1);
            }
        }
        return false;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseSkipping) return 6;
        return 3;
    }
};

YearWeiyangCard::YearWeiyangCard()
{
    target_fixed = true;
}

void YearWeiyangCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QList<ServerPlayer *> cantargets;
    foreach (ServerPlayer *p, room->getAlivePlayers())
        if (p->isWounded()) cantargets << p;
    QList<ServerPlayer *> ntargets =
            room->askForPlayersChosen(source, cantargets, "yearweiyang", 1, subcards.length());
    foreach (ServerPlayer *target, ntargets)
        room->recover(target, RecoverStruct(source, this, 1), true);
    room->setPlayerFlag(source, "YearWeiyangUsed");
}

class YearWeiyang : public ViewAsSkill
{
public:
    YearWeiyang() : ViewAsSkill("yearweiyang")
    {
        inherit_skills << objectName();
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        foreach (const Card *selectedcard, selected)
            if (to_select->getType() == selectedcard->getType())
                return false;
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty())
            return NULL;
        YearWeiyangCard *weiyangc = new YearWeiyangCard;
        weiyangc->addSubcards(cards);
        weiyangc->setSkillName(objectName());
        return weiyangc;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->hasFlag("YearWeiyangUsed")) return false;
        if (player->isWounded()) return true;
        foreach (const Player *p, player->getSiblings())
            if (p->isWounded()) return true;
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &) const
    {
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *) const
    {
        return false;
    }
};

class YearShenhou : public TriggerSkill
{
public:
    YearShenhou() : TriggerSkill("yearshenhou")
    {
        events << SlashEffected;
        frequency = Frequent;
        inherit_skills << objectName();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (room->askForSkillInvoke(player, objectName()))
        {
            room->broadcastSkillInvoke(objectName(), player);
            room->sendCompulsoryTriggerLog(player, objectName());
            JudgeStruct judge;
            judge.pattern = ".|red";
            judge.good = true;
            judge.reason = objectName();
            judge.who = player;
            room->judge(judge);
            if (judge.isGood())
            {
                room->sendCompulsoryTriggerLog(player, objectName());
                LogMessage log;
                log.type = "#shenhoueffectlog";
                log.from = player;
                room->sendLog(log);
                effect.to->setFlags("Global_NonSkillNullify");

                return true;
            }
        }
        return false;
    }
};

class YearYouji : public DrawCardsSkill
{
public:
    YearYouji() : DrawCardsSkill("yearyouji")
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        return n + room->getTurn();
    }
};

class YearXugou : public TriggerSkill
{
public:
    YearXugou() : TriggerSkill("yearxugou")
    {
        events << SlashEffected << DamageCaused;
        frequency = Compulsory;
        inherit_skills << objectName() << "#yearxugou-target";
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && TriggerSkill::triggerable(target);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == SlashEffected)
        {
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->isRed()) {
                player->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());

                LogMessage log;
                log.type = "#SkillNullify";
                log.from = player;
                log.arg = objectName();
                log.arg2 = effect.slash->objectName();
                room->sendLog(log);

                effect.to->setFlags("Global_NonSkillNullify");

                return true;
            }
        }
        else
        {
            if (!player->isAlive())
                return false;
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.chain || damage.transfer || !damage.by_user) return false;
            const Card *reason = damage.card;
            if (reason && reason->isKindOf("Slash") && reason->isRed())
            {
                player->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                LogMessage log;
                log.type = "#YearXugouBuff";
                log.from = player;
                log.to << damage.to;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(++damage.damage);
                room->sendLog(log);
                data = QVariant::fromValue(damage);
            }
        }
        return false;
    }
};

class YearXugouTargetMod : public TargetModSkill
{
public:
    YearXugouTargetMod() : TargetModSkill("#yearxugou-target")
    {
    }

    virtual int getDistanceLimit(const Player *from, const Card *card, const Player *) const
    {
        if (from->hasSkill("yearxugou") && card->isRed() && card->isKindOf("Slash"))
            return 1000;
        else
            return 0;
    }
};

class YearHaizhu : public TriggerSkill
{
public:
    YearHaizhu() : TriggerSkill("yearhaizhu")
    {
        events << CardsMoveOneTime << EventPhaseStart;
        frequency = Compulsory;
        inherit_skills << "yearjinzhu" << "#yearjinzhu-draw" << "#yearjinzhu-maxcard";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *caozhi, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime)
        {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == caozhi || move.from == NULL)
                return false;
            if (move.to_place == Player::DiscardPile
                && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
                || move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE))
            {
                QList<int> card_ids;
                int i = 0;
                foreach (int card_id, move.card_ids)
                {
                    if ((Sanguosha->getCard(card_id)->isBlack()
                        && ((move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                        && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                        && move.active_ids.contains(card_id) && room->getDiscardPile().contains(card_id))
                    {
                        card_ids << card_id;
                    }
                    i++;
                }
                if (card_ids.isEmpty())
                    return false;
                else
                {
                    foreach (int id, card_ids) {
                        move.active_ids.removeOne(id);
                    }
                    data = QVariant::fromValue(move);
                    DummyCard *dummy = new DummyCard(card_ids);
                    room->broadcastSkillInvoke(objectName());
                    room->sendCompulsoryTriggerLog(caozhi, objectName());
                    room->obtainCard(caozhi, dummy);
                    delete dummy;
                }
            }
        }
        else
            if (caozhi->getPhase() == Player::Start)
            {
                foreach (ServerPlayer *p, room->getOtherPlayers(caozhi))
                    if (p->getHandcardNum() > caozhi->getHandcardNum())
                        return false;
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(caozhi, objectName());
                room->loseHp(caozhi);
            }
        return false;
    }
};

class YearJiyuan : public TriggerSkill
{
public:
    YearJiyuan() : TriggerSkill("yearjiyuan")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Finish)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->drawCards(player, player->getMaxHp() / 2 + player->getMaxHp()%2, objectName());
        }
        return false;
    }
};

class YearSuizhongEasy : public TriggerSkill
{
public:
    YearSuizhongEasy() : TriggerSkill("yearsuizhongeasy")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@suizhong";
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@suizhong") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return false;

        if (pangtong->askForSkillInvoke(this, data))
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(pangtong, objectName());
            room->removePlayerMark(pangtong, "@suizhong");
            room->recover(pangtong, RecoverStruct(pangtong, NULL, 1 - pangtong->getHp()));
            if (pangtong->getPhase() == Player::NotActive)
                throw TurnBroken;
        }

        return false;
    }
};

class YearSuizhong : public TriggerSkill
{
public:
    YearSuizhong() : TriggerSkill("yearsuizhong")
    {
        events << AskForPeaches;
        frequency = Limited;
        limit_mark = "@suizhong";
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getMark("@suizhong") > 0;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *pangtong, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != pangtong)
            return false;

        if (pangtong->askForSkillInvoke(this, data))
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(pangtong, objectName());
            room->removePlayerMark(pangtong, "@suizhong");
            room->recover(pangtong, RecoverStruct(pangtong, NULL, 1 - pangtong->getHp()));

            foreach (ServerPlayer *player, room->getOtherPlayers(pangtong))
                player->throwAllHandCardsAndEquips();

            if (pangtong->getPhase() == Player::NotActive)
                throw TurnBroken;
        }

        return false;
    }
};

class YearCuikuEasy : public TriggerSkill
{
public:
    YearCuikuEasy() : TriggerSkill("yearcuikueasy")
    {
        events << RoundStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->getTurn()%6 == 1)
        {
            ServerPlayer *target =
                    room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), QString(), true);
            if (target)
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                room->damage(DamageStruct(objectName(), player, target, 2));
            }
        }
    }
};

class YearCuiku : public TriggerSkill
{
public:
    YearCuiku() : TriggerSkill("yearcuiku")
    {
        events << RoundStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->getTurn()%6 == 1)
        {
            QList<ServerPlayer *> targets =
                    room->askForPlayersChosen(player, room->getOtherPlayers(player), objectName(), 0, 2, QString(), true);
            if (!targets.isEmpty())
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                foreach (ServerPlayer *target, targets)
                {
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                    room->damage(DamageStruct(objectName(), player, target, 2));
                }
            }
        }
    }
};

class YearCuikuDifficult : public TriggerSkill
{
public:
    YearCuikuDifficult() : TriggerSkill("yearcuikudifficult")
    {
        events << RoundStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->getTurn()%6 == 1)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            foreach (ServerPlayer *target, room->getOtherPlayers(player))
            {
                room->damage(DamageStruct(objectName(), player, target, target->getMaxHp() / 2));
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
                if (target->getMaxHp()%2)
                    room->drawCards(player, 1, objectName());
            }
        }
    }
};

class YearNianyi : public TriggerSkill
{
public:
    YearNianyi() : TriggerSkill("yearnianyi")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Start)
        {
            QList<const Card *> tricks = player->getJudgingArea();
            if (!tricks.isEmpty())
            {
                int index = qrand()%tricks.length();
                CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), objectName(), QString());
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                room->throwCard(tricks.at(index), reason, player, NULL);
            }
        }
    }
};

class YearNianyiDifficult : public TriggerSkill
{
public:
    YearNianyiDifficult() : TriggerSkill("yearnianyidifficult")
    {
        events << EventPhaseStart << CardsMoveOneTime;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart)
        {
            if (player->getPhase() == Player::Start)
            {
                QList<const Card *> tricks = player->getJudgingArea();
                if (!tricks.isEmpty())
                {
                    int index = qrand()%tricks.length();
                    CardMoveReason reason(CardMoveReason::S_REASON_THROW, player->objectName(), objectName(), QString());
                    room->broadcastSkillInvoke(objectName());
                    room->sendCompulsoryTriggerLog(player, objectName());
                    room->throwCard(tricks.at(index), reason, player, NULL);
                }
            }
        }
        else if (triggerEvent == CardsMoveOneTime)
        {
            if (player->getPhase() == Player::NotActive)
            {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (move.from && move.from == player && (move.from_places.contains(Player::PlaceHand)
                                            || move.from_places.contains(Player::PlaceEquip))
                    && !(move.to && move.to == player && (move.to_place == Player::PlaceHand
                                               || move.to_place == Player::PlaceEquip)))
                {
                    room->addPlayerMark(player, "#nianyi", move.card_ids.length());
                }
            }
        }
    }
};

class YearNianyiDifficultEffect : public PhaseChangeSkill
{
public:
    YearNianyiDifficultEffect() : PhaseChangeSkill("#yearnianyidifficult-effect")
    {

    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() != Player::Finish) return false;
        foreach (ServerPlayer *boss, room->getAllPlayers())
        {
            if (boss->hasSkill(objectName()))
                if (boss->getMark("#nianyi"))
                {
                    if (boss->getMark("#nianyi") > 2 && boss->getPhase() == Player::NotActive)
                    {
                        room->broadcastSkillInvoke("yearnianyidifficult");
                        room->sendCompulsoryTriggerLog(boss, "yearnianyidifficult");
                        foreach (ServerPlayer *p, room->getOtherPlayers(boss))
                            room->damage(DamageStruct("yearnianyidifficult", boss, p));
                    }
                    room->setPlayerMark(boss, "#nianyi", 0);
                }
        }
    }
};

class YearNianyiTargetMod : public TargetModSkill
{
public:
    YearNianyiTargetMod() : TargetModSkill("#yearnianyi-target")
    {
        pattern = ".";
    }

    virtual int getDistanceLimit(const Player *from, const Card *card, const Player *to) const
    {
        if ((from->hasSkill("yearnianyi") || from->hasSkill("yearnianyidifficult")) && to)
            return 10000;
        return 0;
    }
};

FireCracker::FireCracker(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("firecracker");
    target_fixed = true;
}

bool FireCracker::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty();
}

QList<ServerPlayer *> FireCracker::defaultTargets(Room *, ServerPlayer *source) const
{
    return QList<ServerPlayer *>() << source;
}

bool FireCracker::isAvailable(const Player *player) const
{
    return player->getRole() == "rebel" && !player->isProhibited(player, this) && TrickCard::isAvailable(player) && !player->getMark("#firecracker");
}

void FireCracker::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->setEmotion(source, "firecracker");
    //room->setPlayerMark(source, "#firecracker", 1);
    room->addPlayerTip(source, "#firecracker");
}

bool FireCracker::isCancelable(const CardEffectStruct &effect) const
{
    return false;
}

SpringCouplet::SpringCouplet(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("spring_couplet");
    target_fixed = true;
}

bool SpringCouplet::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty();
}

QList<ServerPlayer *> SpringCouplet::defaultTargets(Room *, ServerPlayer *source) const
{
    return QList<ServerPlayer *>() << source;
}

bool SpringCouplet::isAvailable(const Player *player) const
{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void SpringCouplet::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->setEmotion(source, "spring_couplet");
    room->drawCards(source, 2, objectName());
    LogMessage splog;
    splog.type = "#SpringCoupletH";
    splog.from = source;
    room->sendLog(splog);
    LogMessage log;
    log.type = "#RemoveCard";
    log.card_str = this->toString();
    room->sendLog(log);
    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, source->objectName(), QString(), objectName(), QString());
    room->moveCardTo(this, NULL, Player::PlaceTable, reason, true);
}

bool SpringCouplet::isCancelable(const CardEffectStruct &effect) const
{
    return false;
}

class FireCrackerProhibit : public ProhibitSkill
{
public:
    FireCrackerProhibit() : ProhibitSkill("firecrack-prohibit")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        return to->getMark("#firecracker") && from->getRole() != to->getRole() && !from->getMark("isyearboss");
    }
};

class FireCrackerSkill : public TriggerSkill
{
public:
    FireCrackerSkill() : TriggerSkill("firecrackskill")
    {
        events << DamageForseen << TurnStart;
        frequency = Compulsory;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == DamageForseen)
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.to && damage.to->getMark("#firecracker") && damage.from && damage.from->getMark("isyearboss"))
            {
                LogMessage log;
                log.type = "#firecrackerlog";
                log.from = damage.to;
                log.to.append(damage.from);
                log.arg = QString::number(damage.damage);
                room->sendLog(log);
                return true;
            }
        }
        else
        {
            if (player->getMark("#firecracker"))
                room->setPlayerMark(player, "#firecracker", 0);
        }
        return false;
    }
};

class YearBossRevive : public TriggerSkill
{
public:
    YearBossRevive() : TriggerSkill("yearbossrevive")
    {
        events << RoundStart;
        frequency = Compulsory;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        if (room->getMode() != "04_year" || Config.value("year/Mode", "2018").toString() != "2018")
            return false;
        foreach (ServerPlayer *p, room->getAllPlayers(true))
            if (p->getMark("isyearboss"))
                return false;
        return true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        foreach (ServerPlayer *p, room->getAllPlayers(true))
            if (p->isDead() && p->getRole() == "rebel")
            {
                room->revivePlayer(p);
                room->drawCards(p, 3);
                room->addPlayerMark(p, "yearbossrevived", 1);
                room->setPlayerProperty(p, "hp", p->getMaxHp());
            }
        return false;
    }
};

class YearBossChange : public TriggerSkill
{
public :
    YearBossChange() : TriggerSkill("yearbosschange")
    {
        events << RoundStart;
        global = true;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (room->getTurn() < 7 || room->getMode() != "04_year" || Config.value("year/Mode", "2018").toString() != "2018")
            return false;
        foreach (ServerPlayer *sp, room->getAllPlayers())
            if (sp->getMark("isyearboss"))
                return false;
        room->appearYearBoss(0);
        return false;
    }
};

class GanluYear : public TriggerSkill
{
public:
    GanluYear() : TriggerSkill("#ganluyear")
    {
        events << AskForPeachesDone;
        frequency = Compulsory;
        global = true;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL && room->getMode() == "04_year";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!player->hasFlag("Global_Dying") || player->getHp() > 0) return false;
        QString roleshould;
        int maxseat;
        if (Config.value("year/Mode", "2018").toString() == "2018")
        {
            roleshould = "rebel";
            maxseat = 6;
        }
        else if (Config.value("year/Mode", "2018").toString() == "2019G")
        {
            roleshould = "rebel";
            maxseat = 2;
        }
        else if (Config.value("year/Mode", "2018").toString() == "2019Y")
        {
            roleshould = "loyalist";
            maxseat = 4;
        }
        if (player->getRealSeat() > maxseat || player->getRole() != roleshould) return false;
        int revive = 0;
        foreach (ServerPlayer *sp, room->getAllPlayers(true))
            revive += sp->getMark("#ganlu");
        if (revive > 2) return false;
        for (int i = 1; i <= maxseat; i++)
            foreach (ServerPlayer *rebel, room->getAllPlayers())
                if (rebel->getRole() == roleshould && rebel->getRealSeat() == i)
                {
                    if (room->askForSkillInvoke(rebel, "ganlu"))
                    {
                        room->doGanluRevive(player, player);
                        return false;
                    }
                    break;
                }
        return false;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        return 0;
    }

};

class YearMaotu19 : public TriggerSkill
{
public:
    YearMaotu19() : TriggerSkill("yearmaotu_19")
    {
        events << Death << TurnStart;
        frequency = Compulsory;
        inherit_skills << objectName() << "#yearmaotu-prohibit";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->addPlayerTip(player, "#yearmaotu");
        }
        else
            room->removePlayerTip(player, "#yearmaotu");
        return false;
    }
};

class YearMaotu19Prohibit : public ProhibitSkill
{
public:
    YearMaotu19Prohibit() : ProhibitSkill("#yearmaotu-prohibit")
    {

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &others) const
    {
        return !card->isKindOf("SkillCard") && to->getMark("#yearmaotu") && from->getHp() >= to->getMaxHp();
    }
};

class YearYouji19 : public DrawCardsSkill
{
public:
    YearYouji19() : DrawCardsSkill("yearyouji_19")
    {
        frequency = Compulsory;
        inherit_skills << objectName();
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        if (room->getTurn() > 5)
            return n + 5;
        return n + room->getTurn();
    }
};

class YearJinzhu : public TriggerSkill
{
public:
    YearJinzhu() : TriggerSkill("yearjinzhu")
    {
        events << EnterDying;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        room->recover(player, RecoverStruct(player, NULL, 3 - player->getHp()));
        room->detachSkillFromPlayer(player, objectName());
        return false;
    }
};

class YearJinzhuDraw : public DrawCardsSkill
{
public:
    YearJinzhuDraw() : DrawCardsSkill("#yearjinzhu-draw")
    {

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke("yearjinzhu");
        room->sendCompulsoryTriggerLog(player, "yearjinzhu");
        return n + 1;
    }
};

class YearJinzhuDrawMaxCard : public MaxCardsSkill
{
public:
    YearJinzhuDrawMaxCard() : MaxCardsSkill("#yearjinzhu-maxcard")
    {

    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill(this))
            return 1;
        return 0;
    }
};

class YearWuma19 : public TriggerSkill
{
public:
    YearWuma19() : TriggerSkill("yearwuma_19")
    {
        events << EventPhaseSkipping << TargetConfirmed;
        frequency = Compulsory;
        inherit_skills << objectName();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseSkipping)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            return true;
        }
        else
        {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("TrickCard") && use.to.contains(player) &&
                    !(use.from && use.from->objectName() == player->objectName()))
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                player->drawCards(1);
            }
        }
        return false;
    }

    virtual int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == EventPhaseSkipping) return 6;
        return 3;
    }
};

class YearYangshou : public TriggerSkill
{
public:
    YearYangshou() : TriggerSkill("yearyangshou")
    {
        events << TurnedOver << DamageForseen;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TurnedOver)
        {
            if (player->faceUp()) return false;
            foreach (ServerPlayer *sp, room->getOtherPlayers(player))
                if (sp->getGeneralName().contains("boss_year_yin") || sp->getGeneral2Name().contains("boss_year_yin"))
                    if (!sp->faceUp())
                    {
                        room->broadcastSkillInvoke(objectName());
                        room->sendCompulsoryTriggerLog(player, objectName());
                        sp->turnOver();
                    }
        }
        else
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Fire) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                LogMessage log;
                log.type = "#YangshouProtect";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = objectName();
                room->sendLog(log);

                return true;
            }
        }
        return false;
    }
};

class YearYangshouDraw : public DrawCardsSkill
{
public:
    YearYangshouDraw() : DrawCardsSkill("#yearyangshou-draw")
    {

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke("yearyangshou");
        room->sendCompulsoryTriggerLog(player, "yearyangshou");
        return n + 2;
    }
};

class YearBeimingYang : public TriggerSkill
{
public:
    YearBeimingYang() : TriggerSkill("yearbeiming_yang")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return false;

        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        foreach (ServerPlayer *sp, room->getAlivePlayers())
            if (sp->getRole() == "rebel")
                room->damage(DamageStruct(objectName(), player, sp, 1, DamageStruct::Fire));
        return false;
    }
};

YearNuyanCard::YearNuyanCard()
{
    target_fixed = true;
}

void YearNuyanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->loseHp(source, 1);
    room->addPlayerTip(source, "#yearnuyan");
}

class YearNuyan : public OneCardViewAsSkill
{
public:
    YearNuyan() : OneCardViewAsSkill("yearnuyan")
    {
        filter_pattern = ".|red|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getMark("#yearnuyan");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YearNuyanCard *card = new YearNuyanCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class YearNuyanEffect : public TriggerSkill
{
public:
    YearNuyanEffect() : TriggerSkill("#yearnuyan-effect")
    {
        events << DamageCaused << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target && target->getMark("#yearnuyan");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging)
        {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->removePlayerTip(player, "#yearnuyan");
        }
        else
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card->isRed() && !damage.chain && !damage.transfer)
            {
                damage.nature = DamageStruct::Fire;
                room->broadcastSkillInvoke("yearnuyan");
                room->sendCompulsoryTriggerLog(damage.from, "yearnuyan");
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, damage.from->objectName(), damage.to->objectName());
                LogMessage log;
                log.type = "#YearNuyanBuff";
                log.from = player;
                log.to << damage.to;
                log.arg = QString::number(damage.damage);
                log.arg2 = QString::number(++damage.damage);
                room->sendLog(log);
                data = QVariant::fromValue(damage);
            }
        }

    }
};

class YearHundunYang : public TriggerSkill
{
public:
    YearHundunYang() : TriggerSkill("yearhundun_yang")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMark("yearhundun") || player->getPhase() != Player::Play) return false;
        foreach (ServerPlayer *sp, room->getOtherPlayers(player))
            if (sp->getGeneralName().contains("boss_year_yin") || sp->getGeneral2Name().contains("boss_year_yin"))
                return false;

        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "yearhundun", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            room->acquireSkill(player, "yearhuihun");
        }
        return false;
    }
};

class YearYinshou : public TriggerSkill
{
public:
    YearYinshou() : TriggerSkill("yearyinshou")
    {
        events << TurnedOver << DamageForseen << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TurnedOver)
        {
            if (player->faceUp()) return false;
            foreach (ServerPlayer *sp, room->getOtherPlayers(player))
                if (sp->getGeneralName().contains("boss_year_yang") || sp->getGeneral2Name().contains("boss_year_yang"))
                    if (!sp->faceUp())
                    {
                        room->broadcastSkillInvoke(objectName());
                        room->sendCompulsoryTriggerLog(player, objectName());
                        sp->turnOver();
                    }
        }
        else if (triggerEvent == EventPhaseStart)
        {
            if (player->getPhase() == Player::Finish)
            {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                room->drawCards(player, 2, objectName());
            }
        }
        else
        {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.nature == DamageStruct::Thunder) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(player, objectName());
                LogMessage log;
                log.type = "#YinshouProtect";
                log.from = player;
                log.arg = QString::number(damage.damage);
                log.arg2 = objectName();
                room->sendLog(log);

                return true;
            }
        }
        return false;
    }

};

class YearBeimingYin : public TriggerSkill
{
public:
    YearBeimingYin() : TriggerSkill("yearbeiming_yin")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return false;
        room->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(player, objectName());
        foreach (ServerPlayer *sp, room->getAlivePlayers())
            if (sp->getRole() == "rebel")
                room->damage(DamageStruct(objectName(), player, sp, 1, DamageStruct::Thunder));
        return false;
    }
};

YearHuihunCard::YearHuihunCard()
{
}

void YearHuihunCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->loseHp(source, 1);
    foreach (ServerPlayer *hus, targets)
    room->recover(hus, RecoverStruct(source, NULL, 2), true);
    room->setPlayerFlag(source, "yearhuihun");
}

bool YearHuihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->isWounded();
}

class YearHuihun : public OneCardViewAsSkill
{
public:
    YearHuihun() : OneCardViewAsSkill("yearhuihun")
    {
        filter_pattern = ".|black|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("yearhuihun");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YearHuihunCard *card = new YearHuihunCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class YearHundunYin : public TriggerSkill
{
public:
    YearHundunYin() : TriggerSkill("yearhundun_yin")
    {
        events << EventPhaseStart;
        frequency = Wake;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMark("yearhundun") || player->getPhase() != Player::Play) return false;
        foreach (ServerPlayer *sp, room->getOtherPlayers(player))
            if (sp->getGeneralName().contains("boss_year_yang") || sp->getGeneral2Name().contains("boss_year_yang"))
                return false;

        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());

        room->setPlayerMark(player, "yearhundun", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            room->acquireSkill(player, "yearnuyan");
        }
        return false;
    }
};

YearBuhuoCard::YearBuhuoCard()
{

}

void YearBuhuoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *sp, targets)
    {
        int rchance = qrand() % 10000, chance = 1000 * (1 + Config.GeneralLevel) * (4 - sp->getHp());
        LogMessage chancelog;
        chancelog.type = "#chance-buhuo";
        chancelog.arg = QString::number(rchance / 100);
        chancelog.arg2 = QString::number(chance / 100);
        room->sendLog(chancelog);
        room->setPlayerMark(source, "@buhuo", 0);
        if (chance > rchance)
        {
            room->setPlayerMark(sp, "buhuodead", 1);
            room->killPlayer(sp);
            room->setPlayerProperty(sp, "hp", 0);
        }
        else
        {
            if (sp->getGeneralName().contains("boss_year_yin") || sp->getGeneral2Name().contains("boss_year_yin"))
            {
                foreach (ServerPlayer *p, room->getAllPlayers(true))
                    if (p->isDead())
                    {
                        room->revivePlayer(p, true);
                        room->broadcastProperty(p, "hp", QString::number(1));
                    }
                room->recover(sp, RecoverStruct(sp, NULL, 1));
            }
            if (sp->getGeneralName().contains("boss_year_yang") || sp->getGeneral2Name().contains("boss_year_yang"))
            {
                foreach (ServerPlayer *p, room->getAlivePlayers())
                    if (p->getRole() == "rebel")
                        room->damage(DamageStruct(objectName(), sp, p, 1, DamageStruct::Fire));
                room->recover(sp, RecoverStruct(sp, NULL, 1));
                room->drawCards(sp, 2);
                sp->gainAnImmediateTurn();
            }
        }
    }
}

bool YearBuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *sp, const Player *Self) const
{ return targets.isEmpty() && sp->getHp() < 4 &&
            (sp->getGeneralName().contains("boss_year_yang") || sp->getGeneral2Name().contains("boss_year_yang") ||
             sp->getGeneralName().contains("boss_year_yin") || sp->getGeneral2Name().contains("boss_year_yin"));
}

class YearBuhuo : public ZeroCardViewAsSkill
{
public:
    YearBuhuo() : ZeroCardViewAsSkill("yearbuhuo")
    {
        frequency = Limited;
        limit_mark = "@buhuo";
    }

    virtual const Card *viewAs() const
    {
        return new YearBuhuoCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (player->getMark("@buhuo") < 1) return false;
        foreach (const Player *p, player->getAliveSiblings())
            if (p->getHp() < 4 &&
                (p->getGeneralName().contains("boss_year_yang") || p->getGeneral2Name().contains("boss_year_yang") ||
                 p->getGeneralName().contains("boss_year_yin") || p->getGeneral2Name().contains("boss_year_yin")))
                return true;
        return false;
    }
};

FireCracker19::FireCracker19(Card::Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("firecracker19");
    can_recast = true;
}

bool FireCracker19::targetFilter(const QList<const Player *> &targets, const Player *sp, const Player *Self) const
{
    return targets.isEmpty() &&
            (sp->getGeneralName().contains("boss_year_yang") || sp->getGeneral2Name().contains("boss_year_yang") ||
             sp->getGeneralName().contains("boss_year_yin") || sp->getGeneral2Name().contains("boss_year_yin"));
}

bool FireCracker19::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    bool rec = (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) && can_recast;
    QList<int> sub;
    if (isVirtualCard())
        sub = subcards;
    else
        sub << getEffectiveId();
    foreach (int id, sub) {
        if (Self->getHandPile().contains(id)) {
            rec = false;
            break;
        }
    }

    if (rec && Self->isCardLimited(this, Card::MethodUse))
        return targets.length() == 0;
    if (!rec)
        return targets.length() > 0 && targets.length() <= 1;
    else
        return targets.length() <= 1;
}

bool FireCracker19::isAvailable(const Player *player) const
{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void FireCracker19::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty()) {
        CardMoveReason reason(CardMoveReason::S_REASON_RECAST, source->objectName());
        reason.m_skillName = this->getSkillName();
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);
        if (this->isVirtualCard())
            source->broadcastSkillInvoke(this->getSkillName());
        else
            source->broadcastSkillInvoke("@recast");

        LogMessage log;
        log.type = "#UseCard_Recast";
        log.from = source;
        log.card_str = this->toString();
        room->sendLog(log);

        source->drawCards(1, "recast");
        room->addPlayerHistory(NULL, "pushPile");
    }
    else
        foreach (ServerPlayer *sp, targets)
        {
            room->setEmotion(sp, "firecracker");
            int faceup = qrand() % 10000, losehp = qrand() % 10000;
            LogMessage chancelog;
            chancelog.type = "#chance-firecracker";
            chancelog.arg = QString::number(faceup / 100);
            chancelog.arg2 = QString::number(losehp / 100);
            room->sendLog(chancelog);
            if (faceup < 8000)
                sp->turnOver();
            if (losehp < 8000)
                room->loseHp(sp);
        }
}

bool FireCracker19::isCancelable(const CardEffectStruct &effect) const
{
    return false;
}

class YearXiongshou : public TriggerSkill
{
public:
    YearXiongshou() : TriggerSkill("yearxiongshou")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Finish)
        {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->drawCards(player, 1, objectName());
        }
        return false;
    }
};

class YearXiongshouDraw : public DrawCardsSkill
{
public:
    YearXiongshouDraw() : DrawCardsSkill("#yearxiongshou-draw")
    {

    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        Room *room = player->getRoom();
        room->broadcastSkillInvoke("yearxiongshou");
        room->sendCompulsoryTriggerLog(player, "yearxiongshou");
        return n + 1;
    }
};

class YearXisheng : public TriggerSkill
{
public:
    YearXisheng() : TriggerSkill("yearxisheng")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target, Room *room) const
    {
        return target != NULL && target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who != player) return false;
        foreach (ServerPlayer *sp, room->getOtherPlayers(player))
            if (sp->getRole() != "rebel")
            {
                bool recover = qrand()%2;
                if (sp->isWounded() && recover)
                    room->recover(sp, RecoverStruct(player, NULL, 1));
                else
                    room->drawCards(sp, 1, objectName());
            }
        return false;
    }
};

class YearChenlong19VS : public ZeroCardViewAsSkill
{
public:
    YearChenlong19VS() : ZeroCardViewAsSkill("yearchenlong_19")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@yearchenlong") > 0 && player->getHp() > 0;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &) const
    {
        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *) const
    {
        return false;
    }

    virtual const Card *viewAs() const
    {
        QString user_string = Self->tag["yearchenlong_19"].toString();
        if (user_string.isEmpty()) return NULL;
        YearChenlongCard *skill_card = new YearChenlongCard;
        skill_card->setUserString(user_string);
        skill_card->setSkillName("yearchenlong_19");
        return skill_card;
    }
};

class YearChenlong19 : public TriggerSkill
{
public:
    YearChenlong19() : TriggerSkill("yearchenlong_19")
    {
        events << EnterDying;
        frequency = Limited;
        limit_mark = "@yearchenlong";
        view_as_skill = new YearChenlong19VS;
        inherit_skills << objectName();
    }

    QString getSelectBox() const
    {
        QStringList hp_num;
        for (int i = 1; i <= Self->getHp() && i <= 5; i++)
            hp_num << QString::number(i);
        return hp_num.join("+");
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMark("chenlongusing"))
        {
            room->recover(player, RecoverStruct(player, NULL, 1 - player->getHp()));
            room->loseMaxHp(player, 1);
            room->setPlayerMark(player, "chenlongusing", 0);
        }
        return false;
    }
};

class YearYinhu19VS : public OneCardViewAsSkill
{
public:
    YearYinhu19VS() : OneCardViewAsSkill("yearyinhu_19")
    {

    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("YinhuDying");
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return !Self->isJilei(to_select) && !Self->hasFlag("YinhuUsed"+to_select->getType());
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        YearYinhuCard *card = new YearYinhuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class YearYinhu19 : public TriggerSkill
{
public:
    YearYinhu19() : TriggerSkill("yearyinhu_19")
    {
        events << EnterDying;
        view_as_skill = new YearYinhu19VS;
        inherit_skills << objectName();
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "yearyinhu_19")
        {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "YinhuDying");
            }
        }
        return false;
    }
};

YearBoss18Package::YearBoss18Package()
    : Package("YearBoss2018")
{
    General *zishu = new General(this, "bosszishu", "god", 3, true, true);
    zishu->addSkill(new YearZishu);
    zishu->addSkill(new YearRuishou);

    General *chouniu = new General(this, "bosschouniu", "god", 5, true, true);
    chouniu->addSkill(new YearChouniu);
    chouniu->addSkill(new ChouNiuHpSkill);
    chouniu->addSkill("yearruishou");

    General *yinhu = new General(this, "bossyinhu", "god", 4, true, true);
    yinhu->addSkill(new YearYinhu);
    yinhu->addSkill("yearruishou");

    General *maotu = new General(this, "bossmaotu", "god", 3, false, true);
    maotu->addSkill(new YearMaotu);
    maotu->addSkill("yearruishou");

    General *chenlong = new General(this, "bosschenlong", "god", 4, true, true);
    chenlong->addSkill(new YearChenlong);
    chenlong->addSkill(new YearChuancheng);
    chenlong->addSkill("yearruishou");

    General *sishe = new General(this, "bosssishe", "god", 3, false, true);
    sishe->addSkill(new YearSishe);
    sishe->addSkill("yearruishou");

    General *wuma = new General(this, "bosswuma", "god", 4, true, true);
    wuma->addSkill(new YearWuma);
    wuma->addSkill("yearruishou");

    General *weiyang = new General(this, "bossweiyang", "god", 3, true, true);
    weiyang->addSkill(new YearWeiyang);
    weiyang->addSkill("yearchuancheng");
    weiyang->addSkill("yearruishou");

    General *fe_weiyang = new General(this, "fe_bossweiyang", "god", 3, false, true);
    fe_weiyang->addSkill("yearweiyang");
    fe_weiyang->addSkill("yearchuancheng");
    fe_weiyang->addSkill("yearruishou");

    General *shenhou = new General(this, "bossshenhou", "god", 3, true, true);
    shenhou->addSkill(new YearShenhou);
    shenhou->addSkill("yearchuancheng");
    shenhou->addSkill("yearruishou");

    General *youji = new General(this, "bossyouji", "god", 3, true, true);
    youji->addSkill(new YearYouji);
    youji->addSkill("yearruishou");

    General *xugou = new General(this, "bossxugou", "god", 4, true, true);
    xugou->addSkill(new YearXugou);
    xugou->addSkill(new YearXugouTargetMod);
    xugou->addSkill("yearchuancheng");
    xugou->addSkill("yearruishou");

    General *haizhu = new General(this, "bosshaizhu", "god", 5, true, true);
    haizhu->addSkill(new YearHaizhu);
    haizhu->addSkill("yearruishou");

    General *easyyear = new General(this, "easy_boss_year", "god", 6, true, true);
    easyyear->addSkill(new YearJiyuan);
    easyyear->addSkill(new YearSuizhongEasy);
    easyyear->addSkill(new YearCuikuEasy);

    General *year = new General(this, "boss_year", "god", 8, true, true);
    year->addSkill("yearjiyuan");
    year->addSkill(new YearNianyi);
    year->addSkill(new YearNianyiTargetMod);
    year->addSkill(new YearSuizhong);
    year->addSkill(new YearCuiku);

    General *diyear = new General(this, "difficult_boss_year", "god", 10, true, true);
    diyear->addSkill("yearjiyuan");
    diyear->addSkill(new YearNianyiDifficult);
    diyear->addSkill(new YearNianyiDifficultEffect);
    diyear->addSkill("#yearnianyi-target");
    diyear->addSkill("yearsuizhong");
    diyear->addSkill(new YearCuikuDifficult);

    addMetaObject<YearZishuCard>();
    addMetaObject<YearYinhuCard>();
    addMetaObject<YearChenlongCard>();
    addMetaObject<YearWeiyangCard>();
}

YearBoss18CardPackage::YearBoss18CardPackage()
    : Package("YearBoss2018Card", Package::CardPack)
{
    QList<Card *> cards;

    cards << new FireCracker(Card::Spade, 1)
          << new FireCracker(Card::Club, 1)
          << new FireCracker(Card::Heart, 1)
          << new FireCracker(Card::Diamond, 1);

    cards << new SpringCouplet(Card::Heart, 13)
          << new SpringCouplet(Card::Diamond, 13);

    cards << new Duel(Card::Spade, 1)
          << new Duel(Card::Heart, 1)
          << new Duel(Card::Heart, 12)
          << new Duel(Card::Diamond, 12);

    cards << new FireAttack(Card::Spade, 7)
          << new FireAttack(Card::Spade, 13)
          << new FireAttack(Card::Club, 7);

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new GanluYear;

    skills << new FireCrackerProhibit << new FireCrackerSkill << new YearBossRevive << new YearBossChange;
}

YearBoss19Package::YearBoss19Package()
    : Package("YearBoss2019")
{
    General *zishu = new General(this, "boss19zishu", "qun", 3, true, true);
    zishu->addSkill("yearzishu");
    zishu->addSkill("yearchuancheng");

    General *chouniu = new General(this, "boss19chouniu", "wei", 5, true, true);
    chouniu->addSkill("yearchouniu");
    chouniu->addSkill("#chouniuhp");
    chouniu->addSkill("yearchuancheng");

    General *yinhu = new General(this, "boss19yinhu", "wu", 4, true, true);
    yinhu->addSkill(new YearYinhu19);
    yinhu->addSkill("yearchuancheng");

    General *maotu = new General(this, "boss19maotu", "wei", 3, false, true);
    maotu->addSkill(new YearMaotu19);
    maotu->addSkill(new YearMaotu19Prohibit);
    maotu->addSkill("yearchuancheng");

    General *chenlong = new General(this, "boss19chenlong", "shu", 4, true, true);
    chenlong->addSkill(new YearChenlong19);
    chenlong->addSkill("yearchuancheng");

    General *sishe = new General(this, "boss19sishe", "shu", 3, false, true);
    sishe->addSkill("yearsishe");
    sishe->addSkill("yearchuancheng");

    General *wuma = new General(this, "boss19wuma", "wu", 4, true, true);
    wuma->addSkill(new YearWuma19);
    wuma->addSkill("yearchuancheng");

    General *weiyang = new General(this, "boss19weiyang", "wei", 3, true, true);
    weiyang->addSkill("yearweiyang");
    weiyang->addSkill("yearchuancheng");

    General *fe_weiyang = new General(this, "fe_boss19weiyang", "wei", 3, false, true);
    fe_weiyang->addSkill("yearweiyang");
    fe_weiyang->addSkill("yearchuancheng");

    General *shenhou = new General(this, "boss19shenhou", "wei", 3, true, true);
    shenhou->addSkill("yearshenhou");
    shenhou->addSkill("yearchuancheng");

    General *youji = new General(this, "boss19youji", "wei", 3, true, true);
    youji->addSkill(new YearYouji19);
    youji->addSkill("yearchuancheng");

    General *xugou = new General(this, "boss19xugou", "shu", 4, true, true);
    xugou->addSkill("yearxugou");
    xugou->addSkill("#yearxugou-target");
    xugou->addSkill("yearchuancheng");

    General *haizhu = new General(this, "boss19haizhu", "qun", 5, true, true);
    haizhu->addSkill("yearhaizhu");
    haizhu->addSkill("yearchuancheng");
    haizhu->addRelateSkill("yearjinzhu");

    General *yang = new General(this, "boss_year_yang", "god", 6, true, true);
    yang->addSkill(new YearYangshou);
    yang->addSkill(new YearYangshouDraw);
    yang->addSkill(new YearBeimingYang);
    yang->addSkill(new YearNuyan);
    yang->addSkill(new YearNuyanEffect);
    yang->addSkill(new YearHundunYang);
    yang->addRelateSkill("yearhuihun");

    General *yin = new General(this, "boss_year_yin", "god", 6, false, true);
    yin->addSkill(new YearYinshou);
    yin->addSkill(new YearBeimingYin);
    yin->addSkill(new YearHuihun);
    yin->addSkill(new YearHundunYin);
    yin->addRelateSkill("yearnuyan");

    General *pucong = new General(this, "year_pucong", "qun", 2, true, true);
    pucong->addSkill(new YearXiongshou);
    pucong->addSkill(new YearXiongshouDraw);
    pucong->addSkill(new YearXisheng);

    skills << new YearJinzhu << new YearJinzhuDraw << new YearJinzhuDrawMaxCard << new YearBuhuo;

    addMetaObject<YearHuihunCard>();
    addMetaObject<YearNuyanCard>();
    addMetaObject<YearBuhuoCard>();
}

YearBoss19CardPackage::YearBoss19CardPackage()
    : Package("YearBoss2019Card", Package::CardPack)
{
    QList<Card *> cards;

    cards << new FireCracker19(Card::Heart, 1)
          << new FireCracker19(Card::Heart, 6)
          << new FireCracker19(Card::Heart, 8);

    cards << new SpringCouplet(Card::Diamond, 1)
          << new SpringCouplet(Card::Diamond, 13);

    foreach (Card *card, cards)
        card->setParent(this);
}

ADD_PACKAGE(YearBoss18)
ADD_PACKAGE(YearBoss18Card)
ADD_PACKAGE(YearBoss19)
ADD_PACKAGE(YearBoss19Card)
