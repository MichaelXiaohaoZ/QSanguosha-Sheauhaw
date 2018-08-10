#include "settings.h"
#include "standard.h"
#include "skill.h"
#include "client.h"
#include "engine.h"
#include "ai.h"
#include "general.h"
#include "wind.h"

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

class Guidao : public TriggerSkill
{
public:
    Guidao() : TriggerSkill("guidao")
    {
        events << AskForRetrial;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        if (!TriggerSkill::triggerable(target))
            return false;

        if (target->isKongcheng()) {
            bool has_black = false;
            for (int i = 0; i < 4; i++) {
                const EquipCard *equip = target->getEquip(i);
                if (equip && equip->isBlack()) {
                    has_black = true;
                    break;
                }
            }
            return has_black;
        } else
            return true;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        QStringList prompt_list;
        prompt_list << "@guidao-card" << judge->who->objectName()
            << objectName() << judge->reason << QString::number(judge->card->getEffectiveId());
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, ".|black", prompt, data, Card::MethodResponse, judge->who, true, "guidao");

        if (card != NULL) {
            player->broadcastSkillInvoke(objectName());
			room->notifySkillInvoked(player, objectName());
            room->retrial(card, player, judge, objectName(), true);
        }
        return false;
    }
};

class Leiji : public TriggerSkill
{
public:
    Leiji() : TriggerSkill("leiji")
    {
        events << CardResponded << FinishJudge;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangjiao, QVariant &data) const
    {
        if (triggerEvent == CardResponded && TriggerSkill::triggerable(zhangjiao)) {
            const Card *card_star = data.value<CardResponseStruct>().m_card;
            if (card_star->isKindOf("Jink")) {
                ServerPlayer *target = room->askForPlayerChosen(zhangjiao, room->getOtherPlayers(zhangjiao), objectName(), "leiji-invoke", true, true);
                if (target) {
                    zhangjiao->broadcastSkillInvoke(objectName());

                    JudgeStruct judge;
                    judge.pattern = ".|black";
                    judge.good = false;
                    judge.negative = true;
                    judge.reason = objectName();
                    judge.who = target;

                    room->judge(judge);

                    if (judge.isBad()){
                        int n = 2;
                        if ((Card::Suit)(judge.pattern.toInt()) == Card::Club){
                            n = 1;
                            room->recover(zhangjiao, RecoverStruct(zhangjiao));
                        }
                        room->damage(DamageStruct(objectName(), zhangjiao, target, n, DamageStruct::Thunder));
                    }
                }
            }
        } else if (triggerEvent == FinishJudge) {
            JudgeStruct *judge = data.value<JudgeStruct *>();
            if (judge->reason != objectName()) return false;
            judge->pattern = QString::number(int(judge->card->getSuit()));
        }
        return false;
    }
};

HuangtianCard::HuangtianCard()
{
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "huangtian_attach";
    mute = true;
}

void HuangtianCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const
{
    ServerPlayer *zhangjiao = targets.first();
    room->setPlayerFlag(zhangjiao, "HuangtianInvoked");
    LogMessage log;
    log.type = "#InvokeOthersSkill";
    log.from = source;
    log.to << zhangjiao;
    log.arg = "huangtian";
    room->sendLog(log);
    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, source->objectName(), zhangjiao->objectName());
    zhangjiao->broadcastSkillInvoke("huangtian");

    room->notifySkillInvoked(zhangjiao, "huangtian");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), zhangjiao->objectName(), "huangtian", QString());
    room->obtainCard(zhangjiao, this, reason);

}

bool HuangtianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    return targets.isEmpty() && to_select->hasLordSkill("huangtian")
        && to_select != Self && !to_select->hasFlag("HuangtianInvoked");
}

class HuangtianViewAsSkill : public OneCardViewAsSkill
{
public:
    HuangtianViewAsSkill() :OneCardViewAsSkill("huangtian_attach")
    {
        attached_lord_skill = true;
        filter_pattern = "Jink,Lightning";
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        if (!shouldBeVisible(player) || player->isKongcheng()) return false;
        foreach(const Player *sib, player->getAliveSiblings())
            if (sib->hasLordSkill("huangtian") && !sib->hasFlag("HuangtianInvoked"))
                return true;
        return false;
    }

    virtual bool shouldBeVisible(const Player *Self) const
    {
        return Self && Self->getKingdom() == "qun";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        HuangtianCard *card = new HuangtianCard;
        card->addSubcard(originalCard);

        return card;
    }
};

