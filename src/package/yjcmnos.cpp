#include "yjcmnos.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "engine.h"
#include "settings.h"
#include "ai.h"
#include "general.h"
#include "util.h"
#include "wrapped-card.h"
#include "room.h"
#include "roomthread.h"
#include "windms.h"

ZhenshanCard::ZhenshanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "zhenshan";
}

bool ZhenshanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    if (card == NULL)
        return false;

    card->addCostcard(this);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool ZhenshanCard::targetFixed() const
{
    Card *card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    if (card == NULL)
        return false;

    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool ZhenshanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    if (card == NULL)
        return false;

    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *ZhenshanCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *quancong = card_use.from;
    Room *room = quancong->getRoom();

    askForExchangeHand(quancong);

    QString user_str = user_string;
    Card *c = Sanguosha->cloneCard(user_str, Card::NoSuit, 0);

    c->setSkillName("zhenshan");
    c->deleteLater();
    c->addCostcard(this);
    return c;
}

void ZhenshanCard::askForExchangeHand(ServerPlayer *quancong)
{
    Room *room = quancong->getRoom();
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(quancong)) {
        if (quancong->getHandcardNum() > p->getHandcardNum())
            targets << p;
    }
    ServerPlayer *target = room->askForPlayerChosen(quancong, targets, "zhenshan", "@zhenshan");
    QList<CardsMoveStruct> moves;
    if (!quancong->isKongcheng()) {
        CardMoveReason reason(CardMoveReason::S_REASON_SWAP, quancong->objectName(), target->objectName(), "zhenshan", QString());
        CardsMoveStruct move(quancong->handCards(), target, Player::PlaceHand, reason);
        moves << move;
    }
    if (!target->isKongcheng()) {
        CardMoveReason reason(CardMoveReason::S_REASON_SWAP, target->objectName(), quancong->objectName(), "zhenshan", QString());
        CardsMoveStruct move(target->handCards(), quancong, Player::PlaceHand, reason);
        moves << move;
    }
    if (!moves.isEmpty())
        room->moveCardsAtomic(moves, false);

    room->setPlayerFlag(quancong, "ZhenshanUsed");
}

const Card *ZhenshanCard::validateInResponse(ServerPlayer *quancong) const
{
    Room *room = quancong->getRoom();

    QString user_str = user_string;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        use_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "analeptic";
        user_str = room->askForChoice(quancong, "zhenshan_saveself", use_list.join("+"));
    } else if (user_string == "slash") {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        user_str = room->askForChoice(quancong, "zhenshan_slash", use_list.join("+"));
    } else
        user_str = user_string;

    askForExchangeHand(quancong);

    Card *c = Sanguosha->cloneCard(user_str, Card::NoSuit, 0);
    c->setSkillName("zhenshan");
    c->deleteLater();
    return c;
}

class ZhenshanVS : public ZeroCardViewAsSkill
{
public:
    ZhenshanVS() : ZeroCardViewAsSkill("zhenshan")
    {
    }

    const Card *viewAs() const
    {
        ZhenshanCard *zscard = new ZhenshanCard;
        return zscard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return canExchange(player);
        //return true;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        QList<const Player *> sib = player->getAliveSiblings();
        if (player->isAlive())
            sib << player;

        bool noround = true;

        foreach (const Player *p, sib) {
            if (p->getPhase() != Player::NotActive) {
                noround = false;
                break;
            }
        }

        if (noround)
            return false;

        if (!canExchange(player))
            return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
        if (player->hasFlag("ZhenshanUsed"))
            return false;
        return true;
    }

    static bool canExchange(const Player *player)
    {
        if (player->hasFlag("ZhenshanUsed"))
            return false;
        bool current = player->getPhase() != Player::NotActive, less_hand = false;
        foreach(const Player *p, player->getAliveSiblings()) {
            if (p->getPhase() != Player::NotActive)
                current = true;
            if (player->getHandcardNum() > p->getHandcardNum())
                less_hand = true;
            if (current && less_hand)
                return true;
        }
        return false;
    }
};

class Zhenshan : public TriggerSkill
{
public:
    Zhenshan() : TriggerSkill("zhenshan")
    {
        view_as_skill = new ZhenshanVS;
        events << EventPhaseChanging << CardAsked;
    }

