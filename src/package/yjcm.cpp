#include "yjcm.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "engine.h"
#include "settings.h"
#include "ai.h"
#include "general.h"

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

class Luoying : public TriggerSkill
{
public:
    Luoying() : TriggerSkill("luoying")
    {
        events << CardsMoveOneTime;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *caozhi, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (move.from == caozhi || move.from == NULL)
            return false;
        if (move.to_place == Player::DiscardPile
            && ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD
            || move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE)) {
            QList<int> card_ids;
            int i = 0;
            foreach (int card_id, move.card_ids) {
                if ((Sanguosha->getCard(card_id)->getSuit() == Card::Club
                    && ((move.reason.m_reason == CardMoveReason::S_REASON_JUDGEDONE
                    && move.from_places[i] == Player::PlaceJudge
                    && move.to_place == Player::DiscardPile)
                    || (move.reason.m_reason != CardMoveReason::S_REASON_JUDGEDONE
                    && (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip))))
                    && move.active_ids.contains(card_id) && room->getDiscardPile().contains(card_id))
                    card_ids << card_id;
                i++;
            }
            if (card_ids.isEmpty())
                return false;
            else if (caozhi->askForSkillInvoke(this, data)) {
                caozhi->broadcastSkillInvoke(objectName());
				int ai_delay = Config.AIDelay;
                Config.AIDelay = 0;
                while (card_ids.length() > 1) {
                    room->fillAG(card_ids, caozhi);
                    int id = room->askForAG(caozhi, card_ids, true, objectName());
                    if (id == -1) {
                        room->clearAG(caozhi);
                        break;
                    }
                    card_ids.removeOne(id);
                    room->clearAG(caozhi);
                }
                Config.AIDelay = ai_delay;

                if (!card_ids.isEmpty()) {
                    foreach (int id, card_ids) {
                        move.active_ids.removeOne(id);
                    }
                    data = QVariant::fromValue(move);
					DummyCard *dummy = new DummyCard(card_ids);
                    room->obtainCard(caozhi, dummy);
                    delete dummy;
                }
            }
        }
        return false;
    }
};

class Jiushi : public ZeroCardViewAsSkill
{
public:
    Jiushi() : ZeroCardViewAsSkill("jiushi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Analeptic::IsAvailable(player) && player->faceUp();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return pattern.contains("analeptic") && player->faceUp();
    }

    virtual const Card *viewAs() const
    {
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("jiushi");
        return analeptic;
    }

};

class JiushiFlip : public TriggerSkill
{
public:
    JiushiFlip() : TriggerSkill("#jiushi-flip")
    {
        events << CardExtraCost << PreDamageDone << DamageComplete;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardExtraCost) {
            if (!data.canConvert<CardUseStruct>()) return false;
            if (data.value<CardUseStruct>().card->getSkillName() == "jiushi")
                player->turnOver();
        } else if (triggerEvent == PreDamageDone) {
            player->tag["PredamagedFace"] = !player->faceUp();
        } else if (triggerEvent == DamageComplete) {
            bool facedown = player->tag.value("PredamagedFace").toBool();
            player->tag.remove("PredamagedFace");
            if (facedown && !player->faceUp() && player->askForSkillInvoke("jiushi", data)) {
                player->broadcastSkillInvoke("jiushi");
                player->turnOver();
            }
        }

        return false;
    }
};

class Wuyan : public TriggerSkill
{
public:
    Wuyan() : TriggerSkill("wuyan")
    {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->getTypeId() == Card::TypeTrick) {
            if (triggerEvent == DamageInflicted && TriggerSkill::triggerable(player)) {
                LogMessage log;
                log.type = "#WuyanGood";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());

                return true;
            } else if (triggerEvent == DamageCaused && damage.from && TriggerSkill::triggerable(damage.from)) {
                LogMessage log;
                log.type = "#WuyanBad";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());

                return true;
            }
        }

        return false;
    }
};

JujianCard::JujianCard()
{
}

