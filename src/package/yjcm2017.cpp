#include "yjcm2017.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "engine.h"
#include "clientplayer.h"
#include "settings.h"
#include "json.h"

class dummyVS : public ZeroCardViewAsSkill
{
public:
    dummyVS() : ZeroCardViewAsSkill("dummy")
    {
    }

    virtual const Card *viewAs() const
    {
        return NULL;
    }
};

ZhongjianCard::ZhongjianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

bool ZhongjianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->getHp() < to_select->getHandcardNum() && to_select != Self;
}

void ZhongjianCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *target = effect.to;
    room->showCard(source, getEffectiveId());
    int x = target->getHandcardNum() - target->getHp();
    if (x < 1) return;
    QList<int> cards = room->askForCardsChosen(source, target, "h", "zhongjian", x, x);
    room->showCard(target, cards);
    bool color = false, number = false;
    foreach (int id, cards) {
        const Card *card = Sanguosha->getCard(id);
        if (sameColorWith(card))
            color = true;
        if (card->getNumber() == getNumber())
            number = true;
    }
    if (color) {
        QStringList choices;
        choices << "draw";
        if (source->canDiscard(target, "he"))
            choices << "discard";
        if (room->askForChoice(source, "zhongjian", choices.join("+"), QVariant(), "@zhongjian-choose:" + target->objectName(), "draw+discard") == "draw")
            source->drawCards(1, "zhongjian");
        else {
            int to_throw = room->askForCardChosen(source, target, "he", "zhongjian", false, Card::MethodDiscard);
            room->throwCard(Sanguosha->getCard(to_throw), target, source);
        }
    }
    if (number)
        room->setPlayerFlag(source, "ZhongjianSamePoint");
    if (!color && !number && source->getMaxCards() > 0)
        room->setPlayerMark(source, "#xinxianying_maxcards", source->getMark("#xinxianying_maxcards")-1);
}

class Zhongjian : public OneCardViewAsSkill
{
public:
    Zhongjian() : OneCardViewAsSkill("zhongjian")
    {
        filter_pattern = ".|.|.|hand";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->usedTimes("ZhongjianCard") < (player->hasFlag("ZhongjianSamePoint") ? 2 : 1);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ZhongjianCard *zhongjian = new ZhongjianCard;
        zhongjian->addSubcard(originalCard->getId());
        return zhongjian;
    }
};

class Caishi : public PhaseChangeSkill
{
public:
    Caishi() : PhaseChangeSkill("caishi")
    {
        view_as_skill = new dummyVS;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() == Player::Draw) {
            if (room->askForSkillInvoke(player, "skill_ask", "prompt:::"+objectName())) {
                QStringList choices;
                choices << "maxcards" << "cancel";
                if (player->isWounded())
                    choices << "recover";
                QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(), "@caishi-choose", "maxcards+recover+cancel");
                if (choice != "cancel") {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->notifySkillInvoked(player, objectName());
                    player->broadcastSkillInvoke(objectName());
                    if (choice == "maxcards") {
                        room->addPlayerMark(player, "#xinxianying_maxcards");
                        room->setPlayerFlag(player, "CaishiOthers");
                    } else {
                        room->recover(player, RecoverStruct(player));
                        room->setPlayerFlag(player, "CaishiSelf");
                    }
                }

            }
        }
        return false;
    }
};

class CaishiProhibit : public ProhibitSkill
{
public:
    CaishiProhibit() : ProhibitSkill("#caishi-prohibit")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->isKindOf("SkillCard")) return false;
        return (from && from->hasFlag("CaishiOthers") && from != to) || (from && from->hasFlag("CaishiSelf") && from == to);
    }
};

class XinxianyingMaxCards : public MaxCardsSkill
{
public:
    XinxianyingMaxCards() : MaxCardsSkill("#xinxianying-maxcards")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        return target->getMark("#xinxianying_maxcards");
    }
};

FumianTargetCard::FumianTargetCard()
{
}

bool FumianTargetCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList available_targets = Self->property("fumian_available_targets").toString().split("+");

    return targets.length() < Self->getMark("fumian_num") && available_targets.contains(to_select->objectName());
}

void FumianTargetCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    foreach (ServerPlayer *p, card_use.to) {
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, card_use.from->objectName(), p->objectName());
        p->setFlags("FumianExtraTarget");
    }
}

class FumianTarget : public ZeroCardViewAsSkill
{
public:
    FumianTarget() : ZeroCardViewAsSkill("fumian_target")
    {
        response_pattern = "@@fumian_target";
    }

    virtual const Card *viewAs() const
    {
        return new FumianTargetCard;
    }
};

