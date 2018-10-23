#include "godsreturn.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "roomscene.h"
#include "special3v3.h"

class BossXiongshou : public TriggerSkill
{
public:
    BossXiongshou() : TriggerSkill("bossxiongshou")
    {
        events << DamageCaused;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *boss, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (boss->getHp() > damage.to->getHp() && damage.card && damage.card->isKindOf("Slash")) {
            boss->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(boss, objectName());
            damage.damage++;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class BossXiongshouDistance : public DistanceSkill
{
public:
    BossXiongshouDistance() : DistanceSkill("#bossxiongshou-distance")
    {
    }

    virtual int getCorrect(const Player *from, const Player *) const
    {
        if (from->hasSkill(this))
            return -1;
        else
            return 0;
    }
};

class BossWake : public TriggerSkill
{
public:
    BossWake() : TriggerSkill("#bosswake")
    {
        events << HpChanged << MaxHpChanged;
        global = true;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getHp() <= target->getMaxHp()/2 && target->getGeneralName().startsWith("fierce");
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (const QString &skill_name, player->getGeneral()->getRelatedSkillNames()) {
            const Skill *skill = Sanguosha->getSkill(skill_name);
            if (skill && !player->hasSkill(skill, true))
                room->acquireSkill(player, skill_name);
        }
        return false;
    }
};

class BossWuzang : public DrawCardsSkill
{
public:
    BossWuzang() : DrawCardsSkill("#bosswuzang-draw")
    {
        frequency = Compulsory;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target && target->isAlive() && target->hasSkill("bosswuzang");
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        if (n <= 0) return n;
        player->getRoom()->sendCompulsoryTriggerLog(player, "bosswuzang");
        player->broadcastSkillInvoke("bosswuzang");
        return qMax(player->getHp() / 2, 5);
    }
};

class BossWuzangKeep : public MaxCardsSkill
{
public:
    BossWuzangKeep() : MaxCardsSkill("bosswuzang")
    {
    }

    virtual int getFixed(const Player *target) const
    {
        if (target->hasSkill(objectName()))
            return 0;
        else
            return -1;
    }
};

class BossXiangde : public TriggerSkill
{
public:
    BossXiangde() : TriggerSkill("bossxiangde")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.from || !damage.from->getWeapon()) return false;
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        damage.damage++;
        data = QVariant::fromValue(damage);
        return false;
    }
};

class BossYinzei : public MasochismSkill
{
public:
    BossYinzei() : MasochismSkill("bossyinzei")
    {
        frequency = Compulsory;
    }

    virtual void onDamaged(ServerPlayer *boss, const DamageStruct &damage) const
    {
        ServerPlayer *from = damage.from;
        Room *room = boss->getRoom();
        if (boss->isKongcheng() && from && from->isAlive()) {
            QList<int> ids = from->forceToDiscard(1, true);
            if (ids.isEmpty()) return;
            room->sendCompulsoryTriggerLog(boss, objectName());
            boss->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, boss->objectName(), from->objectName());
            room->throwCard(ids.first(), from);
        }
    }
};

class BossZhue : public TriggerSkill
{
public:
    BossZhue() : TriggerSkill("bosszhue")
    {
        events << Damage;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *target, QVariant &) const
    {
        foreach (ServerPlayer *boss, room->getOtherPlayers(target)) {
            if (!TriggerSkill::triggerable(boss)) continue;
            room->sendCompulsoryTriggerLog(boss, objectName());
            boss->broadcastSkillInvoke(objectName());
            if (target->isAlive())
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, boss->objectName(), target->objectName());
            boss->drawCards(1, objectName());
            target->drawCards(1, objectName());
        }
        return false;
    }
};

class BossFutai : public TriggerSkill
{
public:
    BossFutai() : TriggerSkill("bossfutai")
    {
        events << EventPhaseStart << AskForPeaches;
        frequency = Compulsory;
    }

