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

class YearBossPackage : public Package
{
    Q_OBJECT

public:
    YearBossPackage();

};

#endif // YEARBOSS

