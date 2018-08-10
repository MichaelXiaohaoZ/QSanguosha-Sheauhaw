#include "yjcm2016.h"
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

JiaozhaoCard::JiaozhaoCard()
{
	will_throw = false;
    handling_method = Card::MethodNone;
}

bool JiaozhaoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Self->getMark("danxin_modify") > 1 || !targets.isEmpty()) return false;
    int nearest = 1000;
	foreach (const Player *p, Self->getAliveSiblings()) {
        nearest = qMin(nearest, Self->distanceTo(p));
    }
    return Self->distanceTo(to_select) == nearest;
}

bool JiaozhaoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Self->getMark("danxin_modify") > 1)
        return true;
    else
        return !targets.isEmpty();
}

void JiaozhaoCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    room->showCard(source, getEffectiveId());

	ServerPlayer *target = NULL;
	if (targets.isEmpty())
		target = source;
	else
        target = targets.first();

    QString pattern = "@@jiaozhao_first!";
    if (source->getMark("danxin_modify") > 0)
        pattern = "@@jiaozhao_second!";
    const Card *card = room->askForCard(target, pattern, "@jiaozhao-declare:" + source->objectName(), QVariant(), Card::MethodNone);

    QString card_name = "slash";
    if (card != NULL)
        card_name = card->objectName();

    LogMessage log;
    log.type = "$JiaozhaoDeclare";
    log.arg = card_name;
    log.from = target;
    room->sendLog(log);
    room->setPlayerProperty(source, "jiaozhao_record_id", QString::number(getEffectiveId()));
    room->setPlayerProperty(source, "jiaozhao_record_name", card_name);
    room->setPlayerMark(source, "ViewAsSkill_jiaozhaoEffect", 1);
}

class JiaozhaoViewAsSkill : public OneCardViewAsSkill
{
public:
    JiaozhaoViewAsSkill() : OneCardViewAsSkill("jiaozhao")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
		if (player->hasUsed("JiaozhaoCard")) {
			QString record_id = Self->property("jiaozhao_record_id").toString();
			QString record_name = Self->property("jiaozhao_record_name").toString();
			if (record_id == "" || record_name == "") return false;
            Card *use_card = Sanguosha->cloneCard(record_name);
            if (!use_card) return false;
            use_card->setCanRecast(false);
            use_card->addSubcard(record_id.toInt());
		    use_card->setSkillName("jiaozhao");
			return use_card->isAvailable(player);
		}
        return true;
    }

    virtual bool viewFilter(const Card *card) const
    {
        if (card->isEquipped()) return false;
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !Self->hasUsed("JiaozhaoCard"))
			return true;
	    QString record_id = Self->property("jiaozhao_record_id").toString();
		return record_id != "" && record_id.toInt() == card->getEffectiveId();
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
		if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY && !Self->hasUsed("JiaozhaoCard")) {
		    JiaozhaoCard *jiaozhao = new JiaozhaoCard;
            jiaozhao->addSubcard(originalCard);
            if (Self->getMark("danxin_modify") > 1)
                jiaozhao->setTargetFixed(true);
            return jiaozhao;
		}
		QString record_name = Self->property("jiaozhao_record_name").toString();
	    Card *use_card = Sanguosha->cloneCard(record_name);
		if (!use_card) return NULL;
        use_card->setCanRecast(false);
        use_card->addSubcard(originalCard);
		use_card->setSkillName("jiaozhao");
        return use_card;
	}

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;
		
		if (pattern.startsWith(".") || pattern.startsWith("@"))
			return false;

		QString record_name = player->property("jiaozhao_record_name").toString();
		if (record_name == "") return false;
		QString pattern_names = pattern;
		if (pattern == "slash")
			pattern_names = "slash+fire_slash+thunder_slash";
        else if (pattern == "peach+analeptic")
			return false;
		
		return pattern_names.split("+").contains(record_name);
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->property("jiaozhao_record_name").toString() == "nullification";
    }
};

class Jiaozhao : public TriggerSkill
{
public:
    Jiaozhao() : TriggerSkill("jiaozhao")
    {
        events << CardsMoveOneTime << EventPhaseChanging;
		view_as_skill = new JiaozhaoViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
			if (player != move.from) return false;
			QString record_id = player->property("jiaozhao_record_id").toString();
		    if (record_id == "") return false;
			int id = record_id.toInt();
			if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand) {
                room->setPlayerProperty(player, "jiaozhao_record_id", QString());
			    room->setPlayerProperty(player, "jiaozhao_record_name", QString());
                room->setPlayerMark(player, "ViewAsSkill_jiaozhaoEffect", 0);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
            room->setPlayerProperty(player, "jiaozhao_record_id", QString());
			room->setPlayerProperty(player, "jiaozhao_record_name", QString());
            room->setPlayerMark(player, "ViewAsSkill_jiaozhaoEffect", 0);
        }
        return false;
    }
};

