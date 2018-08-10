#ifndef NewLegend_PACKAGE_H
#define NewLegend_PACKAGE_H

#include "package.h"
#include "card.h"

class NewLegendPackage : public Package
{
    Q_OBJECT

public:
    NewLegendPackage();
};

class NewQiangxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NewQiangxiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual void extraCost(Room *room, const CardUseStruct &card_use) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NewLianhuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NewLianhuanCard();

	virtual bool targetFixed() const;
	virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
	virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
	virtual const Card *validate(CardUseStruct &cardUse) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class NewNiepanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NewNiepanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NewZaiqiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NewZaiqiCard();

	virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NewDimengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NewDimengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

#endif
