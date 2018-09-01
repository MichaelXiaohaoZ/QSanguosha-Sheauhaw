#ifndef ATTACKDONG_H
#define ATTACKDONG_H

#include "package.h"
#include "card.h"
#include "standard.h"

class KuangxiCard: public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE KuangxiCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class AttackDongPackage : public Package
{
    Q_OBJECT

public:
    AttackDongPackage();
};



#endif