class JiaozhaoProhibit : public ProhibitSkill
{
public:
    JiaozhaoProhibit() : ProhibitSkill("#jiaozhao")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        return (!card->isKindOf("SkillCard") && card->getSkillName() == "jiaozhao" && from && from == to);
    }
};

class JiaozhaoFirst : public OneCardViewAsSkill
{
public:
    JiaozhaoFirst() : OneCardViewAsSkill("jiaozhao_first")
    {
        response_pattern = "@@jiaozhao_first!";
        guhuo_type = "b";
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->isVirtualCard();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return Sanguosha->cloneCard(originalCard->objectName());
    }
};

class JiaozhaoSecond : public OneCardViewAsSkill
{
public:
    JiaozhaoSecond() : OneCardViewAsSkill("jiaozhao_second")
    {
        response_pattern = "@@jiaozhao_second!";
        guhuo_type = "bt";
    }

    bool viewFilter(const Card *to_select) const
    {
        return to_select->isVirtualCard();
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return Sanguosha->cloneCard(originalCard->objectName());
    }
};

class Danxin : public MasochismSkill
{
public:
    Danxin() : MasochismSkill("danxin")
    {
    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &) const
    {
        Room *room = target->getRoom();
        if (target->askForSkillInvoke(this)){
            target->broadcastSkillInvoke(objectName());
            if (!target->hasSkill("jiaozhao", true) || target->getMark("danxin_modify") > 1 || room->askForChoice(target, objectName(), "modify+draw") == "draw")
				target->drawCards(1, objectName());
			else {
				room->addPlayerMark(target, "danxin_modify");
                QString translate = Sanguosha->translate(":jiaozhao");
                int i = target->getMark("danxin_modify");
                translate.replace(Sanguosha->translate("jiaozhao:modify"+QString::number(i)), Sanguosha->translate("jiaozhao:modified"+QString::number(i)));
				Sanguosha->addTranslationEntry(":jiaozhao", translate.toStdString().c_str());
				JsonArray args;
                args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
                room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
			}
		}
    }
};

ZhigeCard::ZhigeCard()
{
}

bool ZhigeCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->inMyAttackRange(Self);
}

void ZhigeCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
	bool use_slash = room->askForUseSlashTo(effect.to, room->getAlivePlayers(), "@zhige-slash:" + effect.from->objectName());
    if (!use_slash && effect.to->hasEquip()) {
		QList<int> equips;
        foreach (const Card *card, effect.to->getEquips())
            equips << card->getEffectiveId();
		room->fillAG(equips, effect.to);
        int to_give = room->askForAG(effect.to, equips, false, "zhige");
        room->clearAG(effect.to);
		CardMoveReason reason(CardMoveReason::S_REASON_GIVE, effect.to->objectName(), effect.from->objectName(), "zhige", QString());
		reason.m_playerId = effect.from->objectName();
        room->moveCardTo(Sanguosha->getCard(to_give), effect.to, effect.from, Player::PlaceHand, reason);
	}
}

class Zhige : public ZeroCardViewAsSkill
{
public:
    Zhige() : ZeroCardViewAsSkill("zhige")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("ZhigeCard") && player->getHandcardNum() > player->getHp();
    }

    virtual const Card *viewAs() const
    {
        return new ZhigeCard;
    }
};

class Zongzuo : public TriggerSkill
{
public:
    Zongzuo() : TriggerSkill("zongzuo")
    {
        events << TurnStart << BuryVictim;
		frequency = Compulsory;
	}

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == TurnStart)
            return 1;
        else if (triggerEvent == BuryVictim)
            return -2;
		return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        if (triggerEvent == TurnStart && room->getTag("FirstRound").toBool()) {
            foreach (ServerPlayer *liuyu, room->getAlivePlayers()) {
                if (!TriggerSkill::triggerable(liuyu)) continue;
                QSet<QString> kingdom_set;
                foreach(ServerPlayer *p, room->getAlivePlayers())
                    kingdom_set << p->getKingdom();

                int n = kingdom_set.size();
                if (n > 0) {
                    room->sendCompulsoryTriggerLog(liuyu, objectName());
                    liuyu->broadcastSkillInvoke(objectName());

                    LogMessage log;
                    log.type = "#GainMaxHp";
                    log.from = liuyu;
                    log.arg = QString::number(n);
                    log.arg2 = QString::number(liuyu->getMaxHp() + n);
                    room->sendLog(log);

                    room->setPlayerProperty(liuyu, "maxhp", liuyu->getMaxHp() + n);
                    room->recover(liuyu, RecoverStruct(liuyu, NULL, n));
                }
            }
		} else if (triggerEvent == BuryVictim) {
			foreach (ServerPlayer *liuyu, room->getAlivePlayers()) {
                if (!TriggerSkill::triggerable(liuyu)) continue;
				foreach(ServerPlayer *p, room->getAlivePlayers()) {
					if (p->getKingdom() == player->getKingdom())
						return false;
				}
				room->sendCompulsoryTriggerLog(liuyu, objectName());
                liuyu->broadcastSkillInvoke(objectName());
				room->loseMaxHp(liuyu);
		    }
		}
        return false;
    }
};

