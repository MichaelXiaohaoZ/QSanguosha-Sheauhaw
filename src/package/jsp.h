#ifndef JSP_PACKAGE_H
#define JSP_PACKAGE_H

#include "package.h"
#include "card.h"
#include "standard.h"

class ShiuehjihCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShiuehjihCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JowfuhCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JowfuhCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class MeybuhFilter : public FilterSkill
{
public:
    MeybuhFilter(const QString &skill_name);

    bool viewFilter(const Card *to_select) const;

    const Card *viewAs(const Card *originalCard) const;

private:
    QString n;
};

class XiemuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiemuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class JSPPackage : public Package
{
    Q_OBJECT

public:
    JSPPackage();
};

#endif
