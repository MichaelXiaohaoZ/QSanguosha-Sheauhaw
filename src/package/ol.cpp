#include "ol.h"
#include "general.h"
#include "skill.h"
#include "standard.h"
#include "yjcm2013.h"
#include "engine.h"
#include "clientplayer.h"
#include "json.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCommandLinkButton>
#include "settings.h"

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

class AocaiVeiw : public OneCardViewAsSkill
{
public:
    AocaiVeiw() : OneCardViewAsSkill("aocai_view")
    {
        expand_pile = "#aocai",
        response_pattern = "@@aocai_view";
    }

	bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        QStringList aocai = Self->property("aocai").toString().split("+");
        foreach (QString id, aocai) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return true;
        }
        return false;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class AocaiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    AocaiViewAsSkill() : ZeroCardViewAsSkill("aocai")
    {
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (player->getPhase() != Player::NotActive || player->hasFlag("Global_AocaiFailed")) return false;
        return pattern == "slash" || pattern == "jink" || pattern == "peach" || pattern.contains("analeptic");
    }

    const Card *viewAs() const
    {
        AocaiCard *aocai_card = new AocaiCard;
        aocai_card->setUserString(Sanguosha->currentRoomState()->getCurrentCardUsePattern());
        return aocai_card;
    }
};

class Aocai : public TriggerSkill
{
public:
    Aocai() : TriggerSkill("aocai")
    {
        events << CardAsked;
        view_as_skill = new AocaiViewAsSkill;
    }

    bool trigger(TriggerEvent, Room *, ServerPlayer *, QVariant &) const
    {
        return false;
    }

    static int view(Room *room, ServerPlayer *player, QList<int> &ids, QList<int> &enabled)
    {
        int result = -1, index = -1;
		LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = player;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, player);

        player->broadcastSkillInvoke("aocai");
        room->notifySkillInvoked(player, "aocai");
        
		room->notifyMoveToPile(player, ids, "aocai", Player::PlaceTable, true, true);
        room->setPlayerProperty(player, "aocai", IntList2StringList(enabled).join("+"));
		const Card *card = room->askForCard(player, "@@aocai_view", "@aocai-view", QVariant(), Card::MethodNone);
		room->notifyMoveToPile(player, ids, "aocai", Player::PlaceTable, false, false);
        if (card == NULL)
		    room->setPlayerFlag(player, "Global_AocaiFailed");
		else {
            result = card->getSubcards().first();
			index = ids.indexOf(result);
			LogMessage log;
            log.type = "#AocaiUse";
            log.from = player;
            log.arg = "aocai";
            log.arg2 = QString("CAPITAL(%1)").arg(index + 1);
            room->sendLog(log);
        }
		room->returnToTopDrawPile(ids);
        return result;
    }
};

AocaiCard::AocaiCard()
{
}

bool AocaiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool AocaiCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE)
        return true;

    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetFixed();
}

bool AocaiCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    const Card *card = NULL;
    if (!user_string.isEmpty())
        card = Sanguosha->cloneCard(user_string.split("+").first());
    return card && card->targetsFeasible(targets, Self);
}

const Card *AocaiCard::validateInResponse(ServerPlayer *user) const
{
    Room *room = user->getRoom();
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled;
    foreach (int id, ids)
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = user;
    log.arg = "aocai";
    room->sendLog(log);

    int id = Aocai::view(room, user, ids, enabled);
    return Sanguosha->getCard(id);
}

const Card *AocaiCard::validate(CardUseStruct &cardUse) const
{
    ServerPlayer *user = cardUse.from;
    Room *room = user->getRoom();

	LogMessage log;
    log.from = user;
    log.to = cardUse.to;
    log.type = "#UseCard";
    log.card_str = toString();
    room->sendLog(log);
	
    QList<int> ids = room->getNCards(2, false);
    QStringList names = user_string.split("+");
    if (names.contains("slash")) names << "fire_slash" << "thunder_slash";

    QList<int> enabled;
    foreach (int id, ids)
        if (names.contains(Sanguosha->getCard(id)->objectName()))
            enabled << id;

    int id = Aocai::view(room, user, ids, enabled);
	const Card *card = Sanguosha->getCard(id);
	if (card)
		room->setCardFlag(card, "hasSpecialEffects");
	return card;
}

DuwuCard::DuwuCard()
{
}

bool DuwuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return (targets.isEmpty() && qMax(0, to_select->getHp()) == subcardsLength() && Self->inMyAttackRange(to_select));
}

void DuwuCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.to->getRoom();

    room->damage(DamageStruct("duwu", effect.from, effect.to));
}

class DuwuViewAsSkill : public ViewAsSkill
{
public:
    DuwuViewAsSkill() : ViewAsSkill("duwu")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return player->canDiscard(player, "he") && !player->hasFlag("DuwuEnterDying");
    }

    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !Self->isJilei(to_select);
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        DuwuCard *duwu = new DuwuCard;
        if (!cards.isEmpty())
            duwu->addSubcards(cards);
        return duwu;
    }
};

class Duwu : public TriggerSkill
{
public:
    Duwu() : TriggerSkill("duwu")
    {
        events << QuitDying;
        view_as_skill = new DuwuViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "duwu" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive()) {
                room->setPlayerFlag(from, "DuwuEnterDying");
                from->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(from, objectName());
                room->loseHp(from, 1);
            }
        }
        return false;
    }
};

ShefuCard::ShefuCard()
{
    will_throw = false;
    target_fixed = true;
    handling_method = Card::MethodNone;
}

void ShefuCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    QString mark = "Shefu_" + user_string;
    room->setPlayerMark(source, mark, getEffectiveId() + 1);
    source->addToPile("ambush", this, false);
}

class ShefuViewAsSkill : public OneCardViewAsSkill
{
public:
    ShefuViewAsSkill() : OneCardViewAsSkill("shefu")
    {
        filter_pattern = ".|.|.|hand";
        response_pattern = "@@shefu";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        ShefuCard *card = new ShefuCard;
        card->addSubcard(originalCard);
        card->setSkillName("shefu");
        return card;
    }
};

class Shefu : public PhaseChangeSkill
{
public:
    Shefu() : PhaseChangeSkill("shefu")
    {
        view_as_skill = new ShefuViewAsSkill;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Finish || target->isKongcheng())
            return false;
        room->askForUseCard(target, "@@shefu", "@shefu-prompt", QVariant(), Card::MethodNone);
        return false;
    }

    QString getSelectBox() const
    {
        return "guhuo_sbtd";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &, const QList<const Player *> &) const
    {
        return Self->getMark("Shefu_" + button_name) == 0;
    }
};

class ShefuCancel : public TriggerSkill
{
public:
    ShefuCancel() : TriggerSkill("#shefu-cancel")
    {
        events << CardUsed << JinkEffect;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == JinkEffect) {
            bool invoked = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player, "jink")) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::jink") || p->getMark("Shefu_jink") == 0)
                        continue;

                    p->broadcastSkillInvoke("shefu");

                    invoked = true;

                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = "jink";
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_jink") - 1;
                    room->setPlayerMark(p, "Shefu_jink", 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);
                }
            }
            return invoked;
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeBasic && use.card->getTypeId() != Card::TypeTrick && use.m_isHandcard)
                return false;
            QString card_name = use.card->objectName();
            if (card_name.contains("slash")) card_name = "slash";
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (ShefuTriggerable(p, player, card_name)) {
                    room->setTag("ShefuData", data);
                    if (!room->askForSkillInvoke(p, "shefu_cancel", "data:::" + card_name) || p->getMark("Shefu_" + card_name) == 0)
                        continue;

                    p->broadcastSkillInvoke("shefu");

                    LogMessage log;
                    log.type = "#ShefuEffect";
                    log.from = p;
                    log.to << player;
                    log.arg = card_name;
                    log.arg2 = "shefu";
                    room->sendLog(log);

                    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), "shefu", QString());
                    int id = p->getMark("Shefu_" + card_name) - 1;
                    room->setPlayerMark(p, "Shefu_" + card_name, 0);
                    room->throwCard(Sanguosha->getCard(id), reason, NULL);

                    use.to.clear();
                    use.to_card = NULL;//for nullification
                }
            }
            data = QVariant::fromValue(use);
        }
        return false;
    }

private:
    bool ShefuTriggerable(ServerPlayer *chengyu, ServerPlayer *user, QString card_name) const
    {
        return chengyu->getPhase() == Player::NotActive && chengyu != user && chengyu->hasSkill("shefu")
            && !chengyu->getPile("ambush").isEmpty() && chengyu->getMark("Shefu_" + card_name) > 0;
    }
};

class BenyuViewAsSkill : public ViewAsSkill
{
public:
    BenyuViewAsSkill() : ViewAsSkill("benyu")
    {
        response_pattern = "@@benyu";
    }

    virtual bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() < Self->getMark("benyu"))
            return NULL;

        DummyCard *card = new DummyCard;
        card->addSubcards(cards);
        return card;
    }
};

class Benyu : public MasochismSkill
{
public:
    Benyu() : MasochismSkill("benyu")
    {
        view_as_skill = new BenyuViewAsSkill;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const
    {
        if (!damage.from || damage.from->isDead())
            return;
        Room *room = target->getRoom();
        int from_handcard_num = damage.from->getHandcardNum(), handcard_num = target->getHandcardNum();
        if (handcard_num == from_handcard_num) {
            return;
        } else if (handcard_num < from_handcard_num && handcard_num < 5 && room->askForSkillInvoke(target, objectName(), QVariant::fromValue(damage.from))) {
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, target->objectName(), damage.from->objectName());
            target->broadcastSkillInvoke(objectName());
            room->drawCards(target, qMin(5, from_handcard_num) - handcard_num, objectName());
        } else if (handcard_num > from_handcard_num) {
            room->setPlayerMark(target, objectName(), from_handcard_num + 1);
            //if (room->askForUseCard(target, "@@benyu", QString("@benyu-discard::%1:%2").arg(damage.from->objectName()).arg(from_handcard_num + 1), -1, Card::MethodDiscard))
            if (room->askForCard(target, "@@benyu", QString("@benyu-discard::%1:%2").arg(damage.from->objectName()).arg(from_handcard_num + 1), QVariant(), objectName(), damage.from))
                room->damage(DamageStruct(objectName(), target, damage.from));
        }
        return;
    }
};

class Canshi : public TriggerSkill
{
public:
    Canshi() : TriggerSkill("canshi")
    {
        events << EventPhaseStart << CardUsed << CardResponded;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (TriggerSkill::triggerable(player) && player->getPhase() == Player::Draw) {
                int n = 0;
                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->isWounded() || (player != p && player->hasLordSkill("guiming") && p->getKingdom() == "wu"))
                        ++n;
                }

                if (n > 0 && player->askForSkillInvoke(this, "prompt:::" + QString::number(n))) {
                    player->broadcastSkillInvoke(objectName());
                    player->setFlags(objectName());
                    player->drawCards(n, objectName());
                    return true;
                }
            }
        } else {
            if (player->hasFlag(objectName())) {
                const Card *card = NULL;
                if (triggerEvent == CardUsed)
                    card = data.value<CardUseStruct>().card;
                else {
                    CardResponseStruct resp = data.value<CardResponseStruct>();
                    if (resp.m_isUse)
                        card = resp.m_card;
                }
                if (card != NULL && (card->isKindOf("BasicCard") || card->isKindOf("TrickCard")))
                    room->askForDiscard(player, objectName(), 1, 1, false, true, "@canshi-discard");
            }
        }
        return false;
    }
};

class Chouhai : public TriggerSkill
{
public:
    Chouhai() : TriggerSkill("chouhai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->isKongcheng()) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());

            DamageStruct damage = data.value<DamageStruct>();
            ++damage.damage;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

class Guiming : public TriggerSkill // play audio effect only. This skill is coupled in Canshi.
{
public:
    Guiming() : TriggerSkill("guiming$")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->hasLordSkill(this) && target->getPhase() == Player::RoundStart;
    }

    int getPriority(TriggerEvent) const
    {
        return 6;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (const ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getKingdom() == "wu" && p->isWounded() && p->getHp() == p->getMaxHp()) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                return false;
            }
        }

        return false;
    }
};

class Biluan : public PhaseChangeSkill
{
public:
    Biluan() : PhaseChangeSkill("biluan")
    {
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Draw)
			return false;

