#include "tw.h"
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

class Yinqin : public PhaseChangeSkill
{
public:
    Yinqin() : PhaseChangeSkill("yinqin")
    {

    }

    bool triggerable(const ServerPlayer *target) const
    {
        return PhaseChangeSkill::triggerable(target) && target->getPhase() == Player::Start;
    }

    bool onPhaseChange(ServerPlayer *target) const
    {
        Room *room = target->getRoom();
        QString kingdom = target->getKingdom() == "wei" ? "shu" : target->getKingdom() == "shu" ? "wei" : "wei+shu";
        if (target->askForSkillInvoke(this)) {
            kingdom = room->askForChoice(target, objectName(), kingdom);
            room->broadcastSkillInvoke(objectName());
            room->notifySkillInvoked(target, objectName());
            room->setPlayerProperty(target, "kingdom", kingdom);
        }

        return false;
    }
};

class TWBaobian : public TriggerSkill
{
public:
    TWBaobian() : TriggerSkill("twbaobian")
    {
        events << DamageCaused;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card != NULL && (damage.card->isKindOf("Slash") || damage.card->isKindOf("Duel")) && !damage.chain && !damage.transfer && damage.by_user) {
            if (damage.to->getKingdom() == player->getKingdom()) {
                if (player->askForSkillInvoke(this, data)) {
                    if (damage.to->getHandcardNum() < damage.to->getMaxHp()) {
                        room->broadcastSkillInvoke(objectName());
                        int n = damage.to->getMaxHp() - damage.to->getHandcardNum();
                        room->drawCards(damage.to, n, objectName());
                    }
                    return true;
                }
            } else if (damage.to->getHandcardNum() > qMax(damage.to->getHp(), 0) && player->canDiscard(damage.to, "h")) {
                // Seems it is no need to use FakeMoveSkill & Room::askForCardChosen, so we ignore it.
                // If PlayerCardBox has changed for Room::askForCardChosen, please tell me, I will soon fix this.
                if (player->askForSkillInvoke(this, data)) {
                    room->broadcastSkillInvoke(objectName());
                    QList<int> hc = damage.to->handCards();
                    qShuffle(hc);
                    int n = damage.to->getHandcardNum() - qMax(damage.to->getHp(), 0);
                    QList<int> to_discard = hc.mid(0, n - 1);
                    DummyCard dc(to_discard);
                    room->throwCard(&dc, damage.to, player);
                }
            }
        }

        return false;
    }
};

class Tijin : public TriggerSkill
{
public:
    Tijin() : TriggerSkill("tijin")
    {
        events << TargetSpecifying << CardFinished;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *, QVariant &data) const
    {
        CardUseStruct use = data.value<CardUseStruct>();
        if (triggerEvent == TargetSpecifying) {
            if (use.from != NULL && use.card != NULL && use.card->isKindOf("Slash") && use.to.length() == 1) {
                ServerPlayer *zumao = room->findPlayerBySkillName(objectName());
                if (!TriggerSkill::triggerable(zumao) || use.from == zumao || !use.from->inMyAttackRange(zumao))
                    return false;

                if (!use.from->tag.value("tijin").canConvert(QVariant::Map))
                    use.from->tag["tijin"] = QVariantMap();

                QVariantMap tijin_map = use.from->tag.value("tijin").toMap();
                if (tijin_map.contains(use.card->toString())) {
                    tijin_map.remove(use.card->toString());
                    use.from->tag["tijin"] = tijin_map;
                }

                if (zumao->askForSkillInvoke(this, data)) {
                    room->broadcastSkillInvoke(objectName());
                    use.to.first()->removeQinggangTag(use.card);
                    use.to.clear();
                    use.to << zumao;

                    data = QVariant::fromValue(use);

                    tijin_map[use.card->toString()] = QVariant::fromValue(zumao);
                    use.from->tag["tijin"] = tijin_map;
                }
            }
        } else {
            if (use.from != NULL && use.card != NULL) {
                QVariantMap tijin_map = use.from->tag.value("tijin").toMap();
                if (tijin_map.contains(use.card->toString())) {
                    ServerPlayer *zumao = tijin_map.value(use.card->toString()).value<ServerPlayer *>();
                    if (zumao != NULL && zumao->isAlive() && zumao->canDiscard(use.from, "he")) {
                        int id = room->askForCardChosen(zumao, use.from, "he", objectName(), false, Card::MethodDiscard);
                        room->throwCard(id, use.from, zumao);
                    }
                }
                tijin_map.remove(use.card->toString());
                use.from->tag["tijin"] = tijin_map;
            }

        }

        return false;
    }
};