DuliangCard::DuliangCard()
{

}

bool DuliangCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void DuliangCard::onEffect(const CardEffectStruct &effect) const
{
    Room *room = effect.from->getRoom();
	if (effect.to->isKongcheng()) return;
	int card_id = room->askForCardChosen(effect.from, effect.to, "h", "duliang");
    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
    if (room->askForChoice(effect.from, "duliang", "send+delay", QVariant(), "@duliang-choose::" + effect.to->objectName()) == "delay") {
        room->addPlayerMark(effect.to, "#duliang");
    }
    else {
        QList<int> ids = room->getNCards(2, false);
		LogMessage log;
        log.type = "$ViewDrawPile";
        log.from = effect.to;
        log.card_str = IntList2StringList(ids).join("+");
        room->sendLog(log, effect.to);
		room->fillAG(ids, effect.to);
        room->getThread()->delay(2000);
        room->clearAG(effect.to);
		room->returnToTopDrawPile(ids);
		QList<int> to_obtain;
        foreach (int card_id, ids)
			if (Sanguosha->getCard(card_id)->getTypeId() == Card::TypeBasic)
				to_obtain << card_id;
		if (!to_obtain.isEmpty()) {
			DummyCard *dummy = new DummyCard(to_obtain);
            effect.to->obtainCard(dummy, false);
			delete dummy;
		}
	}
}

class DuliangViewAsSkill : public ZeroCardViewAsSkill
{
public:
    DuliangViewAsSkill() : ZeroCardViewAsSkill("duliang")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("DuliangCard");
    }

    virtual const Card *viewAs() const
    {
        return new DuliangCard;
    }
};

class Duliang : public TriggerSkill
{
public:
    Duliang() : TriggerSkill("duliang")
    {
        events << DrawNCards << EventPhaseEnd;
		view_as_skill = new DuliangViewAsSkill;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
		if (triggerEvent == DrawNCards)
            return 6;
		return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && target->getMark("#duliang") > 0;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *target, QVariant &data) const
    {
        if (triggerEvent == DrawNCards)
			data = data.toInt() + target->getMark("#duliang");
		else if (target->getPhase() == Player::Draw)
			room->setPlayerMark(target, "#duliang", 0);
		return false;
    }
};

class FulinDiscard : public ViewAsSkill
{
public:
    FulinDiscard() : ViewAsSkill("fulin_discard")
    {
        response_pattern = "@@fulin_discard!";
    }

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
		QStringList fulin = Self->property("fulin").toString().split("+");
        foreach (QString id, fulin) {
            bool ok;
            if (id.toInt(&ok) == to_select->getEffectiveId() && ok)
                return false;
        }
        return !Self->isJilei(to_select) && !to_select->isEquipped() && selected.length() < Self->getMark("fulin_discard");
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() == Self->getMark("fulin_discard")) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }

        return NULL;
    }
};