    int getPriority(TriggerEvent triggerEvent) const
    {
        if (triggerEvent == AskForPeaches)
            return 1;
        else
            return TriggerSkill::getPriority(triggerEvent);
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseStart && player->getPhase() == Player::RoundStart && TriggerSkill::triggerable(player)) {
            QList<ServerPlayer *> woundeds;
            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                if (p->isWounded())
                    woundeds << p;
            }
            if (woundeds.isEmpty()) return false;
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            foreach (ServerPlayer *p, woundeds)
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
            foreach (ServerPlayer *p, woundeds)
                room->recover(p, RecoverStruct(player));
        } else if (triggerEvent == AskForPeaches) {
            DyingStruct dying_data = data.value<DyingStruct>();
            if (dying_data.who == player) return false;
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->getPhase() == Player::NotActive && p != player) {
                    room->setTag("SkipGameRule", true);
                    break;
                }
            }
        }
        return false;
    }
};

class BossFutaiCardLimited : public CardLimitedSkill
{
public:
    BossFutaiCardLimited() : CardLimitedSkill("#bossfutai-limited")
    {
    }
    virtual bool isCardLimited(const Player *player, const Card *card, Card::HandlingMethod method) const
    {
        if (!card->isKindOf("Peach") || method != Card::MethodUse) return false;
        foreach(const Player *sib, player->getAliveSiblings()) {
            if (sib->hasSkill("bossfutai") && sib->getPhase() == Player::NotActive)
                return true;
        }
        return false;
    }
};

class BossYanduRecord : public TriggerSkill
{
public:
    BossYanduRecord() : TriggerSkill("#bossyandu-record")
    {
        events << DamageDone << TurnStart;
        global = true;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        if (triggerEvent == DamageDone) {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.from && damage.from->getPhase() != Player::NotActive) {
                damage.from->setMark("yanduHasDamege", 1);
            }
        } else {
            foreach(ServerPlayer *p, room->getAlivePlayers())
                p->setMark("yanduHasDamege", 0);
        }

        return false;
    }
};

class BossYandu : public PhaseChangeSkill
{
public:
    BossYandu() : PhaseChangeSkill("bossyandu")
    {
        frequency = Compulsory;
    }

    virtual int getPriority(TriggerEvent) const
    {
        return 1;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool onPhaseChange(ServerPlayer *player) const
    {
        Room *room = player->getRoom();
        if (player->getPhase() == Player::NotActive && player->getMark("yanduHasDamege") == 0) {
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (TriggerSkill::triggerable(p) && player->isAlive() && !player->isNude()) {
                    room->sendCompulsoryTriggerLog(p, objectName());
                    p->broadcastSkillInvoke(objectName());
                    room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, p->objectName(), player->objectName());
                    int card_id = room->askForCardChosen(p, player, "he", objectName());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, p->objectName());
                    room->obtainCard(p, Sanguosha->getCard(card_id),
                        reason, room->getCardPlace(card_id) != Player::PlaceHand);
                }
            }
        }
        return false;
    }
};

class BossMingwan : public TriggerSkill
{
public:
    BossMingwan() : TriggerSkill("bossmingwan")
    {
        events << Damage << CardUsed << CardResponded << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == Damage) {
            DamageStruct damage = data.value<DamageStruct>();
            QStringList assignee_list = player->property("bossmingwan_targets").toString().split("+");
            if (TriggerSkill::triggerable(player) && !assignee_list.contains(damage.to->objectName()) && player->getPhase() != Player::NotActive) {
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), damage.to->objectName());
                assignee_list << damage.to->objectName();
                room->setPlayerProperty(player, "bossmingwan_targets", assignee_list.join("+"));
            }
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() == Player::NotActive)
                room->setPlayerProperty(player, "bossmingwan_targets", QString());
        } else {
            const Card *card = NULL;
            if (triggerEvent == CardUsed)
                card = data.value<CardUseStruct>().card;
            else {
                CardResponseStruct response = data.value<CardResponseStruct>();
                if (response.m_isUse)
                    card = response.m_card;
            }
            if (card && card->getTypeId() != Card::TypeSkill && card->getHandlingMethod() == Card::MethodUse) {
                if (player->property("bossmingwan_targets").toString().isEmpty()) return false;
                room->sendCompulsoryTriggerLog(player, objectName());
                player->broadcastSkillInvoke(objectName());
                player->drawCards(1, objectName());
            }
        }
        return false;
    }
};

class BossMingwanProhibit : public ProhibitSkill
{
public:
    BossMingwanProhibit() : ProhibitSkill("#bossmingwan-prohibit")
    {
    }

    bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> & /* = QList<const Player *>() */) const
    {
        if (!from || card->isKindOf("SkillCard") || from->property("bossmingwan_targets").toString().isEmpty()
                || !to || from == to) return false;
        return !from->property("bossmingwan_targets").toString().split("+").contains(to->objectName());
    }
};

class BossNitai : public TriggerSkill
{
public:
    BossNitai() : TriggerSkill("bossnitai")
    {
        events << DamageInflicted;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (player->getPhase() == Player::NotActive && damage.nature != DamageStruct::Fire) return false;
        room->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        if (player->getPhase() == Player::NotActive) {
            damage.damage++;
            data = QVariant::fromValue(damage);
        } else
            return true;
        return false;
    }
};

class BossLuanchang : public TriggerSkill
{
public:
    BossLuanchang() : TriggerSkill("bossluanchang")
    {
        events << EventPhaseStart << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *boss, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::NotActive) return false;
            ArcheryAttack *aa = new ArcheryAttack(Card::NoSuit, 0);
            aa->setSkillName("_" + objectName());
            if (!aa->isAvailable(boss)) return false;
            room->sendCompulsoryTriggerLog(boss, objectName());
            boss->broadcastSkillInvoke(objectName());
            room->useCard(CardUseStruct(aa, boss, QList<ServerPlayer *>()));

        } else if (triggerEvent == EventPhaseStart) {
            if (boss->getPhase() != Player::RoundStart) return false;
            SavageAssault *sa = new SavageAssault(Card::NoSuit, 0);
            sa->setSkillName("_" + objectName());
            if (!sa->isAvailable(boss)) return false;
            room->sendCompulsoryTriggerLog(boss, objectName());
            boss->broadcastSkillInvoke(objectName());
            room->useCard(CardUseStruct(sa, boss, QList<ServerPlayer *>()));
        }
        return false;
    }
};

class BossTanyu : public TriggerSkill
{
public:
    BossTanyu() : TriggerSkill("bosstanyu")
    {
        events << EventPhaseStart << EventPhaseChanging;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == EventPhaseChanging) {
            if (data.value<PhaseChangeStruct>().to != Player::Discard || player->isSkipped(Player::Discard)) return false;
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            player->skip(Player::Discard);
        } else if (triggerEvent == EventPhaseStart) {
            if (player->getPhase() != Player::Finish) return false;
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (p->getHandcardNum() > player->getHandcardNum())
                    return false;
            }
            room->sendCompulsoryTriggerLog(player, objectName());
            player->broadcastSkillInvoke(objectName());
            room->loseHp(player);

        }
        return false;
    }
};

class BossCangmu : public DrawCardsSkill
{
public:
    BossCangmu() : DrawCardsSkill("bosscangmu")
    {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const
    {
        if (n <= 0) return n;
        player->getRoom()->sendCompulsoryTriggerLog(player, objectName());
        player->broadcastSkillInvoke(objectName());
        return player->getRoom()->alivePlayerCount();
    }
};

class BossJicai : public TriggerSkill
{
public:
    BossJicai() : TriggerSkill("bossjicai")
    {
        events << HpRecover;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &) const
    {
        foreach (ServerPlayer *boss, room->getAlivePlayers()) {
            if (!TriggerSkill::triggerable(boss)) continue;
            room->sendCompulsoryTriggerLog(boss, objectName());
            boss->broadcastSkillInvoke(objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, boss->objectName(), player->objectName());
            boss->drawCards(1, objectName());
            if (boss != player)
                player->drawCards(1, objectName());
        }
        return false;
    }
};

class GodBladeSkill: public WeaponSkill {
public:
    GodBladeSkill(): WeaponSkill("god_blade") {
        events << TargetSpecified;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (!use.card->isKindOf("Slash") || !use.card->isRed())
            return false;
        QVariantList jink_list = player->tag["Jink_" + use.card->toString()].toList();
        int index = 0;
        foreach (ServerPlayer *p, use.to) {
            LogMessage log;
            log.type = "#NoJink";
            log.from = p;
            room->sendLog(log);
            jink_list.replace(index, QVariant(0));
            ++index;
        }
        player->tag["Jink_" + use.card->toString()] = QVariant::fromValue(jink_list);
        return false;
    }
};

GodBlade::GodBlade(Suit suit, int number)
    : Weapon(suit, number, 3)
{
    setObjectName("god_blade");
}

class GodDiagramSkill: public ArmorSkill {
public:
    GodDiagramSkill(): ArmorSkill("god_diagram") {
        events << SlashEffected;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        //防止随机出现的“装备了青釭剑，使用黑杀已造成伤害，
        //但仍触发仁王盾效果，屏幕上打印出了相关日志”的问题
        if (!effect.from->hasWeapon("qinggang_sword")) {
            LogMessage log;
            log.type = "#ArmorNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            player->getRoom()->sendLog(log);

            room->setEmotion(player, "armor/god_diagram");
            effect.to->setFlags("Global_NonSkillNullify");
            return true;
        } else
            return false;
    }
};

GodDiagram::GodDiagram(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("god_diagram");
}

class GodRobeSkill: public ProhibitSkill {
public:
    GodRobeSkill(): ProhibitSkill("god_robe") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const{
        return card->isNDTrick() && from != to && to->hasArmorEffect("god_robe");
    }
};

GodRobe::GodRobe(Suit suit, int number)
    : Armor(suit, number)
{
    setObjectName("god_robe");
}

class GodZitherSkill: public WeaponSkill {
public:
    GodZitherSkill(): WeaponSkill("god_zither") {
        events << ConfirmDamage;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.nature != DamageStruct::Fire) {
            room->setEmotion(player, "armor/god_zither");
            damage.nature = DamageStruct::Fire;
            data = QVariant::fromValue(damage);
        }
        return false;
    }
};

