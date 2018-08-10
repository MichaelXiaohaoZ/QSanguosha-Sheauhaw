#ifndef OL_PACKAGE_H
#define OL_PACKAGE_H

#include "package.h"
#include "card.h"
#include "standard.h"

class OLPackage : public Package
{
    Q_OBJECT

public:
    OLPackage();
};

class AocaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE AocaiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validateInResponse(ServerPlayer *user) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
};

class DuwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DuwuCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ShefuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShefuCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LuanzhanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LuanzhanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class GusheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GusheCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZiyuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZiyuanCard();

    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DingpanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DingpanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LianzhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianzhuCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class ShanxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShanxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FumanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FumanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LianjiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LianjiCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class SmileDagger : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SmileDagger(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HoneyTrap : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE HoneyTrap(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class JingongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JingongCard();

    bool targetFixed() const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    const Card *validate(CardUseStruct &card_use) const;
};

class QianyaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QianyaCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class CanshibCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE CanshibCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif
