#include "jsp.h"
#include "sp.h"
#include "client.h"
#include "general.h"
#include "skill.h"
#include "standard-skillcards.h"
#include "engine.h"
#include "maneuvering.h"
#include "json.h"
#include "settings.h"
#include "clientplayer.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"

class CihuaiVS : public ZeroCardViewAsSkill
{
public:
    CihuaiVS() : ZeroCardViewAsSkill("cihuai")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("@cihuai") > 0;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern == "slash" && player->getMark("@cihuai") > 0;
    }

    const Card *viewAs() const
    {
        Slash *slash = new Slash(Card::NoSuit, 0);
        slash->setSkillName("_" + objectName());
        return slash;
    }
};

class Cihuai : public TriggerSkill
{
public:
    Cihuai() : TriggerSkill("cihuai")
    {
        events << EventPhaseStart << CardsMoveOneTime << Death;
        view_as_skill = new CihuaiVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && (target->hasSkill(this) || target->getMark("ViewAsSkill_cihuaiEffect") > 0);
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play && !player->isKongcheng() && TriggerSkill::triggerable(player) && player->askForSkillInvoke(this, data)) {
                room->showAllCards(player);
                bool flag = true;
                foreach (const Card *card, player->getHandcards()) {
                    if (card->isKindOf("Slash")) {
                        flag = false;
                        break;
                    }
                }
                room->setPlayerMark(player, "cihuai_handcardnum", player->getHandcardNum());
                if (flag) {
                    room->broadcastSkillInvoke(objectName());
                    room->setPlayerMark(player, "@cihuai", 1);
                    room->setPlayerMark(player, "ViewAsSkill_cihuaiEffect", 1);
                } else
                    room->broadcastSkillInvoke(objectName());
            }
        } else if (triggerEvent == CardsMoveOneTime) {
            if (player->getMark("@cihuai") > 0 && player->getHandcardNum() != player->getMark("cihuai_handcardnum")) {
                room->setPlayerMark(player, "@cihuai", 0);
                room->setPlayerMark(player, "ViewAsSkill_cihuaiEffect", 0);
            }
        } else if (triggerEvent == Death) {
            room->setPlayerMark(player, "@cihuai", 0);
            room->setPlayerMark(player, "ViewAsSkill_cihuaiEffect", 0);
        }
        return false;
    }
};

class NosDanji : public PhaseChangeSkill
{
public:
    NosDanji() : PhaseChangeSkill("nosdanji")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *guanyu, Room *room) const
    {
        return PhaseChangeSkill::triggerable(guanyu) && guanyu->getMark(objectName()) == 0 && guanyu->getPhase() == Player::Start && guanyu->getHandcardNum() > guanyu->getHp();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (room->getLord() != NULL && room->getLord()->getGeneral() && (room->getLord()->getGeneralName().contains("caocao") || room->getLord()->getGeneral2Name().contains("caocao")))
        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$JspdanjiAnimate");
        room->doSuperLightbox("jsp_guanyu", "jspdanji");
        room->setPlayerMark(target, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(target) && target->getMark(objectName()) > 0)
            room->handleAcquireDetachSkills(target, "mashu");

        return false;
    }
};

class Chixin : public OneCardViewAsSkill
{  // Slash::isSpecificAssignee
public:
    Chixin() : OneCardViewAsSkill("chixin")
    {
        filter_pattern = ".|diamond";
        response_or_use = true;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player);
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "jink" || pattern == "slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        //CardUseStruct::CardUseReason r = Sanguosha->currentRoomState()->getCurrentCardUseReason();
        QString p = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        Card *c = NULL;
        if (p == "jink")
            c = new Jink(Card::SuitToBeDecided, -1);
        else
            c = new Slash(Card::SuitToBeDecided, -1);

        if (c == NULL)
            return NULL;

        c->setSkillName(objectName());
        c->addSubcard(originalCard);
        return c;
    }
};

class ChixinTrigger : public TriggerSkill
{
public:
    ChixinTrigger() : TriggerSkill("chixin")
    {
        events << PreCardUsed << EventPhaseEnd;
        view_as_skill = new Chixin;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent) const
    {
        return 8;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && player->getPhase() == Player::Play) {
                QSet<QString> s = player->property("chixin").toString().split("+").toSet();
                foreach(ServerPlayer *p, use.to)
                    s.insert(p->objectName());

                QStringList l = s.toList();
                room->setPlayerProperty(player, "chixin", l.join("+"));
            }
        } else if (player->getPhase() == Player::Play)
            room->setPlayerProperty(player, "chixin", QString());

        return false;
    }
};