GodZither::GodZither(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("god_zither");
}

class GodHalberdsSkill: public TargetModSkill {
public:
    GodHalberdsSkill(): TargetModSkill("#god_halberd") {
    }

    virtual int getExtraTargetNum(const Player *from, const Card *card) const{
        if (from->hasWeapon("god_halberd"))
            return 1000;
        else
            return 0;
    }
};

GodHalberd::GodHalberd(Suit suit, int number)
    : Weapon(suit, number, 4)
{
    setObjectName("god_halberd");
}

class GodHalberdSkill: public WeaponSkill {
public:
    GodHalberdSkill(): WeaponSkill("god_halberd") {
        events << ConfirmDamage << Damage;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.card->isKindOf("Slash")) {
            if (event == ConfirmDamage) {
                room->setEmotion(player, "armor/god_halberd");
                ++damage.damage;
                data = QVariant::fromValue(damage);
            } else {
                room->recover(damage.to, RecoverStruct(damage.from));
            }
        }
        return false;
    }
};

class GodHatSkill: public TreasureSkill {
public:
    GodHatSkill(): TreasureSkill("god_hat") {
        events << DrawNCards;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        room->setEmotion(player, "armor/god_hat");
        data = QVariant::fromValue(data.toInt() + 2);
        return false;
    }
};

GodHat::GodHat(Suit suit, int number)
    : Treasure(suit, number)
{
    setObjectName("god_hat");
}

class GodHatBuff: public MaxCardsSkill {
public:
    GodHatBuff(): MaxCardsSkill("#god_hat") {
    }

    virtual int getExtra(const Player *target) const{
        if (target->hasTreasure("god_hat"))
            return -1;
        else
            return 0;
    }
};

class GodSwordSkill: public WeaponSkill {
public:
    GodSwordSkill(): WeaponSkill("god_sword") {
        events << TargetSpecified << CardFinished;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (event == TargetSpecified) {
            if (use.card->isKindOf("Slash")) {
                bool do_anim = false;
                foreach (ServerPlayer *p, use.to.toSet()) {
                    if (p->getMark("Equips_of_Others_Nullified_to_You") == 0) {
                        room->setPlayerCardLimitation(p, "use,response", ".|.|.|hand", true);
                        do_anim = (p->getArmor() && p->hasArmorEffect(p->getArmor()->objectName())) || p->hasSkills("bazhen|linglong|bossmanjia");
                        p->addQinggangTag(use.card);
                    }
                }
                if (do_anim)
                    room->setEmotion(use.from, "weapon/god_sword");
            }
        } else {
            foreach (ServerPlayer *p, use.to.toSet()) {
                room->removePlayerCardLimitation(p, "use,response", ".|.|.|hand$1");
            }
        }
        return false;
    }
};