class Fumian : public TriggerSkill
{
public:
    Fumian() : TriggerSkill("fumian")
    {
        events << EventPhaseStart << DrawNCards << TargetChosen;
        view_as_skill = new dummyVS;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == DrawNCards)
            return 6;
        return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            switch (player->getPhase()) {
            case Player::Start: {
                if (TriggerSkill::triggerable(player)) {
                    QString lastchoice = player->tag["FumianLastChoice"].toString();
                    int i = 1,j = 1;
                    if (lastchoice == "draw")
                        i++;
                    else if (lastchoice == "target")
                        j++;
                    if (room->askForSkillInvoke(player, "skill_ask", "prompt:::"+objectName())) {
                        QString choice = room->askForChoice(player, objectName(), "draw+target+cancel", QVariant(),
                                "@fumian-choose:::" + QString::number(j) + ":" + QString::number(i));
                        if (choice != "cancel") {
                            LogMessage log;
                            log.type = "#InvokeSkill";
                            log.from = player;
                            log.arg = objectName();
                            room->sendLog(log);

                            room->notifySkillInvoked(player, objectName());
                            player->broadcastSkillInvoke(objectName());

                            player->tag["FumianChoice"] = choice;
                        }
                    }
                }
                break;
            }
            case Player::NotActive: {
                player->tag["FumianLastChoice"] = player->tag["FumianChoice"];
                player->tag.remove("FumianChoice");
                break;
            }
            default:
                break;
            }
        } else if (triggerEvent == DrawNCards) {
            QString choice = player->tag["FumianChoice"].toString();
            QString lastchoice = player->tag["FumianLastChoice"].toString();
            if (choice == "draw") {
                LogMessage log;
                log.type = "#SkillForce";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);
                player->drawCards((lastchoice == "target" ? 2 : 1), "fumian");
            }
        } else if (triggerEvent == TargetChosen) {
            QString choice = player->tag["FumianChoice"].toString();
            QString lastchoice = player->tag["FumianLastChoice"].toString();
            if (choice != "target" || player->hasFlag("FumianTargetUsed")) return false;
            int num = (lastchoice == "draw" ? 2 : 1);
            CardUseStruct use = data.value<CardUseStruct>();
            if ((use.card->getTypeId() != Card::TypeBasic && !use.card->isNDTrick()) || !use.card->isRed()) return false;
            if (use.card->isKindOf("Collateral") || use.card->isKindOf("BeatAnother")) return false;
            if (use.card->isKindOf("Slash") && use.card->hasFlag("slashDisableExtraTarget")) return false;
            QStringList available_targets;
            QList<ServerPlayer *> extra_targets;
            room->setCardFlag(use.card, "Global_NoDistanceChecking");
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                if (use.card->targetFilter(QList<const Player *>(), p, player))
                    available_targets.append(p->objectName());
            }
            room->setCardFlag(use.card, "-Global_NoDistanceChecking");
            if (!available_targets.isEmpty()) {
                room->setPlayerProperty(player, "fumian_available_targets", available_targets.join("+"));
                player->tag["fumian-use"] = data;
                room->setPlayerMark(player, "fumian_num", num);
                room->askForUseCard(player, "@@fumian_target", "@fumian-add:::" + use.card->objectName() + ":" + QString::number(num));
                room->setPlayerMark(player, "fumian_num", 0);
                player->tag.remove("fumian-use");
                room->setPlayerProperty(player, "fumian_available_targets", QString());
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("FumianExtraTarget")) {
                        p->setFlags("-FumianExtraTarget");
                        extra_targets << p;
                    }
                }
            }

            if (!extra_targets.isEmpty()) {
                room->setPlayerFlag(player, "FumianTargetUsed");
                foreach (ServerPlayer *extra, extra_targets)
                    use.to.append(extra);
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};

class Daiyan : public PhaseChangeSkill
{
public:
    Daiyan() : PhaseChangeSkill("daiyan")
    {
        view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        switch (player->getPhase()) {
        case Player::Finish: {
            if (TriggerSkill::triggerable(player)) {
                ServerPlayer *last = player->tag["DaiyanLastTarget"].value<ServerPlayer *>();
                if (last)
                    room->addPlayerTip(last, "#daiyan");
                ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@daiyan-target", true, true);
                if (last)
                    room->removePlayerTip(last, "#daiyan");
                if (target) {
                    player->broadcastSkillInvoke(objectName());
                    player->tag["DaiyanTarget"] = QVariant::fromValue(target);
                    QList<int> peachs;
                    foreach (int card_id, room->getDrawPile())
                        if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeBasic && Sanguosha->getCard(card_id)->getSuit() == Card::Heart)
                            peachs << card_id;
                    if (peachs.isEmpty()){
                        LogMessage log;
                        log.type = "$SearchFailed";
                        log.from = player;
                        log.arg = "heartbasic";
                        room->sendLog(log);
                    } else {
                        int index = qrand() % peachs.length();
                        int id = peachs.at(index);
                        target->obtainCard(Sanguosha->getCard(id), true);
                    }
                    if (last && last == target)
                        room->loseHp(target);
                }
            }
            break;
        }
        case Player::NotActive: {
            player->tag["DaiyanLastTarget"] = player->tag["DaiyanTarget"];
            player->tag.remove("DaiyanTarget");
            break;
        }
        default:
            break;
        }
        return false;
    }
};

WenguaCard::WenguaCard()
{
    target_fixed = true;
    m_skillName = "wengua";
    will_throw = false;
    handling_method = Card::MethodNone;
}

void WenguaCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_PUT, source->objectName(), QString(), "wengua", QString());
    room->moveCardTo(this, NULL, user_string == "top" ? Player::DrawPile : Player::DrawPileBottom, reason, false);
    if (user_string == "top") {
        if (room->getDrawPile().isEmpty())
            room->swapPile();
        int id = room->getDrawPile().last();
        source->obtainCard(Sanguosha->getCard(id), false);
    } else {
        source->drawCards(1, "wengua");
    }
}

class WenguaViewAsSkill : public OneCardViewAsSkill
{
public:
    WenguaViewAsSkill() : OneCardViewAsSkill("wengua")
    {
        filter_pattern = ".|.|.|.";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("WenguaCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        WenguaCard *wengua_card = new WenguaCard;
        wengua_card->addSubcard(originalCard->getId());
        return wengua_card;
    }
};

WenguaAttachCard::WenguaAttachCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "wengua_attach";
    mute = true;
}

void WenguaAttachCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *xushi = targets.first();
    room->setPlayerFlag(xushi, "WenguaInvoked");
    LogMessage log;
    log.type = "#InvokeOthersSkill";
    log.from = source;
    log.to << xushi;
    log.arg = "wengua";
    room->sendLog(log);
    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), xushi->objectName());
    xushi->broadcastSkillInvoke("wengua");

    room->notifySkillInvoked(xushi, "wengua");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), xushi->objectName(), "wengua", QString());
    room->obtainCard(xushi, this, reason, false);
    const Card *card = Sanguosha->getCard(getEffectiveId());
    xushi->setMark("wenguacard_id", getEffectiveId()); // For AI
    QString choice = room->askForChoice(xushi, "wengua", "top+bottom+cancel", QVariant(), "@wengua-choose:::"+card->objectName());
    xushi->setMark("wenguacard_id", getEffectiveId()); // For AI
    if (choice == "cancel") return;
    CardMoveReason reason2(CardMoveReason::S_REASON_PUT, xushi->objectName(), QString(), "wengua", QString());
    room->moveCardTo(this, NULL, choice == "top" ? Player::DrawPile : Player::DrawPileBottom, reason2, false);
    if (choice == "top") {
        if (room->getDrawPile().isEmpty())
            room->swapPile();
        int id = room->getDrawPile().last();
        source->obtainCard(Sanguosha->getCard(id), false);
        if (room->getDrawPile().isEmpty())
            room->swapPile();
        id = room->getDrawPile().last();
        xushi->obtainCard(Sanguosha->getCard(id), false);
    } else {
        source->drawCards(1, "wengua");
        xushi->drawCards(1, "wengua");
    }
}

bool WenguaAttachCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasSkill("wengua") && to_select != Self && !to_select->hasFlag("WenguaInvoked");
}

class WenguaAttach : public OneCardViewAsSkill
{
public:
    WenguaAttach() : OneCardViewAsSkill("wengua_attach")
    {
        filter_pattern = ".|.|.|.";
        attached_lord_skill = true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        foreach(const Player *sib, player->getAliveSiblings())
            if (sib->hasSkill("wengua") && !sib->hasFlag("WenguaInvoked"))
                return true;
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        WenguaAttachCard *wengua_card = new WenguaAttachCard;
        wengua_card->addSubcard(originalCard->getId());
        return wengua_card;
    }
};

class Wengua : public TriggerSkill
{
public:
    Wengua() : TriggerSkill("wengua")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << Debut;
        view_as_skill = new WenguaViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == GameStart && player->hasSkill("wengua", true))
            || (triggerEvent == EventAcquireSkill && data.toString() == "wengua")) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->hasSkill("wengua_attach"))
                    room->attachSkillToPlayer(p, "wengua_attach");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "wengua") {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->hasSkill("wengua_attach"))
                    room->detachSkillFromPlayer(p, "wengua_attach", true);
            }
        } else if (triggerEvent == Debut) {
            QList<ServerPlayer *> liufengs = room->findPlayersBySkillName("wengua");
            foreach (ServerPlayer *liufeng, liufengs) {
                if (player != liufeng && !player->hasSkill("wengua_attach")) {
                    room->attachSkillToPlayer(player, "wengua_attach");
                    break;
                }
            }
        }
        return false;
    }

    QString getSelectBox() const
    {
        return "top+bottom";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &selected, const QList<const Player *> &) const
    {
        return button_name.isEmpty() || !selected.isEmpty();
    }

};

class Fuzhu : public TriggerSkill
{
public:
    Fuzhu() : TriggerSkill("fuzhu")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Finish || !player->isMale()) return false;
        foreach (ServerPlayer *xushi, room->getOtherPlayers(player)) {
            if (!TriggerSkill::triggerable(xushi) || room->getDrawPile().length() > xushi->getHp()*10) continue;
            int times = room->getTag("SwapPile").toInt();
            int max_times = Sanguosha->getPlayerCount(room->getMode());
            bool swap_pile = false;
            for (int i = 0; i < max_times; i++) {
                if (xushi->isDead() || player->isDead() || room->getDrawPile().isEmpty() || room->getTag("SwapPile").toInt() > times) break;
                const Card *slash = NULL;
                for (int i = room->getDrawPile().length()-1; i >= 0; i--) {
                    const Card *card = Sanguosha->getCard(room->getDrawPile().at(i));
                    if (card->isKindOf("Slash") && xushi->canSlash(player, card, false)) {
                        slash = card;
                        break;
                    }
                }
                if (slash) {
                    if (i == 0) {
                        if (!room->askForSkillInvoke(xushi, objectName(), QVariant::fromValue(player))) break;
                        xushi->broadcastSkillInvoke(objectName());
                        swap_pile = true;
                    }
                    room->useCard(CardUseStruct(slash, xushi, player));
                } else
                    break;
            }
            if (swap_pile)
                room->swapPile();
        }
        return false;
    }
};

class ShouxiViewAsSkill : public OneCardViewAsSkill
{
public:
    ShouxiViewAsSkill() : OneCardViewAsSkill("shouxi")
    {
        response_pattern = "@@shouxi";
        guhuo_type = "sbtd";
    }

    bool viewFilter(const Card *to_select) const
    {
        QString classname = to_select->getClassName();
        if (to_select->isKindOf("Slash"))
            classname = "Slash";
        return Self->getMark("Shouxi_" + classname) == 0 && to_select->isVirtualCard();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return Sanguosha->cloneCard(originalCard->objectName());
    }
};