class Xiaolian : public TriggerSkill
{
public:
    Xiaolian() : TriggerSkill("xiaolian")
    {
        events << TargetConfirming << Damaged;
    }

    bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL && target->isAlive();
    }

    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (triggerEvent == TargetConfirming) {
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card != NULL && use.card->isKindOf("Slash") && use.to.length() == 1) {
                ServerPlayer *caoang = room->findPlayerBySkillName(objectName());
                if (!TriggerSkill::triggerable(caoang) || use.to.first() == caoang)
                    return false;

                if (!caoang->tag.value("xiaolian").canConvert(QVariant::Map))
                    caoang->tag["xiaolian"] = QVariantMap();

                QVariantMap xiaolian_map = caoang->tag.value("xiaolian").toMap();
                if (xiaolian_map.contains(use.card->toString())) {
                    xiaolian_map.remove(use.card->toString());
                    caoang->tag["xiaolian"] = xiaolian_map;
                }

                if (caoang->askForSkillInvoke(this, data)) {
                    room->broadcastSkillInvoke(objectName());
                    ServerPlayer *target = use.to.first();
                    use.to.first()->removeQinggangTag(use.card);
                    use.to.clear();
                    use.to << caoang;

                    data = QVariant::fromValue(use);

                    xiaolian_map[use.card->toString()] = QVariant::fromValue(target);
                    caoang->tag["xiaolian"] = xiaolian_map;
                }
            }
        } else {
            DamageStruct damage = data.value<DamageStruct>();
            if (damage.card != NULL) {
                if (!player->tag.value("xiaolian").canConvert(QVariant::Map))
                    return false;

                QVariantMap xiaolian_map = player->tag.value("xiaolian").toMap();
                if (xiaolian_map.contains(damage.card->toString())) {
                    ServerPlayer *target = xiaolian_map.value(damage.card->toString()).value<ServerPlayer *>();
                    if (target != NULL && player->getCardCount(true) > 0) {
                        const Card *c = room->askForExchange(player, objectName(), 1, 1, true, "@xiaolian-put", true);
                        if (c != NULL)
                            target->addToPile("xlhorse", c);
                        delete c;
                    }
                }
                xiaolian_map.remove(damage.card->toString());
                player->tag["xiaolian"] = xiaolian_map;
            }
        }

        return false;
    }
};

class XiaolianDist : public DistanceSkill
{
public:
    XiaolianDist() : DistanceSkill("#xiaolian-dist")
    {

    }

    int getCorrect(const Player *from, const Player *to) const
    {
        if (from != to)
            return to->getPile("xlhorse").length();

        return 0;
    }
};

TaiwanYJCMPackage::TaiwanYJCMPackage()
: Package("Taiwan_yjcm")
{
    General *xiahb = new General(this, "twyj_xiahouba", "shu"); // TAI 001
    xiahb->addSkill(new Yinqin);
    xiahb->addSkill(new TWBaobian);

    General *zumao = new General(this, "twyj_zumao", "wu"); // TAI 002
    zumao->addSkill(new Tijin);

    General *caoang = new General(this, "twyj_caoang", "wei"); // TAI 003
    caoang->addSkill(new XiaolianDist);
    caoang->addSkill(new Xiaolian);
    related_skills.insertMulti("xiaolian", "#xiaolian-dist");
}

ADD_PACKAGE(TaiwanYJCM)
