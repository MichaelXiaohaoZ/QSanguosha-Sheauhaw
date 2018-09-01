#include "attackdong.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "maneuvering.h"
#include "roomscene.h"
#include "special3v3.h"

class Jielve : public TriggerSkill
{
public:
    Jielve() : TriggerSkill("jielve")
    {
        events << Damage;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        if (!damage.to->isAllNude() && damage.to != player) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE,player->objectName(), damage.to->objectName());
            DummyCard *dummy = new DummyCard;
            if (!damage.to->isKongcheng()) {
                int id1 = room->askForCardChosen(player, damage.to, "h", objectName());
                dummy->addSubcard(id1);
            }
            if (!damage.to->getEquips().isEmpty()) {
                int id2 = room->askForCardChosen(player, damage.to, "e", objectName());
                dummy->addSubcard(id2);
            }
            if (!damage.to->getJudgingArea().isEmpty()) {
                int id3 = room->askForCardChosen(player, damage.to, "j", objectName());
                dummy->addSubcard(id3);
            }
            if (dummy->subcardsLength() > 0) {
                room->obtainCard(player, dummy, CardMoveReason(CardMoveReason::S_REASON_EXTRACTION, player->objectName()), false);
                room->loseHp(player);
            }
        }
        return false;
    }
};

class Longying : public TriggerSkill
{
public:
    Longying() : TriggerSkill("longying")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getHp() > 0 && room->getLord() && room->getLord()->isWounded() && player->getPhase() == Player::Play) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), room->getLord()->objectName());
            room->loseHp(player);
            room->recover(room->getLord(), RecoverStruct(player));
            room->getLord()->drawCards(1, objectName());
        }
        return false;
    }
};

class Fangong : public TriggerSkill {
public:
    Fangong() : TriggerSkill("fangong") {
        events << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const {
        return target != NULL && target->isAlive();
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const {
        CardUseStruct use = data.value<CardUseStruct>();
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (TriggerSkill::triggerable(p) && use.from && use.from != p && use.card && use.to.contains(p) && !use.card->isKindOf("Jink"))
                room->askForUseSlashTo(p, player, QString("@fangong-slash:%1:%2").arg(p->objectName()).arg(player->objectName()), false);
        }
        return false;
    }
};

class Huying : public TriggerSkill
{
public:
    Huying() : TriggerSkill("huying")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Play && room->getLord()) {
            QList<int> ids;
            foreach (const Card *card, player->getHandcards()) {
                if (card->isKindOf("Slash"))
                    ids << card->getId();
            }
            QList<ServerPlayer *> players;
            players << room->getLord();
               if (!room->askForYiji(player, ids, objectName(), false, false, true, 1, players, CardMoveReason(), "@huying-distribute", true)) {
                room->sendCompulsoryTriggerLog(player, objectName());
                room->loseHp(player);
                QList<int> slashes;
                   foreach (int card_id, room->getDrawPile()) {
                    if (Sanguosha->getCard(card_id)->isKindOf("Slash"))
                        slashes << card_id;
                }
                if (!slashes.isEmpty())
                    room->getLord()->obtainCard(Sanguosha->getCard(slashes.at(qrand() % slashes.length())));
            }
        }
        return false;
    }
};

class Tunjun : public TriggerSkill
{
public:
    Tunjun() : TriggerSkill("tunjun")
    {
        events << RoundStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getMaxHp() != 1) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            room->loseMaxHp(player);
            player->drawCards(player->getMaxHp());
        }
        return false;
    }
};

class Jiaoxia: public MaxCardsSkill {
public:
    Jiaoxia(): MaxCardsSkill("jiaoxia") {
    }

    virtual int getExtra(const Player *target) const{
        int m = 0;
        foreach (const Player *p, target->getAliveSiblings()) {
            if (p->hasSkill(objectName()) && p->isYourFriend(target) || target->hasSkill(objectName())) {
                foreach (const Card *card, target->getHandcards()) {
                    if (card->isBlack())
                        ++m;
                }
                break;
            }
        }
        return m;
    }
};

class Fengying: public ProhibitSkill {
public:
    Fengying(): ProhibitSkill("fengying") {
    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card, const QList<const Player *> &) const
    {
        QList<const Player*> bosses = to->getAliveSiblings();
        if (to->isAlive()) bosses << to;
        foreach (const Player *fengyao, bosses)
            if (isFriendEach(fengyao, to) && fengyao->hasSkill(objectName()))
            {
                foreach (const Player *boss, to->getAliveSiblings())
                    if (isFriendEach(fengyao, boss) && boss->getHp() <= to->getHp())
                        return false;
                return !(isFriendEach(from, to));
            }
        return false;
    }

    virtual bool isFriendEach(const Player *player, const Player *to) const
    {
        if ((player->isLord() || player->getRole() == "loyalist") && (to->isLord() || to->getRole() == "loyalist"))
            return true;
        if (player->getRole() == to->getRole())
            return true;
        return false;
    }
};

