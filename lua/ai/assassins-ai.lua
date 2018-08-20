sgs.ai_skill_invoke.nosfenxin = function(self, data)
	local target = data:toPlayer()
	local target_role = sgs.evaluatePlayerRole(target)
	local self_role = self.player:getRole()
	if target_role == "renegade" or target_role == "neutral" then return false end
	local process = sgs.gameProcess(self.room)
	return (target_role == "rebel" and self.role ~= "rebel" and process:match("rebel"))
			or (target_role == "loyalist" and self.role ~= "loyalist" and process:match("loyal"))
end

local mixin_skill = {}
mixin_skill.name = "mixin"
table.insert(sgs.ai_skills, mixin_skill)
mixin_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("MixinCard") or self.player:isKongcheng() then return end
	if self:needBear() then return end
	if #self.friends_noself == 0 then return end
	return sgs.Card_Parse("@MixinCard=.")
end

sgs.ai_skill_use_func.MixinCard = function(card, use, self)
	local cards = self.player:getHandcards()
	cards = sgs.QList2Table(cards)
	if #self.enemies < 1 then return end
	local slash
	self:sortByKeepValue(cards)
	for _, acard in ipairs(cards) do
		if acard:isKindOf("Slash") then
			slash = acard
			break
		end
	end

	if slash then
		for _, friend in ipairs(self.friends_noself) do
			if friend:hasSkills("tuntian+zaoxian") and not friend:hasSkill("manjuan") then
				use.card = sgs.Card_Parse("@MixinCard="..slash:getEffectiveId())
				if use.to then use.to:append(friend) end
				return
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if not friend:hasSkill("manjuan") then
				use.card = sgs.Card_Parse("@MixinCard="..slash:getEffectiveId())
				if use.to then use.to:append(friend) end
				return
			end
		end
	else
		local compare_more_slash = function(a, b)
			return getCardsNum("Slash", a) > getCardsNum("Slash", b)
		end
		table.sort(self.friends_noself, compare_more_slash)
		for _, friend in ipairs(self.friends_noself) do
			if not friend:hasSkill("manjuan") and getCardsNum("Slash", friend) >= 1 then
				use.card = sgs.Card_Parse("@MixinCard="..cards[1]:getEffectiveId())
				if use.to then use.to:append(friend) end
				return
			end
		end
		for _, friend in ipairs(self.friends_noself) do
			if friend:hasSkills("tuntian+zaoxian") and not friend:hasSkill("manjuan") then
				use.card = sgs.Card_Parse("@MixinCard="..cards[1]:getEffectiveId())
				if use.to then use.to:append(friend) end
				return
			end
		end
	end
end

sgs.ai_skill_playerchosen.mixin = sgs.ai_skill_playerchosen.zero_card_as_slash

sgs.ai_skill_cardask["#mixin"] = function(self, data, pattern, target)
	if target then
		for _, slash in ipairs(self:getCards("Slash")) do
			if self:isFriend(target) and self:slashIsEffective(slash, target) then
				if self:needLeiji(target, self.player) then return slash:toString() end
				if self:getDamagedEffects(target, self.player) then return slash:toString() end
				if self:needToLoseHp(target, self.player, nil, true) then return slash:toString() end
			end

			if not self:isFriend(target) and self:slashIsEffective(slash, target)
				and not self:getDamagedEffects(target, self.player, true) and not self:needLeiji(target, self.player) then
					return slash:toString()
			end
		end
		for _, slash in ipairs(self:getCards("Slash")) do
			if not self:isFriend(target) then
				if not self:needLeiji(target, self.player) then return slash:toString() end
				if not self:slashIsEffective(slash, target) then return slash:toString() end
			end
		end
	end
	return "."
end

sgs.ai_use_priority.MixinCard = 0
sgs.ai_card_intention.MixinCard = -20

sgs.ai_skill_invoke.cangni = function(self, data)
	local target = self.room:getCurrent()
	if self.player:hasFlag("cangnilose") then return self:isEnemy(target) end
	if self.player:hasFlag("cangniget") then return self:isFriend(target) end
	local hand = self.player:getHandcardNum()
	local hp = self.player:getHp()
	return (hand + 2) <= hp or self.player:isWounded();
end

sgs.ai_skill_choice.cangni = function(self, choices)
	local hand = self.player:getHandcardNum()
	local hp = self.player:getHp()
	if (hand + 2) <= hp then
		return "draw"
	else
		return "recover"
	end
end

duyi_skill = {}
duyi_skill.name = "duyi"
table.insert(sgs.ai_skills, duyi_skill)
duyi_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("DuyiCard") then return end
	return sgs.Card_Parse("@DuyiCard=.")
end

sgs.ai_skill_use_func.DuyiCard = function(card,use,self)
	use.card = card
end

sgs.ai_skill_playerchosen.duyi = function(self, targets)
	if sgs.ai_duyi then
		local tg = sgs.ai_duyi.tg
		local card = sgs.ai_duyi.id and sgs.Sanguosha:getCard(sgs.ai_duyi.id)
		sgs.ai_duyi = nil
		if card and card:isBlack() and tg then return tg end
	end
	if self:needBear() then return self.player end
	local to
	if self:getOverflow() < 0 then
		to = self:findPlayerToDraw(true)
	else
		to = self:findPlayerToDraw(false)
	end
	if to then return to
	else return self.player
	end
end

sgs.ai_playerchosen_intention.duyi = function(self, from, to)
	if sgs.ai_duyi then
		local card = sgs.ai_duyi.id and sgs.Sanguosha:getCard(sgs.ai_duyi.id)
		if card:isRed() then
			sgs.updateIntention(from, to, -80)
		end
	end
	sgs.updateIntention(from, to, -10)
end

sgs.ai_skill_invoke.duanzhi = function(self, data)
	local use = data:toCardUse()
	if use.from and self:isEnemy(use.from) and use.card:getSubtype() == "attack_card" and self.player:getHp() == 1 and not self:getCard("Peach")
		and not self:getCard("Analeptic") and not isLord(self.player) and self:getAllPeachNum() == 0 then
		self.player:setFlags("AI_doNotSave")
		return true
	end
	return use.from and self:isEnemy(use.from) and not self:doNotDiscard(use.from, "he", true, 2) and self.player:getHp() > 2
end

sgs.ai_skill_choice.duanzhi = function(self, choices)
	return "discard"
end

sgs.ai_skill_cardask["@fengyin"] = function(self, data) --yun
    if self:needBear() then return "." end
    local cards = self.player:getHandcards()
    local card
    cards = sgs.QList2Table(cards)
 
    for _,acard in ipairs(cards)  do
        if acard:isKindOf("Slash") then
            card = acard
            break
        end
    end
 
    if not card then
        return "."
    end
 
    local card_id = card:getEffectiveId()
 
    local target = self.room:getCurrent()
    if self:isFriend(target) and self:willSkipPlayPhase(target) and target:getHandcardNum() + 2 > target:getHp() and target:getHp() >= self.player:getHp() then
        return "$" .. card_id
    end
    if self:isEnemy(target) and not target:hasSkill("yongsi") and not self:willSkipPlayPhase(target) and target:getHandcardNum() >= target:getHp()
        and target:getHp() >= self.player:getHp() then
        return "$" .. card_id
    end
    return "."
end