    QString getSelectBox() const
    {
        return "guhuo_b";
    }
/*
    QDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("zhenshan", true, false);
    }
*/
    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;

            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasFlag("ZhenshanUsed"))
                    room->setPlayerFlag(p, "-ZhenshanUsed");
            }
        }
        /*
        else if (triggerEvent == CardAsked && TriggerSkill::triggerable(player)) {
            QString pattern = data.toStringList().first();
            if (ZhenshanVS::canExchange(player) && (pattern == "slash" || pattern == "jink")
                && player->askForSkillInvoke(objectName(), data)) {
                ZhenshanCard::askForExchangeHand(player);
                room->setPlayerFlag(player, "ZhenshanUsed");
                Card *card = Sanguosha->cloneCard(pattern);
                card->setSkillName(objectName());
                room->provide(card);
                return true;
            }
        }
        */
        return false;
    }
};

class Taoxi : public TriggerSkill
{
public:
    Taoxi() : TriggerSkill("taoxi")
    {
        events << TargetSpecified << CardsMoveOneTime << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player)
            && !player->hasFlag("TaoxiUsed") && player->getPhase() == Player::Play) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card && use.card->getTypeId() != Card::TypeSkill && use.to.length() == 1) {
                ServerPlayer *to = use.to.first();
                player->tag["taoxi_carduse"] = data;
                if (to != player && !to->isKongcheng() && player->askForSkillInvoke(objectName(), QVariant::fromValue(to))) {
                    room->broadcastSkillInvoke(objectName());
                    room->setPlayerFlag(player, "TaoxiUsed");
                    room->setPlayerFlag(player, "TaoxiRecord");
                    int id = room->askForCardChosen(player, to, "h", objectName(), false);
                    room->showCard(to, id);
                    TaoxiMove(id, true, player);
                    player->tag["TaoxiId"] = id;
                }
            }
        } else if (triggerEvent == CardsMoveOneTime && player->hasFlag("TaoxiRecord")) {
            bool ok = false;
            int id = player->tag["TaoxiId"].toInt(&ok);
            if (!ok) {
                room->setPlayerFlag(player, "-TaoxiRecord");
                return false;
            }
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != NULL && move.card_ids.contains(id)) {
                if (move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                    TaoxiMove(id, false, player);
                    if (room->getCardOwner(id) != NULL)
                        room->showCard(room->getCardOwner(id), id);
                    room->setPlayerFlag(player, "-TaoxiRecord");
                    player->tag.remove("TaoxiId");
                }
            }
        } else if (triggerEvent == EventPhaseChanging && player->hasFlag("TaoxiRecord")) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            bool ok = false;
            int id = player->tag["TaoxiId"].toInt(&ok);
            if (!ok) {
                room->setPlayerFlag(player, "-TaoxiRecord");
                return false;
            }

            if (TaoxiHere(player))
                TaoxiMove(id, false, player);

            ServerPlayer *owner = room->getCardOwner(id);
            if (owner && room->getCardPlace(id) == Player::PlaceHand) {
                room->sendCompulsoryTriggerLog(player, objectName());
                room->showCard(owner, id);
                room->loseHp(player);
                room->setPlayerFlag(player, "-TaoxiRecord");
                player->tag.remove("TaoxiId");
            }
        }
        return false;
    }

private:
    static void TaoxiMove(int id, bool movein, ServerPlayer *caoxiu)
    {
        Room *room = caoxiu->getRoom();
        if (movein) {
            CardsMoveStruct move(id, NULL, caoxiu, Player::PlaceTable, Player::PlaceSpecial,
                CardMoveReason(CardMoveReason::S_REASON_PUT, caoxiu->objectName(), "taoxi", QString()));
            move.to_pile_name = "&taoxi";
            QList<CardsMoveStruct> moves;
            moves.append(move);
            QList<ServerPlayer *> _caoxiu;
            _caoxiu << caoxiu;
            room->notifyMoveCards(true, moves, false, _caoxiu);
            room->notifyMoveCards(false, moves, false, _caoxiu);
        } else {
            CardsMoveStruct move(id, caoxiu, NULL, Player::PlaceSpecial, Player::PlaceTable,
                CardMoveReason(CardMoveReason::S_REASON_PUT, caoxiu->objectName(), "taoxi", QString()));
            move.from_pile_name = "&taoxi";
            QList<CardsMoveStruct> moves;
            moves.append(move);
            QList<ServerPlayer *> _caoxiu;
            _caoxiu << caoxiu;
            room->notifyMoveCards(true, moves, false, _caoxiu);
            room->notifyMoveCards(false, moves, false, _caoxiu);
        }
        caoxiu->tag["TaoxiHere"] = movein;
    }

    static bool TaoxiHere(ServerPlayer *caoxiu)
    {
        return caoxiu->tag.value("TaoxiHere", false).toBool();
    }
};