class Huangtian : public TriggerSkill
{
public:
    Huangtian() : TriggerSkill("huangtian$")
    {
        events << GameStart << EventAcquireSkill << EventLoseSkill << EventPhaseChanging;
		view_as_skill = new dummyVS;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if ((triggerEvent == GameStart && player->isLord())
            || (triggerEvent == EventAcquireSkill && data.toString() == "huangtian")) {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.isEmpty()) return false;

            QList<ServerPlayer *> players;
            if (lords.length() > 1)
                players = room->getAlivePlayers();
            else
                players = room->getOtherPlayers(lords.first());
            foreach (ServerPlayer *p, players) {
                if (!p->hasSkill("huangtian_attach"))
                    room->attachSkillToPlayer(p, "huangtian_attach");
            }
        } else if (triggerEvent == EventLoseSkill && data.toString() == "huangtian") {
            QList<ServerPlayer *> lords;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->hasLordSkill(this))
                    lords << p;
            }
            if (lords.length() > 2) return false;

            QList<ServerPlayer *> players;
            if (lords.isEmpty())
                players = room->getAlivePlayers();
            else
                players << lords.first();
            foreach (ServerPlayer *p, players) {
                if (p->hasSkill("huangtian_attach"))
                    room->detachSkillFromPlayer(p, "huangtian_attach", true);
            }
        } else if (triggerEvent == EventPhaseChanging) {
            PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();
            if (phase_change.from != Player::Play)
                return false;
            QList<ServerPlayer *> players = room->getOtherPlayers(player);
            foreach (ServerPlayer *p, players) {
                if (p->hasFlag("HuangtianInvoked"))
                    room->setPlayerFlag(p, "-HuangtianInvoked");
            }
        }
        return false;
    }
};

ShensuCard::ShensuCard()
{
}

bool ShensuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    Slash *slash = new Slash(NoSuit, 0);
    slash->setSkillName("shensu");
    slash->setFlags("Global_NoDistanceChecking");
    slash->addCostcards(this->getSubcards());
    slash->deleteLater();
    return targets.isEmpty() && slash->targetFilter(targets, to_select, Self);
}

void ShensuCard::use(Room *room, ServerPlayer *xiahouyuan, QList<ServerPlayer *> &targets) const
{
    if (targets.isEmpty()) return;
    switch (xiahouyuan->getMark("ShensuIndex")) {
    case 1: {
        xiahouyuan->skip(Player::Judge, true);
        xiahouyuan->skip(Player::Draw, true);
        break;
    }
    case 2: {
        xiahouyuan->skip(Player::Play, true);
        break;
    }
    case 3: {
        xiahouyuan->skip(Player::Discard, true);
        xiahouyuan->turnOver();
        break;
    }
    default:
        break;
    }
    Slash *slash = new Slash(Card::NoSuit, 0);
    slash->setSkillName("_shensu");
    room->useCard(CardUseStruct(slash, xiahouyuan, targets));
}

class ShensuViewAsSkill : public ViewAsSkill
{
public:
    ShensuViewAsSkill() : ViewAsSkill("shensu")
    {
    }

    virtual bool isEnabledAtPlay(const Player *) const
    {
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const
    {
        return pattern.startsWith("@@shensu");
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("2"))
            return selected.isEmpty() && to_select->isKindOf("EquipCard") && !Self->isJilei(to_select);
        else
            return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const
    {
        if (Sanguosha->currentRoomState()->getCurrentCardUsePattern().endsWith("2")) {
            if (cards.length() != 1) return NULL;
            ShensuCard *card = new ShensuCard;
            card->addSubcards(cards);
            return card;
        } else
            return cards.isEmpty() ? new ShensuCard : NULL;
    }
};

class Shensu : public TriggerSkill
{
public:
    Shensu() : TriggerSkill("shensu")
    {
        events << EventPhaseChanging;
        view_as_skill = new ShensuViewAsSkill;
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *xiahouyuan, QVariant &data) const
    {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        int index = 0;
        if (change.to == Player::Judge && !xiahouyuan->isSkipped(Player::Judge)
                && !xiahouyuan->isSkipped(Player::Draw) && Slash::IsAvailable(xiahouyuan))
            index = 1;
        else if (Slash::IsAvailable(xiahouyuan) && change.to == Player::Play && !xiahouyuan->isSkipped(Player::Play)
                && xiahouyuan->canDiscard(xiahouyuan, "he"))
            index = 2;
        else if (change.to == Player::Discard && !xiahouyuan->isSkipped(Player::Discard) && Slash::IsAvailable(xiahouyuan))
            index = 3;
        else
            return false;
        room->setPlayerMark(xiahouyuan, "ShensuIndex", index);
        room->askForUseCard(xiahouyuan, "@@shensu"+QString::number(index), "@shensu"+QString::number(index));
        room->setPlayerMark(xiahouyuan, "ShensuIndex", 0);
        return false;
    }
};

class JushouSelect : public OneCardViewAsSkill
{
public:
    JushouSelect() : OneCardViewAsSkill("jushou_select")
    {
        response_pattern = "@@jushou_select!";
    }