GodSword::GodSword(Suit suit, int number)
    : Weapon(suit, number, 2)
{
    setObjectName("god_sword");
}

class GodHorseSkill: public DistanceSkill {
public:
    GodHorseSkill(): DistanceSkill("god_horse_skill") {

    }

    virtual int getCorrect(const Player *from, const Player *to) const
    {
        if (to->getDefensiveHorse() && to->getDefensiveHorse()->objectName() == "god_horse")
            return 0;
        if (((from->isLord() || from->getRole() == "loyalist") && (to->isLord() || to->getRole() == "loyalist")) || from->getRole() == to->getRole())
            return 0;
        foreach (const Player *player, to->getAliveSiblings())
            if (player->getDefensiveHorse() && player->getDefensiveHorse()->objectName() == "god_horse")
                if (((player->isLord() || player->getRole() == "loyalist") && (to->isLord() || to->getRole() == "loyalist")) || player->getRole() == to->getRole())
                    return 1;
        return 0;
    }
};

GodHorse::GodHorse(Card::Suit suit, int number)
    :DefensiveHorse(suit, number)
{
    setObjectName("god_horse");
}

BeanSoldiers::BeanSoldiers(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("bean_soldiers");
}

bool BeanSoldiers::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    if ((to_select->getKingdom() == "god") || (to_select->getKingdom() != "god" && to_select->getHandcardNum() - 1 < qMin(to_select->getMaxHp(), 5))) {
        if (targets.length() == 0) return to_select == Self;
        return targets.length() < total_num && to_select != Self;
    }
    return false;
}

bool BeanSoldiers::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() > 0;
}

void BeanSoldiers::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct use = card_use;
    if (use.to.isEmpty())
        use.to << use.from;
    SingleTargetTrick::onUse(room, use);
}

bool BeanSoldiers::isAvailable(const Player *player) const{
    return !player->isProhibited(player, this) && TrickCard::isAvailable(player);
}

void BeanSoldiers::onEffect(const CardEffectStruct &effect) const{
    if (effect.to->getKingdom() == "god" || qMin(effect.to->getMaxHp(), 5) - effect.to->getHandcardNum() > 0)
        effect.to->drawCards(effect.to->getKingdom() == "god" ? qMin(effect.to->getMaxHp(), 5) : qMin(effect.to->getMaxHp(), 5) - effect.to->getHandcardNum(), "bean_soldiers");
}

Graft::Graft(Suit suit, int number)
    : SingleTargetTrick(suit, number)
{
    setObjectName("graft");
}

bool Graft::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    int total_num = 1 + Sanguosha->correctCardTarget(TargetModSkill::ExtraTarget, Self, this);
    return targets.length() < total_num && to_select != Self && !to_select->isNude();
}

void Graft::onEffect(const CardEffectStruct &effect) const{
    QList<ServerPlayer *> targets;
    Room *room = effect.to->getRoom();
    foreach (ServerPlayer *p, room->getAlivePlayers()) {
        if (effect.to->canSlash(p, NULL, false))
            targets << p;
    }
    if (!room->askForUseSlashTo(effect.to, targets, "@graft") && !effect.to->getCards("he").isEmpty()) {
        const Card *card = effect.to->getCards("he").length() == 1 ? effect.to->getCards("he").first() : room->askForExchange(effect.to, "graft", 2, 2, true, "@graft");
        room->obtainCard(effect.from, card, false);
    }
}