class Myngjiann : public TriggerSkill
{
public:
    Myngjiann() : TriggerSkill("myngjiann")
    {
        events << EventPhaseChanging;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        if (change.to != Player::Play)
            return false;

        if (player->isSkipped(Player::Play))
            return false;

        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@myngjiann-give", true, true);
        if (target == NULL)
            return false;

        room->broadcastSkillInvoke(objectName());
        CardMoveReason r(CardMoveReason::S_REASON_GIVE, player->objectName(), target->objectName(), objectName(), QString());
        DummyCard d(player->handCards());
        room->obtainCard(target, &d, r, false);

        player->tag["myngjiann"] = QVariant::fromValue(target);
        throw TurnBroken;

        return false;
    }
};

class MyngjiannGive : public PhaseChangeSkill
{
public:
    MyngjiannGive() : PhaseChangeSkill("#myngjiann-give")
    {

    }

    int getPriority(TriggerEvent) const
    {
        return -1;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::NotActive && target->tag.contains("myngjiann");
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        ServerPlayer *p = target->tag.value("myngjiann").value<ServerPlayer *>();
        target->tag.remove("myngjiann");

        if (p == NULL)
            return false;

        QList<Player::Phase> phase;
        phase << Player::Play;

        p->play(phase);

        return false;
    }
};

AngwoCard::AngwoCard()
{

}

bool AngwoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty() || to_select == Self)
        return false;

    return !to_select->getEquips().isEmpty();
}

void AngwoCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();
    int beforen = 0;
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        if (effect.to->inMyAttackRange(p))
            beforen++;
    }

    int id = room->askForCardChosen(effect.from, effect.to, "e", "angwo");
    effect.to->obtainCard(Sanguosha->getCard(id));

    int aftern = 0;
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        if (effect.to->inMyAttackRange(p))
            aftern++;
    }

    if (aftern < beforen)
        effect.from->drawCards(1, "angwo");
}

class Angwo : public ZeroCardViewAsSkill
{
public:
    Angwo() : ZeroCardViewAsSkill("angwo")
    {

    }

    const Card *viewAs() const
    {
        return new AngwoCard;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("AngwoCard");
    }
};

class ZhannjyueVS : public ZeroCardViewAsSkill
{
public:
    ZhannjyueVS() : ZeroCardViewAsSkill("zhannjyue")
    {

    }

    const Card *viewAs() const
    {
        Duel *duel = new Duel(Card::SuitToBeDecided, -1);
        duel->addSubcards(Self->getHandcards());
        duel->setSkillName("zhannjyue");
        return duel;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("zhannjyuedraw") < 2 && !player->isKongcheng();
    }
};

class Zhannjyue : public TriggerSkill
{
public:
    Zhannjyue() : TriggerSkill("zhannjyue")
    {
        view_as_skill = new ZhannjyueVS;
        events << CardFinished << PreDamageDone << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL && damage.card->isKindOf("Duel") && damage.card->getSkillName() == "zhannjyue" && damage.from != NULL) {
                QVariantMap m = room->getTag("zhannjyue").toMap();
                QVariantList l = m.value(damage.card->toString(), QVariantList()).toList();
                l << QVariant::fromValue(damage.to);
                m[damage.card->toString()] = l;
                room->setTag("zhannjyue", m);
            }
        } else if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Duel") && use.card->getSkillName() == "zhannjyue") {
                QVariantMap m = room->getTag("zhannjyue").toMap();
                QVariantList l = m.value(use.card->toString(), QVariantList()).toList();
                if (!l.isEmpty()) {
                    QList<ServerPlayer *> l_copy;
                    foreach (const QVariant &s, l)
                        l_copy << s.value<ServerPlayer *>();
                    l_copy << use.from;
                    int n = l_copy.count(use.from);
                    room->addPlayerMark(use.from, "zhannjyuedraw", n);
                    room->sortByActionOrder(l_copy);
                    room->drawCards(l_copy, 1, objectName());
                }
                m.remove(use.card->toString());
                room->setTag("zhannjyue", m);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive)
                room->setPlayerMark(player, "zhannjyuedraw", 0);
        }
        return false;
    }
};