KuangxiCard::KuangxiCard() {
}

bool KuangxiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
     return Self != to_select;
}

void KuangxiCard::onEffect(const CardEffectStruct &effect) const{
    effect.from->getRoom()->damage(DamageStruct("kuangxi", effect.from, effect.to));
    effect.from->getRoom()->loseHp(effect.from);
}

class KuangxiViewAsSkill: public ZeroCardViewAsSkill {
public:
    KuangxiViewAsSkill(): ZeroCardViewAsSkill("kuangxi") {
    }

    virtual const Card *viewAs() const{
        return new KuangxiCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getHp() > 0 && !player->hasFlag("KuangxiEnterDying");
    }
};

class Kuangxi: public TriggerSkill {
public:
    Kuangxi(): TriggerSkill("kuangxi") {
        events << QuitDying;
        view_as_skill = new KuangxiViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *, QVariant &data) const{
        DyingStruct dying = data.value<DyingStruct>();
        if (dying.damage && dying.damage->getReason() == "kuangxi" && !dying.damage->chain && !dying.damage->transfer) {
            ServerPlayer *from = dying.damage->from;
            if (from && from->isAlive())
                room->setPlayerFlag(from, "KuangxiEnterDying");
        }
        return false;
    }
};

class Baoying : public TriggerSkill
{
public:
    Baoying() : TriggerSkill("baoying")
    {
        frequency = Limited;
        events << EnterDying;
        limit_mark = "@baoying";
    }

     virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (TriggerSkill::triggerable(p) && p->getMark("@baoying") > 0 && room->askForSkillInvoke(p, objectName(), data)) {
                room->recover(player, RecoverStruct(p, NULL, 1 - player->getHp()));
                room->removePlayerMark(p, "@baoying");
            }
        }
        return false;
    }
};

class Yangwu : public TriggerSkill
{
public:
    Yangwu() : TriggerSkill("yangwu")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Start) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
            }
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                room->damage(DamageStruct(objectName(), player, p));
            }
            room->loseHp(player);
        }
        return false;
    }
};

class Yanglie : public TriggerSkill
{
public:
    Yanglie() : TriggerSkill("yanglie")
    {
        events << EventPhaseStart;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getPhase() == Player::Start) {
            room->broadcastSkillInvoke(objectName());
            room->sendCompulsoryTriggerLog(player, objectName());
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE, player->objectName(), p->objectName());
            }
            foreach (ServerPlayer *p, room->getOtherPlayers(player)) {
                if (!p->isAllNude()) {
                    int card_id = room->askForCardChosen(player, p, "hej", objectName());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
                }
            }
            room->loseHp(player);
        }
        return false;
    }
};

class Ruiqi: public DrawCardsSkill {
public:
    Ruiqi(): DrawCardsSkill("ruiqi") {
        frequency = Compulsory;
    }

    virtual int getDrawNum(ServerPlayer *player, int n) const{
        Room *room = player->getRoom();
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (TriggerSkill::triggerable(p) && p->isYourFriend(player)) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(p, objectName());
                ++n;
            }
        }
        return n;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
};

class Jingqi: public DistanceSkill {
public:
    Jingqi(): DistanceSkill("jingqi") {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        foreach (const Player *p, from->getAliveSiblings()) {
            if (p->hasSkill(objectName()) && p->isYourFriend(from) && !p->isYourFriend(to))
                return -1;
        }
        return 0;
    }
};

class Mojun : public TriggerSkill
{
public:
    Mojun() : TriggerSkill("mojun")
    {
        events << Damage;
        frequency = Compulsory;
    }

    bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DamageStruct damage = data.value<DamageStruct>();
        foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
            if (damage.card && damage.card->isKindOf("Slash") && !damage.to->hasFlag("Global_DebutFlag") && !damage.chain && !damage.transfer && player->isYourFriend(p)) {
                room->broadcastSkillInvoke(objectName());
                room->sendCompulsoryTriggerLog(p, objectName());
                JudgeStruct judge;
                judge.who = player;
                judge.reason = objectName();
                judge.good = true;
                judge.pattern = ".|black";
                room->judge(judge);
                if (judge.isGood()) {
                    QList<ServerPlayer *> players;
                    foreach (ServerPlayer *pp, room->getAlivePlayers()) {
                        if (player->isYourFriend(pp)) {
                            room->doAnimate(QSanProtocol::S_ANIMATE_INDICATE,p->objectName(), pp->objectName());
                            players << pp;
                        }
                    }
                    room->sortByActionOrder(players);
                    room->drawCards(players, 1, objectName());
                }
            }
        }
        return false;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }
};