class Fulin : public TriggerSkill
{
public:
    Fulin() : TriggerSkill("fulin")
    {
        events << EventPhaseProceeding << CardsMoveOneTime << EventPhaseChanging;
		frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseProceeding && player->getPhase() == Player::Discard && TriggerSkill::triggerable(player)){
			if (player->property("fulin").toString() == "") return false;
			QStringList fulin_list = player->property("fulin").toString().split("+");
			int num = 0;
			foreach (const Card *card, player->getHandcards())
                if (!fulin_list.contains(QString::number(card->getEffectiveId())))
					num++;
            int discard_num = num - player->getMaxCards();
			if (player->getHandcardNum() > player->getMaxCards() && player->getHandcardNum() > num){
				room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
			}
			if (discard_num > 0) {
				QList<int> default_ids;
			    bool will_ask = false;
				foreach (const Card *card, player->getHandcards()) {
                    if (!fulin_list.contains(QString::number(card->getEffectiveId())) && !player->isJilei(card)){
					    if (default_ids.length() < discard_num)
						    default_ids << card->getEffectiveId();
						else {
							will_ask = true;
							break;
						}
				    }
                }
				const Card *card = NULL;
				if (will_ask) {
					room->setPlayerMark(player, "fulin_discard", discard_num);
				    card = room->askForCard(player, "@@fulin_discard!", "@fulin-discard:::" + QString::number(discard_num), data, Card::MethodNone);
				    room->setPlayerMark(player, "fulin_discard", 0);
				}
				if (card == NULL || card->subcardsLength() != discard_num)
					card = new DummyCard(default_ids);
                CardMoveReason mreason(CardMoveReason::S_REASON_THROW, player->objectName(), QString(), "gamerule", QString());
                room->throwCard(card, mreason, player);
			}
			room->setTag("SkipGameRule", true);
		} else if (triggerEvent == CardsMoveOneTime && player->getPhase() != Player::NotActive) {
			CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
			if (!room->getTag("FirstRound").toBool() && move.to == player && move.to_place == Player::PlaceHand) {
                QStringList fulin_list;
				if (player->property("fulin").toString() != "")
					fulin_list = player->property("fulin").toString().split("+");
                fulin_list << IntList2StringList(move.card_ids);
                room->setPlayerProperty(player, "fulin", fulin_list.join("+"));
			} else if (move.from == player && move.from_places.contains(Player::PlaceHand) && player->property("fulin").toString() != "") {
				QStringList fulin_list = player->property("fulin").toString().split("+");
				QStringList copy_list = fulin_list;
				foreach (QString id_str, fulin_list) {
					int id = id_str.toInt();
					if (move.card_ids.contains(id) && move.from_places[move.card_ids.indexOf(id)] == Player::PlaceHand)
						copy_list.removeOne(id_str);
				}
				room->setPlayerProperty(player, "fulin", copy_list.join("+"));
			}
		} else if (triggerEvent == EventPhaseChanging) {
			PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to != Player::NotActive)
                return false;
			room->setPlayerProperty(player, "fulin", QString());
		}
        return false;
    }
};

KuangbiCard::KuangbiCard()
{
}

bool KuangbiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !to_select->isNude() && to_select != Self;
}

void KuangbiCard::onEffect(const CardEffectStruct &effect) const
{
    if (effect.to->isNude()) return;
	const Card *card = effect.from->getRoom()->askForExchange(effect.to, "kuangbi", 3, 1, true, "@kuangbi-put:" + effect.from->objectName(), false);
    effect.from->addToPile("kuangbi", card->getSubcards(), false);
	effect.from->tag["KuangbiTarget"] = QVariant::fromValue(effect.to);
}

class KuangbiViewAsSkill : public ZeroCardViewAsSkill
{
public:
    KuangbiViewAsSkill() : ZeroCardViewAsSkill("kuangbi")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasUsed("KuangbiCard");
    }

    virtual const Card *viewAs() const
    {
        return new KuangbiCard;
    }
};

class Kuangbi : public PhaseChangeSkill
{
public:
    Kuangbi() : PhaseChangeSkill("kuangbi")
    {
        view_as_skill = new KuangbiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target && !target->getPile("kuangbi").isEmpty() && target->getPhase() == Player::RoundStart;
    }

    virtual bool onPhaseChange(ServerPlayer *sundeng) const
    {
        Room *room = sundeng->getRoom();

		LogMessage log;
        log.type = "#SkillEffected";
        log.from = sundeng;
	    log.arg = objectName();
        room->sendLog(log);
        int n = sundeng->getPile("kuangbi").length();
		DummyCard *dummy = new DummyCard(sundeng->getPile("kuangbi"));
        CardMoveReason reason(CardMoveReason::S_REASON_EXCHANGE_FROM_PILE, sundeng->objectName(), "kuangbi", QString());
        room->obtainCard(sundeng, dummy, reason, false);
        delete dummy;
		ServerPlayer *target = sundeng->tag["KuangbiTarget"].value<ServerPlayer *>();
		sundeng->tag.remove("KuangbiTarget");
		if (target)
			target->drawCards(n);

        return false;
    }
};

JisheCard::JisheCard()
{
    target_fixed = true;
}

void JisheCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    source->drawCards(1, "jishe");
	room->addPlayerMark(source, "JisheTimes");
}

JisheChainCard::JisheChainCard()
{
    handling_method = Card::MethodNone;
    m_skillName = "jishe";
}

bool JisheChainCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.length() < Self->getHp() && !to_select->isChained();
}

void JisheChainCard::onEffect(const CardEffectStruct &effect) const
{
    if (!effect.to->isChained())
        effect.to->getRoom()->setPlayerProperty(effect.to, "chained", true);
}

class JisheViewAsSkill : public ZeroCardViewAsSkill
{
public:
    JisheViewAsSkill() : ZeroCardViewAsSkill("jishe")
    {
    }

