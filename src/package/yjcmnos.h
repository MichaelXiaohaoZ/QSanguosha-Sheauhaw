#ifndef _YJCM_H
#define _YJCM_H

#include "package.h"
#include "card.h"
#include "skill.h"

class ZhenshanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhenshanCard();
    bool targetFixed() const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    const Card *validate(CardUseStruct &cardUse) const;
    const Card *validateInResponse(ServerPlayer *user) const;
    static void askForExchangeHand(ServerPlayer *quancong);
    //static void inRoll(ServerPlayer *quancong);
};

class TsydyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TsydyiCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class AngwoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE AngwoCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class DinqpiinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DinqpiinCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void onEffect(const CardEffectStruct &effect) const;
};

class Shangshyh : public TriggerSkill
{
public:
    Shangshyh();
    bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *zhangchunhua, QVariant &data) const;

protected:
    virtual int getMaxLostHp(ServerPlayer *zhangchunhua) const;
};

class JieyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JieyueCard();

    void onEffect(const CardEffectStruct &effect) const;
};

class NostalYJCM2015Package : public Package
{
    Q_OBJECT

public:
    NostalYJCM2015Package();
};

class NostalYJCM2014Package : public Package
{
    Q_OBJECT

public:
    NostalYJCM2014Package();
};

class YJCM2012msPackage : public Package
{
    Q_OBJECT

public:
    YJCM2012msPackage();
};

class YJCMmsPackage : public Package
{
    Q_OBJECT

public:
    YJCMmsPackage();
};

class NostalOLGeneralPackage : public Package
{
    Q_OBJECT

public:
    NostalOLGeneralPackage();
};

#endif

