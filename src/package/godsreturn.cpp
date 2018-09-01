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

ADD_PACKAGE(GodsReturnCard)