class Suiren : public PhaseChangeSkill
{
public:
    Suiren() : PhaseChangeSkill("suiren")
    {
        frequency = Limited;
        limit_mark = "@suiren";
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPhase() == Player::Start && target->getMark("@suiren") > 0;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        ServerPlayer *p = room->askForPlayerChosen(target, room->getAlivePlayers(), objectName(), "@suiren-draw", true);
        if (p == NULL)
            return false;

        room->broadcastSkillInvoke(objectName());
        room->doSuperLightbox("jsp_zhaoyun", "suiren");
        room->setPlayerMark(target, "@suiren", 0);
        
        room->handleAcquireDetachSkills(target, "-yicong");
        int maxhp = target->getMaxHp() + 1;
        room->setPlayerProperty(target, "maxhp", maxhp);
        room->recover(target, RecoverStruct());

        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), p->objectName());
        p->drawCards(3, objectName());

        return false;
    }
};

class Yannyuu : public TriggerSkill
{
public:
    Yannyuu() : TriggerSkill("yannyuu")
    {
        events << EventPhaseStart << BeforeCardsMove << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            ServerPlayer *xiahou = room->findPlayerBySkillName(objectName());
            if (xiahou && player->getPhase() == Player::Play) {
                if (!xiahou->canDiscard(xiahou, "he")) return false;
                const Card *card = room->askForCard(xiahou, "..", "@yannyuu-discard", QVariant(), objectName());
                if (card) {
                    room->broadcastSkillInvoke(objectName());
                    xiahou->addMark("YannyuuDiscard" + QString::number(card->getTypeId()), 3);
                }
            }
        } else if (triggerEvent == BeforeCardsMove && TriggerSkill::triggerable(player)) {
            ServerPlayer *current = room->getCurrent();
            if (!current || current->getPhase() != Player::Play) return false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to_place == Player::DiscardPile) {
                QList<int> ids, disabled;
                QList<int> all_ids = move.card_ids;
                foreach (int id, move.card_ids) {
                    const Card *card = Sanguosha->getCard(id);
                    if (player->getMark("YannyuuDiscard" + QString::number(card->getTypeId())) > 0)
                        ids << id;
                    else
                        disabled << id;
                }
                if (ids.isEmpty()) return false;
                while (!ids.isEmpty()) {
                    room->fillAG(all_ids, player, disabled);
                    bool only = (all_ids.length() == 1);
                    int card_id = -1;
                    if (only)
                        card_id = ids.first();
                    else
                        card_id = room->askForAG(player, ids, true, objectName());
                    room->clearAG(player);
                    if (card_id == -1) break;
                    if (only)
                        player->setMark("YannyuuOnlyId", card_id + 1); // For AI
                    const Card *card = Sanguosha->getCard(card_id);
                    ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(),
                        QString("@yannyuu-give:::%1:%2\\%3").arg(card->objectName())
                        .arg(card->getSuitString() + "_char")
                        .arg(card->getNumberString()),
                        only, true);
                    player->setMark("YannyuuOnlyId", 0);
                    if (target) {
                        player->removeMark("YannyuuDiscard" + QString::number(card->getTypeId()));
                        Player::Place place = move.from_places.at(move.card_ids.indexOf(card_id));
                        QList<int> _card_id;
                        _card_id << card_id;
                        move.removeCardIds(_card_id);
                        data = QVariant::fromValue(move);
                        ids.removeOne(card_id);
                        disabled << card_id;
                        foreach (int id, ids) {
                            const Card *card = Sanguosha->getCard(id);
                            if (player->getMark("YannyuuDiscard" + QString::number(card->getTypeId())) == 0) {
                                ids.removeOne(id);
                                disabled << id;
                            }
                        }
                        if (move.from && move.from->objectName() == target->objectName() && place != Player::PlaceTable) {
                            // just indicate which card she chose...
                            LogMessage log;
                            log.type = "$MoveCard";
                            log.from = target;
                            log.to << target;
                            log.card_str = QString::number(card_id);
                            room->sendLog(log);
                        }

                        room->broadcastSkillInvoke(objectName());
                        target->obtainCard(card);
                    } else
                        break;
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    p->setMark("YannyuuDiscard1", 0);
                    p->setMark("YannyuuDiscard2", 0);
                    p->setMark("YannyuuDiscard3", 0);
                }
            }
        }
        return false;
    }
};