    bool viewFilter(const Card *to_select) const
    {
        if (to_select->isEquipped()) return false;
        if (to_select->getTypeId() == Card::TypeEquip)
            return to_select->isAvailable(Self);
        return !Self->isJilei(to_select);
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        DummyCard *select = new DummyCard;
        select->addSubcard(originalCard);
        return select;
    }
};

class Jushou : public PhaseChangeSkill
{
public:
    Jushou() : PhaseChangeSkill("jushou")
    {

    }

    virtual bool onPhaseChange(ServerPlayer *caoren) const
    {
        if (caoren->getPhase() == Player::Finish) {
            Room *room = caoren->getRoom();
            if (room->askForSkillInvoke(caoren, objectName())) {
                caoren->broadcastSkillInvoke(objectName());
                caoren->turnOver();
                caoren->drawCards(4, objectName());
                const Card *card = NULL;
                foreach (int id, caoren->handCards()) {
                    const Card *c = Sanguosha->getCard(id);
                    if (JushouFilter(caoren, c)) {
                        card = c;
                        break;
                    }
                }
                if (card == NULL) return false;
                const Card *to_select = room->askForCard(caoren, "@@jushou_select!", "@jushou", QVariant(), Card::MethodNone);
                if (to_select != NULL)
                    card = Sanguosha->getCard(to_select->getEffectiveId());
                if (card->getTypeId() == Card::TypeEquip)
                    room->useCard(CardUseStruct(card, caoren, caoren));
                else
                    room->throwCard(card, caoren);
            }
        }
        return false;
    }

private:

    static bool JushouFilter(ServerPlayer *caoren, const Card *to_select)
    {
        if (to_select->getTypeId() == Card::TypeEquip)
            return to_select->isAvailable(caoren);
        return !caoren->isJilei(to_select);
    }
};

class JieweiViewAsSkill : public OneCardViewAsSkill
{
public:
    JieweiViewAsSkill() : OneCardViewAsSkill("jiewei")
    {
        filter_pattern = ".|.|.|equipped";
        response_or_use = true;
        response_pattern = "nullification";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        Nullification *ncard = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        ncard->addSubcard(originalCard);
        ncard->setSkillName(objectName());
        return ncard;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        return player->hasEquip();
    }
};

JieweiMoveCard::JieweiMoveCard()
{
    mute = true;
}

bool JieweiMoveCard::targetsFeasible(const QList<const Player *> &targets, const Player *) const
{
    return targets.length() == 2;
}

bool JieweiMoveCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (targets.isEmpty())
        return (!to_select->getJudgingArea().isEmpty() || !to_select->getEquips().isEmpty());
    else if (targets.length() == 1){
        for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
            if (targets.first()->getEquip(i) && !to_select->getEquip(i))
                return true;
        }
        foreach(const Card *card, targets.first()->getJudgingArea()){
            if (!Sanguosha->isProhibited(NULL, to_select, card))
                return true;
        }

    }
    return false;
}

void JieweiMoveCard::onUse(Room *room, const CardUseStruct &card_use) const
{
    CardUseStruct use = card_use;
    ServerPlayer *caoren = use.from;
    if (use.to.length() != 2) return;

    ServerPlayer *from = use.to.first();
    ServerPlayer *to = use.to.last();

    QList<int> all, ids, disabled_ids;
    for (int i = 0; i < S_EQUIP_AREA_LENGTH; i++) {
        if (from->getEquip(i)){
            if (!to->getEquip(i))
                ids << from->getEquip(i)->getEffectiveId();
            else
                disabled_ids << from->getEquip(i)->getEffectiveId();
            all << from->getEquip(i)->getEffectiveId();
        }
    }

    foreach(const Card *card, from->getJudgingArea()){
        if (!Sanguosha->isProhibited(NULL, to, card))
            ids << card->getEffectiveId();
        else
            disabled_ids << card->getEffectiveId();
        all << card->getEffectiveId();
    }

    room->fillAG(all, caoren, disabled_ids);
    from->setFlags("JieweiTarget");
    int card_id = room->askForAG(caoren, ids, true, "jiewei");
    from->setFlags("-JieweiTarget");
    room->clearAG(caoren);

    if (card_id != -1)
        room->moveCardTo(Sanguosha->getCard(card_id), from, to, room->getCardPlace(card_id), CardMoveReason(CardMoveReason::S_REASON_TRANSFER, caoren->objectName(), "jiewei", QString()));
}