    virtual const Card *viewAs() const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern() == "@@jishe")
            return new JisheChainCard;
        else
            return new JisheCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMaxCards() > 0;
    }

    bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern == "@@jishe";
    }
};

class Jishe : public TriggerSkill
{
public:
    Jishe() : TriggerSkill("jishe")
    {
        events << EventPhaseChanging << EventPhaseStart << PlayCard;
        view_as_skill = new JisheViewAsSkill;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		if (triggerEvent == EventPhaseChanging) {
			if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            room->setPlayerMark(player, "JisheTimes", 0);
			room->setPlayerMark(player, "#jishe", 0);
		} else if (triggerEvent == EventPhaseStart && TriggerSkill::triggerable(player) && player->getPhase() == Player::Finish && player->isKongcheng()) {
			QList<ServerPlayer *> targets;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (!p->isChained())
                    targets << p;
            }
			int x = qMin(targets.length(), player->getHp());
			if (x > 0)
                room->askForUseCard(player, "@@jishe", "@jishe-chain:::" + QString::number(x), QVariant(), Card::MethodNone);
        } else if (triggerEvent == PlayCard && TriggerSkill::triggerable(player))
            room->setPlayerMark(player, "#jishe", player->getMaxCards());
        return false;
    }

    int getEffectIndex(const ServerPlayer *player, const Card *card) const
    {
        Room *room = player->getRoom();
        if (card->isKindOf("JisheCard")) {
            QString tag_name = QString("AudioEffect:cenhun-jishe=1+2");
            int index = room->getTag(tag_name).toInt();
            if (index == 1 || index == 2)
                index = 3 - index;
            else
                index = qrand() % 2 + 1;
            room->setTag(tag_name, index);
            return index;
        } else if (card->isKindOf("JisheChainCard")) {
            QString tag_name = QString("AudioEffect:cenhun-jishe=3+4");
            int index = room->getTag(tag_name).toInt();
            if (index == 3 || index == 4)
                index = 7 - index;
            else
                index = qrand() % 2 + 3;
            room->setTag(tag_name, index);
            return index;
        }
        return -1;
    }
};

class JisheMaxcards : public MaxCardsSkill
{
public:
    JisheMaxcards() : MaxCardsSkill("#jishe")
    {
    }

    virtual int getExtra(const Player *target) const
    {
        return - target->getMark("JisheTimes");
    }
};

class Lianhuo : public TriggerSkill
{
public:
    Lianhuo() : TriggerSkill("lianhuo")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
		DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature == DamageStruct::Fire && player->isChained() && !damage.chain) {
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());

            ++damage.damage;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

QinqingCard::QinqingCard()
{
}

bool QinqingCard::targetFilter(const QList<const Player *> &, const Player *to_select, const Player *Self) const
{
    if (Self->isLord())
        return (to_select->inMyAttackRange(Self));
    foreach(const Player *lord, Self->getAliveSiblings()) {
        if (lord->isLord())
            return (to_select->inMyAttackRange(lord));
    }
    return false;
}

void QinqingCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    foreach (ServerPlayer *target, targets) {
        if (source->canDiscard(target, "he")) {
            int id = room->askForCardChosen(source, target, "he", "qinqing", false, Card::MethodDiscard);
            room->throwCard(id, target, source);
        }
        if (target->isAlive())
            target->drawCards(1, "qinqing");
    }
    ServerPlayer *lord = room->getLord();
    if (lord == NULL) return;
    int n = 0;
    foreach(ServerPlayer *p, targets)
        if (p->getHandcardNum() > lord->getHandcardNum())
            n++;
    if (n > 0)
        source->drawCards(n, "qinqing");
}

class QinqingViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QinqingViewAsSkill() : ZeroCardViewAsSkill("qinqing")
    {
        response_pattern = "@@qinqing";
    }

    virtual const Card *viewAs() const
    {
        return new QinqingCard;
    }
};

class Qinqing : public PhaseChangeSkill
{
public:
    Qinqing() : PhaseChangeSkill("qinqing")
    {
        view_as_skill = new QinqingViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *huanghao) const
    {
        Room *room = huanghao->getRoom();
		if (!isNormalGameMode(room->getMode()) && room->getMode() != "08_zdyj") return false;
		if (huanghao->getPhase() != Player::Finish) return false;
		ServerPlayer *lord = room->getLord();
        if (lord == NULL) return false;
        room->askForUseCard(huanghao, "@@qinqing", "@qinqing", QVariant(), Card::MethodNone);
        return false;
    }
};

class HuishengViewAsSkill : public ViewAsSkill
{
public:
    HuishengViewAsSkill() : ViewAsSkill("huisheng")
    {
        response_pattern = "@@huisheng";
    }

    bool viewFilter(const QList<const Card *> &, const Card *) const
    {
        return true;
    }