bool JujianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void JujianCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    QStringList choicelist;
    choicelist << "draw";
    if (effect.to->isWounded())
        choicelist << "recover";
    if (!effect.to->faceUp() || effect.to->isChained())
        choicelist << "reset";
    QString choice = room->askForChoice(effect.to, "jujian", choicelist.join("+"), QVariant(), QString(), "draw+recover+reset");

    if (choice == "draw")
        effect.to->drawCards(2, "jujian");
    else if (choice == "recover")
        room->recover(effect.to, RecoverStruct(effect.from));
    else if (choice == "reset") {
        if (effect.to->isChained())
            room->setPlayerProperty(effect.to, "chained", false);
        if (!effect.to->faceUp())
            effect.to->turnOver();
    }
}

class JujianViewAsSkill : public OneCardViewAsSkill
{
public:
    JujianViewAsSkill() : OneCardViewAsSkill("jujian")
    {
        filter_pattern = "^BasicCard!";
        response_pattern = "@@jujian";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        JujianCard *jujianCard = new JujianCard;
        jujianCard->addSubcard(originalCard);
        return jujianCard;
    }
};

class Jujian : public PhaseChangeSkill
{
public:
    Jujian() : PhaseChangeSkill("jujian")
    {
        view_as_skill = new JujianViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *xushu) const
    {
        Room *room = xushu->getRoom();
        if (xushu->getPhase() == Player::Finish && xushu->canDiscard(xushu, "he"))
            room->askForUseCard(xushu, "@@jujian", "@jujian-card", QVariant(), Card::MethodDiscard);
        return false;
    }
};

class Enyuan : public TriggerSkill
{
public:
    Enyuan() : TriggerSkill("enyuan")
    {
        events << CardsMoveOneTime << Damaged;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.to == player && move.from && move.from->isAlive() && move.from != move.to && move.to_place == Player::PlaceHand) {
                int n = 0;
                for (int i = 0; i < move.card_ids.length(); i++) {
                    if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip)
                        n++;
                }
                if (n < 2) return false;
                move.from->setFlags("EnyuanDrawTarget");
                bool invoke = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(move.from));
                move.from->setFlags("-EnyuanDrawTarget");
                if (invoke) {
                    player->broadcastSkillInvoke(objectName(), 1);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), move.from->objectName());
                    room->drawCards((ServerPlayer *)move.from, 1, objectName());
                }
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if (!source || source == player) return false;
            int x = damage.damage;
            for (int i = 0; i < x; i++) {
                if (source->isAlive() && player->isAlive() && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(source))) {
                    player->broadcastSkillInvoke(objectName(), 2);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), source->objectName());
                    const Card *card = NULL;
                    if (!source->isKongcheng())
                        card = room->askForExchange(source, objectName(), 1, 1, false, "EnyuanGive::" + player->objectName(), true);
                    if (card) {
                        CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(),
                            player->objectName(), objectName(), QString());
                        reason.m_playerId = player->objectName();
                        room->moveCardTo(card, source, player, Player::PlaceHand, reason);
                        delete card;
                    } else {
                        room->loseHp(source);
                    }
                } else {
                    break;
                }
            }
        }
        return false;
    }
};

class Xuanhuo : public PhaseChangeSkill
{
public:
    Xuanhuo() : PhaseChangeSkill("xuanhuo")
    {
		view_as_skill = new dummyVS;
    }

    virtual bool onPhaseChange(ServerPlayer *fazheng) const
    {
        Room *room = fazheng->getRoom();
        if (fazheng->getPhase() == Player::Draw) {
            ServerPlayer *to = room->askForPlayerChosen(fazheng, room->getOtherPlayers(fazheng), objectName(), "xuanhuo-invoke", true, true);
            if (to) {
                fazheng->broadcastSkillInvoke(objectName(), 1);
                room->drawCards(to, 2, objectName());
                if (!fazheng->isAlive() || !to->isAlive())
                    return true;

                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *vic, room->getOtherPlayers(to)) {
                    if (to->canSlash(vic))
                        targets << vic;
                }
                ServerPlayer *victim = NULL;
                if (!targets.isEmpty()) {
                    victim = room->askForPlayerChosen(fazheng, targets, "xuanhuo_slash", "@dummy-slash2:" + to->objectName());

                    LogMessage log;
                    log.type = "#CollateralSlash";
                    log.from = fazheng;
                    log.to << victim;
                    room->sendLog(log);
					
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, to->objectName(), victim->objectName());
                }

                if (victim == NULL || !room->askForUseSlashTo(to, victim, QString("xuanhuo-slash:%1:%2").arg(fazheng->objectName()).arg(victim->objectName()))) {
                    if (to->isNude())
                        return true;
                    fazheng->broadcastSkillInvoke(objectName(), 2);

                    QList<int> cards = room->askForCardsChosen(fazheng, to, "he", objectName(),2 , 2);

                    DummyCard dummy(cards);
                    room->moveCardTo(&dummy, fazheng, Player::PlaceHand, false);
                }

                return true;
            }
        }

        return false;
    }
};