class Xiaode : public TriggerSkill
{
public:
    Xiaode() : TriggerSkill("xiaode")
    {
        events << BuryVictim;
    }

    int getPriority(TriggerEvent) const
    {
        return -2;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
        ServerPlayer *xiahoushi = room->findPlayerBySkillName(objectName());
        if (!xiahoushi || !xiahoushi->tag["XiaodeSkill"].toString().isEmpty()) return false;
        QStringList skill_list = xiahoushi->tag["XiaodeVictimSkills"].toStringList();
        if (skill_list.isEmpty()) return false;
        if (!room->askForSkillInvoke(xiahoushi, objectName(), QVariant::fromValue(skill_list))) return false;
        QString skill_name = room->askForChoice(xiahoushi, objectName(), skill_list.join("+"));
        room->broadcastSkillInvoke(objectName());
        xiahoushi->tag["XiaodeSkill"] = skill_name;
        room->acquireSkill(xiahoushi, skill_name);
        return false;
    }
};

class XiaodeEx : public TriggerSkill
{
public:
    XiaodeEx() : TriggerSkill("#xiaode")
    {
        events << EventPhaseChanging << EventLoseSkill << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                QString skill_name = player->tag["XiaodeSkill"].toString();
                if (!skill_name.isEmpty()) {
                    room->detachSkillFromPlayer(player, skill_name, false, true);
                    player->tag.remove("XiaodeSkill");
                }
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "xiaode") {
            QString skill_name = player->tag["XiaodeSkill"].toString();
            if (!skill_name.isEmpty()) {
                room->detachSkillFromPlayer(player, skill_name, false, true);
                player->tag.remove("XiaodeSkill");
            }
        } else if (triggerEvent == Death && TriggerSkill::triggerable(player)) {
            DeathStruct death = data.value<DeathStruct>();
            QStringList skill_list;
            skill_list.append(addSkillList(death.who->getGeneral()));
            skill_list.append(addSkillList(death.who->getGeneral2()));
            player->tag["XiaodeVictimSkills"] = QVariant::fromValue(skill_list);
        }
        return false;
    }

private:
    QStringList addSkillList(const General *general) const
    {
        if (!general) return QStringList();
        QStringList skill_list;
        foreach (const Skill *skill, general->getSkillList()) {
            if (skill->isVisible() && !skill->isLordSkill() && skill->getFrequency() != Skill::Wake)
                skill_list.append(skill->objectName());
        }
        return skill_list;
    }
};

ShiuehjihCard::ShiuehjihCard()
{
}

bool ShiuehjihCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.length() >= Self->getLostHp())
        return false;

    if (to_select == Self)
        return false;

    int range_fix = 0;
    if (Self->getWeapon() && Self->getWeapon()->getEffectiveId() == getEffectiveId()) {
        const Weapon *weapon = qobject_cast<const Weapon *>(Self->getWeapon()->getRealCard());
        range_fix += weapon->getRange() - Self->getAttackRange(false);
    } else if (Self->getOffensiveHorse() && Self->getOffensiveHorse()->getEffectiveId() == getEffectiveId())
        range_fix += 1;

    return Self->inMyAttackRange(to_select, range_fix);
}

void ShiuehjihCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    DamageStruct damage;
    damage.from = source;
    damage.reason = "shiuehjih";

    foreach (ServerPlayer *p, targets) {
        damage.to = p;
        room->damage(damage);
    }
    foreach (ServerPlayer *p, targets) {
        if (p->isAlive())
            p->drawCards(1, "shiuehjih");
    }
}

class Shiuehjih : public OneCardViewAsSkill
{
public:
    Shiuehjih() : OneCardViewAsSkill("shiuehjih")
    {
        filter_pattern = ".|red!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getLostHp() > 0 && player->canDiscard(player, "he") && !player->hasUsed("ShiuehjihCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        ShiuehjihCard *first = new ShiuehjihCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Huushiaw : public TargetModSkill
{
public:
    Huushiaw() : TargetModSkill("huushiaw")
    {
    }

    int getResidueNum(const Player *from, const Card *) const
    {
        if (from->hasSkill(this))
            return from->getMark(objectName());
        else
            return 0;
    }
};

class HuushiawCount : public TriggerSkill
{
public:
    HuushiawCount() : TriggerSkill("#huushiaw-count")
    {
        events << SlashMissed << EventPhaseChanging;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == SlashMissed) {
            if (player->getPhase() == Player::Play)
                room->addPlayerMark(player, "huushiaw");
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                if (player->getMark("huushiaw") > 0)
                    room->setPlayerMark(player, "huushiaw", 0);
        }

        return false;
    }
};

class HuushiawClear : public DetachEffectSkill
{
public:
    HuushiawClear() : DetachEffectSkill("huushiaw")
    {
    }