class GanluSWZS : public TriggerSkill
{
public:
    GanluSWZS() : TriggerSkill("#ganluswzs")
    {
        events << AskForPeachesDone;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->getRole() == "rebel";
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (!player->hasFlag("Global_Dying") || player->getHp() > 0) return false;
        ServerPlayer *lord = room->getLord();
        if (lord->getMark("#ganlu") > 2) return false;
        for (int i = 1; i < 7; i++)
            foreach (ServerPlayer *rebel, room->getAllPlayers())
                if (rebel->getRole() == "rebel" && rebel->getSeat()%6 == i%6)
                {
                    if (room->askForSkillInvoke(rebel, "ganlu"))
                    {
                        player->clearFlags();
                        player->clearHistory();
                        player->throwAllCards();
                        player->throwAllMarks();

                        room->clearPlayerCardLimitation(player, false);

                        room->setPlayerFlag(player, "-Global_Dying");
                        room->setEmotion(player, "revive");

                        if (player->getMaxHp() < 3)
                            room->recover(player, RecoverStruct(player, NULL, player->getMaxHp() - player->getHp()));
                        else
                            room->recover(player, RecoverStruct(player, NULL, 3 - player->getHp()));

                        room->drawCards(player, 3, "ganlu");

                        room->addPlayerMark(lord, "#ganlu", 1);
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

GodsReturnPackage::GodsReturnPackage()
    : Package("GodsReturn")
{

    General *boss_zhuyin = new General(this, "fierce_zhuyin", "qun", 4, true, true);
    boss_zhuyin->addSkill(new BossXiongshou);
    boss_zhuyin->addSkill(new BossXiongshouDistance);
    related_skills.insertMulti("bossxiongshou", "#bossxiongshou-distance");

    General *boss_hundun = new General(this, "fierce_hundun", "qun", 20, true, true);
    boss_hundun->addSkill("bossxiongshou");
    boss_hundun->addSkill(new BossWuzang);
    boss_hundun->addSkill(new BossWuzangKeep);
    related_skills.insertMulti("bosswuzang", "#bosswuzang-keep");
    boss_hundun->addSkill(new BossXiangde);
    boss_hundun->addRelateSkill("bossyinzei");

    General *boss_qiongqi = new General(this, "fierce_qiongqi", "qun", 16, true, true);
    boss_qiongqi->addSkill("bossxiongshou");
    boss_qiongqi->addSkill(new BossZhue);
    boss_qiongqi->addSkill(new BossFutai);
    boss_qiongqi->addSkill(new BossFutaiCardLimited);
    related_skills.insertMulti("bossfutai", "#bossfutai-limited");
    boss_qiongqi->addRelateSkill("bossyandu");

    General *boss_taowu = new General(this, "fierce_taowu", "qun", 16, true, true);
    boss_taowu->addSkill("bossxiongshou");
    boss_taowu->addSkill(new BossMingwan);
    boss_taowu->addSkill(new BossMingwanProhibit);
    related_skills.insertMulti("bossmingwan", "#bossmingwan-prohibit");
    boss_taowu->addSkill(new BossNitai);
    boss_taowu->addRelateSkill("bossluanchang");

    General *boss_taotie = new General(this, "fierce_taotie", "qun", 20, true, true);
    boss_taotie->addSkill("bossxiongshou");
    boss_taotie->addSkill(new BossTanyu);
    boss_taotie->addSkill(new BossCangmu);
    boss_taotie->addRelateSkill("bossjicai");

    skills << new BossYinzei << new BossYandu << new BossYanduRecord
           << new BossLuanchang << new BossJicai << new BossWake;
}

GodsReturnCardPackage::GodsReturnCardPackage()
    : Package("GodsReturnCard", Package::CardPack)
{
    QList<Card *> cards;

    cards << new GodBlade(Card::Spade, 5)
          << new GodRobe(Card::Spade, 9)
          << new GodZither(Card::Diamond, 1)
          << new GodDiagram(Card::Spade, 2)
          << new GodDiagram(Card::Club, 2)
          << new GodHorse(Card::Spade, 5)
          << new GodHalberd(Card::Diamond, 12)
          << new GodSword(Card::Spade, 6)
          << new GodHat(Card::Club, 1);

    cards << new BeanSoldiers(Card::Heart, 7)
          << new BeanSoldiers(Card::Heart, 8)
          << new BeanSoldiers(Card::Heart, 9)
          << new BeanSoldiers(Card::Heart, 11);

    cards << new Graft(Card::Club, 12)
          << new Graft(Card::Club, 13);

    foreach (Card *card, cards)
        card->setParent(this);

    skills << new GanluSWZS;

    skills << new GodBladeSkill << new GodRobeSkill << new GodZitherSkill << new GodDiagramSkill << new GodHorseSkill
           << new GodHalberdSkill << new GodHalberdsSkill << new GodSwordSkill << new GodHatBuff << new GodHatSkill;

}

ADD_PACKAGE(GodsReturn)
ADD_PACKAGE(GodsReturnCard)