class Xuanfeng : public TriggerSkill
{
public:
    Xuanfeng() : TriggerSkill("xuanfeng")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    void perform(Room *room, ServerPlayer *lingtong) const
    {
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
            if (lingtong->canDiscard(target, "he"))
                targets << target;
        }
        if (targets.isEmpty())
            return;

        ServerPlayer *first = room->askForPlayerChosen(lingtong, targets, objectName(), "xuanfeng-invoke", true, true);
            ServerPlayer *second = NULL;
            int first_id = -1;
            int second_id = -1;
            if (first != NULL) {
                lingtong->broadcastSkillInvoke(objectName());
                first_id = room->askForCardChosen(lingtong, first, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(first_id, first, lingtong);
            
            if (!lingtong->isAlive())
                return;
            targets.clear();
            foreach (ServerPlayer *target, room->getOtherPlayers(lingtong)) {
                if (lingtong->canDiscard(target, "he"))
                    targets << target;
            }
            if (!targets.isEmpty())
                second = room->askForPlayerChosen(lingtong, targets, objectName(), "xuanfeng-continue", true);
            if (second != NULL) {
				room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, lingtong->objectName(), second->objectName());
                second_id = room->askForCardChosen(lingtong, second, "he", "xuanfeng", false, Card::MethodDiscard);
                room->throwCard(second_id, second, lingtong);
            }
        }
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *lingtong, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            lingtong->setMark("xuanfeng", 0);
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from != lingtong)
                return false;

            if (lingtong->getPhase() == Player::Discard
                && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD)
                lingtong->addMark("xuanfeng", move.card_ids.length());

            if (move.from_places.contains(Player::PlaceEquip) && TriggerSkill::triggerable(lingtong))
                perform(room, lingtong);
        } else if (triggerEvent == EventPhaseEnd && TriggerSkill::triggerable(lingtong)
            && lingtong->getPhase() == Player::Discard && lingtong->getMark("xuanfeng") >= 2) {
            perform(room, lingtong);
        }

        return false;
    }
};

XianzhenCard::XianzhenCard()
{
}

bool XianzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self && !to_select->isKongcheng();
}

void XianzhenCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.from->pindian(effect.to, "xianzhen", NULL)) {
        QStringList assignee_list = effect.from->property("xianzhen_targets").toString().split("+");
        assignee_list << effect.to->objectName();
        room->setPlayerProperty(effect.from, "xianzhen_targets", assignee_list.join("+"));
        room->addPlayerMark(effect.to, "Armor_Nullified");
    } else {
        room->setPlayerCardLimitation(effect.from, "use", "Slash", true);
    }
}

class XianzhenViewAsSkill : public ZeroCardViewAsSkill
{
public:
    XianzhenViewAsSkill() : ZeroCardViewAsSkill("xianzhen")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("XianzhenCard") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new XianzhenCard;
    }
};

class Xianzhen : public TriggerSkill
{
public:
    Xianzhen() : TriggerSkill("xianzhen")
    {
        events << EventPhaseStart;
        view_as_skill = new XianzhenViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::NotActive;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *gaoshun, QVariant &) const
    {
        QStringList assignee_list = gaoshun->property("xianzhen_targets").toString().split("+");
        room->setPlayerProperty(gaoshun, "xianzhen_targets", QVariant());
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (assignee_list.contains(p->objectName()))
                room->removePlayerMark(p, "Armor_Nullified");
        }
        return false;
    }
};

class XianzhenTargetMod : public TargetModSkill
{
public:
    XianzhenTargetMod() : TargetModSkill("#xianzhen-target")
    {
        pattern = "^SkillCard";
    }

    int getResidueNum(const Player *from, const Card *, const Player *to) const
    {
        QStringList assignee_list = from->property("xianzhen_targets").toString().split("+");
        if (to && assignee_list.contains(to->objectName()))
            return 10000;
        return 0;
    }

