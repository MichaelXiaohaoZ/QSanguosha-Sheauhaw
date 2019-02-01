#ifndef YEARBOSS
#define YEARBOSS

#include "package.h"
#include "card.h"
#include "standard.h"

class YearZishuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YearZishuCard();

    bool targetFixed() const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    bool canUseFromOther(Room *room, ServerPlayer *source, ServerPlayer *target) const;
};

class YearYinhuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YearYinhuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YearChenlongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YearChenlongCard();

    void extraCost(Room *room, const CardUseStruct &card_use) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YearWeiyangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YearWeiyangCard();

    bool targetFixed() const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YearBoss18Package : public Package
{
    Q_OBJECT

public:
    YearBoss18Package();

};

class FireCracker : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE FireCracker(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual QList<ServerPlayer *> defaultTargets(Room *room, ServerPlayer *source) const;
    virtual bool isAvailable(const Player *player) const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;
};

class SpringCouplet : public SingleTargetTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE SpringCouplet(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual QList<ServerPlayer *> defaultTargets(Room *room, ServerPlayer *source) const;
    virtual bool isAvailable(const Player *player) const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;
};

class YearBoss18CardPackage : public Package
{
    Q_OBJECT

public:
    YearBoss18CardPackage();
};

class YearBoss19Package : public Package
{
    Q_OBJECT

public:
    YearBoss19Package();

};

class YearBoss19CardPackage : public Package
{
    Q_OBJECT

public:
    YearBoss19CardPackage();
};

#endif // YEARBOSS