class Moqu: public TriggerSkill {
public:
    Moqu(): TriggerSkill("moqu") {
        events << EventPhaseChanging << Damaged;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room *room, ServerPlayer *player, QVariant &data) const{
        if (event == EventPhaseChanging) {
            PhaseChangeStruct change = data.value<PhaseChangeStruct>();
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (change.to == Player::NotActive && p->getHandcardNum() < p->getHp()) {
                    room->broadcastSkillInvoke(objectName());
                    room->sendCompulsoryTriggerLog(p, objectName());
                    p->drawCards(2, objectName());
                }
            }
        } else {
            foreach (ServerPlayer *p, room->findPlayersBySkillName(objectName())) {
                if (p->isYourFriend(player))
                    room->askForDiscard(p, objectName(), 1, 1, false, true);
            }
        }
        return false;
    }
};

class BossPolu: public TriggerSkill
{
public:
    BossPolu() : TriggerSkill("bosspolu")
    {
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const
    {
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasSkill(objectName()) && death.who->getRole() == "rebel" && death.who == player)
        {
            room->addPlayerMark(room->getLord(), "bosspolunum", 1);
            foreach (ServerPlayer *rebel, room->getAlivePlayers())
                if (rebel->getRole() == "rebel")
                    rebel->drawCards(room->getLord()->getMark("bosspolunum"), objectName());
        }
        else
            if (player->hasSkill(objectName()) && player->getRole() == "rebel" && death.damage->from
                    && death.damage->from->getRole() == "rebel" && death.who->getRole() != "rebel")
            {
                room->addPlayerMark(room->getLord(), "bosspolunum", 1);
                room->addPlayerMark(player, "#bosspolu", 1);
                foreach (ServerPlayer *rebel, room->getAlivePlayers())
                    if (rebel->getRole() == "rebel")
                        rebel->drawCards(player->getMark("#bosspolu"), objectName());
            }
        return false;
    }
};

class BossHuaxiongTurnStart : public TriggerSkill
{
public:
    BossHuaxiongTurnStart() : TriggerSkill("#bosshuaxiongturnstart")
    {
        events << TurnStart;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const
    {
        if (player->getHp() > 1)
            room->loseHp(player, 1);
        foreach (ServerPlayer *mate, room->getAlivePlayers())
            if (mate->getRole() == "loyalist" && mate->getHp() > 1)
                room->loseHp(mate, 1);
    }
};

AttackDongPackage::AttackDongPackage()
    : Package("AttackDong")
{

    General *zhangji = new General(this, "zhangji", "qun", 4, true, true);
    zhangji->addSkill(new Jielve);
    zhangji->addSkill(new Mojun);

    General *longxiang = new General(this, "longxiang", "qun", 4, true, true);
    longxiang->addSkill(new Longying);

    General *fanchou = new General(this, "fanchou", "qun", 4, true, true);
    fanchou->addSkill(new Fangong);
    fanchou->addSkill("mojun");

    General *huben = new General(this, "huben", "qun", 5, true, true);
    huben->addSkill(new Huying);

    General *niufudongxie = new General(this, "niufudongxie", "qun", 4, true, true);
    niufudongxie->addSkill(new Tunjun);
    niufudongxie->addSkill(new Jiaoxia);
    niufudongxie->addSkill("mojun");

    General *fengyao = new General(this, "fengyao", "qun", 3, false, true);
    fengyao->addSkill(new Fengying);

    General *dongyue = new General(this, "dongyue", "qun", 4, true, true);
    dongyue->addSkill(new Kuangxi);
    dongyue->addSkill("mojun");

    General *baolve = new General(this, "baolve", "qun", 3, true, true);
    baolve->addSkill(new Baoying);

    General *lijue = new General(this, "lijue", "qun", 5, true, true);
    lijue->addSkill(new Yangwu);
    lijue->addSkill("mojun");

    General *guosi = new General(this, "guosi", "qun", 4, true, true);
    guosi->addSkill(new Yanglie);
    guosi->addSkill("mojun");

    General *feixiong_left = new General(this, "feixiong_left", "qun", 4, true, true);
    feixiong_left->addSkill(new Jingqi);

    General *feixiong_right = new General(this, "feixiong_right", "qun", 4, true, true);
    feixiong_right->addSkill(new Ruiqi);

    General *boss_huaxiong = new General(this, "boss_huaxiong", "qun", 8, true, true);
    boss_huaxiong->addSkill(new Moqu);
    boss_huaxiong->addSkill("yaowu");
    boss_huaxiong->addSkill("mojun");
    boss_huaxiong->addSkill(new BossHuaxiongTurnStart);

    General *ai_sunjian = new General(this, "sp_sunjian", "qun", 6, true, true);
    ai_sunjian->addSkill("yinghun");
    ai_sunjian->addSkill(new BossPolu);

    addMetaObject<KuangxiCard>();
}

ADD_PACKAGE(AttackDong)