    void onSkillDetached(Room *room, ServerPlayer *player) const
    {
        room->setPlayerMark(player, "huushiaw", 0);
    }
};

class Wuujih : public PhaseChangeSkill
{
public:
    Wuujih() : PhaseChangeSkill("wuujih")
    {
        frequency = Wake;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Finish
            && target->getMark("wuujih") == 0
            && target->getMark("damage_point_round") >= 3;
    }

    bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        room->notifySkillInvoked(player, objectName());

        LogMessage log;
        log.type = "#WuujihWake";
        log.from = player;
        log.arg = QString::number(player->getMark("damage_point_round"));
        log.arg2 = objectName();
        room->sendLog(log);

        room->broadcastSkillInvoke(objectName());
        //room->doLightbox("$WuujihAnimate", 4000);

        room->doSuperLightbox("guanyinping", "wuujih");

        room->setPlayerMark(player, "wuujih", 1);
        if (room->changeMaxHpForAwakenSkill(player, 1)) {
            room->recover(player, RecoverStruct(player));
            if (player->getMark("wuujih") == 1)
                room->detachSkillFromPlayer(player, "huushiaw");
        }

        return false;
    }
};

class Shennshyan : public TriggerSkill
{
public:
    Shennshyan() : TriggerSkill("shennshyan")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getPhase() == Player::NotActive && move.from && move.from->isAlive()
            && move.from->objectName() != player->objectName()
            && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
            && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
            foreach (int id, move.card_ids) {
                if (Sanguosha->getCard(id)->getTypeId() == Card::TypeBasic) {
                    if (room->askForSkillInvoke(player, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        player->drawCards(1, "shennshyan");
                    }
                    break;
                }
            }
        }
        return false;
    }
};

JowfuhCard::JowfuhCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool JowfuhCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && to_select->getPile("nostalincantation").isEmpty();
}

void JowfuhCard::use(Room *, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    target->tag["JowfuhSource" + QString::number(getEffectiveId())] = QVariant::fromValue(source);
    target->addToPile("nostalincantation", this);
}

class JowfuhViewAsSkill : public OneCardViewAsSkill
{
public:
    JowfuhViewAsSkill() : OneCardViewAsSkill("jowfuh")
    {
        filter_pattern = ".|.|.|hand";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JowfuhCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        Card *card = new JowfuhCard;
        card->addSubcard(originalcard);
        return card;
    }
};

class Jowfuh : public TriggerSkill
{
public:
    Jowfuh() : TriggerSkill("jowfuh")
    {
        events << StartJudge << EventPhaseChanging;
        view_as_skill = new JowfuhViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPile("nostalincantation").length() > 0;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == StartJudge) {
            int card_id = player->getPile("nostalincantation").first();

            JudgeStruct *judge = data.value<JudgeStruct *>();
            judge->card = Sanguosha->getCard(card_id);

            LogMessage log;
            log.type = "$JowfuhJudge";
            log.from = player;
            log.arg = objectName();
            log.card_str = QString::number(judge->card->getEffectiveId());
            room->sendLog(log);

            room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                CardMoveReason(CardMoveReason::S_REASON_JUDGE,
                judge->who->objectName(),
                QString(), QString(), judge->reason), true);
            judge->updateResult();
            room->setTag("SkipGameRule", true);
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                int id = player->getPile("nostalincantation").first();
                ServerPlayer *zhangbao = player->tag["JowfuhSource" + QString::number(id)].value<ServerPlayer *>();
                if (zhangbao && zhangbao->isAlive())
                    zhangbao->obtainCard(Sanguosha->getCard(id));
            }
        }
        return false;
    }
};

class Yiingbing : public TriggerSkill
{
public:
    Yiingbing() : TriggerSkill("yiingbing")
    {
        events << StartJudge;
        frequency = Frequent;
    }