		foreach(ServerPlayer *p, room->getOtherPlayers(target)){
			if (p->distanceTo(target) == 1){
				if (target->askForSkillInvoke(this)) {
                    target->broadcastSkillInvoke(objectName());
                    QSet<QString> kingdom_set;
                    foreach(ServerPlayer *p, room->getAlivePlayers())
                        kingdom_set << p->getKingdom();

                    int n = kingdom_set.size();
					room->addPlayerMark(target, "#shixie_distance", n);
                    return true;
                }
				break;
			}
		}
        return false;
    }
};

class Lixia : public PhaseChangeSkill
{
public:
    Lixia() : PhaseChangeSkill("lixia")
    {
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        if (target->getPhase() != Player::Finish)
			return false;
		ServerPlayer *shixie = room->findPlayerBySkillName(objectName());
        if (shixie && shixie != target && !target->inMyAttackRange(shixie)) {
            room->sendCompulsoryTriggerLog(shixie, objectName());
            shixie->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, shixie->objectName(), target->objectName());
			ServerPlayer *to_draw = shixie;
            if (room->askForChoice(shixie, objectName(), "self+other", QVariant(), "@lixia-choose::"+target->objectName()) == "other")
			    to_draw = target;
			to_draw->drawCards(1, objectName());
			room->setPlayerMark(shixie, "#shixie_distance", shixie->getMark("#shixie_distance")-1);
		}
        return false;
    }
};

class ShixieDistance : public DistanceSkill
{
public:
    ShixieDistance() : DistanceSkill("#shixie-distance")
    {
    }

    virtual int getCorrect(const Player *, const Player *to) const
    {
        return to->getMark("#shixie_distance");
    }
};

class Yishe : public TriggerSkill
{
public:
    Yishe() : TriggerSkill("yishe")
    {
        events << EventPhaseStart << CardsMoveOneTime;
	}

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::Finish || !player->getPile("rice").isEmpty())
				return false;
            if (!player->askForSkillInvoke(this))
                return false;
            player->broadcastSkillInvoke(objectName());
            player->drawCards(2);
			const Card *card = room->askForExchange(player, objectName(), 2, 2, true, "YishePush");
			player->addToPile("rice", card);
			delete card;
        } else if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && move.from_places.contains(Player::PlaceSpecial)
                && move.from_pile_names.contains("rice")) {
                if (player->getPile("rice").isEmpty() && player->isWounded()){
                    room->sendCompulsoryTriggerLog(player, objectName());
                    player->broadcastSkillInvoke(objectName());
					room->recover(player, RecoverStruct(player));
				}
            }
        }
        return false;
    }
};

class Bushi : public TriggerSkill
{
public:
    Bushi() : TriggerSkill("bushi")
    {
        events << Damage << Damaged;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
		for (int i = 0; i < damage.damage; i++) {
		    if (player->getPile("rice").isEmpty() || !target->isAlive() || !room->askForSkillInvoke(target, "bushi_obtain", "prompt:" + player->objectName()))
			    break;
			QString log_type = "#InvokeOthersSkill";
            if (target == player)
			    log_type = "#InvokeSkill";
			LogMessage log;
            log.type = log_type;
            log.from = target;
            log.to << player;
            log.arg = objectName();
            room->sendLog(log);
		    room->notifySkillInvoked(player, objectName());
            player->broadcastSkillInvoke(objectName());
			QList<int> ids = player->getPile("rice");
            room->fillAG(ids, target);
            int id = room->askForAG(target, ids, false, objectName());
            room->clearAG(target);
			CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, target->objectName(), objectName(), QString());
            room->obtainCard(target, Sanguosha->getCard(id), reason, true);
        }
        return false;
    }
};

class MidaoVS : public OneCardViewAsSkill
{
public:
    MidaoVS() : OneCardViewAsSkill("midao")
    {
        expand_pile = "rice";
        filter_pattern = ".|.|.|rice";
        response_pattern = "@@midao";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Midao : public TriggerSkill
{
public:
    Midao() : TriggerSkill("midao")
    {
        events << AskForRetrial;
		view_as_skill = new MidaoVS;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPile("rice").isEmpty())
            return false;

        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@midao-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, "@@midao", prompt, data, Card::MethodResponse, judge->who, true);
        if (card) {
			if (judge->who)
			    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), judge->who->objectName());
            player->broadcastSkillInvoke(objectName());
			room->notifySkillInvoked(player, objectName());
            room->retrial(card, player, judge, objectName());
        }
        return false;
    }
};

class FengpoRecord : public TriggerSkill
{
public:
    FengpoRecord() : TriggerSkill("#fengpo-record")
    {
        events << EventPhaseChanging << PreCardUsed << CardResponded;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::Play)
                room->setPlayerFlag(player, "-fengporec");
        } else {
            if (player->getPhase() != Player::Play || player->hasFlag("fengporec"))
                return false;

            const Card *c = NULL;
            if (triggerEvent == PreCardUsed)
                c = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    c = resp.m_card;
            }

            if (c && (c->isKindOf("Slash") || c->isKindOf("Duel"))){
				room->setCardFlag(c, "fengporecc");
                room->setPlayerFlag(player, "fengporec");
			}
            return false;
        }

        return false;
    }
};

class Fengpo : public TriggerSkill
{
public:
    Fengpo() : TriggerSkill("fengpo")
    {
        events << TargetSpecified << ConfirmDamage << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (event == TargetSpecified) {
            if (!TriggerSkill::triggerable(player))
				return false;
			CardUseStruct use = data.value<CardUseStruct>();
            if (use.to.length() != 1) return false;
			ServerPlayer *target = use.to.first();
            if (use.card == NULL || (!use.card->hasFlag("fengporecc"))) return false;
            int n = 0;
            foreach (const Card *card, target->getHandcards())
                if (card->getSuit() == Card::Diamond)
                    ++n;
			if (!room->askForSkillInvoke(player, objectName(), QVariant::fromValue(target)))
				return false;
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            player->broadcastSkillInvoke(objectName());
            QString choice = room->askForChoice(player, objectName(), "drawCards+addDamage");
            if (choice == "drawCards"){
				if (n > 0)
				    player->drawCards(n);
			}else if (choice == "addDamage")
				use.card->setTag("FengpoAddDamage", n);
        } else if (event == ConfirmDamage) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card == NULL)
                return false;
			int n = damage.card->tag["FengpoAddDamage"].toInt();
            damage.damage = damage.damage+n;
            data = QVariant::fromValue(damage);
        } else if (event == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            use.card->setTag("FengpoAddDamage", 0);
        }
        return false;
    }
};

class Ranshang : public TriggerSkill
{
public:
    Ranshang() : TriggerSkill("ranshang")
    {
        events << EventPhaseStart << Damaged;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::Finish) {
            if (player->getMark("#kindle") > 0){
				room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
				room->loseHp(player, player->getMark("#kindle"));
			}
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
			if (damage.nature == DamageStruct::Fire) {
				room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
				player->gainMark("#kindle", damage.damage);
			}
        }
        return false;
    }
};

class Hanyong : public TriggerSkill
{
public:
    Hanyong() : TriggerSkill("hanyong")
    {
        events << CardUsed << ConfirmDamage;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == CardUsed){
			if (!TriggerSkill::triggerable(player))
				return false;
			CardUseStruct use = data.value<CardUseStruct>();
		    if (player->getHp() >= room->getTurn())
			    return false;
            if (use.card->isKindOf("SavageAssault") || use.card->isKindOf("ArcheryAttack")) {
                if (player->askForSkillInvoke(objectName(), data)){
                    player->broadcastSkillInvoke(objectName());
					room->setCardFlag(use.card, "HanyongEffect");
				}
			}
		} else if (triggerEvent == ConfirmDamage){
			DamageStruct damage = data.value<DamageStruct>();
            if (!damage.card || !damage.card->hasFlag("HanyongEffect"))
                return false;
            damage.damage++;
			data = QVariant::fromValue(damage);
		}
        return false;
    }
};

LuanzhanCard::LuanzhanCard()
{
}

bool LuanzhanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList available_targets = Self->property("luanzhan_available_targets").toString().split("+");

    return targets.length() < Self->getMark("#luanzhan") && available_targets.contains(to_select->objectName());
}

void LuanzhanCard::use(Room *, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
	foreach (ServerPlayer *p, targets)
	    p->setFlags("LuanzhanExtraTarget");
}

class LuanzhanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    LuanzhanViewAsSkill() : ZeroCardViewAsSkill("luanzhan")
    {
		response_pattern = "@@luanzhan";
    }

    virtual const Card *viewAs() const
    {
        return new LuanzhanCard;
    }
};

class LuanzhanColl : public ZeroCardViewAsSkill
{
public:
    LuanzhanColl() : ZeroCardViewAsSkill("luanzhan_coll")
    {
		response_pattern = "@@luanzhan_coll";
    }

    virtual const Card *viewAs() const
    {
        return new ExtraCollateralCard;
    }
};

class Luanzhan : public TriggerSkill
{
public:
    Luanzhan() : TriggerSkill("luanzhan")
    {
        events << HpChanged << TargetChosen << TargetSpecified;
		view_as_skill = new LuanzhanViewAsSkill;
    }

    bool triggerable(const ServerPlayer *player) const
    {
        return player != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == HpChanged && !data.isNull() && data.canConvert<DamageStruct>()) {
			DamageStruct damage = data.value<DamageStruct>();
			if (damage.from && TriggerSkill::triggerable(damage.from)){
                room->sendCompulsoryTriggerLog(damage.from, objectName());
                damage.from->broadcastSkillInvoke(objectName());
                room->addPlayerMark(damage.from, "#luanzhan");
			}
		} else if (triggerEvent == TargetSpecified && player->getMark("#luanzhan") > 0 && TriggerSkill::triggerable(player)) {
            CardUseStruct use = data.value<CardUseStruct>();
			if ((use.card->isKindOf("Slash") || (use.card->isNDTrick() && use.card->isBlack())) && use.to.length() < player->getMark("#luanzhan")) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                room->setPlayerMark(player, "#luanzhan", 0);
			}
		} else if (triggerEvent == TargetChosen && player->getMark("#luanzhan") > 0 && TriggerSkill::triggerable(player)) {
			CardUseStruct use = data.value<CardUseStruct>();
			if (!use.card->isKindOf("Slash") && !(use.card->isNDTrick() && use.card->isBlack())) return false;
			bool no_distance_limit = false;
			if (use.card->isKindOf("Slash")) {
				if (use.card->hasFlag("slashDisableExtraTarget")) return false;
				if (use.card->hasFlag("slashNoDistanceLimit")){
				    no_distance_limit = true;
				    room->setPlayerFlag(player, "slashNoDistanceLimit");
			    }
			}
			QStringList available_targets;
			QList<ServerPlayer *> extra_targets;
			if (!use.card->isKindOf("AOE") && !use.card->isKindOf("GlobalEffect")) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                    if (use.card->targetFixed()) {
                        if (!use.card->isKindOf("Peach") || p->isWounded())
							available_targets.append(p->objectName());
                    } else {
                        if (use.card->targetFilter(QList<const Player *>(), p, player))
							available_targets.append(p->objectName());
                    }
                }
            }
			if (no_distance_limit)
				room->setPlayerFlag(player, "-slashNoDistanceLimit");
			if (!available_targets.isEmpty()) {
				if (use.card->isKindOf("Collateral")) {
					QStringList tos;
                    foreach(ServerPlayer *t, use.to)
                        tos.append(t->objectName());
					for (int i = player->getMark("#luanzhan"); i > 0; i--) {
                        if (available_targets.isEmpty()) break;
						room->setPlayerProperty(player, "extra_collateral", use.card->toString());
                        room->setPlayerProperty(player, "extra_collateral_current_targets", tos.join("+"));
						player->tag["luanzhan-use"] = data;
                        room->askForUseCard(player, "@@luanzhan_coll", "@luanzhan-add:::collateral:" + QString::number(i), QVariant(), Card::MethodUse, false);
						player->tag.remove("luanzhan-use");
                        room->setPlayerProperty(player, "extra_collateral", QString());
                        room->setPlayerProperty(player, "extra_collateral_current_targets", QString());
						ServerPlayer *extra = NULL;
						foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                            if (p->hasFlag("ExtraCollateralTarget")) {
                                p->setFlags("-ExtraCollateralTarget");
                                extra = p;
                                break;
                            }
                        }
                        if (extra == NULL)
							break;
						extra->setFlags("LuanzhanExtraTarget");
						tos.append(extra->objectName());
						available_targets.removeOne(extra->objectName());
					}
				} else {
					room->setPlayerProperty(player, "luanzhan_available_targets", available_targets.join("+"));
					player->tag["luanzhan-use"] = data;
					room->askForUseCard(player, "@@luanzhan", "@luanzhan-add:::" + use.card->objectName() + ":" + QString::number(player->getMark("#luanzhan")));
				    player->tag.remove("luanzhan-use");
					room->setPlayerProperty(player, "luanzhan_available_targets", QString());
				}
				foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("LuanzhanExtraTarget")) {
                        p->setFlags("-LuanzhanExtraTarget");
						extra_targets << p;
                    }
                }
			}

			if (!extra_targets.isEmpty()) {
				if (use.card->isKindOf("Collateral")) {
					LogMessage log;
                    log.type = "#UseCard";
                    log.from = player;
                    log.to = extra_targets;
                    log.card_str = "@LuanzhanCard[no_suit:-]=.";
                    room->sendLog(log);
                    player->broadcastSkillInvoke("luanzhan");
					foreach (ServerPlayer *p, extra_targets) {
						ServerPlayer *victim = p->tag["collateralVictim"].value<ServerPlayer *>();
                        if (victim) {
                            LogMessage log;
                            log.type = "#LuanzhanCollateralSlash";
                            log.from = p;
                            log.to << victim;
                            room->sendLog(log);
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), victim->objectName());
                        }
					}
				}

				foreach (ServerPlayer *extra, extra_targets){
					use.to.append(extra);
				}
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
			}
		}
        return false;
    }
};