class Shouxi : public TriggerSkill
{
public:
    Shouxi() : TriggerSkill("shouxi")
    {
        events << TargetConfirmed;
        view_as_skill = new ShouxiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caojie, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            foreach (ServerPlayer *to, use.to) {
                if (use.from->isAlive() && to == caojie && TriggerSkill::triggerable(caojie)) {
                    if (!room->askForSkillInvoke(caojie, "skill_ask", "prompt:::"+objectName())) continue;
                    const Card *declare = room->askForCard(caojie, "@@shouxi", "@shouxi:" + use.from->objectName(), QVariant(), Card::MethodNone);
                    if (!declare) continue;
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = caojie;
                    log.arg = objectName();
                    room->sendLog(log);

                    room->notifySkillInvoked(caojie, objectName());
                    caojie->broadcastSkillInvoke(objectName());
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, caojie->objectName(), use.from->objectName());

                    QString classname;
                    if (declare->isKindOf("Slash"))
                        classname = "Slash";
                    else
                        classname = declare->getClassName();

                    room->setPlayerMark(caojie, "Shouxi_" + classname, 1);

                    QVariant dataforai = QVariant::fromValue(caojie);
                    if (room->askForCard(use.from, declare->getClassName(), "@shouxi-discard:"+caojie->objectName()+"::"+declare->objectName(), dataforai)) {
                        if (!caojie->isNude() && use.from->isAlive() && caojie->isAlive()) {
                            int card_id = room->askForCardChosen(use.from, caojie, "he", objectName());
                            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, use.from->objectName());
                            room->obtainCard(use.from, Sanguosha->getCard(card_id), reason, false);
                        }
                    } else {
                        use.nullified_list << caojie->objectName();
                        data = QVariant::fromValue(use);
                    }
                }
            }
        }
        return false;
    }
};

HuiminGraceCard::HuiminGraceCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

bool HuiminGraceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->property("huimin_targets").toString().split("+").contains(to_select->objectName());
}

void HuiminGraceCard::use(Room *room, ServerPlayer *caojie, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *first = targets.first();
    QStringList target_names = caojie->property("huimin_targets").toString().split("+");

    CardMoveReason reason(CardMoveReason::S_REASON_SHOW, caojie->objectName(), "huimin", QString());
    room->moveCardTo(this, NULL, Player::PlaceTable, reason, true);

    QList<int> card_ids = getSubcards();
    room->fillAG(card_ids);

    bool start = false;
    QList<ServerPlayer *> huimin_targets;
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        if (target_names.contains(p->objectName())) {
            if (p == first || start) {
                start = true;
                huimin_targets << p;
            }
        }
    }
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        if (target_names.contains(p->objectName())) {
            if (p == first) break;
            huimin_targets << p;
        }
    }

    foreach (ServerPlayer *p, huimin_targets) {
        if (card_ids.isEmpty()) break;
        int card_id = room->askForAG(p, card_ids, false, "huimin");
        card_ids.removeOne(card_id);
        room->takeAG(p, card_id);
    }
    room->clearAG();
    if (!card_ids.isEmpty()) {
        DummyCard *dummy = new DummyCard(card_ids);
        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "huimin", QString());
        room->throwCard(dummy, reason, NULL);
        delete dummy;
    }
}

class HuiminGrace : public ViewAsSkill
{
public:
    HuiminGrace() : ViewAsSkill("huimingrace")
    {
        response_pattern = "@@huimingrace!";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (to_select->isEquipped())
            return false;

        QStringList targets = Self->property("huimin_targets").toString().split("+");
        return selected.length() < qMin(targets.length(), Self->getHandcardNum());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        QStringList targets = Self->property("huimin_targets").toString().split("+");
        if (cards.length() != qMin(targets.length(), Self->getHandcardNum()))
            return NULL;

        HuiminGraceCard *card = new HuiminGraceCard;
        card->addSubcards(cards);
        return card;
    }
};

class Huimin : public PhaseChangeSkill
{
public:
    Huimin() : PhaseChangeSkill("huimin")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *caojie) const
    {
        if (caojie->getPhase() == Player::Finish) {
            Room *room = caojie->getRoom();
            QList<ServerPlayer *> targets;
            QStringList target_names;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getHandcardNum() < p->getHp()) {
                    targets << p;
                    target_names << p->objectName();
                }
            }
            if (targets.isEmpty()) return false;
            if (room->askForSkillInvoke(caojie, objectName(), "prompt:::"+QString::number(targets.length()))) {
                caojie->broadcastSkillInvoke(objectName());
                foreach (ServerPlayer *p, targets)
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, caojie->objectName(), p->objectName());
                caojie->drawCards(targets.length(), objectName());
                int n = qMin(targets.length(), caojie->getHandcardNum());
                if (n < 1) return false;
                room->setPlayerProperty(caojie, "huimin_targets", target_names.join("+"));
                if (!room->askForUseCard(caojie, "@@huimingrace!", "@huimin:::"+QString::number(n), QVariant(), Card::MethodNone)) {
                    QList<int> to_give = caojie->handCards().mid(0, n);
                    HuiminGraceCard *huimin_card = new HuiminGraceCard;
                    huimin_card->addSubcards(to_give);
                    QList<ServerPlayer *> s_targets;
                    s_targets << targets.first();
                    huimin_card->use(room, caojie, s_targets);
                    delete huimin_card;
                }
                room->setPlayerProperty(caojie, "huimin_targets", QVariant());
            }
        }
        return false;
    }
};

class Pizhuan : public TriggerSkill
{
public:
    Pizhuan() : TriggerSkill("pizhuan")
    {
        events << CardUsed << CardResponded << TargetConfirmed;
    }

    bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirmed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.from != player && use.card && use.card->getTypeId() != Card::TypeSkill && use.card->getSuit() == Card::Spade){
                foreach (ServerPlayer *to, use.to) {
                    if (to == player)
                        InvokePizhuan(player);
                }
            }

        } else {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    card = resp.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill && card->getSuit() == Card::Spade)
                InvokePizhuan(player);
        }
        return false;
    }