    int getDistanceLimit(const Player *from, const Card *, const Player *to) const
    {
        QStringList assignee_list = from->property("xianzhen_targets").toString().split("+");
        if (to && assignee_list.contains(to->objectName()))
            return 10000;
        return 0;
    }

};

class Jinjiu : public FilterSkill
{
public:
    Jinjiu() : FilterSkill("jinjiu")
    {
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->objectName() == "analeptic";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

MingceCard::MingceCard()
{
    will_throw = false;
	will_sort = false;
    handling_method = Card::MethodNone;
}

bool MingceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && to_select != Self) || (targets.length() == 1 && targets.first()->canSlash(to_select, NULL, false) && targets.first()->inMyAttackRange(to_select));
}

bool MingceCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    if (targets.length() == 1) {
		foreach(const Player *sib, targets.first()->getAliveSiblings())
			if (targets.first()->canSlash(sib, NULL, false) && targets.first()->inMyAttackRange(sib))
				return false;
        return true;
	}
	return targets.length() == 2;
}

void MingceCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
	CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "mingce", QString());
    room->obtainCard(card_use.to.first(), this, reason);
}

void MingceCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
	QList<ServerPlayer *> copy = targets;
	ServerPlayer *target = copy.takeFirst();
    if (copy.isEmpty())
		target->drawCards(1, "mingce");
	else {
	    ServerPlayer *victim = copy.takeFirst();
		victim->setFlags("MingceTarget"); // For AI
        QString choice = room->askForChoice(target, "mingce", "use+draw", QVariant(), "@mingce-choose::"+victim->objectName());
        if (victim && victim->hasFlag("MingceTarget")) victim->setFlags("-MingceTarget");

        if (choice == "use") {
            if (target->canSlash(victim, NULL, false)) {
                Slash *slash = new Slash(Card::NoSuit, 0);
                slash->setSkillName("_mingce");
                room->useCard(CardUseStruct(slash, target, victim));
            }
        } else if (choice == "draw") {
            target->drawCards(1, "mingce");
        }
	}
}

class Mingce : public OneCardViewAsSkill
{
public:
    Mingce() : OneCardViewAsSkill("mingce")
    {
        filter_pattern = "EquipCard,Slash";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("MingceCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        MingceCard *mingceCard = new MingceCard;
        mingceCard->addSubcard(originalCard);
        return mingceCard;
    }
};

class Zhichi : public TriggerSkill
{
public:
    Zhichi() : TriggerSkill("zhichi")
    {
        events << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::NotActive)
            return false;

            player->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(player, objectName());
            room->addPlayerTip(player, "#zhichi");

            LogMessage log;
            log.type = "#ZhichiDamaged";
            log.from = player;
            room->sendLog(log);

        return false;
    }
};

class ZhichiProtect : public TriggerSkill
{
public:
    ZhichiProtect() : TriggerSkill("#zhichi-protect")
    {
        events << CardEffected;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardEffectStruct effect = data.value<CardEffectStruct>();
        if ((effect.card->isKindOf("Slash") || effect.card->isNDTrick()) && effect.to->getMark("#zhichi") > 0) {
            effect.to->broadcastSkillInvoke("zhichi");
            room->notifySkillInvoked(effect.to, "zhichi");
            LogMessage log;
            log.type = "#ZhichiAvoid";
            log.from = effect.to;
            log.arg = "zhichi";
            room->sendLog(log);

            return true;
        }
        return false;
    }
};

class ZhichiClear : public TriggerSkill
{
public:
    ZhichiClear() : TriggerSkill("#zhichi-clear")
    {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::NotActive;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *, QVariant &) const
    {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("#zhichi") > 0)
                room->removePlayerTip(p, "#zhichi");
        }
        return false;
    }
};

GanluCard::GanluCard()
{
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    switch (targets.length()) {
    case 0: return true;
    case 1: {
        int n1 = targets.first()->getEquips().length();
        int n2 = to_select->getEquips().length();
        return qAbs(n1 - n2) <= Self->getLostHp();
    }
    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);

    room->swapCards(targets.at(0), targets.at(1), "e", "ganlu");
}

class Ganlu : public ZeroCardViewAsSkill
{
public:
    Ganlu() : ZeroCardViewAsSkill("ganlu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("GanluCard");
    }