NostalYJCM2015Package::NostalYJCM2015Package()
    : Package("NostalYJCM2015")
{

    General *caorui = new General(this, "nos_caorui$", "wei", 3);
    caorui->addSkill("huituo");
    caorui->addSkill("#huituo");
    caorui->addSkill(new Myngjiann);
    caorui->addSkill(new MyngjiannGive);
    related_skills.insertMulti("myngjiann", "#myngjiann-give");
    caorui->addSkill("xingshuai");

    General *caoxiu = new General(this, "nos_caoxiu", "wei");
    caoxiu->addSkill(new Taoxi);

    General *quancong = new General(this, "nos_quancong", "wu");
    quancong->addSkill(new Zhenshan);

    General *zhuzhi = new General(this, "nos_zhuzhi", "wu");
    zhuzhi->addSkill(new Angwo);

    General *liuchen = new General(this, "nos_liuchen$", "shu");
    liuchen->addSkill(new Zhannjyue);
    liuchen->addSkill("qinwang");

    addMetaObject<AngwoCard>();
    addMetaObject<ZhenshanCard>();

}
ADD_PACKAGE(NostalYJCM2015)

TsydyiCard::TsydyiCard()
{
    target_fixed = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void TsydyiCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &) const
{
    room->throwCard(this, NULL);
}

class TsydyiVS : public OneCardViewAsSkill
{
public:
    TsydyiVS() : OneCardViewAsSkill("tsydyi")
    {
        response_pattern = "@@tsydyi";
        filter_pattern = ".|.|.|tsydyi";
        expand_pile = "tsydyi";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        TsydyiCard *sd = new TsydyiCard;
        sd->addSubcard(originalCard);
        return sd;
    }
};

class Tsydyi : public TriggerSkill
{
public:
    Tsydyi() : TriggerSkill("tsydyi")
    {
        events << CardResponded << EventPhaseStart << EventPhaseChanging;
        //frequency = Frequent;
        view_as_skill = new TsydyiVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerMark(player, "tsydyi", 0);
        } else if (triggerEvent == CardResponded) {
            CardResponseStruct resp = data.value<CardResponseStruct>();
            if (resp.m_isUse && resp.m_card->isKindOf("Jink")) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (TriggerSkill::triggerable(p) && (p == player || p->getPhase() != Player::NotActive)
                        && room->askForSkillInvoke(p, objectName(), data)) {
                        room->broadcastSkillInvoke(objectName());
                        QList<int> ids = room->getNCards(1, false); // For UI
                        CardsMoveStruct move(ids, p, Player::PlaceTable,
                            CardMoveReason(CardMoveReason::S_REASON_TURNOVER, p->objectName(), "tsydyi", QString()));
                        room->moveCardsAtomic(move, true);
                        p->addToPile("tsydyi", ids);
                    }
                }
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Play) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (player->getPhase() != Player::Play) return false;
                if (TriggerSkill::triggerable(p) && p->getPile("tsydyi").length() > 0 && room->askForUseCard(p, "@@tsydyi", "tsydyi_remove:remove", -1, Card::MethodNone))
                    room->addPlayerMark(player, "tsydyi");
            }
        }
        return false;
    }
};

class TsydyiTargetMod : public TargetModSkill
{
public:
    TsydyiTargetMod() : TargetModSkill("#tsydyi-target")
    {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return -from->getMark("tsydyi");
    }
};