private:
    void InvokePizhuan(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPile("books").length() < 4 && player->askForSkillInvoke("pizhuan")) {
            player->broadcastSkillInvoke("pizhuan");
            QList<ServerPlayer *> players;
            players << player;
            player->addToPile("books", room->drawCard(), false, players);
        }
    }
};

class PizhuanKeep : public MaxCardsSkill
{
public:
    PizhuanKeep() : MaxCardsSkill("#pizhuan-keep")
    {
        frequency = Frequent;
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("pizhuan"))
            return target->getPile("books").length();
        else
            return 0;
    }
};

TongboAllotCard::TongboAllotCard()
{
    will_throw = false;
    mute = true;
    handling_method = Card::MethodNone;
}

void TongboAllotCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QList<int> rende_list = StringList2IntList(source->property("tongbo_give").toString().split("+"));
    foreach (int id, subcards) {
        rende_list.removeOne(id);
    }
    room->setPlayerProperty(source, "tongbo_give", IntList2StringList(rende_list).join("+"));

    QList<int> give_list = StringList2IntList(target->property("rende_give").toString().split("+"));
    foreach (int id, subcards) {
        give_list.append(id);
    }
    room->setPlayerProperty(target, "rende_give", IntList2StringList(give_list).join("+"));
}

class TongboAllot : public ViewAsSkill
{
public:
    TongboAllot() : ViewAsSkill("tongbo_allot")
    {
        expand_pile = "books";
        response_pattern = "@@tongbo_allot!";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QList<int> tongbo_list = StringList2IntList(Self->property("tongbo_give").toString().split("+"));
        return tongbo_list.contains(to_select->getEffectiveId());
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.isEmpty()) return NULL;
        TongboAllotCard *tongbo_card = new TongboAllotCard;
        tongbo_card->addSubcards(cards);
        return tongbo_card;
    }
};

TongboCard::TongboCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    target_fixed = true;
}

void TongboCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    ServerPlayer *caiyong = card_use.from;
    QList<int> pile = caiyong->getPile("books");
    QList<int> subCards = card_use.card->getSubcards();
    QList<int> to_handcard;
    QList<int> to_pile;
    foreach (int id, (subCards + pile).toSet()) {
        if (!subCards.contains(id))
            to_handcard << id;
        else if (!pile.contains(id))
            to_pile << id;
    }

    Q_ASSERT(to_handcard.length() == to_pile.length());

    if (to_pile.length() == 0 || to_handcard.length() != to_pile.length())
        return;

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = caiyong;
    log.arg = "tongbo";
    room->sendLog(log);
    room->notifySkillInvoked(caiyong, "tongbo");
    caiyong->broadcastSkillInvoke("tongbo");

    caiyong->addToPile("books", to_pile, false);

    DummyCard *to_handcard_x = new DummyCard(to_handcard);
    CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, caiyong->objectName());
    room->obtainCard(caiyong, to_handcard_x, reason, false);
    to_handcard_x->deleteLater();

    pile = caiyong->getPile("books");
    if (pile.length() != 4) return;
    QStringList suitlist;
    foreach(int card_id, pile){
        const Card *card = Sanguosha->getCard(card_id);
        QString suit = card->getSuitString();
        if (!suitlist.contains(suit))
            suitlist << suit;
        else{
            return;
        }
    }

    room->setPlayerProperty(caiyong, "tongbo_give", IntList2StringList(pile).join("+"));
    do {
        const Card *use = room->askForUseCard(caiyong, "@@tongbo_allot!", "@tongbo-give", QVariant(), Card::MethodNone);
        pile = StringList2IntList(caiyong->property("tongbo_give").toString().split("+"));
        if (use == NULL){
            TongboAllotCard *tongbo_card = new TongboAllotCard;
            tongbo_card->addSubcards(pile);
            QList<ServerPlayer *> targets;
            targets << room->getOtherPlayers(caiyong).first();
            tongbo_card->use(room, caiyong, targets);
            delete tongbo_card;
            break;
        }
    } while (!pile.isEmpty() && caiyong->isAlive());
    room->setPlayerProperty(caiyong, "tongbo_give", QString());
    QList<CardsMoveStruct> moves;
    foreach (ServerPlayer *p, room->getAllPlayers()) {
        QList<int> give_list = StringList2IntList(p->property("rende_give").toString().split("+"));
        if (give_list.isEmpty())
            continue;
        room->setPlayerProperty(p, "rende_give", QString());
        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, p->objectName(), "tongbo", QString());
        CardsMoveStruct move(give_list, p, Player::PlaceHand, reason);
        moves.append(move);
    }
    if (!moves.isEmpty())
        room->moveCardsAtomic(moves, false);
}

class TongboVS : public ViewAsSkill
{
public:
    TongboVS() : ViewAsSkill("tongbo")
    {
        response_pattern = "@@tongbo";
        expand_pile = "books";
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *) const
    {
        return selected.length() < Self->getPile("books").length();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getPile("books").length()) {
            TongboCard *c = new TongboCard;
            c->addSubcards(cards);
            return c;
        }
        return NULL;
    }
};


class Tongbo : public TriggerSkill
{
public:
    Tongbo() : TriggerSkill("tongbo")
    {
        view_as_skill = new TongboVS;
        events << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return TriggerSkill::triggerable(target) && target->getPile("books").length() > 0
            && target->getPhase() == Player::Draw;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caiyong, QVariant &) const
    {
        room->askForUseCard(caiyong, "@@tongbo", "@tongbo-exchange:::"+QString::number(caiyong->getPile("books").length()), QVariant(), Card::MethodNone);
        return false;
    }
};

