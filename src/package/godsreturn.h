#ifndef GODSRETURN_H
#define GODSRETURN_H

#include "package.h"
#include "card.h"
#include "standard.h"

class GodBlade: public Weapon {
    Q_OBJECT

public:
    Q_INVOKABLE GodBlade(Card::Suit suit, int number);
};

class GodHalberd: public Weapon {
    Q_OBJECT

public:
    Q_INVOKABLE GodHalberd(Card::Suit suit, int number);
};

class GodZither: public Weapon {
    Q_OBJECT

public:
    Q_INVOKABLE GodZither(Card::Suit suit, int number);
};

class GodSword: public Weapon {
    Q_OBJECT

public:
    Q_INVOKABLE GodSword(Card::Suit suit, int number);
};

class GodDiagram: public Armor {
    Q_OBJECT

public:
    Q_INVOKABLE GodDiagram(Card::Suit suit, int number);
};

class GodHorse : public DefensiveHorse
{
    Q_OBJECT

public:
    Q_INVOKABLE GodHorse(Card::Suit suit, int number);
};

class GodRobe: public Armor {
    Q_OBJECT

public:
    Q_INVOKABLE GodRobe(Card::Suit suit, int number);
};

class GodHat: public Treasure {
    Q_OBJECT

public:
    Q_INVOKABLE GodHat(Card::Suit suit, int number);
};

class BeanSoldiers: public SingleTargetTrick {
    Q_OBJECT

public:
    Q_INVOKABLE BeanSoldiers(Card::Suit suit, int number);

    virtual bool isAvailable(const Player *player) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class Graft: public SingleTargetTrick {
    Q_OBJECT

public:
    Q_INVOKABLE Graft(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class GodsReturnPackage : public Package
{
    Q_OBJECT

public:
    GodsReturnPackage();
};

class GodsReturnCardPackage : public Package
{
    Q_OBJECT

public:
    GodsReturnCardPackage();
};



#endif