class Zhongyoong : public TriggerSkill
{
public:
    Zhongyoong() : TriggerSkill("zhongyoong")
    {
        events << SlashMissed;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        const Card *jink = effect.jink;
        if (!jink) return false;
        QList<int> ids;
        if (!jink->isVirtualCard()) {
            if (room->getCardPlace(jink->getEffectiveId()) == Player::DiscardPile)
                ids << jink->getEffectiveId();
        } else {
            foreach (int id, jink->getSubcards()) {
                if (room->getCardPlace(id) == Player::DiscardPile)
                    ids << id;
            }
        }
        if (ids.isEmpty()) return false;

        room->fillAG(ids, player);
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(effect.to), objectName(),
            "zhongyoong-invoke:" + effect.to->objectName(), true, true);
        room->clearAG(player);
        if (!target) return false;
        room->broadcastSkillInvoke(objectName());
        DummyCard *dummy = new DummyCard(ids);
        room->obtainCard(target, dummy);
        delete dummy;

        if (player->isAlive() && effect.to->isAlive() && target != player) {
            if (!player->canSlash(effect.to, NULL, false))
                return false;
            if (room->askForUseSlashTo(player, effect.to, QString("zhongyoong-slash:%1").arg(effect.to->objectName()), false, true))
                return true;
        }
        return false;
    }
};

class Youdi : public PhaseChangeSkill
{
public:
    Youdi() : PhaseChangeSkill("youdi")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        if (target->getPhase() != Player::Finish || target->isNude()) return false;
        Room *room = target->getRoom();
        QList<ServerPlayer *> players;
        foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
            if (p->canDiscard(target, "he")) players << p;
        }
        if (players.isEmpty()) return false;
        ServerPlayer *player = room->askForPlayerChosen(target, players, objectName(), "youdi-invoke", true, true);
        if (player) {
            room->broadcastSkillInvoke(objectName());
            int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, target, player);
            if (!Sanguosha->getCard(id)->isKindOf("Slash") && player->isAlive() && !player->isNude()) {
                int id2 = room->askForCardChosen(target, player, "he", "youdi_obtain");
                room->obtainCard(target, id2, false);
            }
        }
        return false;
    }
};

DinqpiinCard::DinqpiinCard()
{
}

bool DinqpiinCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->isWounded() && !to_select->hasFlag("dinqpiin");
}

void DinqpiinCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();

    JudgeStruct judge;
    judge.who = effect.to;
    judge.good = true;
    judge.pattern = ".|black";
    judge.reason = "dinqpiin";

    room->judge(judge);

    if (judge.isGood()) {
        room->setPlayerFlag(effect.to, "dinqpiin");
        effect.to->drawCards(effect.to->getLostHp(), "dinqpiin");
    } else {
        effect.from->turnOver();
    }
}

class DinqpiinViewAsSkill : public OneCardViewAsSkill
{
public:
    DinqpiinViewAsSkill() : OneCardViewAsSkill("dinqpiin")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        if (!player->canDiscard(player, "h") || player->getMark("dinqpiin") == 0xE) return false;
        if (!player->hasFlag("dinqpiin") && player->isWounded()) return true;
        foreach (const Player *p, player->getAliveSiblings()) {
            if (!p->hasFlag("dinqpiin") && p->isWounded()) return true;
        }
        return false;
    }

    bool viewFilter(const Card *to_select) const
    {
        return !to_select->isEquipped() && (Self->getMark("dinqpiin") & (1 << int(to_select->getTypeId()))) == 0;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        DinqpiinCard *card = new DinqpiinCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Dinqpiin : public TriggerSkill
{
public:
    Dinqpiin() : TriggerSkill("dinqpiin")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded << BeforeCardsMove;
        view_as_skill = new DinqpiinViewAsSkill;
        global = true;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("dinqpiin"))
                        room->setPlayerFlag(p, "-dinqpiin");
                }
                if (player->getMark("dinqpiin") > 0)
                    room->setPlayerMark(player, "dinqpiin", 0);
            }
        } else {
            if (!player->isAlive() || player->getPhase() == Player::NotActive) return false;
            if (triggerEvent == PreCardUsed || triggerEvent == CardResponded) {
                const Card *card = NULL;
                if (triggerEvent == PreCardUsed) {
                    card = data.value<CardUseStruct>().card;
                } else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (!card || card->getTypeId() == Card::TypeSkill) return false;
                recordDinqpiinCardType(room, player, card);
            } else if (triggerEvent == BeforeCardsMove) {
                CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
                if (player != move.from
                    || ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_DISCARD))
                    return false;
                foreach (int id, move.card_ids) {
                    const Card *c = Sanguosha->getCard(id);
                    recordDinqpiinCardType(room, player, c);
                }
            }
        }
        return false;
    }