    int getPriority(TriggerEvent) const
    {
        return -1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();
        int id = judge->card->getEffectiveId();
        ServerPlayer *zhangbao = player->tag["JowfuhSource" + QString::number(id)].value<ServerPlayer *>();
        if (zhangbao && TriggerSkill::triggerable(zhangbao)
            && zhangbao->askForSkillInvoke(this, data)) {
            room->broadcastSkillInvoke(objectName());
            zhangbao->drawCards(2, "yiingbing");
        }
        return false;
    }
};

class Meybuh : public TriggerSkill
{
public:
    Meybuh() : TriggerSkill("meybuh")
    {
        events << EventPhaseStart << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *sunluyu, room->getOtherPlayers(player)) {
                if (!player->inMyAttackRange(sunluyu) && TriggerSkill::triggerable(sunluyu) && room->askForSkillInvoke(sunluyu, objectName())) {
                    room->broadcastSkillInvoke(objectName());
                    if (!player->hasSkill("#meybuh-filter", true)) {
                        room->acquireSkill(player, "#meybuh-filter", false);
                        room->filterCards(player, player->getCards("he"), false);
                    }
                    QVariantList sunluyus = player->tag[objectName()].toList();
                    sunluyus << QVariant::fromValue(sunluyu);
                    player->tag[objectName()] = QVariant::fromValue(sunluyus);
                    room->insertAttackRangePair(player, sunluyu);
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive) return false;

            QVariantList sunluyus = player->tag[objectName()].toList();
            foreach (QVariant sunluyu, sunluyus) {
                ServerPlayer *s = sunluyu.value<ServerPlayer *>();
                room->removeAttackRangePair(player, s);
            }
            room->detachSkillFromPlayer(player, "#meybuh-filter");

            player->tag[objectName()] = QVariantList();

            room->filterCards(player, player->getCards("he"), true);
        }
        return false;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return -2;

        return -1;
    }
};


MeybuhFilter::MeybuhFilter(const QString &skill_name) : FilterSkill(QString("#%1-filter").arg(skill_name)), n(skill_name)
{

}

bool MeybuhFilter::viewFilter(const Card *to_select) const
{
    return to_select->getTypeId() == Card::TypeTrick;
}

const Card * MeybuhFilter::viewAs(const Card *originalCard) const
{
    Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
    slash->setSkillName("_" + n);
    WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
    card->takeOver(slash);
    return card;
}

class Muhmuh : public TriggerSkill
{
public:
    Muhmuh() : TriggerSkill("muhmuh")
    {
        events << EventPhaseStart;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Finish && player->getMark("damage_point_play_phase") == 0) {
            QList<ServerPlayer *> weapon_players, armor_players;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getWeapon() && player->canDiscard(p, p->getWeapon()->getEffectiveId()))
                    weapon_players << p;
                if (p != player && p->getArmor())
                    armor_players << p;
            }
            QStringList choices;
            choices << "cancel";
            if (!armor_players.isEmpty()) choices.prepend("armor");
            if (!weapon_players.isEmpty()) choices.prepend("weapon");
            if (choices.length() == 1) return false;
            QString choice = room->askForChoice(player, objectName(), choices.join("+"));
            if (choice == "cancel") {
                return false;
            } else {
                room->notifySkillInvoked(player, objectName());
                if (choice == "weapon") {
                    room->broadcastSkillInvoke(objectName());
                    ServerPlayer *victim = room->askForPlayerChosen(player, weapon_players, objectName(), "@muhmuh-weapon");
                    room->throwCard(victim->getWeapon(), victim, player);
                    player->drawCards(1, objectName());
                } else {
                    room->broadcastSkillInvoke(objectName());
                    ServerPlayer *victim = room->askForPlayerChosen(player, armor_players, objectName(), "@muhmuh-armor");
                    int equip = victim->getArmor()->getEffectiveId();
                    QList<CardsMoveStruct> exchangeMove;
                    CardsMoveStruct move1(equip, player, Player::PlaceEquip, CardMoveReason(CardMoveReason::S_REASON_ROB, player->objectName()));
                    exchangeMove.push_back(move1);
                    if (player->getArmor()) {
                        CardsMoveStruct move2(player->getArmor()->getEffectiveId(), NULL, Player::DiscardPile,
                            CardMoveReason(CardMoveReason::S_REASON_CHANGE_EQUIP, player->objectName()));
                        exchangeMove.push_back(move2);
                    }
                    room->moveCardsAtomic(exchangeMove, true);
                }
            }
        }
        return false;
    }
};

XiemuCard::XiemuCard()
{
    target_fixed = true;
}

void XiemuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QString kingdom = room->askForKingdom(source, "xiemu");
    room->setPlayerMark(source, "@xiemu_" + kingdom, 1);
}

class XiemuViewAsSkill : public OneCardViewAsSkill
{
public:
    XiemuViewAsSkill() : OneCardViewAsSkill("xiemu")
    {
        filter_pattern = "Slash";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("XiemuCard");
    }

    const Card *viewAs(const Card *originalCard) const
    {
        XiemuCard *card = new XiemuCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Xiemu : public TriggerSkill
{
public:
    Xiemu() : TriggerSkill("xiemu")
    {
        events << TargetConfirmed << EventPhaseStart;
        view_as_skill = new XiemuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from && player != use.from && use.card->getTypeId() != Card::TypeSkill
                && use.card->isBlack() && use.to.contains(player)
                && player->getMark("@xiemu_" + use.from->getKingdom()) > 0) {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                room->notifySkillInvoked(player, objectName());
                player->drawCards(2, objectName());
            }
        } else {
            if (player->getPhase() == Player::RoundStart) {
                foreach (QString kingdom, Sanguosha->getKingdoms()) {
                    QString markname = "@xiemu_" + kingdom;
                    if (player->getMark(markname) > 0)
                        room->setPlayerMark(player, markname, 0);
                }
            }
        }
        return false;
    }
};

class Naman : public TriggerSkill
{
public:
    Naman() : TriggerSkill("naman")
    {
        events << BeforeCardsMove;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.to_place != Player::DiscardPile) return false;
        const Card *to_obtain = NULL;
        if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_RESPONSE) {
            if (move.from && player->objectName() == move.from->objectName())
                return false;
            to_obtain = move.reason.m_extraData.value<const Card *>();
            if (!to_obtain || !to_obtain->isKindOf("Slash"))
                return false;
        } else {
            return false;
        }
        if (to_obtain && room->askForSkillInvoke(player, objectName(), data)) {
            room->broadcastSkillInvoke(objectName());
            room->obtainCard(player, to_obtain);
            move.removeCardIds(move.card_ids);
            data = QVariant::fromValue(move);
        }

        return false;
    }
};

JSPPackage::JSPPackage()
    : Package("jiexian_sp")
{
    General *nossp_guanyu = new General(this, "nossp_guanyu", "wei"); // JSP 003
    nossp_guanyu->addSkill("wusheng");
    nossp_guanyu->addSkill(new NosDanji);

    General *guanyinping = new General(this, "nos_guanyinping", "shu", 3,false);
    guanyinping->addSkill(new Shiuehjih);
    guanyinping->addSkill(new Huushiaw);
    guanyinping->addSkill(new Wuujih);

    General *sp_xiahoushi = new General(this, "sp_xiahoushi", "shu", 3, false); // SP 023
    sp_xiahoushi->addSkill(new Yannyuu);
    sp_xiahoushi->addSkill(new Xiaode);
    sp_xiahoushi->addSkill(new XiaodeEx);
    related_skills.insertMulti("xiaode", "#xiaode");

    General *zhangbao = new General(this, "nos_zhangbao", "qun", 3);
    zhangbao->addSkill(new Jowfuh);
    zhangbao->addSkill(new Yiingbing);

    General *xingcai = new General(this, "nos_xingcai", "shu", 3, false);
    xingcai->addSkill(new Shennshyan);
    xingcai->addSkill("qiangwu");

    General *sunluyu = new General(this, "nos_sunluyu", "wu", 3, false);
    sunluyu->addSkill(new Meybuh);
    sunluyu->addSkill(new Muhmuh);

    General *maliang = new General(this, "nos_maliang", "shu", 3);
    maliang->addSkill(new Xiemu);
    maliang->addSkill(new Naman);

    General *jsp_machao = new General(this, "jsp_machao", "qun"); // JSP 002
    jsp_machao->addSkill("zhuiji");
    jsp_machao->addSkill(new Cihuai);

    General *jsp_zhaoyun = new General(this, "jsp_zhaoyun", "qun", 3);
    jsp_zhaoyun->addSkill(new ChixinTrigger);
    jsp_zhaoyun->addSkill(new Suiren);
    jsp_zhaoyun->addSkill("yicong");

    addMetaObject<ShiuehjihCard>();
    addMetaObject<JowfuhCard>();
    addMetaObject<XiemuCard>();

    skills << new MeybuhFilter("meybuh");
}

ADD_PACKAGE(JSP)