class Qingxian : public TriggerSkill
{
public:
    Qingxian() : TriggerSkill("qingxian")
    {
        events << HpRecover << Damaged;
        view_as_skill = new dummyVS;
    }

    static bool NobodyDying(Room *room)
    {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->hasFlag("Global_Dying"))
                return false;
        }
        return true;
    }

    static const Card *UseEquip(ServerPlayer *target)
    {
        if (!target->isAlive()) return NULL;
        Room *room = target->getRoom();
        QList<int> equips;
        foreach (int card_id, room->getDrawPile())
            if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeEquip)
                equips << card_id;
        if (equips.isEmpty()){
            LogMessage log;
            log.type = "$SearchFailed";
            log.from = target;
            log.arg = "equip";
            room->sendLog(log);
            return NULL;
        }
        int index = qrand() % equips.length();
        int id = equips.at(index);
        const Card *equip = Sanguosha->getCard(id);
        room->useCard(CardUseStruct(equip, target, target));
        return equip;
    }

    static const Card *ThrowEquip(ServerPlayer *target, QString skill_name)
    {
        if (!target->isAlive()) return NULL;
        Room *room = target->getRoom();
        QList<int> defaults = target->forceToDiscard(1, true, true, "EquipCard");
        if (defaults.isEmpty()) return NULL;
        const Card *equip = room->askForCard(target, ".Equip!", QString("@%1-discard").arg(skill_name), QVariant());
        if (equip == NULL) {
            equip = Sanguosha->getCard(defaults.first());
            room->throwCard(equip, target);
        }
        return equip;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!NobodyDying(room)) return false;
        ServerPlayer *target = NULL;
        if (triggerEvent == HpRecover)
            target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@qingxian-target", true, true);
        else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->isAlive() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage.from))) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.from->objectName());
                target = damage.from;
            }
        }
        if (target) {
            player->broadcastSkillInvoke(objectName());
            QStringList choices;
            choices << "losehp";
            if (target->isWounded())
                choices << "recover";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(), "@qingxian-choose:"+target->objectName(), "losehp+recover");
            const Card *equip;
            if (choice == "losehp") {
                room->loseHp(target);
                equip = Qingxian::UseEquip(target);
            } else {
                room->recover(target, RecoverStruct(player));
                equip = Qingxian::ThrowEquip(target, objectName());
            }
            if (equip && equip->getSuit() == Card::Club)
                player->drawCards(1, objectName());
        }
        return false;
    }

};

class Jixian : public MasochismSkill
{
public:
    Jixian() : MasochismSkill("jixian")
    {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        Room *room = player->getRoom();
        if (!Qingxian::NobodyDying(room)) return;
        ServerPlayer *from = damage.from;
        if (from && from->isAlive() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(from))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());
            player->broadcastSkillInvoke(objectName());
            room->loseHp(from);
            Qingxian::UseEquip(from);
        }
    }
};

class Liexian : public TriggerSkill
{
public:
    Liexian() : TriggerSkill("liexian")
    {
        events << HpRecover;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!Qingxian::NobodyDying(room)) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@liexian-target", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            room->loseHp(target);
            Qingxian::UseEquip(target);
        }
        return false;
    }
};

class Rouxian : public MasochismSkill
{
public:
    Rouxian() : MasochismSkill("rouxian")
    {
    }

    virtual void onDamaged(ServerPlayer *player, const DamageStruct &damage) const
    {
        Room *room = player->getRoom();
        if (!Qingxian::NobodyDying(room)) return;
        ServerPlayer *from = damage.from;
        if (from && from->isAlive() && from->isWounded() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(from))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), from->objectName());
            player->broadcastSkillInvoke(objectName());
            room->recover(from, RecoverStruct(player));
            Qingxian::ThrowEquip(from, objectName());
        }
    }
};

class Hexian : public TriggerSkill
{
public:
    Hexian() : TriggerSkill("hexian")
    {
        events << HpRecover;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (!Qingxian::NobodyDying(room)) return false;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->isWounded())
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@hexian-target", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            room->recover(target, RecoverStruct(player));
            Qingxian::ThrowEquip(target, objectName());
        }
        return false;
    }
};

class Juexiang : public TriggerSkill
{
public:
    Juexiang() : TriggerSkill("juexiang")
    {
        events << Death << EventPhaseStart;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Death && player->hasSkill(objectName()) && data.value<DeathStruct>().who == player) {
            ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@juexiang-target", true, true);
            if (target) {
                player->broadcastSkillInvoke(objectName());
                QStringList qingxian;
                qingxian << "jixian" << "liexian" << "rouxian" << "hexian";
                QString skill_name = qingxian.at(qrand() % 3);
                room->acquireSkill(target, skill_name);
                room->addPlayerTip(target, "#"+skill_name);
                room->addPlayerTip(target, "#juexiang");
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart)
            room->removePlayerTip(player, "#juexiang");
        return false;
    }
};

class JuexiangProhibit : public ProhibitSkill
{
public:
    JuexiangProhibit() : ProhibitSkill("#juexiang-prohibit")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->isKindOf("SkillCard")) return false;

        return (from && to->getMark("#juexiang") > 0 && card->getSuit() == Card::Club && from != to);
    }
};