private:
    void recordDinqpiinCardType(Room *room, ServerPlayer *player, const Card *card) const
    {
        if (player->getMark("dinqpiin") == 0xE) return;
        int typeID = (1 << int(card->getTypeId()));
        int mark = player->getMark("dinqpiin");
        if ((mark & typeID) == 0)
            room->setPlayerMark(player, "dinqpiin", mark | typeID);
    }
};

NostalYJCM2014Package::NostalYJCM2014Package()
    : Package("NostalYJCM2014")
{
    General *zhoucang = new General(this, "nos_zhoucang", "shu"); // YJ 310
    zhoucang->addSkill(new Zhongyoong);

    General *caozhen = new General(this, "nos_caozhen", "wei"); // YJ 302
    caozhen->addSkill(new Tsydyi);
    caozhen->addSkill(new TsydyiTargetMod);
    related_skills.insertMulti("tsydyi", "#tsydyi-target");

    General *zhuhuan = new General(this, "nos_zhuhuan", "wu");
    zhuhuan->addSkill(new Youdi);

    General *chenqun = new General(this, "nos_chenqun", "wei", 3);
    chenqun->addSkill(new Dinqpiin);
    chenqun->addSkill("faen");

    addMetaObject<DinqpiinCard>();
    addMetaObject<TsydyiCard>();
}
ADD_PACKAGE(NostalYJCM2014)

class Mihjih : public TriggerSkill
{
public:
    Mihjih() : TriggerSkill("mihjih")
    {
        events << EventPhaseStart << ChoiceMade;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (TriggerSkill::triggerable(target) && triggerEvent == EventPhaseStart
            && target->getPhase() == Player::Finish && target->isWounded() && target->askForSkillInvoke(this)) {
            room->broadcastSkillInvoke(objectName());
            QStringList draw_num;
            for (int i = 1; i <= target->getLostHp(); draw_num << QString::number(i++)) {

            }
            int num = room->askForChoice(target, "mihjih_draw", draw_num.join("+")).toInt();
            target->drawCards(num, objectName());
            target->setMark(objectName(), 0);
            if (!target->isKongcheng()) {
                forever {
                    int n = target->getMark(objectName());
                    if (n < num && !target->isKongcheng()) {
                        QList<int> handcards = target->handCards();
                        if (!room->askForYiji(target, handcards, objectName(), false, false, false, num - n))
                            break;
                    } else {
                        break;
                    }
                }
                // give the rest cards randomly
                if (target->getMark(objectName()) < num && !target->isKongcheng()) {
                    int rest_num = num - target->getMark(objectName());
                    forever {
                        QList<int> handcard_list = target->handCards();
                        qShuffle(handcard_list);
                        int give = qrand() % rest_num + 1;
                        rest_num -= give;
                        QList<int> to_give = handcard_list.length() < give ? handcard_list : handcard_list.mid(0, give);
                        ServerPlayer *receiver = room->getOtherPlayers(target).at(qrand() % (target->aliveCount() - 1));
                        DummyCard *dummy = new DummyCard(to_give);
                        room->obtainCard(receiver, dummy, false);
                        delete dummy;
                        if (rest_num == 0 || target->isKongcheng())
                            break;
                    }
                }
            }
        } else if (triggerEvent == ChoiceMade) {
            QString str = data.toString();
            if (str.startsWith("Yiji:" + objectName()))
                target->addMark(objectName(), str.split(":").last().split("+").length());
        }
        return false;
    }
};

class Tzyhshour : public DrawCardsSkill
{
public:
    Tzyhshour() : DrawCardsSkill("tzyhshour")
    {
    }