class JieweiMove : public ZeroCardViewAsSkill
{
public:
    JieweiMove() : ZeroCardViewAsSkill("jiewei_move")
    {
        response_pattern = "@@jiewei_move";
    }

    virtual const Card *viewAs() const
    {
        return new JieweiMoveCard;
    }
};

class Jiewei : public TriggerSkill
{
public:
    Jiewei() : TriggerSkill("jiewei")
    {
        events << TurnedOver;
        view_as_skill = new JieweiViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!player->faceUp() || player->isNude()) return false;
        if (room->askForCard(player, "..", "@jiewei", data, objectName()))
            room->askForUseCard(player, "@@jiewei_move", "@jiewei-move");
    }
};

class Liegong : public TriggerSkill
{
public:
    Liegong() : TriggerSkill("liegong")
    {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash"))
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        QVariantList liegong_list = use.card->tag["SlashAddDamage"].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            bool nojink = (player->getHandcardNum() >= p->getHandcardNum());
            bool adddamage = (player->getHp() <= p->getHp());
            bool invoked = ((nojink || adddamage) && player->askForSkillInvoke(this, QVariant::fromValue(p)));
            if (invoked) {
                player->broadcastSkillInvoke(objectName());
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
                if (nojink) {
                    LogMessage log;
                    log.type = "#NoJink";
                    log.from = p;
                    room->sendLog(log);
                    jink_list.replace(index, QVariant(0));
                }
                if (adddamage)
                    liegong_list.replace(index, QVariant(liegong_list.at(index).toInt() + 1));
            }
            index++;
        }
        use.card->setTag("SlashAddDamage", QVariant::fromValue(liegong_list));
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

class Kuanggu : public TriggerSkill
{
public:
    Kuanggu() : TriggerSkill("kuanggu")
    {
        events << Damage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!player->tag["KuangguDamage"].toList().last().toBool()) return false;
        for (int i = 0; i < damage.damage; i++) {
            if (player->isAlive() && player->askForSkillInvoke(this, data)) {
                player->broadcastSkillInvoke(objectName());
                if (player->isWounded() && room->askForChoice(player, objectName(), "recover+draw", data) == "recover")
                    room->recover(player, RecoverStruct(player, NULL, 1));
                else
                    player->drawCards(1);
            } else
                break;
        }
        return false;
    }
};

class KuangguRecord : public TriggerSkill
{
public:
    KuangguRecord() : TriggerSkill("#kuanggu-record")
    {
        events << PreDamageDone << DamageComplete << EventPhaseChanging;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == PreDamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *weiyan = damage.from;
            if (weiyan == NULL) return false;
            QVariantList kuanggu_list = weiyan->tag["KuangguDamage"].toList();
            kuanggu_list << (weiyan->distanceTo(player) <= 1);
            weiyan->tag["KuangguDamage"] = QVariant::fromValue(kuanggu_list);
        } else if (triggerEvent == DamageComplete) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.prevented) return false;
            ServerPlayer *weiyan = damage.from;
            if (weiyan == NULL) return false;
            QVariantList kuanggu_list = weiyan->tag["KuangguDamage"].toList();
            kuanggu_list.takeLast();
            weiyan->tag["KuangguDamage"] = QVariant::fromValue(kuanggu_list);
        } else if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
                foreach (ServerPlayer *p, room->getAllPlayers())
                    p->tag.remove("KuangguDamage");
            }
        }
        return false;
    }
};

QimouCard::QimouCard()
{
    target_fixed = true;
}

void QimouCard::extraCost(Room *room, const CardUseStruct &card_use) const
{
    room->removePlayerMark(card_use.from, "@strategy");
    room->loseHp(card_use.from, user_string.toInt());
}

void QimouCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const
{
    room->setPlayerMark(source, "#qimou", user_string.toInt());
}