    virtual const Card *viewAs() const
    {
        return new GanluCard;
    }
};

class Buyi : public TriggerSkill
{
public:
    Buyi() : TriggerSkill("buyi")
    {
        events << Dying;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *wuguotai, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        ServerPlayer *player = dying.who;
        if (player->isKongcheng()) return false;
        if (player->getHp() < 1 && wuguotai->askForSkillInvoke(this, QVariant::fromValue(player))) {
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, wuguotai->objectName(), player->objectName());
            wuguotai->broadcastSkillInvoke(objectName());
            const Card *card = Sanguosha->getCard(room->askForCardChosen(wuguotai, player, "h", "buyi"));
            room->showCard(player, card->getEffectiveId());

            if (card->getTypeId() != Card::TypeBasic) {
                if (!player->isJilei(card)) {
                    room->throwCard(card, player);
                    room->recover(player, RecoverStruct(wuguotai));
                }
            }
        }
        return false;
    }
};

class Quanji : public MasochismSkill
{
public:
    Quanji() : MasochismSkill("quanji")
    {
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const
    {
        Room *room = zhonghui->getRoom();

        int x = damage.damage;
        for (int i = 0; i < x; i++) {
            if (zhonghui->askForSkillInvoke(objectName())) {
                zhonghui->broadcastSkillInvoke(objectName());
                room->drawCards(zhonghui, 1, objectName());
                if (!zhonghui->isKongcheng()) {
                    const Card *card = room->askForExchange(zhonghui, objectName(), 1, 1, false, "QuanjiPush");
                    zhonghui->addToPile("power", card);
                }
            } else break;
        }

    }
};

class QuanjiKeep : public MaxCardsSkill
{
public:
    QuanjiKeep() : MaxCardsSkill("#quanji")
    {
        frequency = Frequent;
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill("quanji"))
            return target->getPile("power").length();
        else
            return 0;
    }
};

class Zili : public PhaseChangeSkill
{
public:
    Zili() : PhaseChangeSkill("zili")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getPhase() == Player::Start
            && target->getMark("zili") == 0
            && target->getPile("power").length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const
    {
        Room *room = zhonghui->getRoom();
        room->sendCompulsoryTriggerLog(zhonghui, objectName());

        zhonghui->broadcastSkillInvoke(objectName());
        //room->doLightbox("$ZiliAnimate", 4000);

        room->setPlayerMark(zhonghui, "zili", 1);
        if (room->changeMaxHpForAwakenSkill(zhonghui)) {
            if (zhonghui->isWounded() && room->askForChoice(zhonghui, objectName(), "recover+draw") == "recover")
                room->recover(zhonghui, RecoverStruct(zhonghui));
            else
                room->drawCards(zhonghui, 2, objectName());
            if (zhonghui->getMark("zili") == 1)
                room->acquireSkill(zhonghui, "paiyi");
        }

        return false;
    }
};

PaiyiCard::PaiyiCard()
{
    will_throw = true;
    handling_method = Card::MethodNone;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *, const Player *) const
{
    return targets.isEmpty();
}

void PaiyiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *zhonghui = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhonghui->getRoom();

    room->drawCards(target, 2, "paiyi");
    if (target->getHandcardNum() > zhonghui->getHandcardNum())
        room->damage(DamageStruct("paiyi", zhonghui, target));
}

class Paiyi : public OneCardViewAsSkill
{
public:
    Paiyi() : OneCardViewAsSkill("paiyi")
    {
        expand_pile = "power";
        filter_pattern = ".|.|.|power";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->getPile("power").isEmpty() && !player->hasUsed("PaiyiCard");
    }

    virtual const Card *viewAs(const Card *c) const
    {
        PaiyiCard *py = new PaiyiCard;
        py->addSubcard(c);
        return py;
    }
};

class Jueqing : public TriggerSkill
{
public:
    Jueqing() : TriggerSkill("jueqing")
    {
        frequency = Compulsory;
        events << Predamage;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *zhangchunhua = damage.from;
        if (TriggerSkill::triggerable(zhangchunhua)) {
            zhangchunhua->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(zhangchunhua, objectName());
            room->loseHp(damage.to, damage.damage);
            return true;
        }
        return false;
    }
};