class Jianzheng : public TriggerSkill
{
public:
    Jianzheng() : TriggerSkill("jianzheng")
    {
        events << TargetSpecifying;
        view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash")) return false;
        foreach (ServerPlayer *qinmi, room->getOtherPlayers(player)) {
            if (player->inMyAttackRange(qinmi) && !use.to.contains(qinmi)&& TriggerSkill::triggerable(qinmi) && !qinmi->isKongcheng()) {
                const Card *c = room->askForCard(qinmi, ".", "@jianzheng-put:"+use.from->objectName(), data, Card::MethodNone);
                if (c == NULL) continue;
                LogMessage log;
                log.from = qinmi;
                log.type = "#InvokeSkill";
                log.arg = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(qinmi, objectName());
                qinmi->broadcastSkillInvoke(objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_PUT, qinmi->objectName(), QString(), "jianzheng", QString());
                room->moveCardTo(c, NULL, Player::DrawPile, reason, false);
                use.to.clear();
                if (!use.card->isBlack())
                    use.to.append(qinmi);
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};

class Zhuandui : public TriggerSkill
{
public:
    Zhuandui() : TriggerSkill("zhuandui")
    {
        events << TargetSpecified << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash")) {
            if (triggerEvent == TargetSpecified) {
                QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
                int index = 0;
                foreach (ServerPlayer *to, use.to) {
                    if (!TriggerSkill::triggerable(player) || player->isKongcheng()) break;
                    if (!to->isKongcheng() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(to))) {
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), to->objectName());
                        player->broadcastSkillInvoke(objectName());
                        if (player->pindian(to, objectName())) {
                            LogMessage log;
                            log.type = "#NoJink";
                            log.from = to;
                            room->sendLog(log);
                            jink_list.replace(index, QVariant(0));
                        }
                    }
                    index++;
                }
                player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
            } else {
                foreach (ServerPlayer *to, use.to) {
                    if (use.from == NULL || use.from->isDead() || use.from->isKongcheng()) break;
                    if (to == player && !player->isKongcheng() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(use.from))) {
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), use.from->objectName());
                        player->broadcastSkillInvoke(objectName());
                        if (player->pindian(use.from, objectName())) {
                            use.nullified_list << player->objectName();
                            data = QVariant::fromValue(use);
                        }
                    }
                }
            }
        }
        return false;
    }
};

TianbianCard::TianbianCard()
{
    target_fixed = true;
}

const Card *TianbianCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    LogMessage log;
    log.from = user;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);
    room->notifySkillInvoked(user, "tianbian");
    user->broadcastSkillInvoke("tianbian");

    int id = room->drawCard();
    return Sanguosha->getCard(id);
}

class TianbianViewAsSkill : public ZeroCardViewAsSkill
{
public:
    TianbianViewAsSkill() : ZeroCardViewAsSkill("tianbian")
    {
        response_pattern = "pindian";
    }

    virtual const Card *viewAs() const
    {
        return new TianbianCard;
    }
};

class Tianbian : public TriggerSkill
{
public:
    Tianbian() : TriggerSkill("tianbian")
    {
        events << PindianVerifying;
        view_as_skill = new TianbianViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const
    {
        PindianStruct *pindian = data.value<PindianStruct *>();
        bool isFrom = (pindian->from == player);
        if (isFrom && pindian->from_card->getSuit() != Card::Heart) return false;
        if (!isFrom && pindian->to_card->getSuit() != Card::Heart) return false;
        LogMessage log;
        log.type = "$TianbianNumber";
        log.from = player;
        log.arg = objectName();
        log.arg2 = QString::number(13);
        room->sendLog(log);
        room->notifySkillInvoked(player, objectName());
        player->broadcastSkillInvoke(objectName());
        if (isFrom)
            pindian->from_number = 13;
        else
            pindian->to_number = 13;

        return false;
    }
};

class Funan : public TriggerSkill
{
public:
    Funan() : TriggerSkill("funan")
    {
        events << CardUsed << CardResponded << EventPhaseStart << PreCardsMoveOneTime;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardUsed || triggerEvent == CardResponded) {
            const Card *card = NULL;
            const Card *e_card = NULL;
            ServerPlayer *xuezong = NULL;
            if (triggerEvent == CardUsed) {
                CardUseStruct use = data.value<CardUseStruct>();
                QVariant m_data = use.m_data;
                if (m_data.canConvert<CardEffectStruct>()) {
                    CardEffectStruct effect = m_data.value<CardEffectStruct>();
                    xuezong = effect.from;
                    e_card = effect.card;
                    card = use.card;
                }
            } else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                QVariant m_data = response.m_data;
                if (m_data.canConvert<CardEffectStruct>()) {
                    CardEffectStruct effect = m_data.value<CardEffectStruct>();
                    xuezong = effect.from;
                    e_card = effect.card;
                    card = response.m_card;
                } else if (m_data.canConvert<SlashEffectStruct>()) {
                    SlashEffectStruct effect = m_data.value<SlashEffectStruct>();
                    xuezong = effect.from;
                    e_card = effect.slash;
                    card = response.m_card;
                }
            }
            if (card && card->getTypeId() != Card::TypeSkill && room->isAllOnPlace(card, Player::PlaceTable)) {
                if (!TriggerSkill::triggerable(xuezong) || xuezong == player) return false;
                if (xuezong->getMark("funan_upgrade") == 0 && (player->isDead() || !room->isAllOnPlace(e_card, Player::PlaceTable))) return false;
                if (xuezong->askForSkillInvoke(objectName())) {
                    if (xuezong->getMark("funan_upgrade") == 0) {
                        player->obtainCard(e_card);
                        QStringList funan_ids = player->property("funan_record").toString().split("+");
                        foreach (int id, e_card->getSubcards()) {
                            if (room->getCardOwner(id) == player && room->getCardPlace(id) == Player::PlaceHand) {
                                funan_ids << QString::number(id);
                            }
                        }
                        room->setPlayerProperty(player, "funan_record", funan_ids.join("+"));
                    }
                    xuezong->obtainCard(card);
                }
            }
        } else if (triggerEvent == PreCardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && !player->property("funan_record").toString().isEmpty()) {
                QStringList funan_ids = player->property("funan_record").toString().split("+");
                QStringList funan_copy = funan_ids;
                foreach (QString card_data, funan_copy) {
                    int id = card_data.toInt();
                    if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                        funan_ids.removeOne(card_data);
                    }
                }
                room->setPlayerProperty(player, "funan_record", funan_ids.join("+"));
            }
        } else if (triggerEvent == EventPhaseStart && player->getPhase() == Player::NotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                room->setPlayerProperty(p, "funan_record", QStringList());
            }
        }
        return false;
    }
};

