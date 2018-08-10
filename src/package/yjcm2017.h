#ifndef _YJCM2017_H
#define _YJCM2017_H

#include "package.h"
#include "card.h"

class YJCM2017Package : public Package
{
    Q_OBJECT

public:
    YJCM2017Package();
};

class ZhongjianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhongjianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FumianTargetCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FumianTargetCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &use) const;
};

class WenguaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WenguaCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class WenguaAttachCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WenguaAttachCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class HuiminGraceCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuiminGraceCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TongboCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TongboCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class TongboAllotCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TongboAllotCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TianbianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianbianCard();

    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

#endif