    const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (cards.length() > 0) {
            DummyCard *xt = new DummyCard;
            xt->addSubcards(cards);
            return xt;
        }

        return NULL;
    }
};

class HuishengObtain : public OneCardViewAsSkill
{
public:
    HuishengObtain() : OneCardViewAsSkill("huisheng_obtain")
    {
        expand_pile = "#huisheng";
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@huisheng_obtain");
    }

    bool viewFilter(const Card *to_select) const
    {
        return Self->getPile("#huisheng").contains(to_select->getEffectiveId());
    }

    const Card *viewAs(const Card *originalCard) const
    {
        return originalCard;
    }
};

class Huisheng : public TriggerSkill
{
public:
    Huisheng() : TriggerSkill("huisheng")
    {
        events << DamageInflicted;
        view_as_skill = new HuishengViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *huanghao, QVariant &data) const
    {
		ServerPlayer *target = data.value<DamageStruct>().from;
		if (huanghao->isNude() || !target || target->isDead() || target == huanghao || target->getMark("huisheng" + huanghao->objectName()) > 0) return false;
		const Card *card = room->askForCard(huanghao, "@@huisheng", "@huisheng-show::" + target->objectName(), data, Card::MethodNone);
		if (card){
			LogMessage log;
            log.type = "#InvokeSkill";
            log.arg = objectName();
            log.from = huanghao;
            room->sendLog(log);
            room->notifySkillInvoked(huanghao, objectName());
            huanghao->broadcastSkillInvoke(objectName());
			room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, huanghao->objectName(), target->objectName());
            QList<int> to_show = card->getSubcards();
			int n = card->subcardsLength();
			room->notifyMoveToPile(target, to_show, "huisheng", Player::PlaceTable, true, true);
			QString prompt = "@huisheng-obtain:" + huanghao->objectName() + "::" + QString::number(n);
			QString pattern = "@@huisheng_obtain";
			bool optional = true;
			if (target->forceToDiscard(n, true).length() < n) {
				optional = false;
                pattern = "@@huisheng_obtain!";
			}
			const Card *to_obtain = room->askForCard(target, pattern, prompt, data, Card::MethodNone);
			room->notifyMoveToPile(target, to_show, "huisheng", Player::PlaceTable, false, false);
			if (to_obtain) {
				target->obtainCard(to_obtain, false);
                room->addPlayerMark(target, "huisheng" + huanghao->objectName());
                return true;
			} else if (optional)
				room->askForDiscard(target, "huisheng", n, n, false, true);
			else {
                room->addPlayerMark(target, "huisheng" + huanghao->objectName());
                target->obtainCard(Sanguosha->getCard(to_show.first()), false);
                return true;
			}
		}
        return false;
    }
};

class Guizao : public TriggerSkill
{
public:
    Guizao() : TriggerSkill("guizao")
    {
        events << CardsMoveOneTime << EventPhaseEnd << EventPhaseChanging;
        view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == CardsMoveOneTime && player->getPhase() == Player::Discard) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QVariantList guizaoRecord = player->tag["GuizaoRecord"].toList();
            if (move.from == player && (move.reason.m_reason & CardMoveReason::S_MASK_BASIC_REASON) == CardMoveReason::S_REASON_DISCARD) {
                foreach (int card_id, move.card_ids) {
                    guizaoRecord << card_id;
                }
            }
            player->tag["GuizaoRecord"] = guizaoRecord;
        } else if (triggerEvent == EventPhaseEnd && player->getPhase() == Player::Discard && TriggerSkill::triggerable(player)) {
            QVariantList guizaoRecord = player->tag["GuizaoRecord"].toList();
			if (guizaoRecord.length() < 2) return false;
			QStringList suitlist;
			foreach (QVariant card_data, guizaoRecord) {
                int card_id = card_data.toInt();
                const Card *card = Sanguosha->getCard(card_id);
                QString suit = card->getSuitString();
                if (!suitlist.contains(suit))
                    suitlist << suit;
                else{
					return false;
				}
            }
            QStringList choices;
            if (player->isWounded())
                choices << "recover";
            choices << "draw" << "cancel";
            QString choice = room->askForChoice(player, objectName(), choices.join("+"), data, QString(), "recover+draw+cancel");
            if (choice != "cancel") {
                LogMessage log;
                log.type = "#InvokeSkill";
                log.from = player;
                 log.arg = objectName();
                room->sendLog(log);

                room->notifySkillInvoked(player, objectName());
                player->broadcastSkillInvoke(objectName());
                if (choice == "recover")
                    room->recover(player, RecoverStruct(player));
                else
                    player->drawCards(1, objectName());
            }
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::Discard) {
                player->tag.remove("GuizaoRecord");
            }
        }

        return false;
    }
};