class Shangshi : public TriggerSkill
{
public:
    Shangshi() : TriggerSkill("shangshi")
    {
        events << HpChanged << MaxHpChanged << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *, ServerPlayer *zhangchunhua, QVariant &data) const
    {
        int losthp = zhangchunhua->getLostHp();
        if (triggerEvent == CardsMoveOneTime) {
            bool can_invoke = false;
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == zhangchunhua && move.from_places.contains(Player::PlaceHand))
                can_invoke = true;
            if (move.to == zhangchunhua && move.to_place == Player::PlaceHand)
                can_invoke = true;
            if (!can_invoke) return false;
        }
        if (zhangchunhua->getHandcardNum() < losthp && zhangchunhua->askForSkillInvoke(this)) {
            zhangchunhua->broadcastSkillInvoke("shangshi");
            zhangchunhua->drawCards(losthp - zhangchunhua->getHandcardNum(), objectName());
        }
        return false;
    }
};

SanyaoCard::SanyaoCard()
{
}

bool SanyaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (!targets.isEmpty()) return false;
    QList<const Player *> players = Self->getAliveSiblings();
    players << Self;
    int max = -1000;
    foreach (const Player *p, players) {
        if (max < p->getHp()) max = p->getHp();
    }
    return to_select->getHp() == max;
}

void SanyaoCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->damage(DamageStruct("sanyao", effect.from, effect.to));
}

class Sanyao : public OneCardViewAsSkill
{
public:
    Sanyao() : OneCardViewAsSkill("sanyao")
    {
        filter_pattern = ".!";
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasUsed("SanyaoCard");
    }

    const Card *viewAs(const Card *originalcard) const
    {
        SanyaoCard *first = new SanyaoCard;
        first->addSubcard(originalcard->getId());
        first->setSkillName(objectName());
        return first;
    }
};

class Zhiman : public TriggerSkill
{
public:
    Zhiman() : TriggerSkill("zhiman")
    {
        events << DamageCaused;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();

        if (player != damage.to && player->askForSkillInvoke(this, QVariant::fromValue(damage.to))) {
            player->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
            if (damage.to->getEquips().isEmpty() && damage.to->getJudgingArea().isEmpty())
                return true;
            int card_id = room->askForCardChosen(player, damage.to, "ej", objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, Sanguosha->getCard(card_id), reason);
            return true;
        }
        return false;
    }
};

class ZhenjunDiscard : public ViewAsSkill
{
public:
    ZhenjunDiscard() : ViewAsSkill("zhenjun_discard")
    {
        response_pattern = "@@zhenjun_discard";
    }


    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        return selected.length() < Self->getMark("zhenjun_num") && !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getMark("zhenjun_num")) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }
        return NULL;
    }
};

class Zhenjun : public PhaseChangeSkill
{
public:
    Zhenjun() : PhaseChangeSkill("zhenjun")
    {
        view_as_skill = new dummyVS;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() != Player::Start) return false;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getHandcardNum() > p->getHp())
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "zhenjun-invoke", true, true);
        if (target) {
            int x = target->getHandcardNum() - target->getHp();
            QList<int> cards = room->askForCardsChosen(player, target, "he", objectName(), x, x, false, Card::MethodDiscard);
            int y = 0;
            foreach (int id, cards) {
                if (Sanguosha->getCard(id)->getTypeId() != Card::TypeEquip)
                    y++;
            }
            DummyCard *dummy = new DummyCard(cards);
            room->throwCard(dummy, target, player);
            delete dummy;
            room->setPlayerMark(player, "zhenjun_num", y);
            const Card *to_discard = room->askForCard(player, "@@zhenjun_discard", "zhenjun-discard::"+target->objectName()+
                    ":"+QString::number(y)+":"+QString::number(x), QVariant(), Card::MethodNone);
            room->setPlayerMark(player, "zhenjun_num", 0);
            if (to_discard) {
                if (to_discard->subcardsLength() > 0) {
                    CardMoveReason mreason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), objectName(), QString());
                    room->throwCard(to_discard, mreason, player);
                }
            } else
                target->drawCards(x, "zhenjun");
        }
        return false;
    }
};

