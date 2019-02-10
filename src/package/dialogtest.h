#ifndef DIALOGTEST
#define DIALOGTEST

#include "package.h"
#include "card.h"
#include "guhuobox.h"

class NosZhenshanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NosZhenshanCard();
    bool targetFixed() const;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    const Card *validate(CardUseStruct &cardUse) const;
    const Card *validateInResponse(ServerPlayer *user) const;
    static void askForExchangeHand(ServerPlayer *quancong);
};

class DialogTsPackage : public Package
{
    Q_OBJECT

public:
    DialogTsPackage();
};

#endif // DIALOGTEST