class QimouViewAsSkill : public ZeroCardViewAsSkill
{
public:
    QimouViewAsSkill() : ZeroCardViewAsSkill("qimou")
    {
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        return player->getMark("@strategy") > 0 && player->getHp() > 0;
    }

    virtual const Card *viewAs() const
    {
        QString user_string = Self->tag["qimou"].toString();
        if (user_string.isEmpty()) return NULL;
        QimouCard *skill_card = new QimouCard;
        skill_card->setUserString(user_string);
        skill_card->setSkillName("qimou");
        return skill_card;
    }
};

class Qimou : public TriggerSkill
{
public:
    Qimou() : TriggerSkill("qimou")
    {
        events << EventPhaseChanging;
        frequency = Limited;
        limit_mark = "@strategy";
        view_as_skill = new QimouViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *weiyan, QVariant &data) const
    {
        if (data.value<PhaseChangeStruct>().to == Player::NotActive) {
            room->setPlayerMark(weiyan, "#qimou", 0);
        }
        return false;
    }

    QString getSelectBox() const
    {
        QStringList hp_num;
        for (int i = 1; i <= Self->getHp(); hp_num << QString::number(i++)) {}
        return hp_num.join("+");
    }

};

class QimouDistance : public DistanceSkill
{
public:
    QimouDistance() : DistanceSkill("#qimou-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        return -from->getMark("#qimou");
    }
};

class QimouTargetMod : public TargetModSkill
{
public:
    QimouTargetMod() : TargetModSkill("#qimou-target")
    {
    }

    virtual int getResidueNum(const Player *from, const Card *, const Player *) const
    {
        return from->getMark("#qimou");
    }
};

class Buqu : public TriggerSkill
{
public:
    Buqu() : TriggerSkill("buqu")
    {
        events << AskForPeaches;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *zhoutai, QVariant &data) const
    {
        DyingStruct dying_data = data.value<DyingStruct>();
        if (dying_data.who != zhoutai)
            return false;

        if (zhoutai->getHp() > 0) return false;
        zhoutai->broadcastSkillInvoke(objectName());
        room->sendCompulsoryTriggerLog(zhoutai, objectName());

        int id = room->drawCard();
        int num = Sanguosha->getCard(id)->getNumber();
        bool duplicate = false;
        foreach (int card_id, zhoutai->getPile("buqu_chuang")) {
            if (Sanguosha->getCard(card_id)->getNumber() == num) {
                duplicate = true;
                break;
            }
        }
        zhoutai->addToPile("buqu_chuang", id);
        if (duplicate) {
            CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, QString(), objectName(), QString());
            room->throwCard(Sanguosha->getCard(id), reason, NULL);
        } else {
            room->recover(zhoutai, RecoverStruct(zhoutai, NULL, 1 - zhoutai->getHp()));
        }
        return false;
    }
};

class BuquMaxCards : public MaxCardsSkill
{
public:
    BuquMaxCards() : MaxCardsSkill("#buqu")
    {
    }

    virtual int getFixed(const Player *target) const
    {
        int len = target->getPile("buqu_chuang").length();
        if (len > 0)
            return len;
        else
            return -1;
    }
};

class Fenji : public TriggerSkill
{
public:
    Fenji() : TriggerSkill("fenji")
    {
        events << CardsMoveOneTime;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        if (player->getHp() > 0 && move.from && move.from->isAlive() && move.from_places.contains(Player::PlaceHand)
            && ((move.reason.m_reason == CardMoveReason::S_REASON_DISMANTLE
            && move.reason.m_playerId != move.reason.m_targetId)
            || (move.to && move.to != move.from && move.to_place == Player::PlaceHand
            && move.reason.m_reason != CardMoveReason::S_REASON_GIVE))) {
            move.from->setFlags("FenjiMoveFrom"); // For AI
            bool invoke = room->askForSkillInvoke(player, objectName(), QVariant::fromValue(move.from));
            move.from->setFlags("-FenjiMoveFrom");
            if (invoke) {
                player->broadcastSkillInvoke(objectName());
                if (move.from->isAlive())
					room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), move.from->objectName());
				
                room->loseHp(player);
				if (move.from->isAlive())
                    room->drawCards((ServerPlayer *)move.from, 2, "fenji");
            }
        }
        return false;
    }
};

class Hongyan : public FilterSkill
{
public:
    Hongyan() : FilterSkill("hongyan")
    {
    }