JiyuCard::JiyuCard()
{
    
}

bool JiyuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && !Self->hasFlag("jiyu" + to_select->objectName()) && !to_select->isKongcheng();
}

void JiyuCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *source = effect.from;
	ServerPlayer *target = effect.to;
	Room *room = source->getRoom();
	room->setPlayerFlag(source, "jiyu" + target->objectName());
	if (target->canDiscard(target, "h")) {
		const Card *c = room->askForCard(target, ".!", "@jiyu-discard:" + source->objectName());
        if (c == NULL) {
            c = target->getCards("h").at(0);
            room->throwCard(c, target);
        }
		room->setPlayerCardLimitation(source, "use", QString(".|%1|.|.").arg(c->getSuitString()), true);
		if (c->getSuit() == Card::Spade) {
			source->turnOver();
        	room->loseHp(target);
		}
	}
}

class Jiyu : public ZeroCardViewAsSkill
{
public:
    Jiyu() : ZeroCardViewAsSkill("jiyu")
    {
    }

    virtual const Card *viewAs() const
    {
        return new JiyuCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
		foreach (const Card *card, player->getHandcards()) {
            if (card->isAvailable(player))
                return true;
        }
        return false;
    }
};

TaoluanCard::TaoluanCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "taoluan";
}

bool TaoluanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->addSubcard(this);
    card->setSkillName("taoluan");
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool TaoluanCard::targetFixed() const
{
    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->addSubcard(this);
    card->setSkillName("taoluan");
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool TaoluanCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->addSubcard(this);
    card->setSkillName("taoluan");
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *TaoluanCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *zhangrang = card_use.from;
    Room *room = zhangrang->getRoom();

    Card *c = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);

    QString classname;
    if (c->isKindOf("Slash"))
        classname = "Slash";
    else
        classname = c->getClassName();

    room->setPlayerMark(zhangrang, "Taoluan_" + classname, 1);

    QStringList taoluanList = zhangrang->tag.value("taoluanClassName").toStringList();
    taoluanList << classname;
    zhangrang->tag["taoluanClassName"] = taoluanList;

    c->addSubcard(this);
    c->setSkillName("taoluan");
    c->deleteLater();
    return c;
}

const Card *TaoluanCard::validateInResponse(ServerPlayer *zhangrang) const
{
    Room *room = zhangrang->getRoom();

    Card *c = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);

    QString classname;
    if (c->isKindOf("Slash"))
        classname = "Slash";
    else
        classname = c->getClassName();

    room->setPlayerMark(zhangrang, "Taoluan_" + classname, 1);

    QStringList taoluanList = zhangrang->tag.value("taoluanClassName").toStringList();
    taoluanList << classname;
    zhangrang->tag["taoluanClassName"] = taoluanList;

    c->addSubcard(this);
    c->setSkillName("taoluan");
    c->deleteLater();
    return c;

}

class TaoluanVS : public OneCardViewAsSkill
{
public:
    TaoluanVS() : OneCardViewAsSkill("taoluan")
    {
        filter_pattern = ".";
        response_or_use = true;
    }

    const Card *viewAs(const Card *originalCard) const
    {
        TaoluanCard *skill_card = new TaoluanCard;
        skill_card->addSubcard(originalCard);
        return skill_card;
    }

    bool isEnabledAtPlay(const Player *player) const
    {
        return !player->hasFlag("TaoluanInvalid");
    }

    bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUseReason() != CardUseStruct::CARD_USE_REASON_RESPONSE_USE)
            return false;

        if (player->hasFlag("Global_Dying") || player->hasFlag("TaoluanInvalid"))
            return false;
        foreach(const Player *sib, player->getAliveSiblings()) {
            if (sib->hasFlag("Global_Dying"))
                return false;
        }

#define TAOLUAN_CAN_USE(x) (player->getMark("Taoluan_" #x) == 0)

        if (pattern == "slash")
            return TAOLUAN_CAN_USE(Slash);
        else if (pattern == "peach")
            return TAOLUAN_CAN_USE(Peach);
        else if (pattern.contains("analeptic"))
            return TAOLUAN_CAN_USE(Peach) || TAOLUAN_CAN_USE(Analeptic);
        else if (pattern == "jink")
            return TAOLUAN_CAN_USE(Jink);
		else if (pattern == "nullification")
            return TAOLUAN_CAN_USE(Nullification);

        return false;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        if (player->hasFlag("Global_Dying") || player->hasFlag("TaoluanInvalid"))
            return false;
        foreach(const Player *sib, player->getAliveSiblings()) {
            if (sib->hasFlag("Global_Dying"))
                return false;
        }
        if (player->isNude() && player->getHandPile().isEmpty()) return false;
        return TAOLUAN_CAN_USE(Nullification);