class Zhidao : public TriggerSkill
{
public:
    Zhidao() : TriggerSkill("zhidao")
    {
        events << Damage;
		frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if (!player->hasFlag("ZhidaoInvoked") && !target->isAllNude() && player->getPhase() == Player::Play) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            
            room->setPlayerFlag(player, "ZhidaoInvoked");
            room->setPlayerFlag(player, "DisabledOtherTargets");
            int n = 0;
            if (!target->isKongcheng()) n++;
            if (!target->getEquips().isEmpty()) n++;
            if (!target->getJudgingArea().isEmpty()) n++;
            QList<int> ids = room->askForCardsChosen(player, target, "hej", objectName(), n, n);
			DummyCard *dummy = new DummyCard(ids);
			CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            room->obtainCard(player, dummy, reason, false);
			delete dummy;
        }

        return false;
    }
};

class Jili : public TriggerSkill
{
public:
    Jili() : TriggerSkill("jili")
    {
        events << TargetConfirming;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if ((use.card->getTypeId() == Card::TypeBasic || use.card->isNDTrick()) && use.card->isRed()) {
            foreach (ServerPlayer *ybh, room->getOtherPlayers(target)) {
				if (TriggerSkill::triggerable(ybh) && use.from != ybh && !use.to.contains(ybh) && target->distanceTo(ybh) == 1){
                    ybh->broadcastSkillInvoke(objectName(), isGoodEffect(use.card, ybh) ? 2 : 1);
					room->notifySkillInvoked(ybh, objectName());
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, ybh->objectName(), target->objectName());
					LogMessage log;
                    log.type = "#JiliAdd";
                    log.from = ybh;
                    log.to << target;
                    log.card_str = use.card->toString();
                    log.arg = objectName();
                    room->sendLog(log);
					use.to.append(ybh);
                    room->sortByActionOrder(use.to);
                    data = QVariant::fromValue(use);
                    room->getThread()->trigger(TargetConfirming, room, ybh, data);
                    use = data.value<CardUseStruct>();//some terrible situation
				}
            }
        }
        return false;
    }

private:
    static bool isGoodEffect(const Card *card, ServerPlayer *yanbaihu)
    {
        return card->isKindOf("Peach") || card->isKindOf("Analeptic") || card->isKindOf("ExNihilo")
                || card->isKindOf("AmazingGrace") || card->isKindOf("GodSalvation")
                || (card->isKindOf("IronChain") && yanbaihu->isChained());
    }
};

class Zhengnan : public TriggerSkill
{
public:
    Zhengnan() : TriggerSkill("zhengnan")
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
		foreach (ServerPlayer *guansuo, room->getAlivePlayers()) {
            if (TriggerSkill::triggerable(guansuo) && guansuo->askForSkillInvoke(this)){
                guansuo->broadcastSkillInvoke(objectName());
				guansuo->drawCards(3, objectName());
				QStringList skill_list;
				if (!guansuo->hasSkill("wusheng", true))
                    skill_list << "wusheng";
                if (!guansuo->hasSkill("dangxian", true))
                    skill_list << "dangxian";
				if (!guansuo->hasSkill("zhiman", true))
                    skill_list << "zhiman";
				if (!skill_list.isEmpty())
                    room->acquireSkill(guansuo, room->askForChoice(guansuo, objectName(), skill_list.join("+"), QVariant(), "@zhengnan-choose"));
			}
        }
        return false;
    }
};

class Xiefang : public DistanceSkill
{
public:
    Xiefang() : DistanceSkill("xiefang")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill(this)){
			int extra = 0;
			if (from->isFemale())
				extra ++;
            QList<const Player *> players = from->getAliveSiblings();
            foreach (const Player *p, players) {
                if (p->isFemale())
                    extra ++;
            }
			return -extra;
		} else
            return 0;
    }
};

GusheCard::GusheCard()
{
}

bool GusheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < 3 && !to_select->isKongcheng() && to_select != Self;
}

void GusheCard::use(Room *, ServerPlayer *wangsitu, QList<ServerPlayer *> &targets) const
{
    PindianStruct *pd = wangsitu->pindianStart(targets, "gushe");
    for (int i = 1; i <= targets.length(); i++)
        wangsitu->pindianResult(pd, i);
    wangsitu->pindianFinish(pd);
}

class GusheViewAsSkill : public ZeroCardViewAsSkill
{
public:
    GusheViewAsSkill() : ZeroCardViewAsSkill("gushe")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->usedTimes("GusheCard") <= player->getMark("jici") && !player->isKongcheng();
    }

    virtual const Card *viewAs() const
    {
        return new GusheCard;
    }
};

class Gushe : public TriggerSkill
{
public:
    Gushe() : TriggerSkill("gushe")
    {
        events << Pindian;
		view_as_skill = new GusheViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *wangsitu, QVariant &data) const
    {
        PindianStruct * pindian = data.value<PindianStruct *>();
		if (pindian->reason != "gushe") return false;
		QList<ServerPlayer *> losers;
		if (pindian->success) {
			losers << pindian->to;
		} else {
			losers << wangsitu;
			if (pindian->from_number == pindian->to_number)
                losers << pindian->to;
			wangsitu->gainMark("#rap");
			if (wangsitu->getMark("#rap") >= 7)
				room->killPlayer(wangsitu);
		}
		foreach (ServerPlayer *loser, losers) {
			if (loser->isDead()) continue;
			if (!room->askForDiscard(loser, "gushe", 1, 1, wangsitu->isAlive(), true, "@gushe-discard:" + wangsitu->objectName())) {
				wangsitu->drawCards(1);
			}
		}
        return false;
    }
};

class Jici : public TriggerSkill
{
public:
    Jici() : TriggerSkill("jici")
    {
        events << PindianVerifying << EventPhaseChanging;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room* room, ServerPlayer *wangsitu, QVariant &data) const
    {
        if (triggerEvent == PindianVerifying && TriggerSkill::triggerable(wangsitu)) {
            PindianStruct *pindian = data.value<PindianStruct *>();
            if (pindian->reason != "gushe" || pindian->from != wangsitu) return false;
            int n = wangsitu->getMark("#rap");
            if (pindian->from_number == n && wangsitu->askForSkillInvoke(objectName(), "prompt1")) {
                wangsitu->broadcastSkillInvoke(objectName());
                LogMessage log;
                log.type = "$JiciAddTimes";
                log.from = wangsitu;
                log.arg = "gushe";
                room->sendLog(log);
                room->addPlayerMark(wangsitu, "jici");
            } else if (pindian->from_number < n && wangsitu->askForSkillInvoke(objectName(), "prompt2:::" + QString::number(n))) {
                wangsitu->broadcastSkillInvoke(objectName());
                LogMessage log;
                log.type = "$JiciAddNumber";
                log.from = wangsitu;
                pindian->from_number = qMin(pindian->from_number + n, 13);
                log.arg = QString::number(n);
                log.arg2 = QString::number(pindian->from_number);
                room->sendLog(log);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(wangsitu, "jici", 0);
        }
        return false;
    }
};

class Qizhi : public TriggerSkill
{
public:
    Qizhi() : TriggerSkill("qizhi")
    {
        events << TargetSpecified << EventPhaseStart;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified && TriggerSkill::triggerable(player) && player->getPhase() != Player::NotActive) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getTypeId() != Card::TypeBasic && use.card->getTypeId() != Card::TypeTrick)
                return false;
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!use.to.contains(p) && player->canDiscard(p, "he"))
                    targets << p;
            }
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@qizhi-target", true, true);
            if (target){
                room->addPlayerMark(player, "#qizhi");
                player->broadcastSkillInvoke(objectName());
                int id = room->askForCardChosen(player, target, "he", objectName(), false, Card::MethodDiscard);
                CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, player->objectName(), target->objectName(), objectName(), QString());
                room->throwCard(Sanguosha->getCard(id), reason, target, player);
                target->drawCards(1, objectName());
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive)
                room->setPlayerMark(player, "#qizhi", 0);
        }
        return false;
    }
};

class Jinqu : public PhaseChangeSkill
{
public:
    Jinqu() : PhaseChangeSkill("jinqu")
    {
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        if (player->getPhase() == Player::Finish) {
            Room *room = player->getRoom();
            if (room->askForSkillInvoke(player, objectName())) {
                player->broadcastSkillInvoke(objectName());
                player->drawCards(2, objectName());
                int n = player->getHandcardNum() - player->getMark("#qizhi");
                if (n > 0)
                    room->askForDiscard(player, objectName(), n, n, false, false, "@jinqu-discard");
            }
        }

        return false;
    }
};

class Cuifeng : public TriggerSkill
{
public:
    Cuifeng() : TriggerSkill("cuifeng")
    {
        events << Damaged << EventPhaseStart << EventPhaseChanging;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged && TriggerSkill::triggerable(player)) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int i = 0; i < damage.damage; i++) {
                if (player->isNude()) break;
                const Card *card = room->askForCard(player, "..", "@cuifeng-put", data, Card::MethodNone);
                if (card) {
                    player->broadcastSkillInvoke(objectName());
                    room->notifySkillInvoked(player, objectName());
                    LogMessage log;
                    log.from = player;
                    log.type = "#InvokeSkill";
                    log.arg = objectName();
                    room->sendLog(log);
                    player->addToPile("cuifeng_feng", card);
                } else break;
            }
        } else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)) {
            if (player->getPhase() != Player::Start || player->getPile("cuifeng_feng").isEmpty()) return false;
            player->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            int x = player->getPile("cuifeng_feng").length();
            player->clearOnePrivatePile("cuifeng_feng");
            player->drawCards(2*x);
            room->setPlayerMark(player, "cuifeng", x);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(player, "cuifeng", 0);
        }
        return false;
    }
};

class CuifengTargetMod : public TargetModSkill
{
public:
    CuifengTargetMod() : TargetModSkill("#cuifeng-target")
    {

    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return from->getMark("cuifeng");
    }
};

ZiyuanCard::ZiyuanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void ZiyuanCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "ziyuan", QString());
    room->obtainCard(card_use.to.first(), this, reason, false);
}

void ZiyuanCard::onEffect(const CardEffectStruct &effect) const
{
    effect.from->getRoom()->recover(effect.to, RecoverStruct(effect.from));
}

class Ziyuan : public ViewAsSkill
{
public:
    Ziyuan() : ViewAsSkill("ziyuan")
    {
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        int sum = 0;
        foreach(const Card *card, selected)
            sum += card->getNumber();

        sum += to_select->getNumber();
        return !to_select->isEquipped() && sum <= 13;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZiyuanCard");
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        int sum = 0;
        foreach(const Card *card, cards)
            sum += card->getNumber();

        if (sum != 13)
            return NULL;

        ZiyuanCard *rende_card = new ZiyuanCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Jugu : public TriggerSkill
{
public:
    Jugu() : TriggerSkill("jugu")
    {
        events << TurnStart;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
        if (!room->getTag("FirstRound").toBool()) return false;
        foreach (ServerPlayer *mizhu, room->getAlivePlayers()) {
            if (!TriggerSkill::triggerable(mizhu)) continue;
            room->sendCompulsoryTriggerLog(mizhu, objectName());
            mizhu->broadcastSkillInvoke(objectName());
            mizhu->drawCards(mizhu->getMaxHp(), objectName());
        }
        return false;
    }
};

class JuguKeep : public MaxCardsSkill
{
public:
    JuguKeep() : MaxCardsSkill("#jugu-keep")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        if (target->hasSkill(this))
            return target->getMaxHp();
        else
            return 0;
    }
};

class Hongde : public TriggerSkill
{
public:
    Hongde() : TriggerSkill("hongde")
    {
        events << CardsMoveOneTime;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (!room->getTag("FirstRound").toBool() && move.to == player && move.to_place == Player::PlaceHand) {
            if (move.card_ids.size() < 2) return false;
        } else if (move.from == player && !(move.to == player && (move.to_place == Player::PlaceHand || move.to_place == Player::PlaceEquip))) {
            int lose_num = 0;
            for (int i = 0; i < move.card_ids.length(); i++) {
                if (move.from_places[i] == Player::PlaceHand || move.from_places[i] == Player::PlaceEquip) {
                    lose_num ++;
                }
            }
            if (lose_num < 2) return false;
        } else return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@hongde-invoke", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), target->objectName());
            target->drawCards(1, objectName());
        }
        return false;
    }
};