    static WrappedCard *changeToHeart(int cardId)
    {
        WrappedCard *new_card = Sanguosha->getWrappedCard(cardId);
        new_card->setSkillName("hongyan");
        new_card->setSuit(Card::Heart);
        new_card->setModified(true);
        return new_card;
    }

    virtual bool viewFilter(const Card *to_select) const
    {
        return to_select->getSuit() == Card::Spade;
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        return changeToHeart(originalCard->getEffectiveId());
    }
};

TianxiangCard::TianxiangCard()
{
}

void TianxiangCard::onEffect(const CardEffectStruct &effect) const
{
    ServerPlayer *xiaoqiao = effect.from, *target = effect.to;
    Room *room = xiaoqiao->getRoom();
    QVariant data = xiaoqiao->tag.value("TianxiangDamage");
    DamageStruct damage = data.value<DamageStruct>();
    const Card *card = Sanguosha->getCard(getEffectiveId());
    if (room->askForChoice(xiaoqiao, "tianxiang", "damage+losehp", data,
                           "@tianxiang-choose::"+target->objectName()+":"+card->objectName())=="damage") {
        room->damage(DamageStruct("tianxiang", damage.from, target));
        if (target->isAlive() && target->getLostHp() > 0)
            target->drawCards(qMin(target->getLostHp(), 5), "tianxiang");
    } else {
        room->loseHp(target);
        int id = getEffectiveId();
        Player::Place place = room->getCardPlace(id);
        if (place == Player::DiscardPile || place == Player::DrawPile)
            target->obtainCard(this);
    }
}

class TianxiangViewAsSkill : public OneCardViewAsSkill
{
public:
    TianxiangViewAsSkill() : OneCardViewAsSkill("tianxiang")
    {
        filter_pattern = ".|heart|.|hand!";
        response_pattern = "@@tianxiang";
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        TianxiangCard *tianxiangCard = new TianxiangCard;
        tianxiangCard->addSubcard(originalCard);
        return tianxiangCard;
    }
};

class Tianxiang : public TriggerSkill
{
public:
    Tianxiang() : TriggerSkill("tianxiang")
    {
        events << DamageInflicted;
        view_as_skill = new TianxiangViewAsSkill;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *xiaoqiao, QVariant &data) const
    {
        if (!xiaoqiao->isKongcheng()) {
            xiaoqiao->tag["TianxiangDamage"] = data;
            return room->askForUseCard(xiaoqiao, "@@tianxiang", "@tianxiang-card", QVariant(), Card::MethodDiscard);
        }
        return false;
    }
};

GuhuoCard::GuhuoCard()
{
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
    m_skillName = "guhuo";
}

bool GuhuoCard::guhuo(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();

    CardMoveReason reason1(CardMoveReason::S_REASON_SECRETLY_PUT, yuji->objectName(), QString(), "guhuo", QString());
    room->moveCardTo(this, NULL, Player::PlaceTable, reason1, false);

    QList<ServerPlayer *> players = room->getOtherPlayers(yuji);

    room->setTag("GuhuoType", user_string);

    ServerPlayer *questioned = NULL;
    foreach (ServerPlayer *player, players) {
        if (player->hasSkill("chanyuan")) {
            LogMessage log;
            log.type = "#Chanyuan";
            log.from = player;
            log.arg = "chanyuan";
            room->sendLog(log);

            continue;
        }

        QString choice = room->askForChoice(player, "guhuo", "noquestion+question");

        LogMessage log;
        log.type = "#GuhuoQuery";
        log.from = player;
        log.arg = choice;
        room->sendLog(log);
        if (choice == "question") {
            questioned = player;
            break;
        }
    }

    LogMessage log;
    log.type = "$GuhuoResult";
    log.from = yuji;
    log.card_str = QString::number(subcards.first());
    room->sendLog(log);

    const Card *card = Sanguosha->getCard(subcards.first());

    CardMoveReason reason_ui(CardMoveReason::S_REASON_TURNOVER, yuji->objectName(), QString(), "guhuo", QString());
    reason_ui.m_extraData = QVariant::fromValue(card);
    room->showVirtualMove(reason_ui);

    bool success = true;
    if (questioned) {
        success = (card->objectName() == user_string);

        if (success) {
            room->acquireSkill(questioned, "chanyuan");
        } else {
            room->moveCardTo(this, yuji, NULL, Player::DiscardPile,
                CardMoveReason(CardMoveReason::S_REASON_PUT, yuji->objectName(), QString(), "guhuo"), true);
        }
    }
    room->setPlayerFlag(yuji, "GuhuoUsed");
    return success;
}

bool GuhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return false;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card, targets);
}

bool GuhuoCard::targetFixed() const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetFixed();
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetFixed();
}

bool GuhuoCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const
{
    if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE) {
        const Card *card = NULL;
        if (!user_string.isEmpty())
            card = Sanguosha->cloneCard(user_string.split("+").first());
        return card && card->targetsFeasible(targets, Self);
    } else if (Sanguosha->currentRoomState()->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE) {
        return true;
    }

    Card *card = Sanguosha->cloneCard(user_string);
    if (card == NULL)
        return false;
    card->setCanRecast(false);
    card->deleteLater();
    return card && card->targetsFeasible(targets, Self);
}

const Card *GuhuoCard::validate(CardUseStruct &card_use) const
{
    ServerPlayer *yuji = card_use.from;
    Room *room = yuji->getRoom();

    QString to_guhuo = user_string;
    yuji->broadcastSkillInvoke("guhuo");

    LogMessage log;
    log.type = card_use.to.isEmpty() ? "#GuhuoNoTarget" : "#Guhuo";
    log.from = yuji;
    log.to = card_use.to;
    log.arg = to_guhuo;
    log.arg2 = "guhuo";
    room->sendLog(log);

    const Card *guhuo_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    if (card_use.to.isEmpty())
        card_use.to = guhuo_card->defaultTargets(room, yuji);

    foreach (ServerPlayer *to, card_use.to)
        room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, yuji->objectName(), to->objectName());

    CardMoveReason reason_ui(CardMoveReason::S_REASON_USE, yuji->objectName(), QString(), "guhuo", QString());
    if (card_use.to.size() == 1 && !card_use.card->targetFixed())
        reason_ui.m_targetId = card_use.to.first()->objectName();
    reason_ui.m_extraData = QVariant::fromValue(guhuo_card);
    room->showVirtualMove(reason_ui);

    if (guhuo(card_use.from)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_guhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("_guhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

const Card *GuhuoCard::validateInResponse(ServerPlayer *yuji) const
{
    Room *room = yuji->getRoom();
    yuji->broadcastSkillInvoke("guhuo");

    QString to_guhuo = user_string;

    LogMessage log;
    log.type = "#GuhuoNoTarget";
    log.from = yuji;
    log.arg = to_guhuo;
    log.arg2 = "guhuo";
    room->sendLog(log);

    CardMoveReason reason_ui(CardMoveReason::S_REASON_RESPONSE, yuji->objectName(), QString(), "guhuo", QString());
    const Card *guhuo_card = Sanguosha->cloneCard(user_string, Card::NoSuit, 0);
    reason_ui.m_extraData = QVariant::fromValue(guhuo_card);
    room->showVirtualMove(reason_ui);

    if (guhuo(yuji)) {
        const Card *card = Sanguosha->getCard(subcards.first());
        Card *use_card = Sanguosha->cloneCard(to_guhuo, card->getSuit(), card->getNumber());
        use_card->setSkillName("_guhuo");
        use_card->addSubcard(subcards.first());
        use_card->deleteLater();
        return use_card;
    } else
        return NULL;
}

class Guhuo : public OneCardViewAsSkill
{
public:
    Guhuo() : OneCardViewAsSkill("guhuo")
    {
        filter_pattern = ".|.|.|hand";
        response_or_use = true;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const
    {
        bool current = false;
        QList<const Player *> players = player->getAliveSiblings();
        players.append(player);
        foreach (const Player *p, players) {
            if (p->getPhase() != Player::NotActive) {
                current = true;
                break;
            }
        }
        if (!current) return false;

        if (player->hasFlag("GuhuoUsed") || pattern.startsWith(".") || pattern.startsWith("@"))
            return false;
        for (int i = 0; i < pattern.length(); i++) {
            QChar ch = pattern[i];
            if (ch.isUpper() || ch.isDigit()) return false; // This is an extremely dirty hack!! For we need to prevent patterns like 'BasicCard'
        }
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const
    {
        bool current = false;
        QList<const Player *> players = player->getAliveSiblings();
        players.append(player);
        foreach (const Player *p, players) {
            if (p->getPhase() != Player::NotActive) {
                current = true;
                break;
            }
        }
        if (!current) return false;
        return !player->hasFlag("GuhuoUsed");
    }

    virtual const Card *viewAs(const Card *originalCard) const
    {
        GuhuoCard *card = new GuhuoCard;
        card->addSubcard(originalCard);
        return card;
    }

    QString getSelectBox() const
    {
        return "guhuo_bt";
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *card) const
    {
        if (card->isKindOf("GuhuoCard"))
            return -1;
        else
            return 0;
    }

    virtual bool isEnabledAtNullification(const ServerPlayer *player) const
    {
        ServerPlayer *current = player->getRoom()->getCurrent();
        if (!current || current->isDead() || current->getPhase() == Player::NotActive) return false;
        return !player->isKongcheng() && !player->hasFlag("GuhuoUsed");
    }
};

class Chanyuan : public TriggerSkill
{
public:
    Chanyuan() : TriggerSkill("chanyuan")
    {
        events << GameStart << HpChanged << MaxHpChanged << EventAcquireSkill << EventLoseSkill;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 5;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventLoseSkill) {
            if (data.toString() != objectName()) return false;
            room->removePlayerTip(player, "#chanyuan");
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != objectName()) return false;
            room->addPlayerTip(player, "#chanyuan");
        }
        if (triggerEvent != EventLoseSkill && !player->hasSkill(this)) return false;

        foreach(ServerPlayer *p, room->getOtherPlayers(player))
            room->filterCards(p, p->getCards("he"), true);
        JsonArray args;
        args << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
        room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
        return false;
    }
};

class ChanyuanInvalidity : public InvaliditySkill
{
public:
    ChanyuanInvalidity() : InvaliditySkill("#chanyuan-inv")
    {
    }

    virtual bool isSkillValid(const Player *player, const Skill *skill) const
    {
        return skill->objectName() == "chanyuan" || !player->hasSkill("chanyuan")
            || player->getHp() != 1 || skill->isAttachedLordSkill();
    }
};

WindPackage::WindPackage()
    :Package("wind")
{
    General *xiahouyuan = new General(this, "xiahouyuan", "wei"); // WEI 008
    xiahouyuan->addSkill(new Shensu);

    General *caoren = new General(this, "caoren", "wei"); // WEI 011
    caoren->addSkill(new Jushou);
    caoren->addSkill(new Jiewei);

    General *huangzhong = new General(this, "huangzhong", "shu");
    huangzhong->addSkill(new Liegong);

    General *weiyan = new General(this, "weiyan", "shu");
    weiyan->addSkill(new Kuanggu);
    weiyan->addSkill(new KuangguRecord);
    related_skills.insertMulti("kuanggu", "#kuanggu-record");
    weiyan->addSkill(new Qimou);
    weiyan->addSkill(new QimouDistance);
    weiyan->addSkill(new QimouTargetMod);
    related_skills.insertMulti("qimou", "#qimou-distance");
    related_skills.insertMulti("qimou", "#qimou-target");

    General *xiaoqiao = new General(this, "xiaoqiao", "wu", 3, false); // WU 011
    xiaoqiao->addSkill(new Tianxiang);
    xiaoqiao->addSkill(new Hongyan);

    General *zhoutai = new General(this, "zhoutai", "wu"); // WU 013
    zhoutai->addSkill(new Buqu);
    zhoutai->addSkill(new BuquMaxCards);
    zhoutai->addSkill(new DetachEffectSkill("buqu", "buqu_chuang"));
    related_skills.insertMulti("buqu", "#buqu");
	related_skills.insertMulti("buqu", "#buqu-clear");
    zhoutai->addSkill(new Fenji);

    General *zhangjiao = new General(this, "zhangjiao$", "qun", 3); // QUN 010
    zhangjiao->addSkill(new Leiji);
    zhangjiao->addSkill(new Guidao);
    zhangjiao->addSkill(new Huangtian);

    General *yuji = new General(this, "yuji", "qun", 3); // QUN 011
    yuji->addSkill(new Guhuo);
    yuji->addRelateSkill("chanyuan");

    addMetaObject<ShensuCard>();
    addMetaObject<JieweiMoveCard>();
    addMetaObject<QimouCard>();
    addMetaObject<TianxiangCard>();
    addMetaObject<HuangtianCard>();
    addMetaObject<GuhuoCard>();

    skills << new HuangtianViewAsSkill << new Chanyuan << new ChanyuanInvalidity << new JushouSelect << new JieweiMove;
    related_skills.insertMulti("chanyuan", "#chanyuan-inv");
}

ADD_PACKAGE(Wind)