class FunanCardLimited : public CardLimitedSkill
{
public:
    FunanCardLimited() : CardLimitedSkill("#funan-limited")
    {
    }
    virtual bool isCardLimited(const Player *player, const Card *card, Card::HandlingMethod method) const
    {
        if (player->property("funan_record").toString().isEmpty()) return false;
        if (method == Card::MethodUse || method == Card::MethodResponse) {
            QStringList funan_ids = player->property("funan_record").toString().split("+");
            foreach (QString card_str, funan_ids) {
                bool ok;
                int id = card_str.toInt(&ok);
                if (!ok) continue;
                if (!card->isVirtualCard()) {
                    if (card->getEffectiveId() == id)
                        return true;
                } else if (card->getSubcards().contains(id))
                    return true;
            }
        }
        return false;
    }
};

class Jiexun : public PhaseChangeSkill
{
public:
    Jiexun() : PhaseChangeSkill("jiexun")
    {
        view_as_skill = new dummyVS;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        if (player->getPhase() == Player::Finish) {
            Room *room = player->getRoom();
            int x = 0;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                foreach(const Card *card, p->getCards("ej")){
                    if (card->getSuit() == Card::Diamond)
                        x++;
                }
            }
            int y = player->getMark("JiexueTimes");
            ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(),
                    "@jiexun-target:::"+QString::number(x)+":"+QString::number(y), true, true);
            if (target) {
                player->broadcastSkillInvoke(objectName());
                room->addPlayerMark(player, "JiexueTimes");
                if (x>0)
                    target->drawCards(x, objectName());
                if (y < 1) return false;
                if (target->forceToDiscard(y,true).length() >= target->getCardCount()) {
                    room->detachSkillFromPlayer(player, objectName());
                    QString translation = Sanguosha->translate(":funan-upgrade");
                    room->setPlayerMark(player, "funan_upgrade", 1);
                    Sanguosha->addTranslationEntry(":funan", translation.toStdString().c_str());
                    JsonArray args;
                    args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
                }
                room->askForDiscard(target, objectName(), y, y, false, true, "@jiexun-discard:::"+QString::number(y));
            }
        }
        return false;
    }
};

YJCM2017Package::YJCM2017Package()
: Package("YJCM2017")
{
    General *xinxianying = new General(this, "xinxianying", "wei", 3, false);
    xinxianying->addSkill(new Zhongjian);
    xinxianying->addSkill(new Caishi);
    xinxianying->addSkill(new CaishiProhibit);
    related_skills.insertMulti("caishi", "#caishi-prohibit");

    General *wuxian = new General(this, "wuxian", "shu", 3, false);
    wuxian->addSkill(new Fumian);
    wuxian->addSkill(new Daiyan);

    General *xushi = new General(this, "xushi", "wu", 3, false);
    xushi->addSkill(new Wengua);
    xushi->addSkill(new Fuzhu);

    General *caojie = new General(this, "caojie", "qun", 3, false);
    caojie->addSkill(new Shouxi);
    caojie->addSkill(new Huimin);

    General *caiyong = new General(this, "caiyong", "qun", 3);
    caiyong->addSkill(new Pizhuan);
    caiyong->addSkill(new PizhuanKeep);
    caiyong->addSkill(new DetachEffectSkill("pizhuan", "book"));
    related_skills.insertMulti("pizhuan", "#pizhuan-keep");
    related_skills.insertMulti("pizhuan", "#pizhuan-clear");
    caiyong->addSkill(new Tongbo);

    General *jikang = new General(this, "jikang", "wei", 3);
    jikang->addSkill(new Qingxian);
    jikang->addSkill(new Juexiang);
    jikang->addSkill(new JuexiangProhibit);
    related_skills.insertMulti("juexiang", "#juexiang-prohibit");
    jikang->addRelateSkill("jixian");
    jikang->addRelateSkill("liexian");
    jikang->addRelateSkill("rouxian");
    jikang->addRelateSkill("hexian");

    General *qinmi = new General(this, "qinmi", "shu", 3);
    qinmi->addSkill(new Jianzheng);
    qinmi->addSkill(new Zhuandui);
    qinmi->addSkill(new Tianbian);

    General *xuezong = new General(this, "xuezong", "wu", 3);
    xuezong->addSkill(new Funan);
    xuezong->addSkill(new FunanCardLimited);
    xuezong->addSkill(new Jiexun);
    related_skills.insertMulti("funan", "#funan-limited");

    addMetaObject<ZhongjianCard>();
    addMetaObject<FumianTargetCard>();
    addMetaObject<WenguaCard>();
    addMetaObject<WenguaAttachCard>();
    addMetaObject<HuiminGraceCard>();
    addMetaObject<TongboCard>();
    addMetaObject<TongboAllotCard>();
    addMetaObject<TianbianCard>();

    skills << new XinxianyingMaxCards << new FumianTarget << new WenguaAttach << new HuiminGrace
           << new TongboAllot << new Jixian << new Liexian << new Rouxian << new Hexian;
}

ADD_PACKAGE(YJCM2017)