DingpanCard::DingpanCard()
{
}

bool DingpanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *) const
{
    return targets.isEmpty() && to_select->hasEquip();
}

void DingpanCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = source->getRoom();
    target->drawCards(1, "dingpan");
    if (!target->hasEquip()) return;
    QStringList choices;
    foreach (const Card *card, target->getEquips()) {
        if (source->canDiscard(target, card->getEffectiveId())) {
            choices << "disequip";
            break;
        }
    }
    choices << "takeback";
    if (room->askForChoice(target, "dingpan", choices.join("+"), QVariant::fromValue(source), "@dingpan-choose:"+source->objectName(), "disequip+takeback") == "disequip")
        room->throwCard(room->askForCardChosen(source, target, "e", "dingpan", false, Card::MethodDiscard), target, source);
    else {
        QList<const Card *> equips = target->getEquips();
        if (equips.isEmpty()) return;
        DummyCard *card = new DummyCard;
        foreach (const Card *equip, equips) {
            card->addSubcard(equip);
        }
        if (card->subcardsLength() > 0)
            target->obtainCard(card);
        room->damage(DamageStruct("dingpan", source, target));
    }
}

class DingpanViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DingpanViewAsSkill() : ZeroCardViewAsSkill("dingpan")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("#dingpan") > 0;
    }

    virtual const Card *viewAs() const
    {
        return new DingpanCard;
    }
};

class Dingpan : public TriggerSkill
{
public:
    Dingpan() : TriggerSkill("dingpan")
    {
        events << PlayCard << EventPhaseChanging;
        view_as_skill = new DingpanViewAsSkill;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PlayCard) {
            int rebel_num = 0;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->getRole() == "rebel")
                    rebel_num++;
            }
            room->setPlayerMark(player, "#dingpan", rebel_num - player->usedTimes("DingpanCard"));
        } else {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.from == Player::Play)
                room->setPlayerMark(player, "#dingpan", 0);
        }
        return false;
    }
};

LianzhuCard::LianzhuCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void LianzhuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *dongbai = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = dongbai->getRoom();
    room->showCard(dongbai, getEffectiveId());
    room->getThread()->delay(1000);

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, dongbai->objectName(), target->objectName(), "lianzhu", QString());
    room->obtainCard(target, this, reason);

    if (target->isAlive() && isBlack() && !room->askForDiscard(target, "lianzhu", 2, 2, true, true, "@lianzhu-discard:"+dongbai->objectName()))
        dongbai->drawCards(2, "lianzhu");
}

class Lianzhu : public OneCardViewAsSkill
{
public:
    Lianzhu() : OneCardViewAsSkill("lianzhu")
    {
        filter_pattern = ".";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LianzhuCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LianzhuCard *card = new LianzhuCard;
        card->addSubcard(originalCard);
        card->setSkillName(objectName());
        return card;
    }
};

class Xiahui : public TriggerSkill
{
public:
    Xiahui() : TriggerSkill("xiahui")
    {
        events << EventPhaseProceeding << CardsMoveOneTime << PreCardsMoveOneTime << HpChanged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseProceeding && player->getPhase() == Player::Discard && TriggerSkill::triggerable(player)) {
            int num = 0;
            foreach (const Card *card, player->getHandcards()) {
                if (card->isRed())
                    num++;
            }
            int discard_num = num - player->getMaxCards();
            if (player->getHandcardNum() > player->getMaxCards() && player->getHandcardNum() > num){
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
            }
            if (discard_num > 0)
                room->askForDiscard(player, "gamerule", discard_num, discard_num, false, false, "@xiahui-discard", ".|red|.|hand");
            room->setTag("SkipGameRule", true);
        } else if (triggerEvent == PreCardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && !player->property("xiahui_record").toString().isEmpty()) {
                QStringList xiahui_ids = player->property("xiahui_record").toString().split("+");
                QStringList xiahui_copy = xiahui_ids;
                foreach (QString card_data, xiahui_copy) {
                    int id = card_data.toInt();
                    if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                        xiahui_ids.removeOne(card_data);
                    }
                }
                room->setPlayerProperty(player, "xiahui_record", xiahui_ids.join("+"));
            }
        } else if (triggerEvent == CardsMoveOneTime && TriggerSkill::triggerable(player)) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (move.from == player && (move.from_places.contains(Player::PlaceHand) || move.from_places.contains(Player::PlaceEquip))
                && (move.to != player && move.to_place == Player::PlaceHand)) {
                ServerPlayer *target = (ServerPlayer *)move.to;
                if (!target || target->isDead()) return false;
                QStringList xiahui_ids = target->property("xiahui_record").toString().split("+");
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == target && room->getCardPlace(id) == Player::PlaceHand && Sanguosha->getCard(id)->isBlack()) {
                        xiahui_ids << QString::number(id);
                    }
                }
                room->setPlayerProperty(target, "xiahui_record", xiahui_ids.join("+"));
            }
        } else if (triggerEvent == HpChanged)
            room->setPlayerProperty(player, "xiahui_record", QStringList());
        return false;
    }
};

class XiahuiCardLimited : public CardLimitedSkill
{
public:
    XiahuiCardLimited() : CardLimitedSkill("#xiahui-limited")
    {
    }
    virtual bool isCardLimited(const Player *player, const Card *card, Card::HandlingMethod method) const
    {
        if (player->property("xiahui_record").toString().isEmpty()) return false;
        if (method == Card::MethodUse || method == Card::MethodResponse || method == Card::MethodDiscard) {
            QStringList xiahui_ids = player->property("xiahui_record").toString().split("+");
            foreach (QString card_str, xiahui_ids) {
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

class FanghunViewAsSkill : public OneCardViewAsSkill
{
public:
    FanghunViewAsSkill() : OneCardViewAsSkill("fanghun")
    {
        response_or_use = true;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        const Card *card = to_select;

        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            return card->isKindOf("Jink");
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE:
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            if (pattern == "slash")
                return card->isKindOf("Jink");
            else if (pattern == "jink")
                return card->isKindOf("Slash");
        }
        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("#plumshadow") > 0;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern == "jink" || pattern == "slash") && player->getMark("#plumshadow") > 0;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        if (originalCard->isKindOf("Slash")) {
            Jink *jink = new Jink(originalCard->getSuit(), originalCard->getNumber());
            jink->addSubcard(originalCard);
            jink->setSkillName(objectName());
            return jink;
        } else if (originalCard->isKindOf("Jink")) {
            Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
            slash->addSubcard(originalCard);
            slash->setSkillName(objectName());
            return slash;
        } else
            return NULL;
    }
};

class Fanghun : public TriggerSkill
{
public:
    Fanghun() : TriggerSkill("fanghun")
    {
        events << Damage << CardFinished << CardResponded << CardExtraCost << EventLoseSkill;
        view_as_skill = new FanghunViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhaoxiang, QVariant &data) const
    {
        if (triggerEvent == Damage && TriggerSkill::triggerable(zhaoxiang)) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->isKindOf("Slash")) {
                room->sendCompulsoryTriggerLog(zhaoxiang, objectName());
                zhaoxiang->broadcastSkillInvoke(objectName());
                zhaoxiang->gainMark("#plumshadow");
            }
        } else if (triggerEvent == CardFinished || triggerEvent == CardResponded) {
            const Card *fanghun_card = NULL;
            if (triggerEvent == CardFinished)
                fanghun_card = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (!resp.m_isUse)
                    fanghun_card = resp.m_card;
            }
            if (fanghun_card && fanghun_card->getSkillName() == "fanghun")
                zhaoxiang->drawCards(1, "fanghun");
        } else if (triggerEvent == CardExtraCost) {
            const Card *fanghun_card = NULL;
            if (data.canConvert<CardUseStruct>())
                fanghun_card = data.value<CardUseStruct>().card;
            else if (data.canConvert<CardResponseStruct>())
                fanghun_card = data.value<CardResponseStruct>().m_card;
            if (fanghun_card && fanghun_card->getSkillName() == "fanghun") {
                zhaoxiang->loseMark("#plumshadow");
                room->addPlayerMark(zhaoxiang, "plumshadow_removed");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == objectName()) {
            room->addPlayerMark(zhaoxiang, "plumshadow_removed", zhaoxiang->getMark("#plumshadow"));
            room->setPlayerMark(zhaoxiang, "#plumshadow", 0);
        }
        return false;
    }
};

class Fuhan : public TriggerSkill
{
public:
    Fuhan() : TriggerSkill("fuhan")
    {
        events << EventPhaseStart;
        frequency = Limited;
        limit_mark = "@assisthan";
    }

    static QStringList GetAvailableGenerals(ServerPlayer *zuoci)
    {
        QSet<QString> all = Sanguosha->getLimitedGeneralNames("shu").toSet();
        Room *room = zuoci->getRoom();
        if (isNormalGameMode(room->getMode())
            || room->getMode().contains("_mini_")
            || room->getMode() == "custom_scenario")
            all.subtract(Config.value("Banlist/Roles", "").toStringList().toSet());
        else if (room->getMode() == "06_XMode") {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["XModeBackup"].toStringList().toSet());
        } else if (room->getMode() == "02_1v1") {
            all.subtract(Config.value("Banlist/1v1", "").toStringList().toSet());
            foreach(ServerPlayer *p, room->getAlivePlayers())
                all.subtract(p->tag["1v1Arrange"].toStringList().toSet());
        }
        QSet<QString> huashen_set, room_set;
        foreach (ServerPlayer *player, room->getAlivePlayers()) {
            QVariantList huashens = player->tag["Huashens"].toList();
            foreach(QVariant huashen, huashens)
                huashen_set << huashen.toString();

            QString name = player->getGeneralName();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->getMainGeneral(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;

            if (!player->getGeneral2()) continue;

            name = player->getGeneral2Name();
            if (Sanguosha->isGeneralHidden(name)) {
                QString fname = Sanguosha->getMainGeneral(name);
                if (!fname.isEmpty()) name = fname;
            }
            room_set << name;
        }

        static QSet<QString> banned;
        if (banned.isEmpty())
            banned << "zhaoxiang";

        return (all - banned - huashen_set - room_set).toList();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::RoundStart)
            return false;
        if (player->getMark(limit_mark) > 0 && (player->getMark("#plumshadow") > 0 || player->getMark("plumshadow_removed") > 0) && player->askForSkillInvoke(this)) {
            room->removePlayerMark(player, limit_mark);
            room->addPlayerMark(player, "plumshadow_removed", player->getMark("#plumshadow"));
            room->setPlayerMark(player, "#plumshadow", 0);
            player->broadcastSkillInvoke(objectName());
            QList<const Skill *> skills = player->getVisibleSkillList();
            QStringList detachList, acquireList;
            foreach (const Skill *skill, skills) {
                if (!skill->isAttachedLordSkill())
                    detachList.append("-" + skill->objectName());
            }
            room->handleAcquireDetachSkills(player, detachList);
            QStringList all_generals = GetAvailableGenerals(player);
            qShuffle(all_generals);
            if (all_generals.isEmpty()) return false;
            QStringList general_list = all_generals.mid(0, qMin(5, all_generals.length()));
            QString general_name = room->askForGeneral(player, general_list, true, "fuhan");
            const General *general = Sanguosha->getGeneral(general_name);
            foreach (const Skill *skill, general->getVisibleSkillList()) {
                if (skill->isLordSkill() && !player->isLord())
                    continue;

                acquireList.append(skill->objectName());
            }
            room->handleAcquireDetachSkills(player, acquireList);
            room->setPlayerProperty(player, "kingdom", general->getKingdom());
            player->setGender(general->getGender());
            player->setGeneralName(general_name);
            room->broadcastProperty(player, "general");

            int maxhp = qMin(player->getMark("plumshadow_removed"), Sanguosha->getPlayerCount(room->getMode()));
            room->setPlayerProperty(player, "maxhp", maxhp);
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (player->getHp() > p->getHp()) {
                    return false;
                }
            }
            room->recover(player, RecoverStruct(player));
        }
        return false;
    }
};

