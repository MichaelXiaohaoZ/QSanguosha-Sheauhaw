#include "yjcm2015.h"
#include "dialogtest.h"
#include "general.h"
#include "player.h"
#include "structs.h"
#include "room.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "settings.h"
#include "wrapped-card.h"
#include "roomthread.h"
#include "standard-equips.h"
#include "standard-skillcards.h"
#include "json.h"
#include "guhuodialog.h"


NosZhenshanCard::NosZhenshanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool NosZhenshanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    }

    const Card *_card = Self->tag.value("noszhenshan").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card->objectName(), Card::NoSuit, 0);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool NosZhenshanCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    }

    const Card *_card = Self->tag.value("noszhenshan").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card->objectName(), Card::NoSuit, 0);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool NosZhenshanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    }

    const Card *_card = Self->tag.value("noszhenshan").value<const Card *>();
    if (_card == NULL)
        return false;

    Card *card = Sanguosha->cloneCard(_card->objectName(), Card::NoSuit, 0);
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *NosZhenshanCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *quancong = card_use.from;
    Room *room = quancong->getRoom();

    QString user_str = user_string;
    if (user_string == "slash" && Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        user_str = room->askForChoice(quancong, "noszhenshan_slash", use_list.join("+"));
    }

    askForExchangeHand(quancong);

    Card *c = Sanguosha->cloneCard(user_str, Card::NoSuit, 0);
    c->setSkillName("noszhenshan");
    c->deleteLater();
    return c;
}

const Card *NosZhenshanCard::validateInResponse(ServerPlayer *quancong) const
{
    Room *room = quancong->getRoom();

    QString user_str = user_string;
    if (user_string == "peach+analeptic") {
        QStringList use_list;
        use_list << "peach";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "analeptic";
        user_str = room->askForChoice(quancong, "noszhenshan_saveself", use_list.join("+"));
    } else if (user_string == "slash") {
        QStringList use_list;
        use_list << "slash";
        if (!Config.BanPackages.contains("maneuvering"))
            use_list << "thunder_slash" << "fire_slash";
        user_str = room->askForChoice(quancong, "noszhenshan_slash", use_list.join("+"));
    } else
        user_str = user_string;

    askForExchangeHand(quancong);

    Card *c = Sanguosha->cloneCard(user_str, Card::NoSuit, 0);
    c->setSkillName("noszhenshan");
    c->deleteLater();
    return c;
}

void NosZhenshanCard::askForExchangeHand(ServerPlayer *quancong)
{
    Room *room = quancong->getRoom();
    QList<ServerPlayer *> targets;
    foreach (ServerPlayer *p, room->getOtherPlayers(quancong)) {
        if (quancong->getHandcardNum() > p->getHandcardNum())
            targets << p;
    }
    ServerPlayer *target = room->askForPlayerChosen(quancong, targets, "noszhenshan", "@noszhenshan");
    QList<CardsMoveStruct> moves;
    if (!quancong->isKongcheng()) {
        CardMoveReason reason(CardMoveReason::S_REASON_SWAP, quancong->objectName(), target->objectName(), "noszhenshan", QString());
        CardsMoveStruct move(quancong->handCards(), target, Player::PlaceHand, reason);
        moves << move;
    }
    if (!target->isKongcheng()) {
        CardMoveReason reason(CardMoveReason::S_REASON_SWAP, target->objectName(), quancong->objectName(), "noszhenshan", QString());
        CardsMoveStruct move(target->handCards(), quancong, Player::PlaceHand, reason);
        moves << move;
    }
    if (!moves.isEmpty())
        room->moveCardsAtomic(moves, false);

    room->setPlayerFlag(quancong, "NosZhenshanUsed");
}

class NosZhenshanVS : public ZeroCardViewAsSkill
{
public:
    NosZhenshanVS() : ZeroCardViewAsSkill("noszhenshan")
    {
    }

    const Card *viewAs() const
    {
        QString pattern;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            const Card *c = Self->tag["noszhenshan"].value<const Card *>();
            if (c == NULL)
                return NULL;
            pattern = c->objectName();
        } else {
            pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "peach+analeptic" && Self->getMark("Global_PreventPeach") > 0)
                pattern = "analeptic";
        }

        NosZhenshanCard *zs = new NosZhenshanCard;
        zs->setUserString(pattern);
        return zs;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return canExchange(player);
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (!canExchange(player))
            return false;
        if (pattern == "peach")
            return player->getMark("Global_PreventPeach") == 0;
        if (pattern == "slash")
            return Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE;
        if (pattern.contains("analeptic"))
            return true;
        return false;
    }

    static bool canExchange(const Player *player)
    {
        if (player->hasFlag("NosZhenshanUsed"))
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

class NosZhenshan : public TriggerSkill
{
public:
    NosZhenshan() : TriggerSkill("noszhenshan")
    {
        view_as_skill = new NosZhenshanVS;
        events << EventPhaseChanging << CardAsked;
    }

    GuhuoDialog *getDialog() const
    {
        return GuhuoDialog::getInstance("noszhenshan", true, false);
    }

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
                if (p->hasFlag("NosZhenshanUsed"))
                    room->setPlayerFlag(p, "-NosZhenshanUsed");
            }
        } else if (triggerEvent == CardAsked && TriggerSkill::triggerable(player)) {
            QString pattern = data.toStringList().first();
            if (NosZhenshanVS::canExchange(player) && (pattern == "slash" || pattern == "jink")
                && player->askForSkillInvoke(objectName(), data)) {
                NosZhenshanCard::askForExchangeHand(player);
                room->setPlayerFlag(player, "NosZhenshanUsed");
                Card *card = Sanguosha->cloneCard(pattern);
                card->setSkillName(objectName());
                room->provide(card);
                return true;
            }
        }
        return false;
    }
};

DialogTsPackage::DialogTsPackage()
    : Package("DialogTs")
{

    addMetaObject<NosZhenshanCard>();

    General *ts = new General(this, "tsqc", "god", 100, false, true);
    ts->addSkill(new NosZhenshan);

}
ADD_PACKAGE(DialogTs)