    int getDrawNum(ServerPlayer *liubiao, int n) const
    {
        Room *room = liubiao->getRoom();
        if (liubiao->isWounded() && room->askForSkillInvoke(liubiao, objectName())) {
            int losthp = liubiao->getLostHp();
            room->broadcastSkillInvoke(objectName());
            liubiao->clearHistory();
            liubiao->skip(Player::Play);
            return n + losthp;
        } else
            return n;
    }
};

class Qianxigz : public TriggerSkill
{
public:
    Qianxigz() : TriggerSkill("qianxigz")
    {
        events << EventPhaseStart << FinishJudge;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(target)
            && target->getPhase() == Player::Start) {
            if (room->askForSkillInvoke(target, objectName())) {
                JudgeStruct judge;
                judge.reason = objectName();
                judge.play_animation = false;
                judge.who = target;

                room->judge(judge);
                if (!target->isAlive()) return false;
                QString color = judge.pattern;
                QList<ServerPlayer *> to_choose;
                foreach (ServerPlayer *p, room->getOtherPlayers(target)) {
                    if (target->distanceTo(p) == 1)
                        to_choose << p;
                }
                if (to_choose.isEmpty())
                    return false;

                ServerPlayer *victim = room->askForPlayerChosen(target, to_choose, objectName());
                QString pattern = QString(".|%1|.|hand$0").arg(color);

                room->broadcastSkillInvoke(objectName());
                room->setPlayerFlag(victim, "QianxigzTarget");
                room->addPlayerMark(victim, QString("@qianxi_%1").arg(color));
                room->setPlayerCardLimitation(victim, "use,response", pattern, false);

                LogMessage log;
                log.type = "#Qianxi";
                log.from = victim;
                log.arg = QString("no_suit_%1").arg(color);
                room->sendLog(log);
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName() || !target->isAlive()) return false;

            QString color = judge->card->isRed() ? "red" : "black";
            target->tag[objectName()] = QVariant::fromValue(color);
            judge->pattern = color;
        }
        return false;
    }
};

class QianxigzClear : public TriggerSkill
{
public:
    QianxigzClear() : TriggerSkill("#qianxigz-clear")
    {
        events << EventPhaseChanging << Death;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return !target->tag["qianxigz"].toString().isNull();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
        } else if (triggerEvent == Death) {
            DeathStruct death = data.value<DeathStruct>();
            if (death.who != player)
                return false;
        }

        QString color = player->tag["qianxigz"].toString();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->hasFlag("QianxigzTarget")) {
                room->removePlayerCardLimitation(p, "use,response", QString(".|%1|.|hand$0").arg(color));
                room->setPlayerMark(p, QString("@qianxi_%1").arg(color), 0);
            }
        }
        return false;
    }
};

YJCM2012msPackage::YJCM2012msPackage()
    : Package("YJCM2012ms")
{
    General *liubiao = new General(this, "miansha_liubiao", "qun");
    liubiao->addSkill(new Tzyhshour);
    liubiao->addSkill("zongshi");

    General *madai = new General(this, "miansha_madai", "shu", 4); // SHU 019
    madai->addSkill("mashu");
    madai->addSkill(new Qianxigz);
    madai->addSkill(new QianxigzClear);

    General *wangyi = new General(this, "miansha_wangyi", "wei", 3, false);
    wangyi->addSkill("zhenlie");
    wangyi->addSkill(new Mihjih);
}

ADD_PACKAGE(YJCM2012ms)

Shangshyh::Shangshyh() : TriggerSkill("shangshyh")
{
    events << HpChanged << MaxHpChanged << CardsMoveOneTime;
    frequency = Frequent;
}

int Shangshyh::getMaxLostHp(ServerPlayer *zhangchunhua) const
{
    int losthp = zhangchunhua->getLostHp();
    if (losthp > 2)
        losthp = 2;
    return qMin(losthp, zhangchunhua->getMaxHp());
}