class Qizhou : public TriggerSkill
{
public:
    Qizhou() : TriggerSkill("qizhou")
    {
        events << CardsMoveOneTime << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() == objectName()) {
                QStringList qizhou_skills = player->tag["QizhouSkills"].toStringList();
                QStringList detachList;
                foreach(QString skill_name, qizhou_skills)
                    detachList.append("-" + skill_name);
                room->handleAcquireDetachSkills(player, detachList);
                player->tag["QizhouSkills"] = QVariant();
            }
            return false;
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
        }

        if (!player->isAlive() || !player->hasSkill(this, true)) return false;

        acquired_skills.clear();
        detached_skills.clear();
        QizhouChange(room, player, 1, "mashu");
        QizhouChange(room, player, 2, "nosyingzi");
        QizhouChange(room, player, 3, "duanbing");
        QizhouChange(room, player, 4, "fenwei");
        if (!acquired_skills.isEmpty() || !detached_skills.isEmpty()) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            room->handleAcquireDetachSkills(player, acquired_skills + detached_skills);
        }
        return false;
    }

private:
    void QizhouChange(Room *room, ServerPlayer *player, int x, const QString &skill_name) const
    {
        QStringList qizhou_skills = player->tag["QizhouSkills"].toStringList();
        QStringList suitlist;
        foreach (const Card *card, player->getEquips()) {
            QString suit = card->getSuitString();
            if (!suitlist.contains(suit))
                suitlist << suit;
        }
        if (suitlist.length() >= x) {
            if (!qizhou_skills.contains(skill_name)) {
                room->notifySkillInvoked(player, "qizhou");
                acquired_skills.append(skill_name);
                qizhou_skills << skill_name;
            }
        } else {
            if (qizhou_skills.contains(skill_name)) {
                detached_skills.append("-" + skill_name);
                qizhou_skills.removeOne(skill_name);
            }
        }
        player->tag["QizhouSkills"] = QVariant::fromValue(qizhou_skills);
    }

    mutable QStringList acquired_skills, detached_skills;
};

ShanxiCard::ShanxiCard()
{
}

bool ShanxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && Self->inMyAttackRange(to_select) && Self->canDiscard(to_select, "he");
}

void ShanxiCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    if (effect.from->canDiscard(effect.to, "he")) {
        const Card *card = Sanguosha->getCard(room->askForCardChosen(effect.from, effect.to, "he", "shanxi", false, Card::MethodDiscard));
		bool isjink = card->isKindOf("Jink");
        room->throwCard(card, effect.to, effect.from);
        if (isjink && !effect.to->isKongcheng())
            room->doGongxin(effect.from, effect.to, QList<int>(), "shanxi");
        else if (!isjink && !effect.from->isKongcheng())
            room->doGongxin(effect.to, effect.from, QList<int>(), "shanxi");
    }
}

class Shanxi : public OneCardViewAsSkill
{
public:
    Shanxi() : OneCardViewAsSkill("shanxi")
    {
        filter_pattern = "BasicCard|red!";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ShanxiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        ShanxiCard *sx = new ShanxiCard;
        sx->addSubcard(originalCard);
        return sx;
    }
};

class Bingzheng : public TriggerSkill
{
public:
    Bingzheng() : TriggerSkill("bingzheng")
    {
        events << EventPhaseEnd;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play) return false;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            if (p->getHandcardNum() != p->getHp())
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@bingzheng-invoke", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            QStringList choices;
            choices << "draw";
            if (!target->isKongcheng())
                choices << "discard";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(), "@bingzheng-choice:"+target->objectName(), "draw+discard");
            if (choice == "draw")
                target->drawCards(1, objectName());
            else if (target->canDiscard(target, "h"))
                room->askForDiscard(target, objectName(), 1, 1);
            if (target->getHandcardNum() == target->getHp()) {
                player->drawCards(1, objectName());
                if (player == target) return false;
                const Card *card = room->askForExchange(player, objectName(), 1, 1, true, "@bingzheng-give:"+target->objectName(), true);
                if (card) {
                    CardMoveReason r(CardMoveReason::S_REASON_GIVE, player->objectName(), target->objectName(), objectName(), QString());
                    room->obtainCard(target, card, r, false);
                }
            }
        }
        return false;
    }
};

class Sheyan : public TriggerSkill
{
public:
    Sheyan() : TriggerSkill("sheyan")
    {
        events << TargetConfirming;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isNDTrick() && !use.card->isKindOf("Collateral") && !use.card->isKindOf("BeatAnother")) {
            QList<ServerPlayer *> targets;
            room->setCardFlag(use.card, "Global_NoDistanceChecking");
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (use.to.contains(p)) {
                    if (use.to.length() > 1)
                        targets << p;
                } else if (!room->isProhibited(use.from, p, use.card) && use.card->targetFilter(QList<const Player *>(), p, use.from))
                    targets << p;
            }
            room->setCardFlag(use.card, "-Global_NoDistanceChecking");
            if (targets.isEmpty()) return false;
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@sheyan-invoke:::" + use.card->objectName(), true, true);
            if (target) {
                player->broadcastSkillInvoke(objectName());
                if (use.to.contains(target))
                    use.nullified_list << target->objectName();
                else {
                    use.to.append(target);
                    room->sortByActionOrder(use.to);
                }
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};

FumanCard::FumanCard()
{
    handling_method = Card::MethodNone;
}

bool FumanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    QStringList fuman_prop = Self->property("fuman").toString().split("+");
    return !fuman_prop.contains(to_select->objectName()) && targets.isEmpty() && to_select != Self;
}

void FumanCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, card_use.from->objectName(), card_use.to.first()->objectName(), "fuman", QString());
    room->obtainCard(card_use.to.first(), this, reason);
}

void FumanCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *target = targets.first();
    QSet<QString> fuman_prop = source->property("fuman").toString().split("+").toSet();
    fuman_prop.insert(target->objectName());
    room->setPlayerProperty(source, "fuman", QStringList(fuman_prop.toList()).join("+"));
}

class FumanViewAsSkill : public OneCardViewAsSkill
{
public:
    FumanViewAsSkill() : OneCardViewAsSkill("fuman")
    {
        filter_pattern = "Slash";
    }

    bool isEnabledAtPlay(const Player *) const
    {
        return true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        FumanCard *card = new FumanCard;
        card->addSubcard(originalCard);
        return card;
    }
};

class Fuman : public TriggerSkill
{
public:
    Fuman() : TriggerSkill("fuman")
    {
        events << CardUsed << CardResponded << CardsMoveOneTime << EventPhaseChanging;
        view_as_skill = new FumanViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            if (!move.from || move.from != player) return false;
            if (move.to && move.to_place == Player::PlaceHand && move.reason.m_reason == CardMoveReason::S_REASON_GIVE && move.reason.m_skillName == objectName()) {
                QVariantList fuman_ids = player->tag["fuman_ids"].toList();
                ServerPlayer *target = (ServerPlayer *)move.to;
                foreach (int id, move.card_ids) {
                    if (room->getCardOwner(id) == target && room->getCardPlace(id) == Player::PlaceHand)
                        fuman_ids << id;
                }
                player->tag["fuman_ids"] = fuman_ids;
            }else if ((move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) != CardMoveReason::S_REASON_USE) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    QVariantList fuman_ids = p->tag["fuman_ids"].toList();
                    QVariantList new_ids;
                    foreach (QVariant card_data, fuman_ids) {
                        int card_id = card_data.toInt();
                        if (!move.card_ids.contains(card_id))
                            new_ids << card_id;
                    }
                    p->tag["fuman_ids"] = new_ids;
                }
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                QVariantList fuman_ids = p->tag["fuman_ids"].toList();
                QVariantList new_ids;
                foreach (QVariant card_data, fuman_ids) {
                    int card_id = card_data.toInt();
                    if (!player->handCards().contains(card_id))
                        new_ids << card_id;
                }
                p->tag["fuman_ids"] = new_ids;
            }
        } else {
            const Card *usecard = NULL;
            if (triggerEvent == CardUsed)
                usecard = data.value<CardUseStruct>().card;
            else if (triggerEvent == CardResponded) {
                CardResponseStruct resp = data.value<CardResponseStruct>();
                if (resp.m_isUse)
                    usecard = resp.m_card;
            }
            if (usecard && usecard->getTypeId() != Card::TypeSkill) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    bool can_invoke = false;
                    QVariantList fuman_ids = p->tag["fuman_ids"].toList();
                    QVariantList new_ids;
                    foreach (QVariant card_data, fuman_ids) {
                        int card_id = card_data.toInt();
                        if (usecard->getSubcards().contains(card_id))
                            can_invoke = true;
                        else
                            new_ids << card_id;
                    }
                    p->tag["fuman_ids"] = new_ids;
                    if (can_invoke) {
                        LogMessage log;
                        log.type = "#SkillForce";
                        log.from = p;
                        log.arg = objectName();
                        room->sendLog(log);
                        p->drawCards(1, "fuman");
                    }
                }
            }
        }
        return false;
    }
};

class Xiashu : public TriggerSkill
{
public:
    Xiashu() : TriggerSkill("xiashu")
    {
        events << EventPhaseStart;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play || player->isKongcheng()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName(), "@xiashu-invoke", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            DummyCard *handcards = player->wholeHandCards();
            handcards->deleteLater();
            CardMoveReason r(CardMoveReason::S_REASON_GIVE, player->objectName(), target->objectName(), objectName(), QString());
            room->obtainCard(target, handcards, r, false);
            if (target->isKongcheng()) return false;
            const Card *to_show = room->askForExchange(target, objectName(), 1000, 1, false, "@xiashu-show:"+player->objectName());
            room->showCard(target, to_show->getSubcards());
            DummyCard *other = new DummyCard;
            foreach (const Card *card, target->getHandcards()) {
                int id = card->getEffectiveId();
                if (!to_show->getSubcards().contains(id))
                    other->addSubcard(id);
            }
            QStringList choices;
            choices << "showcards";
            if (!other->getSubcards().isEmpty())
                choices << "othercards";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), QVariant(), "@xiashu-choice:" + target->objectName(), "showcards+othercards");
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
            if (choice == "showcards")
                room->obtainCard(player, to_show, reason, true);
            else
                room->obtainCard(player, other, reason, false);
            delete other;
        }
        return false;
    }
};

class Kuanshi : public TriggerSkill
{
public:
    Kuanshi() : TriggerSkill("kuanshi")
    {
        events << EventPhaseStart << DamageInflicted;
        view_as_skill = new dummyVS;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            switch (player->getPhase()) {
            case Player::Finish: {
                if (!TriggerSkill::triggerable(player)) break;
                ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@kuanshi-invoke", true);
                if (target) {
                    LogMessage log;
                    log.type = "#InvokeSkill";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(player, objectName());
                    player->broadcastSkillInvoke(objectName());
                    player->tag["KuanshiTarget"] = QVariant::fromValue(target);
                }
                break;
            }
            case Player::RoundStart: {
                player->tag.remove("KuanshiTarget");
                if (player->getMark("#kuanshi") > 0) {
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = player;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->removePlayerTip(player, "#kuanshi");
                    player->skip(Player::Draw);
                }
                break;
            }
            default:
                break;
            }
        } else if (triggerEvent == DamageInflicted) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.damage > 1) {
                foreach (ServerPlayer *kanze, room->getAlivePlayers()) {
                    if (kanze->tag["KuanshiTarget"].value<ServerPlayer *>() != player) continue;
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = kanze;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, kanze->objectName(), player->objectName());
                    kanze->tag.remove("KuanshiTarget");
                    room->addPlayerTip(kanze, "#kuanshi");
                    return true;
                }
            }
        }
        return false;
    }
};

LianjiCard::LianjiCard()
{
    will_throw = false;
	will_sort = false;
    handling_method = Card::MethodNone;
}

void LianjiCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *target = effect.to;

    QString card_name = Sanguosha->getCard(getEffectiveId())->objectName();

    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "lianji", QString());
    room->obtainCard(target, this, reason);

    QList<int> weapons;
    foreach (int card_id, room->getDrawPile()) {
        const Card *card = Sanguosha->getCard(card_id);
        if (card->isKindOf("Weapon")) {
            weapons << card_id;
        }
    }

    if (weapons.isEmpty()){
        LogMessage log;
        log.type = "$SearchFailed";
        log.from = source;
        log.arg = "weapon";
        room->sendLog(log);
    } else {
        int index = qrand() % weapons.length();
        int id = weapons.at(index);
        Card *weapon = Sanguosha->getCard(id);
        if (weapon->objectName() == "qinggang_sword") {
            int sword_id = -1;
            Package *package = PackageAdder::packages()["BestLoyalistCard"];
            if (package) {
                QList<Card *> all_cards = package->findChildren<Card *>();
                foreach (Card *card, all_cards) {
                    if (card->objectName() == "treasured_sword") {
                        sword_id = card->getEffectiveId();
                        break;
                    }
                }
            }
            if (sword_id > 0) {
                LogMessage log;
                log.type = "#RemoveCard";
                log.card_str = weapon->toString();
                room->sendLog(log);
                room->getDrawPile().removeOne(id);
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_UPDATE_PILE, QVariant(room->getDrawPile().length()));
                weapon = Sanguosha->getCard(sword_id);
                log.type = "#AddCard";
                log.card_str = weapon->toString();
                room->sendLog(log);
                room->setCardMapping(sword_id, NULL, Player::DrawPile);
            }
        }
        CardMoveReason reason(CardMoveReason::S_REASON_USE, target->objectName(), target->objectName(), QString(), QString());
        room->moveCardTo(weapon, NULL, Player::PlaceTable, reason, true);
        room->useCard(CardUseStruct(weapon, target, target));
    }
    int give_id = getEffectiveId();
    if (room->getCardOwner(give_id) != target || room->getCardPlace(give_id) != Player::PlaceHand) return;
    room->setCardFlag(give_id, "lianjiGiveCard");
    bool usecard = Sanguosha->getCard(give_id)->isAvailable(target) &&
        room->askForUseCard(target, QString::number(give_id), "@lianji-use:::"+Sanguosha->getCard(give_id)->objectName());
    room->setCardFlag(give_id, "-lianjiGiveCard");
    if (usecard) {
        QVariantList target_list = target->tag["lianjiuse_target"].toList();
        target->tag.remove("lianjiuse_target");
        QList<ServerPlayer *> targets;
        foreach (QVariant x, target_list) {
            ServerPlayer *t = x.value<ServerPlayer *>();
            if (t && t->isAlive() && t != target)
                targets << t;
        }
        if (target->getWeapon() && !targets.isEmpty()) {
            ServerPlayer *to_give = room->askForPlayerChosen(target, targets, objectName(), "@lianji-giveweapon");
            CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, target->objectName(), to_give->objectName(), "lianji", QString());
            room->obtainCard(to_give, target->getWeapon(), reason2);
        }
    } else {
        if (card_name != "collateral" && card_name != "beat_another") {
            Card *to_use = Sanguosha->cloneCard(card_name, Card::NoSuit, 0, QStringList("Global_NoDistanceChecking"));
            to_use->setSkillName("_lianji");
            if (to_use->isAvailable(source) && !to_use->isKindOf("DelayedTrick") && !room->isProhibited(source, target, to_use) && to_use->targetFilter(QList<const Player *>(), target, source)) {
                if (to_use->isKindOf("AOE") || to_use->isKindOf("GlobalEffect"))
                    room->useCard(CardUseStruct(to_use, source, QList<ServerPlayer *>()));
                else
                    room->useCard(CardUseStruct(to_use, source, target));
            }
        }
        if (target->getWeapon()) {
            CardMoveReason reason2(CardMoveReason::S_REASON_GIVE, target->objectName(), source->objectName(), "lianji", QString());
            room->obtainCard(source, target->getWeapon(), reason2);
        }
    }
}

class LianjiViewAsSkill : public OneCardViewAsSkill
{
public:
    LianjiViewAsSkill() : OneCardViewAsSkill("lianji")
    {

    }

    virtual bool viewFilter(const Card *card) const
    {
        return card->isKindOf("Slash") || (card->isKindOf("TrickCard") && card->isBlack());
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("LianjiCard");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        LianjiCard *lianjiCard = new LianjiCard;
        lianjiCard->addSubcard(originalCard);
        return lianjiCard;
    }
};

class Lianji : public TriggerSkill
{
public:
    Lianji() : TriggerSkill("lianji")
    {
        events << TargetSpecified << PreDamageDone;
        view_as_skill = new LianjiViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *taiget, QVariant &data) const
    {
        if (triggerEvent == TargetSpecified) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (!use.card->hasFlag("lianjiGiveCard")) return false;
            QVariantList target_list;
            foreach (ServerPlayer *t, use.to) {
                target_list << QVariant::fromValue(t);
            }
            taiget->tag["lianjiuse_target"] = target_list;
        } else if (triggerEvent == PreDamageDone) {
            ServerPlayer *wangyun = room->getCurrent();
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card && damage.card->hasFlag("lianjiGiveCard") && damage.by_user)
                room->addPlayerMark(wangyun, "lianji", damage.damage);
        }
        return false;
    }
};

class LianjiProhibit : public ProhibitSkill
{
public:
    LianjiProhibit() : ProhibitSkill("#lianji-prohibit")
    {
    }

    bool isProhibited(const Player *, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (card->hasFlag("lianjiGiveCard"))
            return to->getPhase() != Player::NotActive;
        return false;
    }
};

class Moucheng : public TriggerSkill
{
public:
    Moucheng() : TriggerSkill("moucheng")
    {
        events << Damage;
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &) const
    {
        foreach (ServerPlayer *wangyun, room->getAlivePlayers()) {
            if (TriggerSkill::triggerable(wangyun) && wangyun->getMark(objectName()) < 1 && wangyun->getMark("lianji") > 2) {
                wangyun->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(wangyun, objectName());
                room->setPlayerMark(wangyun, objectName(), 1);
                room->addPlayerMark(wangyun, "@waked");
                room->handleAcquireDetachSkills(wangyun, "-lianji|jingong");
            }
        }
        return false;
    }
};

SmileDagger::SmileDagger(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("SmileDagger");
}

bool SmileDagger::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select != Self;
}

void SmileDagger::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
    effect.to->drawCards(effect.to->getLostHp(), objectName());
    room->damage(DamageStruct(this, effect.from, effect.to, 1));
}

HoneyTrap::HoneyTrap(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("HoneyTrap");
}

bool HoneyTrap::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->isMale() && !to_select->isKongcheng() && to_select != Self;
}

void HoneyTrap::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *target = effect.to;
    foreach (ServerPlayer *female, room->getAllPlayers()) {
        if (female->isFemale() && target->isAlive() && !target->isNude()) {
            int card_id = room->askForCardChosen(female, target, "he", objectName());
            CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, female->objectName());
            room->obtainCard(female, Sanguosha->getCard(card_id), reason, false);
            if (!female->isNude() && source->isAlive()) {
                const Card *card = room->askForExchange(female, objectName(), 1, 1, false, "@honeytrap-give::" + source->objectName());
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, female->objectName(),
                    source->objectName(), objectName(), QString());
                reason.m_playerId = source->objectName();
                room->moveCardTo(card, female, source, Player::PlaceHand, reason);
                delete card;
            }
        }
    }
    if (source->isAlive() && target->isAlive()) {
        if (source->getHandcardNum() > target->getHandcardNum())
            room->damage(DamageStruct(this, target, source, 1));
        else if (source->getHandcardNum() < target->getHandcardNum())
            room->damage(DamageStruct(this, source, target, 1));
    }
}

JingongCard::JingongCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "jingong";
}

bool JingongCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *mutable_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, mutable_card, targets);
}

bool JingongCard::targetFixed() const
{
    Card *mutable_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetFixed();
}

bool JingongCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *mutable_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    if (mutable_card) {
        mutable_card->addSubcards(this->subcards);
        mutable_card->setCanRecast(false);
        mutable_card->deleteLater();
    }
    return mutable_card && mutable_card->targetsFeasible(targets, Self);
}

const Card *JingongCard::validate(CardUseStruct &) const
{
    Card *use_card = Sanguosha->cloneCard(Self->tag["jingong"].toString());
    use_card->setSkillName("jingong");
    use_card->addSubcards(this->subcards);
    return use_card;
}

class JingongViewAsSkill : public OneCardViewAsSkill
{
public:
    JingongViewAsSkill() : OneCardViewAsSkill("jingong")
    {
        filter_pattern = "EquipCard,Slash";
    }

    const Card *viewAs(const Card *originalCard) const
    {
        JingongCard *trick = new JingongCard;
        trick->addSubcard(originalCard->getId());
        return trick;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("JingongCard");
    }
};

class Jingong : public TriggerSkill
{
public:
    Jingong() : TriggerSkill("jingong")
    {
        events << PreCardUsed << EventPhaseChanging << EventPhaseStart << EventAcquireSkill;
        view_as_skill = new JingongViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *wangyun, QVariant &data) const
    {
        if (triggerEvent == PreCardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() != objectName()) return false;
            room->setPlayerFlag(wangyun, "jingongInvoked");
        } else if (triggerEvent == EventPhaseStart) {
            if (wangyun->getPhase() == Player::Play && wangyun->hasSkill("jingong", true))
                createRandomTrickCardName(room, wangyun);
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() == objectName())
                createRandomTrickCardName(room, wangyun);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                if (wangyun->getMark("damage_point_round") == 0 && wangyun->hasFlag("jingongInvoked")) {
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = wangyun;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(wangyun, objectName());
                    wangyun->broadcastSkillInvoke(objectName());
                    room->loseHp(wangyun);
                }
            }
        }
        return false;
    }

    QString getSelectBox() const
    {
        return Self->property("jingong_tricks").toString();
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &selected, const QList<const Player *> &) const
    {
        if (button_name.isEmpty()) return true;
        if (selected.isEmpty()) return false;
        return true;
    }

private:
    void createRandomTrickCardName(Room *room, ServerPlayer *player) const
    {
        QStringList card_list, all_tricks;
        QList<const TrickCard*> tricks = Sanguosha->findChildren<const TrickCard*>();
        foreach (const TrickCard *card, tricks) {
            if (!ServerInfo.Extensions.contains("!" + card->getPackage()) && card->isNDTrick()
                && !all_tricks.contains(card->objectName()) && !card->isKindOf("Nullification") && !card->isKindOf("Escape"))
                    all_tricks.append(card->objectName());
        }
        qShuffle(all_tricks);

        foreach (QString name, all_tricks) {
            card_list.append(name);
            if (card_list.length() >= 2) break;
        }

        QString exclusive_trick;
        if (qrand() % 2 == 0)
            exclusive_trick = "SmileDagger";
        else
            exclusive_trick = "HoneyTrap";
        card_list.append(exclusive_trick);
        room->setPlayerProperty(player, "jingong_tricks", card_list.join("+"));

    }
};

class Fuji : public TriggerSkill
{
public:
    Fuji() : TriggerSkill("fuji")
    {
        events << CardUsed;
        frequency = Compulsory;
    }
    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
            QStringList fuji_tag = use.card->tag["Fuji_tag"].toStringList();
            bool invoke_skill = false;
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->distanceTo(player) == 1) {
                    fuji_tag << p->objectName();
                    invoke_skill = true;
                }
            }
            if (invoke_skill) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                use.card->setTag("Fuji_tag", fuji_tag);
            }
        }
        return false;
    }
};