class Pojun : public TriggerSkill
{
public:
    Pojun() : TriggerSkill("pojun")
    {
        events << TargetSpecified << EventPhaseChanging;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && TriggerSkill::triggerable(player) && player->getPhase() == Player::Play) {
                foreach (ServerPlayer *t, use.to) {
                    int n = qMin(t->getCards("he").length(), t->getHp());
                    if (n > 0 && player->askForSkillInvoke(this, QVariant::fromValue(t))) {
                        player->broadcastSkillInvoke(objectName());
                        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), t->objectName());

                        QList<int> cards = room->askForCardsChosen(player, t, "he", objectName(), 1, n);

                        DummyCard dummy(cards);
                        t->addToPile(objectName(), &dummy, false, QList<ServerPlayer *>() << t);

                        // for record
                        if (!t->tag.contains(objectName()) || !t->tag.value(objectName()).canConvert(QVariant::Map))
                            t->tag[objectName()] = QVariantMap();

                        QVariantMap vm = t->tag[objectName()].toMap();
                        foreach (int id, cards)
                            vm[QString::number(id)] = player->objectName();

                        t->tag[objectName()] = vm;
                    }
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->tag.contains(objectName())) {
                    QVariantMap vm = p->tag.value(objectName(), QVariantMap()).toMap();
                    if (vm.values().contains(player->objectName())) {
                        QList<int> to_obtain;
                        foreach (const QString &key, vm.keys()) {
                            if (vm.value(key) == player->objectName())
                                to_obtain << key.toInt();
                        }

                        DummyCard dummy(to_obtain);
						room->obtainCard(p, &dummy, false);

                        foreach (int id, to_obtain)
                            vm.remove(QString::number(id));

                        p->tag[objectName()] = vm;
                    }
                }
            }
        }

        return false;
    }
};

YJCMPackage::YJCMPackage()
    : Package("YJCM")
{
    General *caozhi = new General(this, "caozhi", "wei", 3); // YJ 001
    caozhi->addSkill(new Luoying);
    caozhi->addSkill(new Jiushi);
    caozhi->addSkill(new JiushiFlip);
    related_skills.insertMulti("jiushi", "#jiushi-flip");

    General *chengong = new General(this, "chengong", "qun", 3); // YJ 002
    chengong->addSkill(new Mingce);
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiProtect);
    chengong->addSkill(new ZhichiClear);
    related_skills.insertMulti("zhichi", "#zhichi-protect");
    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *fazheng = new General(this, "fazheng", "shu", 3); // YJ 003
    fazheng->addSkill(new Enyuan);
    fazheng->addSkill(new Xuanhuo);

    General *gaoshun = new General(this, "gaoshun", "qun"); // YJ 004
    gaoshun->addSkill(new Xianzhen);
    gaoshun->addSkill(new XianzhenTargetMod);
    gaoshun->addSkill(new Jinjiu);
    related_skills.insertMulti("xianzhen", "#xianzhen-target");

    General *lingtong = new General(this, "lingtong", "wu"); // YJ 005
    lingtong->addSkill(new Xuanfeng);

    General *masu = new General(this, "masu", "shu", 3); // YJ 006
    masu->addSkill(new Sanyao);
    masu->addSkill(new Zhiman);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false); // YJ 007
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *xusheng = new General(this, "xusheng", "wu"); // YJ 008
    xusheng->addSkill(new Pojun);

    General *xushu = new General(this, "yj_xushu", "shu", 3); // YJ 009
    xushu->addSkill(new Wuyan);
    xushu->addSkill(new Jujian);

    General *yujin = new General(this, "yujin", "wei"); // YJ 010
    yujin->addSkill(new Zhenjun);

    General *zhangchunhua = new General(this, "zhangchunhua", "wei", 3, false); // YJ 011
    zhangchunhua->addSkill(new Jueqing);
    zhangchunhua->addSkill(new Shangshi);

    General *zhonghui = new General(this, "zhonghui", "wei"); // YJ 012
    zhonghui->addSkill(new QuanjiKeep);
    zhonghui->addSkill(new Quanji);
	zhonghui->addSkill(new DetachEffectSkill("quanji", "power"));
    related_skills.insertMulti("quanji", "#quanji");
	related_skills.insertMulti("quanji", "#quanji-clear");
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");

    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XianzhenCard>();
    addMetaObject<JujianCard>();
    addMetaObject<PaiyiCard>();
    addMetaObject<SanyaoCard>();

    skills << new ZhenjunDiscard << new Paiyi;
}

ADD_PACKAGE(YJCM)