bool Shangshyh::trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data) const
{
    int losthp = getMaxLostHp(zhangchunhua);
    if (triggerEvent == CardsMoveOneTime) {
        bool can_invoke = false;
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == zhangchunhua && move.from_places.contains(Player::PlaceHand))
            can_invoke = true;
        if (move.to == zhangchunhua && move.to_place == Player::PlaceHand)
            can_invoke = true;
        if (!can_invoke)
            return false;
    } else if (triggerEvent == HpChanged || triggerEvent == MaxHpChanged) {
        if (zhangchunhua->getPhase() == Player::Discard) {
            zhangchunhua->addMark("shangshyh");
            return false;
        }
    }

    if (zhangchunhua->getHandcardNum() < losthp && zhangchunhua->askForSkillInvoke(this)) {
        room->broadcastSkillInvoke("shangshyh");
        zhangchunhua->drawCards(losthp - zhangchunhua->getHandcardNum(), objectName());
    }

    return false;
}

JieyueCard::JieyueCard()
{

}

void JieyueCard::onEffect(const CardEffectStruct &effect) const
{
    if (!effect.to->isNude()) {
        Room *room = effect.to->getRoom();
        const Card *card = room->askForExchange(effect.to, "jieyue", 1, 1, true, QString("@jieyue_put:%1").arg(effect.from->objectName()), true);

        if (card != NULL)
            effect.from->addToPile("jieyue_pile", card);
        else if (effect.from->canDiscard(effect.to, "he")) {
            int id = room->askForCardChosen(effect.from, effect.to, "he", objectName(), false, Card::MethodDiscard);
            room->throwCard(id, effect.to, effect.from);
        }
    }
}

class JieyueVS : public OneCardViewAsSkill
{
public:
    JieyueVS() : OneCardViewAsSkill("jieyue")
    {
    }

    bool isResponseOrUse() const
    {
        return !Self->getPile("jieyue_pile").isEmpty();
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@jieyue") {
            return !Self->isJilei(to_select);
        }

        if (pattern == "jink")
            return to_select->isRed();
        else if (pattern == "nullification")
            return to_select->isBlack();
        return false;
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (!player->getPile("jieyue_pile").isEmpty() && (pattern == "jink" || pattern == "nullification")) || (pattern == "@@jieyue" && !player->isKongcheng());
    }

    bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (!player->getPile("jieyue_pile").isEmpty()) {
            foreach(const Card *card, player->getHandcards() + player->getEquips()) {
                if (card->isBlack())
                    return true;
            }

            foreach(int id, player->getHandPile())  {
                if (Sanguosha->getCard(id)->isBlack())
                    return true;
            }
        }

        return false;
    }

    const Card *viewAs(const Card *card) const
    {
        QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
        if (pattern == "@@jieyue") {
            JieyueCard *jy = new JieyueCard;
            jy->addSubcard(card);
            return jy;
        }

        if (card->isRed()) {
            Jink *jink = new Jink(Card::SuitToBeDecided, 0);
            jink->addSubcard(card);
            jink->setSkillName(objectName());
            return jink;
        } else if (card->isBlack()) {
            Nullification *nulli = new Nullification(Card::SuitToBeDecided, 0);
            nulli->addSubcard(card);
            nulli->setSkillName(objectName());
            return nulli;
        }
        return NULL;
    }

    int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Nullification"))
            return 3;
        else if (card->isKindOf("Jink"))
            return 2;

        return 1;
    }
};

class Jieyue : public TriggerSkill
{
public:
    Jieyue() : TriggerSkill("jieyue")
    {
        events << EventPhaseStart;
        view_as_skill = new JieyueVS;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() == Player::Start && !player->getPile("jieyue_pile").isEmpty()) {
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = player;
            log.arg = objectName();
            room->sendLog(log);
            DummyCard *dummy = new DummyCard(player->getPile("jieyue_pile"));
            player->obtainCard(dummy);
            delete dummy;
        } else if (player->getPhase() == Player::Finish) {
            room->askForUseCard(player, "@@jieyue", "@jieyue", -1, Card::MethodDiscard, false);
        }
        return false;
    }
};

YJCMmsPackage::YJCMmsPackage()
    : Package("YJCMms")
{
    General *zhangchunhua = new General(this, "miansha_zhangchunhua", "wei", 3, false);
    zhangchunhua->addSkill("jueqing");
    zhangchunhua->addSkill(new Shangshyh);

    General *antikuaibo_yuji = new General(this, "nosol_yujin", "wei");
    antikuaibo_yuji->addSkill(new Jieyue);
}

ADD_PACKAGE(YJCMms)