class Jiaozi : public TriggerSkill
{
public:
    Jiaozi() : TriggerSkill("jiaozi")
    {
        events << DamageCaused << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (p->getHandcardNum() >= player->getHandcardNum()) {
                return false;
            }
        }
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        damage.damage++;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class Xianfu : public TriggerSkill
{
public:
    Xianfu() : TriggerSkill("xianfu")
    {
        events << TurnStart << Damaged << HpRecover;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TurnStart && room->getTag("FirstRound").toBool()) {
            foreach (ServerPlayer *xizhicai, room->getAlivePlayers()) {
                if (!TriggerSkill::triggerable(xizhicai)) continue;
                room->sendCompulsoryTriggerLog(xizhicai, objectName());
                room->notifySkillInvoked(xizhicai, objectName());
                QList<int> types;
                types << 1;
                types << 2;
                room->broadcastSkillInvoke(objectName(), xizhicai, types);
                ServerPlayer *target = room->askForPlayerChosen(xizhicai, room->getOtherPlayers(xizhicai), objectName(), "@xianfu");
                xizhicai->tag["XianfuTarget"] = QVariant::fromValue(target);
            }
        } else if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            foreach (ServerPlayer *xizhicai, room->getAllPlayers()) {
                ServerPlayer *AssistTarget = xizhicai->tag["XianfuTarget"].value<ServerPlayer *>();
                if (AssistTarget && AssistTarget == player && player->isAlive() && xizhicai->isAlive()) {
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = xizhicai;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(xizhicai, objectName());
                    QList<int> types;
                    types << 5;
                    types << 6;
                    room->broadcastSkillInvoke(objectName(), xizhicai, types);
                    room->addPlayerTip(player, "#xianfu");
                    room->damage(DamageStruct(objectName(), NULL, xizhicai, damage.damage));
                }
            }
        } else if (triggerEvent == HpRecover) {
            foreach (ServerPlayer *xizhicai, room->getAllPlayers()) {
                ServerPlayer *AssistTarget = xizhicai->tag["XianfuTarget"].value<ServerPlayer *>();
                if (AssistTarget && AssistTarget == player && player->isAlive() && xizhicai->isAlive() && xizhicai->isWounded()) {
                    LogMessage log;
                    log.type = "#SkillForce";
                    log.from = xizhicai;
                    log.arg = objectName();
                    room->sendLog(log);
                    room->notifySkillInvoked(xizhicai, objectName());
                    QList<int> types;
                    types << 3;
                    types << 4;
                    room->broadcastSkillInvoke(objectName(), xizhicai, types);
                    room->addPlayerTip(player, "#xianfu");
                    room->recover(xizhicai, RecoverStruct(xizhicai, NULL, data.value<RecoverStruct>().recover));

                }
            }
        }
        return false;
    }
};

class Chouce : public TriggerSkill
{
public:
    Chouce() : TriggerSkill("chouce")
    {
        events << Damaged << FinishJudge;
        frequency = Frequent;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damaged) {
            DamageStruct damage = data.value<DamageStruct>();
            for (int i = 0; i < damage.damage; i++) {
                if (TriggerSkill::triggerable(player) && room->askForSkillInvoke(player, objectName(), QVariant::fromValue(damage))) {
                    player->broadcastSkillInvoke(objectName());
                    JudgeStruct judge;
                    judge.good = true;
                    judge.who = player;
                    judge.reason = objectName();
                    judge.play_animation = false;
                    room->judge(judge);
                    if (judge.pattern == "red") {
                        ServerPlayer *target = room->askForPlayerChosen(player, room->getAlivePlayers(), objectName(), "@chouce-draw");
                        ServerPlayer *AssistTarget = player->tag["XianfuTarget"].value<ServerPlayer *>();
                        if (AssistTarget && AssistTarget == target) {
                            room->addPlayerTip(target, "#xianfu");
                            target->drawCards(2, objectName());
                        } else
                            target->drawCards(1, objectName());
                    } else if (judge.pattern == "black") {
                        QList<ServerPlayer *> targets;
                        foreach (ServerPlayer *p, room->getAlivePlayers()) {
                            if (player->canDiscard(p, "hej"))
                                targets << p;
                        }
                        if (!targets.isEmpty()) {
                            player->setFlags("ChouceAIDiscard");
                            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@chouce-dis");
                            player->setFlags("-ChouceAIDiscard");
                            int card_id = room->askForCardChosen(player, target, "hej", objectName(), false, Card::MethodDiscard);
                            room->throwCard(card_id, room->getCardPlace(card_id) == Player::PlaceDelayedTrick ? NULL : target, player);
                        }
                    }
                } else {
                    break;
                }
            }
        } else {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return false;
            judge->pattern = judge->card->isRed() ? "red" : (judge->card->isBlack() ? "black" : "no_suit");
        }
        return false;
    }
};

QianyaCard::QianyaCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
}

void QianyaCard::onEffect(const CardEffectStruct &effect) const
{
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.from->objectName(), effect.to->objectName(), "qianya", QString());
    effect.from->getRoom()->obtainCard(effect.to, this, reason);
}

class QianyaViewAsSkill : public ViewAsSkill
{
public:
    QianyaViewAsSkill() : ViewAsSkill("qianya")
    {
        response_pattern = "@@qianya";
    }


    bool viewFilter(const QList<const Card *> &, const Card *to_select) const
    {
        return !to_select->isEquipped();
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            QianyaCard *qianya = new QianyaCard;
            qianya->addSubcards(cards);
            return qianya;
        }
        return NULL;
    }
};

class Qianya : public TriggerSkill
{
public:
    Qianya() : TriggerSkill("qianya")
    {
        events << TargetConfirmed;
        view_as_skill = new QianyaViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->isKindOf("TrickCard") && use.to.contains(player) && !player->isKongcheng()) {
            room->askForUseCard(player, "@@qianya", "@qianya-give", QVariant(), Card::MethodNone);
        }
        return false;
    }
};

class Shuimeng : public TriggerSkill
{
public:
    Shuimeng() : TriggerSkill("shuimeng")
    {
        events << EventPhaseEnd;
        view_as_skill = new dummyVS;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play || player->isKongcheng()) return false;
        QList<ServerPlayer *> targets;
        foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
            if (!p->isKongcheng())
                targets << p;
        }
        if (targets.isEmpty()) return false;
        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@shuimeng-invoke", true, true);
        if (target) {
            player->broadcastSkillInvoke(objectName());
            if (player->pindian(target, objectName())) {
                Card *to_use = Sanguosha->cloneCard("ex_nihilo", Card::NoSuit, 0);
                to_use->setSkillName("_shuimeng");
                if (!room->isProhibited(player, player, to_use) && to_use->targetFilter(QList<const Player *>(), player, player))
                    room->useCard(CardUseStruct(to_use, player, player));
            } else {
                Card *to_use = Sanguosha->cloneCard("dismantlement", Card::NoSuit, 0);
                to_use->setSkillName("_shuimeng");
                if (!room->isProhibited(target, player, to_use) && to_use->targetFilter(QList<const Player *>(), player, target))
                    room->useCard(CardUseStruct(to_use, target, player));
            }
        }
        return false;
    }
};

class Zongkui : public PhaseChangeSkill
{
public:
    Zongkui() : PhaseChangeSkill("zongkui")
    {
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getPhase() == Player::RoundStart;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->hasFlag("Global_TurnFirstRound")) {
            foreach (ServerPlayer *beimihu, room->getAlivePlayers()) {
                if (!TriggerSkill::triggerable(beimihu)) continue;
                QList<ServerPlayer *> targets;
                int min_hp = 100000;
                foreach (ServerPlayer *p, room->getOtherPlayers(beimihu)) {
                    if (p->getHp() < min_hp) {
                        targets.clear();
                        min_hp = p->getHp();
                    }
                    if (p->getHp() == min_hp && p->getMark("@puppet") == 0)
                        targets << p;
                }
                if (!targets.isEmpty()) {
                    room->sendCompulsoryTriggerLog(beimihu, objectName());
                    beimihu->broadcastSkillInvoke(objectName());
                    ServerPlayer *target = room->askForPlayerChosen(beimihu, targets, objectName(), "@zongkui2-invoke");
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, beimihu->objectName(), target->objectName());
                    target->gainMark("@puppet");
                }
            }
        }
        if (TriggerSkill::triggerable(player)) {
            QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getMark("@puppet") == 0)
                    targets << p;
            }
            if (targets.isEmpty()) return false;

            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@zongkui-invoke", true, true);
            if (target) {
                player->broadcastSkillInvoke(objectName());
                target->gainMark("@puppet");
            }
        }
        return false;
    }
};

class Guju : public MasochismSkill
{
public:
    Guju() : MasochismSkill("guju")
    {
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive() && target->getMark("@puppet") > 0;
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &) const
    {
        Room *room = target->getRoom();
        foreach (ServerPlayer *beimihu, room->getAlivePlayers()) {
            if (target->isDead() || target->getMark("@puppet") == 0) break;
            if (!TriggerSkill::triggerable(beimihu)) continue;
            room->sendCompulsoryTriggerLog(beimihu, objectName());
            beimihu->broadcastSkillInvoke(objectName());

            beimihu->drawCards(1, objectName());
            room->addPlayerMark(beimihu, "#guju");
        }
    }
};

class Baijia : public PhaseChangeSkill
{
public:
    Baijia() : PhaseChangeSkill("baijia")
    {
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target)
            && target->getMark("baijia") == 0
            && target->getPhase() == Player::Start
            && target->getMark("#guju") > 6;
    }

    virtual bool onPhaseChange(ServerPlayer *beimihu) const
    {
        Room *room = beimihu->getRoom();
        room->sendCompulsoryTriggerLog(beimihu, objectName());

        beimihu->broadcastSkillInvoke(objectName());

        room->setPlayerMark(beimihu, objectName(), 1);
        if (room->changeMaxHpForAwakenSkill(beimihu, 1) && beimihu->getMark(objectName()) == 1) {
            room->recover(beimihu, RecoverStruct(beimihu));
            foreach (ServerPlayer *p, room->getOtherPlayers(beimihu)) {
                if (p->getMark("@puppet") == 0)
                    p->gainMark("@puppet");
            }
            room->handleAcquireDetachSkills(beimihu, "-guju|canshib");
        }
        return false;
    }
};

CanshibCard::CanshibCard()
{
}

bool CanshibCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const
{
    QStringList available_targets = Self->property("canshib_available_targets").toString().split("+");

    return to_select->getMark("@puppet") > 0 && available_targets.contains(to_select->objectName());
}

void CanshibCard::use(Room *room, ServerPlayer *, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *p, targets) {
        room->removePlayerMark(p, "@puppet");
        p->setFlags("CanshibExtraTarget");
    }
}

class CanshibViewAsSkill : public ZeroCardViewAsSkill
{
public:
    CanshibViewAsSkill() : ZeroCardViewAsSkill("canshib")
    {
        response_pattern = "@@canshib";
    }

    virtual const Card *viewAs() const
    {
        return new CanshibCard;
    }
};

class Canshib : public TriggerSkill
{
public:
    Canshib() : TriggerSkill("canshib")
    {
        events << TargetChosen << TargetConfirming;
        view_as_skill = new CanshibViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() != Card::TypeBasic && !use.card->isNDTrick()) return false;
        if (use.card->isKindOf("Collateral") || use.card->isKindOf("BeatAnother")) return false;
        if (triggerEvent == TargetChosen) {
            QStringList available_targets;
            QList<ServerPlayer *> extra_targets;
            room->setCardFlag(use.card, "Global_NoDistanceChecking");
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (use.to.contains(p) || room->isProhibited(player, p, use.card)) continue;
                if (use.card->targetFilter(QList<const Player *>(), p, player) && p->getMark("@puppet") > 0)
                    available_targets.append(p->objectName());
            }
            room->setCardFlag(use.card, "-Global_NoDistanceChecking");
            if (!available_targets.isEmpty()) {
                room->setPlayerProperty(player, "canshib_available_targets", available_targets.join("+"));
                player->tag["canshib-use"] = data;
                room->askForUseCard(player, "@@canshib", "@canshib-add:::" + use.card->objectName());
                player->tag.remove("canshib-use");
                room->setPlayerProperty(player, "canshib_available_targets", QString());

                foreach (ServerPlayer *p, room->getAllPlayers()) {
                    if (p->hasFlag("CanshibExtraTarget")) {
                        p->setFlags("-CanshibExtraTarget");
                        extra_targets << p;
                    }
                }
            }
            if (!extra_targets.isEmpty()) {
                foreach (ServerPlayer *extra, extra_targets){
                    use.to.append(extra);
                }
                room->sortByActionOrder(use.to);
                data = QVariant::fromValue(use);
            }
        } else if (use.from && use.from->isAlive() && use.from->getMark("@puppet") > 0 && use.to.size() == 1) {
            if (player->askForSkillInvoke(objectName(), "prompt:::" + use.card->objectName())) {
                player->broadcastSkillInvoke(objectName());
                room->removePlayerMark(use.from, "@puppet");
                use.to.removeOne(player);
                data = QVariant::fromValue(use);
            }
        }
        return false;
    }
};