#undef TAOLUAN_CAN_USE

    }
};

class Taoluan : public TriggerSkill
{
public:
    Taoluan() : TriggerSkill("taoluan")
    {
        view_as_skill = new TaoluanVS;
        events << CardFinished << EventPhaseChanging;
    }

    QString getSelectBox() const
    {
        return "guhuo_bt";
    }

    bool buttonEnabled(const QString &button_name, const QList<const Card *> &, const QList<const Player *> &) const
    {
        if (button_name.isEmpty())
            return true;
        Card *card = Sanguosha->cloneCard(button_name, Card::NoSuit, 0);
        if (card == NULL)
            return false;
        card->setSkillName("taoluan");
        QString classname = card->getClassName();
        if (card->isKindOf("Slash"))
            classname = "Slash";
        if (Self->getMark("Taoluan_" + classname) > 0)
            return false;
        return Skill::buttonEnabled(button_name);
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangrang, QVariant &data) const
    {
        if (triggerEvent == CardFinished) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->getSkillName() != objectName()) return false;
            ServerPlayer *target = room->askForPlayerChosen(zhangrang, room->getOtherPlayers(zhangrang), objectName(), "@taoluan-choose");
            QString type_name[4] = { QString(), "BasicCard", "TrickCard", "EquipCard" };
            QStringList types;
            types << "BasicCard" << "TrickCard" << "EquipCard";
            types.removeOne(type_name[use.card->getTypeId()]);
            const Card *card = room->askForCard(target, types.join(",") + "|.|.|.",
                    "@taoluan-give:" + zhangrang->objectName() + "::" + use.card->getType(), data, Card::MethodNone);
            if (card) {
                CardMoveReason reason(CardMoveReason::S_REASON_GIVE, target->objectName(), zhangrang->objectName(), objectName(), QString());
                reason.m_playerId = zhangrang->objectName();
                room->moveCardTo(card, target, zhangrang, Player::PlaceHand, reason);
                delete card;
            } else {
                room->loseHp(zhangrang);
                room->setPlayerFlag(zhangrang, "TaoluanInvalid");
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            if (change.to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAlivePlayers()) {
                    if (p->hasFlag("TaoluanInvalid"))
                        room->setPlayerFlag(p, "-TaoluanInvalid");
                }
            }
        }
        return false;
    }
};

YJCM2016Package::YJCM2016Package()
: Package("YJCM2016")
{
    General *cenhun = new General(this, "cenhun", "wu", 3);
	cenhun->addSkill(new Jishe);
	cenhun->addSkill(new JisheMaxcards);
	related_skills.insertMulti("jishe", "#jishe");
	cenhun->addSkill(new Lianhuo);

	General *guohuanghou = new General(this, "guohuanghou", "wei", 3, false);
    guohuanghou->addSkill(new Jiaozhao);
	guohuanghou->addSkill(new JiaozhaoProhibit);
	related_skills.insertMulti("jiaozhao", "#jiaozhao");
    guohuanghou->addSkill(new Danxin);

	General *huanghao = new General(this, "huanghao", "shu", 3);
	huanghao->addSkill(new Qinqing);
	huanghao->addSkill(new Huisheng);

	General *liyan = new General(this, "liyan", "shu", 3);
	liyan->addSkill(new Duliang);
	liyan->addSkill(new Fulin);

	General *liuyu = new General(this, "liuyu", "qun", 2);
	liuyu->addSkill(new Zhige);
	liuyu->addSkill(new Zongzuo);

	General *sundeng = new General(this, "sundeng", "wu");
	sundeng->addSkill(new Kuangbi);
    sundeng->addSkill(new DetachEffectSkill("kuangbi", "kuangbi"));
    related_skills.insertMulti("kuangbi", "#kuangbi-clear");

	General *sunziliufang = new General(this, "sunziliufang", "wei", 3);
	sunziliufang->addSkill(new Guizao);
	sunziliufang->addSkill(new Jiyu);

	General *zhangrang = new General(this, "zhangrang", "qun", 3);
	zhangrang->addSkill(new Taoluan);

	addMetaObject<JiaozhaoCard>();
	addMetaObject<ZhigeCard>();
    addMetaObject<DuliangCard>();
	addMetaObject<KuangbiCard>();
	addMetaObject<JisheCard>();
	addMetaObject<JisheChainCard>();
    addMetaObject<QinqingCard>();
	addMetaObject<JiyuCard>();
	addMetaObject<TaoluanCard>();

    skills << new FulinDiscard << new HuishengObtain << new JiaozhaoFirst << new JiaozhaoSecond;
}

ADD_PACKAGE(YJCM2016)