class Wenji : public TriggerSkill
{
public:
    Wenji() : TriggerSkill("wenji")
    {
        events << EventPhaseStart << CardUsed;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::Play && TriggerSkill::triggerable(player)) {
                QList<ServerPlayer *> targets;
                foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                    if (!p->isNude())
                        targets << p;
                }
                if (targets.isEmpty()) return false;

                ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@wenji-invoke", true, true);
                if (target) {
                    player->broadcastSkillInvoke(objectName());
                    const Card *card = room->askForExchange(target, objectName(), 1, 1, true, "@wenji-give:" + player->objectName());

                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), player->objectName(), objectName(), QString());
                    reason.m_playerId = player->objectName();
                    room->moveCardTo(card, target, player, Player::PlaceHand, reason, true);


                    const Card *record_card = Sanguosha->getCard(card->getEffectiveId());

                    if (!player->getHandcards().contains(record_card)) return false;

                    QString classname = record_card->getClassName();
                    if (record_card->isKindOf("Slash")) classname = "Slash";
                    QStringList list = player->tag[objectName()].toStringList();
                    list.append(classname);
                    player->tag[objectName()] = list;

                }
            } else if (player->getPhase() == Player::NotActive) {
                QList<ServerPlayer *> players = room->getAlivePlayers();
                foreach (ServerPlayer *p, players) {
                    p->tag.remove(objectName());
                }
            }
        } else if (triggerEvent == CardUsed) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isKindOf("Slash") || use.card->isNDTrick()) {
                QStringList wenji_list = player->tag[objectName()].toStringList();

                QString classname = use.card->getClassName();
                if (use.card->isKindOf("Slash")) classname = "Slash";
                if (!wenji_list.contains(classname)) return false;


                QStringList fuji_tag = use.card->tag["Fuji_tag"].toStringList();

                QList<ServerPlayer *> players = room->getOtherPlayers(player);
                foreach (ServerPlayer *p, players) {
                    fuji_tag << p->objectName();
                }
                use.card->setTag("Fuji_tag", fuji_tag);
            }
        }

        return false;
    }
};

class Tunjiang : public PhaseChangeSkill
{
public:
    Tunjiang() : PhaseChangeSkill("tunjiang")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() != Player::Finish) return false;
        if (player->hasFlag("TunjiangDisabled") || player->hasSkipped(Player::Play)) return false;
        if (room->askForSkillInvoke(player, objectName())) {
            player->broadcastSkillInvoke(objectName());
            QSet<QString> kingdomSet;
            foreach (ServerPlayer *p, room->getAlivePlayers())
                kingdomSet.insert(p->getKingdom());

            player->drawCards(kingdomSet.count(), objectName());
        }
        return false;
    }
};

class TunjiangRecord : public TriggerSkill
{
public:
    TunjiangRecord() : TriggerSkill("#tunjiang-record")
    {
        events << TargetSpecified;
        global = true;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 6;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.card->getTypeId() == Card::TypeSkill || player->getPhase() != Player::Play) return false;
        foreach (ServerPlayer *p, use.to) {
            if (p != player) {
                room->setPlayerFlag(player, "TunjiangDisabled");
                break;
            }
        }
        return false;
    }
};

class Qingzhong : public TriggerSkill
{
public:
    Qingzhong() : TriggerSkill("qingzhong")
    {
        events << EventPhaseStart << EventPhaseEnd;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (player->getPhase() != Player::Play) return false;
        if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player)) {
            if (room->askForSkillInvoke(player, objectName())) {
                player->broadcastSkillInvoke(objectName());
                player->drawCards(2, objectName());
                player->setFlags("qingzhongUsed");
            }
        } else if (triggerEvent == EventPhaseEnd && player->hasFlag("qingzhongUsed") && !player->isKongcheng()) {
            QList<ServerPlayer *> targets, players = room->getOtherPlayers(player);
            int x = player->getHandcardNum();
            foreach (ServerPlayer *p, players) {
                if (p->getHandcardNum() > x) continue;
                if (p->getHandcardNum() < x)
                    targets.clear();
                x = p->getHandcardNum();
                targets << p;

            }
            if (targets.isEmpty()) return false;
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName(), "@qingzhong-choose", x==player->getHandcardNum());
            if (target)
                room->swapCards(player, target, "h", objectName());
        }
        return false;
    }
};

class WeijingViewAsSkill : public ZeroCardViewAsSkill
{
public:
    WeijingViewAsSkill() : ZeroCardViewAsSkill("weijing")
    {
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return Slash::IsAvailable(player) && player->getMark("weijing_times") == 0;
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        return (pattern == "jink" || pattern == "slash") && player->getMark("weijing_times") == 0;
    }

    virtual const Card *viewAs() const
    {
        QString card_name;
        switch (Sanguosha->currentRoomState()->getCurrentCardUseReason()) {
        case CardUseStruct::CARD_USE_REASON_PLAY: {
            card_name = "slash";
            break;
        }
        case CardUseStruct::CARD_USE_REASON_RESPONSE_USE: {
            QString pattern = Sanguosha->currentRoomState()->getCurrentCardUsePattern();
            card_name = pattern;
        }
        default:
            break;
        }
        if (card_name.isEmpty()) return NULL;
        Card *use_card = Sanguosha->cloneCard(card_name);
        use_card->setSkillName(objectName());
        return use_card;
    }
};

class Weijing : public TriggerSkill
{
public:
    Weijing() : TriggerSkill("weijing")
    {
        events << PreCardUsed << PreCardResponded << RoundStart;
        view_as_skill = new WeijingViewAsSkill;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == RoundStart) {
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                room->setPlayerMark(p, "weijing_times", 0);
            }

        }
        const Card *card = NULL;
        if (triggerEvent == PreCardUsed)
            card = data.value<CardUseStruct>().card;
        else {
            CardResponseStruct response = data.value<CardResponseStruct>();
            if (response.m_isUse)
                card = response.m_card;
        }
        if (card && card->getSkillName() == objectName() && card->getHandlingMethod() == Card::MethodUse) {
            room->addPlayerMark(player, "weijing_times");
        }
        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("Slash"))
            return 1;
        else
            return 2;
    }
};





OLPackage::OLPackage()
: Package("OL")
{
    General *zhugeke = new General(this, "zhugeke", "wu", 3); // OL 002
    zhugeke->addSkill(new Aocai);
    zhugeke->addSkill(new Duwu);

    General *chengyu = new General(this, "chengyu", "wei", 3);
    chengyu->addSkill(new Shefu);
    chengyu->addSkill(new ShefuCancel);
    chengyu->addSkill(new DetachEffectSkill("shefu", "ambush"));
    related_skills.insertMulti("shefu", "#shefu-clear");
    related_skills.insertMulti("shefu", "#shefu-cancel");
    chengyu->addSkill(new Benyu);

    General *sunhao = new General(this, "sunhao$", "wu", 5); // SP 041
    sunhao->addSkill(new Canshi);
    sunhao->addSkill(new Chouhai);
    sunhao->addSkill(new Guiming); // in Player::isWounded()

	General *wutugu = new General(this, "wutugu", "qun", 15);
	wutugu->addSkill(new Ranshang);
	wutugu->addSkill(new Hanyong);

	General *zhanglu = new General(this, "zhanglu", "qun", 3);
	zhanglu->addSkill(new Yishe);
	zhanglu->addSkill(new DetachEffectSkill("yishe", "rice"));
	related_skills.insertMulti("yishe", "#yishe-clear");
	zhanglu->addSkill(new Bushi);
	zhanglu->addSkill(new Midao);

    General *shixie = new General(this, "shixie", "qun", 3);
    shixie->addSkill(new Biluan);
    shixie->addSkill(new Lixia);

    General *mayunlu = new General(this, "mayunlu", "shu", 4, false);
    mayunlu->addSkill(new Fengpo);
    mayunlu->addSkill("mashu");

    General *tadun = new General(this, "tadun", "qun");
    tadun->addSkill(new Luanzhan);

    General *yanbaihu = new General(this, "yanbaihu", "qun");
    yanbaihu->addSkill(new Zhidao);
    yanbaihu->addSkill(new Jili);

	General *guansuo = new General(this, "guansuo", "shu");
	guansuo->addSkill(new Zhengnan);
	guansuo->addSkill(new Xiefang);
	guansuo->addRelateSkill("wusheng");
	guansuo->addRelateSkill("dangxian");
	guansuo->addRelateSkill("zhiman");

	General *wanglang = new General(this, "wanglang", "wei", 3);
    wanglang->addSkill(new Gushe);
    wanglang->addSkill(new Jici);

    General *wangji = new General(this, "wangji", "wei", 3);
    wangji->addSkill(new Qizhi);
    wangji->addSkill(new Jinqu);

    General *buzhi = new General(this, "buzhi", "wu", 3);
    buzhi->addSkill(new Hongde);
    buzhi->addSkill(new Dingpan);

    General *litong = new General(this, "litong", "wei");
    litong->addSkill(new Cuifeng);
    litong->addSkill(new CuifengTargetMod);
    litong->addSkill(new DetachEffectSkill("cuifeng", "cuifeng_feng"));
    related_skills.insertMulti("cuifeng", "#cuifeng-target");
    related_skills.insertMulti("cuifeng", "#cuifeng-clear");

    General *mizhu = new General(this, "mizhu", "shu", 3);
    mizhu->addSkill(new Ziyuan);
    mizhu->addSkill(new Jugu);
    mizhu->addSkill(new JuguKeep);
    related_skills.insertMulti("jugu", "#jugu-keep");

    General *dongbai = new General(this, "dongbai", "qun", 3, false);
    dongbai->addSkill(new Lianzhu);
    dongbai->addSkill(new Xiahui);
    dongbai->addSkill(new XiahuiCardLimited);
    related_skills.insertMulti("xiahui", "#xiahui-limited");

    General *zhaoxiang = new General(this, "zhaoxiang", "shu", 4, false);
    zhaoxiang->addSkill(new Fanghun);
    zhaoxiang->addSkill(new Fuhan);

    General *heqi = new General(this, "heqi", "wu");
    heqi->addSkill(new Qizhou);
    heqi->addRelateSkill("mashu");
    heqi->addRelateSkill("nosyingzi");
    heqi->addRelateSkill("duanbing");
    heqi->addRelateSkill("fenwei");
    heqi->addSkill(new Shanxi);

    General *dongyun = new General(this, "dongyun", "shu", 3);
    dongyun->addSkill(new Bingzheng);
    dongyun->addSkill(new Sheyan);

    General *mazhong = new General(this, "mazhong", "shu");
    mazhong->addSkill(new Fuman);

    General *kanze = new General(this, "kanze", "wu", 3);
    kanze->addSkill(new Xiashu);
    kanze->addSkill(new Kuanshi);

    General *wangyun = new General(this, "wangyun", "qun");
    wangyun->addSkill(new Lianji);
    wangyun->addSkill(new LianjiProhibit);
    related_skills.insertMulti("lianji", "#lianji-prohibit");
    wangyun->addSkill(new Moucheng);
    wangyun->addRelateSkill("jingong");

    General *quyi = new General(this, "quyi", "qun");
    quyi->addSkill(new Fuji);
    quyi->addSkill(new Jiaozi);

    General *xizhicai = new General(this, "xizhicai", "wei", 3);
    xizhicai->addSkill("tiandu");
    xizhicai->addSkill(new Xianfu);
    xizhicai->addSkill(new Chouce);

    General *sunqian = new General(this, "sunqian", "shu", 3);
    sunqian->addSkill(new Qianya);
    sunqian->addSkill(new Shuimeng);

    General *beimihu = new General(this, "beimihu", "qun", 3, false);
    beimihu->addSkill(new Zongkui);
    beimihu->addSkill(new Guju);
    beimihu->addSkill(new DetachEffectSkill("guju", QString(), "#guju"));
    related_skills.insertMulti("guju", "#guju-clear");
    beimihu->addSkill(new Baijia);
    beimihu->addRelateSkill("canshib");

    General *liuqi = new General(this, "liuqi", "qun", 3);
    liuqi->addSkill(new Wenji);
    liuqi->addSkill(new Tunjiang);
    liuqi->addSkill(new TunjiangRecord);
    related_skills.insertMulti("tunjiang", "#tunjiang-record");

    General *luzhi = new General(this, "luzhi", "wei", 3);
    luzhi->addSkill(new Qingzhong);
    luzhi->addSkill(new Weijing);

    addMetaObject<AocaiCard>();
    addMetaObject<DuwuCard>();
    addMetaObject<ShefuCard>();
    addMetaObject<LuanzhanCard>();
	addMetaObject<GusheCard>();
    addMetaObject<ZiyuanCard>();
    addMetaObject<DingpanCard>();
    addMetaObject<LianzhuCard>();
    addMetaObject<ShanxiCard>();
    addMetaObject<FumanCard>();
    addMetaObject<LianjiCard>();
    addMetaObject<SmileDagger>();
    addMetaObject<HoneyTrap>();
    addMetaObject<JingongCard>();
    addMetaObject<QianyaCard>();
    addMetaObject<CanshibCard>();
	
    skills << new AocaiVeiw << new ShixieDistance << new FengpoRecord << new LuanzhanColl << new Jingong << new Canshib;
}

ADD_PACKAGE(OL)
