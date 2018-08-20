sgs.weapon_range.SPMoonSpear = 3

sgs.ai_skill_playerchosen.sp_moonspear = function(self, targets)
	targets = sgs.QList2Table(targets)
	self:sort(targets, "defense")
	for _, target in ipairs(targets) do
		if self:isEnemy(target) and self:damageIsEffective(target) and sgs.isGoodTarget(target, targets, self) then
			return target
		end
	end
	return nil
end

sgs.ai_playerchosen_intention.sp_moonspear = 80

sgs.weapon_range.MoonSpear = 3
sgs.ai_use_priority.MoonSpear = 2.635

sgs.ai_cardneed.shiuehjih = function(to, card)
	return to:getHandcardNum() < 3 and card:isRed()
end

local shiuehjih_skill = {}
shiuehjih_skill.name = "shiuehjih"
table.insert(sgs.ai_skills, shiuehjih_skill)
shiuehjih_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("ShiuehjihCard") then return end
	if not self.player:isWounded() then return end

	local card
	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)

	for _, acard in ipairs(cards) do
		if acard:isRed() then
			card = acard
			break
		end
	end
	if card then
		card = sgs.Card_Parse("@ShiuehjihCard=" .. card:getEffectiveId())
		return card
	end

	return nil
end

local function can_be_selected_as_target_shiuehjih(self, card, who)
	-- validation of rule
	if self.player:getWeapon() and self.player:getWeapon():getEffectiveId() == card:getEffectiveId() then
		if self.player:distanceTo(who, sgs.weapon_range[self.player:getWeapon():getClassName()] - self.player:getAttackRange(false)) > self.player:getAttackRange() then return false end
	elseif self.player:getOffensiveHorse() and self.player:getOffensiveHorse():getEffectiveId() == card:getEffectiveId() then
		if self.player:distanceTo(who, 1) > self.player:getAttackRange() then return false end
	elseif self.player:distanceTo(who) > self.player:getAttackRange() then
		return false
	elseif who:getMark("shibei") == 0 and who:hasSkill("shibei") then
		return false
	end
	-- validation of strategy
	if self:isEnemy(who) and self:damageIsEffective(who) and not self:cantbeHurt(who) and not self:getDamagedEffects(who) and not self:needToLoseHp(who) then
		if not self.player:hasSkill("jueqing") then
			if who:hasSkill("guixin") and (self.room:getAliveCount() >= 4 or not who:faceUp()) and not who:hasSkill("manjuan") then return false end
			if (who:hasSkill("ganglie") or who:hasSkill("neoganglie")) and (self.player:getHp() == 1 and self.player:getHandcardNum() <= 2) then return false end
			if who:hasSkill("jieming") then
				for _, enemy in ipairs(self.enemies) do
					if enemy:getHandcardNum() <= enemy:getMaxHp() - 2 and not enemy:hasSkill("manjuan") then return false end
				end
			end
			if who:hasSkill("fangzhu") then
				for _, enemy in ipairs(self.enemies) do
					if not enemy:faceUp() then return false end
				end
			end
			if who:hasSkill("yiji") then
				local huatuo = self.room:findPlayerBySkillName("jijiu")
				if huatuo and self:isEnemy(huatuo) and huatuo:getHandcardNum() >= 3 then
					return false
				end
			end
		end
		return true
	elseif self:isFriend(who) then
		if who:hasSkill("yiji") and not self.player:hasSkill("jueqing") then
			local huatuo = self.room:findPlayerBySkillName("jijiu")
			if (huatuo and self:isFriend(huatuo) and huatuo:getHandcardNum() >= 3 and huatuo ~= self.player)
				or (who:getLostHp() == 0 and who:getMaxHp() >= 3) then
				return true
			end
		end
		if who:hasSkill("hunzi") and who:getMark("hunzi") == 0 and who:objectName() == self.player:getNextAlive():objectName() and who:getHp() == 2 then
			return true
		end
		if self:cantbeHurt(who) and not self:damageIsEffective(who) and not (who:hasSkill("manjuan") and who:getPhase() == sgs.Player_NotActive) and not (who:hasSkill("kongcheng") and who:isKongcheng()) then
			return true
		end
		return false
	end
	return false
end

sgs.ai_skill_use_func.ShiuehjihCard = function(card, use, self)
	if self.player:getLostHp() == 0 or self.player:hasUsed("ShiuehjihCard") then return end
	self:sort(self.enemies)
	local to_use = false
	for _, enemy in ipairs(self.enemies) do
		if can_be_selected_as_target_shiuehjih(self, card, enemy) then
			to_use = true
			break
		end
	end
	if not to_use then
		for _, friend in ipairs(self.friends_noself) do
			if can_be_selected_as_target_shiuehjih(self, card, friend) then
				to_use = true
				break
			end
		end
	end
	if to_use then
		use.card = card
		if use.to then
			for _, enemy in ipairs(self.enemies) do
				if can_be_selected_as_target_shiuehjih(self, card, enemy) then
					use.to:append(enemy)
					if use.to:length() == self.player:getLostHp() then return end
				end
			end
			for _, friend in ipairs(self.friends_noself) do
				if can_be_selected_as_target_shiuehjih(self, card, friend) then
					use.to:append(friend)
					if use.to:length() == self.player:getLostHp() then return end
				end
			end
			assert(use.to:length() > 0)
		end
	end
end

sgs.ai_card_intention.ShiuehjihCard = function(self, card, from, tos)
	local room = from:getRoom()
	local huatuo = room:findPlayerBySkillName("jijiu")
	for _,to in ipairs(tos) do
		local intention = 60
		if to:hasSkill("yiji") and not from:hasSkill("jueqing") then
			if (huatuo and self:isFriend(huatuo) and huatuo:getHandcardNum() >= 3 and huatuo:objectName() ~= from:objectName()) then
				intention = -30
			end
			if to:getLostHp() == 0 and to:getMaxHp() >= 3 then
				intention = -10
			end
		end
		if to:hasSkill("hunzi") and to:getMark("hunzi") == 0 then
			if to:objectName() == from:getNextAlive():objectName() and to:getHp() == 2 then
				intention = -20
			end
		end
		if self:cantbeHurt(to) and not self:damageIsEffective(to) then intention = -20 end
		sgs.updateIntention(from, to, intention)
	end
end

sgs.ai_use_value.ShiuehjihCard = 3
sgs.ai_use_priority.ShiuehjihCard = 2.35

sgs.ai_skill_cardask["@yannyuu-discard"] = function(self, data)
	if self.player:getHandcardNum() < 3 and self.player:getPhase() ~= sgs.Player_Play then
		if self:needToThrowArmor() then return "$" .. self.player:getArmor():getEffectiveId()
		elseif self:needKongcheng(self.player, true) and self.player:getHandcardNum() == 1 then return "$" .. self.player:handCards():first()
		else return "." end
	end
	local current = self.room:getCurrent()
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByKeepValue(cards)
	if current:objectName() == self.player:objectName() then
		local ex_nihilo, savage_assault, archery_attack
		for _, card in ipairs(cards) do
			if card:isKindOf("ExNihilo") then ex_nihilo = card
			elseif card:isKindOf("SavageAssault") and not current:hasSkills("noswuyan|wuyan") then savage_assault = card
			elseif card:isKindOf("ArcheryAttack") and not current:hasSkills("noswuyan|wuyan") then archery_attack = card
			end
		end
		if savage_assault and self:getAoeValue(savage_assault) <= 0 then savage_assault = nil end
		if archery_attack and self:getAoeValue(archery_attack) <= 0 then archery_attack = nil end
		local aoe = archery_attack or savage_assault
		if ex_nihilo then
			for _, card in ipairs(cards) do
				if card:getTypeId() == sgs.Card_TypeTrick and not card:isKindOf("ExNihilo") and card:getEffectiveId() ~= ex_nihilo:getEffectiveId() then
					return "$" .. card:getEffectiveId()
				end
			end
		end
		if self.player:isWounded() then
			local peach
			for _, card in ipairs(cards) do
				if card:isKindOf("Peach") then
					peach = card
					break
				end
			end
			local dummy_use = { isDummy = true }
			self:useCardPeach(peach, dummy_use)
			if dummy_use.card and dummy_use.card:isKindOf("Peach") then
				for _, card in ipairs(cards) do
					if card:getTypeId() == sgs.Card_TypeBasic and card:getEffectiveId() ~= peach:getEffectiveId() then
						return "$" .. card:getEffectiveId()
					end
				end
			end
		end
		if aoe then
			for _, card in ipairs(cards) do
				if card:getTypeId() == sgs.Card_TypeTrick and card:getEffectiveId() ~= aoe:getEffectiveId() then
					return "$" .. card:getEffectiveId()
				end
			end
		end
		if self:getCardsNum("Slash") > 1 then
			for _, card in ipairs(cards) do
				if card:objectName() == "slash" then
					return "$" .. card:getEffectiveId()
				end
			end
		end
	else
		local throw_trick
		local aoe_type
		if getCardsNum("ArcheryAttack", current, self.player) >= 1 and not current:hasSkills("noswuyan|wuyan") then aoe_type = "archery_attack" end
		if getCardsNum("SavageAssault", current, self.player) >= 1 and not current:hasSkills("noswuyan|wuyan") then aoe_type = "savage_assault" end
		if aoe_type then
			local aoe = sgs.Sanguosha:cloneCard(aoe_type)
			if self:getAoeValue(aoe, current) > 0 then throw_trick = true end
		end
		if getCardsNum("ExNihilo", current, self.player) > 0 then throw_trick = true end
		if throw_trick then
			for _, card in ipairs(cards) do
				if card:getTypeId() == sgs.Card_TypeTrick and not isCard("ExNihilo", card, self.player) then
					return "$" .. card:getEffectiveId()
				end
			end
		end
		if self:getCardsNum("Slash") > 1 then
			for _, card in ipairs(cards) do
				if card:objectName() == "slash" then
					return "$" .. card:getEffectiveId()
				end
			end
		end
		if self:getCardsNum("Jink") > 1 then
			for _, card in ipairs(cards) do
				if card:isKindOf("Jink") then
					return "$" .. card:getEffectiveId()
				end
			end
		end
		if self.player:getHp() >= 3 and (self.player:getHandcardNum() > 3 or self:getCardsNum("Peach") > 0) then
			for _, card in ipairs(cards) do
				if card:isKindOf("Slash") then
					return "$" .. card:getEffectiveId()
				end
			end
		end
		if getCardsNum("TrickCard", current, self.player) - getCardsNum("Nullification", current, self.player) > 0 then
			for _, card in ipairs(cards) do
				if card:getTypeId() == sgs.Card_TypeTrick and not isCard("ExNihilo", card, self.player) then
					return "$" .. card:getEffectiveId()
				end
			end
		end
	end
	if self:needToThrowArmor() then return "$" .. self.player:getArmor():getEffectiveId() else return "." end
end

sgs.ai_skill_askforag.yannyuu = function(self, card_ids)
	local cards = {}
	for _, id in ipairs(card_ids) do
		table.insert(cards, sgs.Sanguosha:getEngineCard(id))
	end
	self.yannyuu_need_player = nil
	local card, player = self:getCardNeedPlayer(cards, true)
	if card and player then
		self.yannyuu_need_player = player
		return card:getEffectiveId()
	end
	return -1
end

sgs.ai_skill_playerchosen.yannyuu = function(self, targets)
	local only_id = self.player:getMark("YannyuuOnlyId") - 1
	if only_id < 0 then
		assert(self.yannyuu_need_player ~= nil)
		return self.yannyuu_need_player
	else
		local card = sgs.Sanguosha:getEngineCard(only_id)
		if card:getTypeId() == sgs.Card_TypeTrick and not card:isKindOf("Nullification") then
			return self.player
		end
		local cards = { card }
		local c, player = self:getCardNeedPlayer(cards, true)
		return player
	end
end

sgs.ai_playerchosen_intention.yannyuu = function(self, from, to)
	if hasManjuanEffect(to) then return end
	local intention = -60
	if self:needKongcheng(to, true) then intention = 10 end
	sgs.updateIntention(from, to, intention)
end

sgs.ai_skill_invoke.xiaode = function(self, data)
	local round = self:playerGetRound(self.player)
	local xiaode_skill = sgs.ai_skill_choice.huashen(self, table.concat(data:toStringList(), "+"), nil, math.random(1 - round, 7 - round))
	if xiaode_skill then
		sgs.xiaode_choice = xiaode_skill
		return true
	else
		sgs.xiaode_choice = nil
		return false
	end
end

sgs.ai_skill_choice.xiaode = function(self, choices)
	return sgs.xiaode_choice
end

local jowfuh_skill = {}
jowfuh_skill.name = "jowfuh"
table.insert(sgs.ai_skills, jowfuh_skill)
jowfuh_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("JowfuhCard") or self.player:isKongcheng() then return end
	return sgs.Card_Parse("@JowfuhCard=.")
end

sgs.ai_skill_use_func.JowfuhCard = function(card, use, self)
	local cards = {}
	for _, card in sgs.qlist(self.player:getHandcards()) do
		table.insert(cards, sgs.Sanguosha:getEngineCard(card:getEffectiveId()))
	end
	self:sortByKeepValue(cards)
	self:sort(self.friends_noself)
	local zhenji
	for _, friend in ipairs(self.friends_noself) do
		if friend:getPile("incantation"):length() > 0 then continue end
		local reason = getNextJudgeReason(self, friend)
		if reason then
			if reason == "luoshen" then
				zhenji = friend
			elseif reason == "indulgence" then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Heart or (friend:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade)
						and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "supply_shortage" then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Club and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "lightning" and not friend:hasSkills("hongyan|wuyan") then
				for _, card in ipairs(cards) do
					if (card:getSuit() ~= sgs.Card_Spade or card:getNumber() == 1 or card:getNumber() > 9)
						and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "nosmiji" then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Club or (card:getSuit() == sgs.Card_Spade and not friend:hasSkill("hongyan")) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "nosqianxi" or reason == "tuntian" then
				for _, card in ipairs(cards) do
					if (card:getSuit() ~= sgs.Card_Heart and not (card:getSuit() == sgs.Card_Spade and friend:hasSkill("hongyan")))
						and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			elseif reason == "tieji" or reason == "caizhaoji_hujia" then
				for _, card in ipairs(cards) do
					if (card:isRed() or card:getSuit() == sgs.Card_Spade and friend:hasSkill("hongyan"))
						and (friend:hasSkill("tiandu") or not self:isValuableCard(card)) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(friend) end
						return
					end
				end
			end
		end
	end
	if zhenji then
		for _, card in ipairs(cards) do
			if card:isBlack() and not (zhenji:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade) then
				use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
				if use.to then use.to:append(zhenji) end
				return
			end
		end
	end
	self:sort(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if enemy:getPile("incantation"):length() > 0 then continue end
		local reason = getNextJudgeReason(self, enemy)
		if not enemy:hasSkill("tiandu") and reason then
			if reason == "indulgence" then
				for _, card in ipairs(cards) do
					if not (card:getSuit() == sgs.Card_Heart or (enemy:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade))
						and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "supply_shortage" then
				for _, card in ipairs(cards) do
					if card:getSuit() ~= sgs.Card_Club and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "lightning" and not enemy:hasSkills("hongyan|wuyan") then
				for _, card in ipairs(cards) do
					if card:getSuit() == sgs.Card_Spade and card:getNumber() >= 2 and card:getNumber() <= 9 then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "nosmiji" then
				for _, card in ipairs(cards) do
					if card:isRed() or card:getSuit() == sgs.Card_Spade and enemy:hasSkill("hongyan") then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "nosqianxi" or reason == "tuntian" then
				for _, card in ipairs(cards) do
					if (card:getSuit() == sgs.Card_Heart or card:getSuit() == sgs.Card_Spade and enemy:hasSkill("hongyan"))
						and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			elseif reason == "tieji" or reason == "caizhaoji_hujia" then
				for _, card in ipairs(cards) do
					if (card:getSuit() == sgs.Card_Club or (card:getSuit() == sgs.Card_Spade and not enemy:hasSkill("hongyan")))
						and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			end
		end
	end

	local has_indulgence, has_supplyshortage
	local friend
	for _, p in ipairs(self.friends) do
		if getKnownCard(p, self.player, "Indulgence", true, "he") > 0 then
			has_indulgence = true
			friend = p
			break
		end
		if getKnownCard(p, self.player, "SupplySortage", true, "he") > 0 then
			has_supplyshortage = true
			friend = p
			break
		end
	end
	if has_indulgence then
		local indulgence = sgs.Sanguosha:cloneCard("indulgence")
		for _, enemy in ipairs(self.enemies) do
			if enemy:getPile("incantation"):length() > 0 then continue end
			if self:hasTrickEffective(indulgence, enemy, friend) and self:playerGetRound(friend) < self:playerGetRound(enemy) and not self:willSkipPlayPhase(enemy) then
				for _, card in ipairs(cards) do
					if not (card:getSuit() == sgs.Card_Heart or (enemy:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade))
						and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			end
		end
	elseif has_supplyshortage then
		local supplyshortage = sgs.Sanguosha:cloneCard("supply_shortage")
		local distance = self:getDistanceLimit(supplyshortage, friend)
		for _, enemy in ipairs(self.enemies) do
			if enemy:getPile("incantation"):length() > 0 then continue end
			if self:hasTrickEffective(supplyshortage, enemy, friend) and self:playerGetRound(friend) < self:playerGetRound(enemy)
				and not self:willSkipDrawPhase(enemy) and friend:distanceTo(enemy) <= distance then
				for _, card in ipairs(cards) do
					if card:getSuit() ~= sgs.Card_Club and not self:isValuableCard(card) then
						use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
						if use.to then use.to:append(enemy) end
						return
					end
				end
			end
		end
	end

	for _, target in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if target:getPile("incantation"):length() > 0 then continue end
		if self:hasEightDiagramEffect(target) then
			for _, card in ipairs(cards) do
				if (card:isRed() and self:isFriend(target)) or (card:isBlack() and self:isEnemy(target)) and not self:isValuableCard(card) then
					use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
					if use.to then use.to:append(target) end
					return
				end
			end
		end
	end

	if self:getOverflow() > 0 then
		for _, target in sgs.qlist(self.room:getOtherPlayers(self.player)) do
		if target:getPile("incantation"):length() > 0 then continue end
			for _, card in ipairs(cards) do
				if not self:isValuableCard(card) and math.random() > 0.5 then
					use.card = sgs.Card_Parse("@JowfuhCard=" .. card:getEffectiveId())
					if use.to then use.to:append(target) end
					return
				end
			end
		end
	end

end

sgs.ai_card_intention.JowfuhCard = 0
sgs.ai_use_value.JowfuhCard = 2
sgs.ai_use_priority.JowfuhCard = sgs.ai_use_priority.Indulgence - 0.1

sgs.ai_skill_invoke.shennshyan = sgs.ai_skill_invoke.luoying

sgs.ai_skill_invoke.meybuh = function (self, data)
	local target = self.room:getCurrent()
	if self:isFriend(target) then
		--锦囊不如杀重要的情况
		local trick = sgs.Sanguosha:cloneCard("nullification")
		if target:hasSkill("wumou") or target:isJilei(trick) then return true end
		local slash = sgs.Sanguosha:cloneCard("Slash")
		dummy_use = {isDummy = true, from = target, to = sgs.SPlayerList()}
		self:useBasicCard(slash, dummy_use)
		if target:getWeapon() and target:getWeapon():isKindOf("Crossbow") and not dummy_use.to:isEmpty() then return true end
		if target:hasSkills("paoxiao|tianyi|xianzhen|jiangchi|fuhun|qiangwu") and not self:isWeak(target) and not dummy_use.to:isEmpty() then return true end
	else
		local slash2 = sgs.Sanguosha:cloneCard("Slash")
		if target:isJilei(slash2) then return true end
		if target:getWeapon() and target:getWeapon():isKindOf("blade") then return false end
		if target:hasSkills("paoxiao|tianyi|xianzhen|jiangchi|fuhun|qiangwu") or (target:getWeapon() and target:getWeapon():isKindOf("Crossbow")) then return false end
		if target:hasSkills("wumou|gongqi") then return false end
		if target:hasSkills("guose|qixi|duanliang|ol_duanliang|luanji") and target:getHandcardNum() > 1 then return true end
		if target:hasSkills("shuangxiong") and not self:isWeak(target) then return true end
		if not self:slashIsEffective(slash2, self.player, target) and not self:isWeak() then return true end
		if self.player:getArmor() and self.player:getArmor():isKindOf("Vine") and not self:isWeak() then return true end
		if self.player:getArmor() and not self:isWeak() and self:getCardsNum("Jink") > 0 then return true end
	end
	return false
end

sgs.ai_skill_choice.muhmuh = function(self, choices)
	local armorPlayersF = {}
	local weaponPlayersE = {}
	local armorPlayersE = {}

	for _,p in ipairs(self.friends_noself) do
		if p:getArmor() and p:objectName() ~= self.player:objectName() then
			table.insert(armorPlayersF, p)
		end
	end
	for _,p in ipairs(self.enemies) do
		if p:getWeapon() and self.player:canDiscard(p, p:getWeapon():getEffectiveId()) then
			table.insert(weaponPlayersE, p)
		end
		if p:getArmor() and p:objectName() ~= self.player:objectName() then
			table.insert(armorPlayersE, p)
		end
	end

	self.player:setFlags("muhmuh_armor")
	if #armorPlayersF > 0 then
		for _,friend in ipairs(armorPlayersF) do
			if (friend:getArmor():isKindOf("Vine") and not self.player:getArmor() and not friend:hasSkills("kongcheng|zhiji")) or (friend:getArmor():isKindOf("SilverLion") and friend:getLostHp() > 0) then
				return "armor"
			end
		end
	end

	if #armorPlayersE > 0 then
		if not self.player:getArmor() then return "armor" end
		if self.player:getArmor() and self.player:getArmor():isKindOf("SilverLion") and self.player:getLostHp() > 0 then return "armor" end
		for _,enemy in ipairs(armorPlayersE) do
			if enemy:getArmor():isKindOf("Vine") or self:isWeak(enemy) then
				return "armor"
			end
		end
	end

	self.player:setFlags("-muhmuh_armor")
	if #weaponPlayersE > 0 then
		return "weapon"
	end
	self.player:setFlags("muhmuh_armor")
	if #armorPlayersE > 0 then
		for _,enemy in ipairs(armorPlayersE) do
			if not enemy:getArmor():isKindOf("SilverLion") and enemy:getLostHp() > 0 then
				return "armor"
			end
		end
	end
	self.player:setFlags("-muhmuh_armor")
	return "cancel"
end

sgs.ai_skill_playerchosen.muhmuh = function(self, targets)
	if self.player:hasFlag("muhmuh_armor") then
		for _,target in sgs.qlist(targets) do
			if self:isFriend(target) and target:getArmor():isKindOf("SilverLion") and target:getLostHp() > 0 then return target end
			if self:isEnemy(target) and target:getArmor():isKindOf("SilverLion") and target:getLostHp() == 0 then return target end
		end
		for _,target in sgs.qlist(targets) do
			if self:isEnemy(target) and (self:isWeak(target) or target:getArmor():isKindOf("Vine")) then return target end
		end
		for _,target in sgs.qlist(targets) do
			if self:isEnemy(target) then return target end
		end
	else
		for _,target in sgs.qlist(targets) do
			if self:isEnemy(target) and target:hasSkills("liegong|qiangxi|jijiu|guidao|anjian") then return target end
		end
		for _,target in sgs.qlist(targets) do
			if self:isEnemy(target) then return target end
		end
	end
	return targets:at(0)
end

local xiemu_skill = {}
xiemu_skill.name = "xiemu"
table.insert(sgs.ai_skills, xiemu_skill)
xiemu_skill.getTurnUseCard = function(self)
	if self.player:hasUsed("XiemuCard") then return end
	if self:getCardsNum("Slash") == 0 then return end

	local kingdomDistribute = {}
	kingdomDistribute["wei"] = 0;
	kingdomDistribute["shu"] = 0;
	kingdomDistribute["wu"] = 0;
	kingdomDistribute["qun"] = 0;
	for _,p in sgs.qlist(self.room:getAlivePlayers()) do
		if kingdomDistribute[p:getKingdom()] and self:isEnemy(p) and p:inMyAttackRange(self.player)
			then kingdomDistribute[p:getKingdom()] = kingdomDistribute[p:getKingdom()] + 1
			else kingdomDistribute[p:getKingdom()] = kingdomDistribute[p:getKingdom()] + 0.2 end
		if p:hasSkill("luanji") and p:getHandcardNum() > 2 then kingdomDistribute[p:getKingdom()] = kingdomDistribute[p:getKingdom()] + 3 end
		if p:hasSkill("qixi") and self:isEnemy(p) and p:getHandcardNum() > 2 then kingdomDistribute[p:getKingdom()] = kingdomDistribute[p:getKingdom()] + 2 end
		if p:hasSkill("zaoxian") and self:isEnemy(p) and p:getPile("field"):length() > 1 then kingdomDistribute[p:getKingdom()] = kingdomDistribute[p:getKingdom()] + 2 end
	end
	maxK = "wei"
	if kingdomDistribute["shu"] > kingdomDistribute[maxK] then maxK = "shu" end
	if kingdomDistribute["wu"] > kingdomDistribute[maxK] then maxK = "wu" end
	if kingdomDistribute["qun"] > kingdomDistribute[maxK] then maxK = "qun" end
	if self.player:getHandcardNum() > self.player:getMaxCards() then kingdomDistribute[maxK] = kingdomDistribute[maxK] + 4 end
	if kingdomDistribute[maxK] + self:getCardsNum("Slash") < 4 then return end
	self.room:setTag("xiemu_choice", sgs.QVariant(maxK))
	local subcard
	for _,c in sgs.qlist(self.player:getHandcards()) do
		if c:isKindOf("Slash") then subcard = c end
	end
	if not subcard then return end
	return sgs.Card_Parse("@XiemuCard=" .. subcard:getEffectiveId())
end

sgs.ai_skill_use_func.XiemuCard = function(card, use, self)
	if self.player:hasUsed("XiemuCard") then return end
	use.card = card
end

sgs.ai_skill_choice.xiemu = function(self, choices)
	local choice = self.room:getTag("xiemu_choice"):toString()
	self.room:setTag("xiemu_choice", sgs.QVariant())
	return choice
end

sgs.ai_use_value.XiemuCard = 5
sgs.ai_use_priority.XiemuCard = 10

sgs.ai_skill_invoke.naman = function(self, data)
	if self:needKongcheng(self.player, true) and self.player:getHandcardNum() == 0 then return false end
	return true
end

sgs.ai_skill_invoke.cihuai = function(self, data)
	local has_slash = false
	local cards = self.player:getCards("h")
	cards=sgs.QList2Table(cards)
	for _, card in ipairs(cards) do
		if card:isKindOf("Slash") then has_slash = true end
	end
	if has_slash then return false end

	self:sort(self.enemies, "defenseSlash")
	for _, enemy in ipairs(self.enemies) do
		local slash = sgs.Sanguosha:cloneCard("slash", sgs.Card_NoSuit, 0)
		local eff = self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)
		if eff and self.player:canSlash(enemy) and not self:slashProhibit(nil, enemy) then
			return true
		end
	end

	return false
end

local cihuai_skill = {}
cihuai_skill.name = "cihuai"
table.insert(sgs.ai_skills, cihuai_skill)
cihuai_skill.getTurnUseCard = function(self)
	if self.player:getMark("@cihuai") > 0 then
		local card_str = ("slash:_cihuai[no_suit:0]=.")
		local slash = sgs.Card_Parse(card_str)
		assert(slash)
		return slash
	end
end
sgs.ai_use_priority.cihuai_skill = sgs.ai_use_priority.ExNihilo + 1
sgs.ai_use_value.cihuai_skill = 6.7

local chixin_skill={}
chixin_skill.name="chixin"
table.insert(sgs.ai_skills,chixin_skill)
chixin_skill.getTurnUseCard = function(self, inclusive)
	local cards = self.player:getCards("he")
	cards=sgs.QList2Table(cards)

	local diamond_card

	self:sortByUseValue(cards,true)

	local useAll = false
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if enemy:getHp() == 1 and not enemy:hasArmorEffect("EightDiagram") and self.player:distanceTo(enemy) <= self.player:getAttackRange() and self:isWeak(enemy)
			and getCardsNum("Jink", enemy, self.player) + getCardsNum("Peach", enemy, self.player) + getCardsNum("Analeptic", enemy, self.player) == 0 then
			useAll = true
			break
		end
	end

	local disCrossbow = false
	if self:getCardsNum("Slash") < 2 or self.player:hasSkill("paoxiao") then
		disCrossbow = true
	end


	for _,card in ipairs(cards)  do
		if card:getSuit() == sgs.Card_Diamond
		and (not isCard("Peach", card, self.player) and not isCard("ExNihilo", card, self.player) and not useAll)
		and (not isCard("Crossbow", card, self.player) and not disCrossbow)
		and (self:getUseValue(card) < sgs.ai_use_value.Slash or inclusive or sgs.Sanguosha:correctCardTarget(sgs.TargetModSkill_Residue, self.player, sgs.Sanguosha:cloneCard("slash")) > 0) then
			diamond_card = card
			break
		end
	end

	if not diamond_card then return nil end
	local suit = diamond_card:getSuitString()
	local number = diamond_card:getNumberString()
	local card_id = diamond_card:getEffectiveId()
	local card_str = ("slash:chixin[%s:%s]=%d"):format(suit, number, card_id)
	local slash = sgs.Card_Parse(card_str)
	assert(slash)

	return slash

end

sgs.ai_view_as.chixin = function(card, player, card_place, class_name)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card_place ~= sgs.Player_PlaceSpecial and card:getSuit() == sgs.Card_Diamond and not card:isKindOf("Peach") and not card:hasFlag("using") then
		if class_name == "Slash" then
			return ("slash:chixin[%s:%s]=%d"):format(suit, number, card_id)
		elseif class_name == "Jink" then
			return ("jink:chixin[%s:%s]=%d"):format(suit, number, card_id)
		end
	end
end

sgs.ai_cardneed.chixin = function(to, card)
	return card:getSuit() == sgs.Card_Diamond
end

sgs.ai_skill_playerchosen.suiren = function(self, targets)
	if self.player:getMark("@suiren") == 0 then return "." end
	if self:isWeak() and (self:getOverflow() < -2 or not self:willSkipPlayPhase()) then return self.player end
	self:sort(self.friends_noself, "defense")
	for _, friend in ipairs(self.friends) do
		if self:isWeak(friend) and not self:needKongcheng(friend) then
			return friend
		end
	end
	self:sort(self.enemies, "defense")
	for _, enemy in ipairs(self.enemies) do
		if (self:isWeak(enemy) and enemy:getHp() == 1)
			and self.player:getHandcardNum() < 2 and not self:willSkipPlayPhase() and self.player:inMyAttackRange(enemy) then
			return self.player
		end
	end
end

sgs.ai_playerchosen_intention.suiren = -60

sgs.ai_skill_invoke.nosfengpo = function(self, data)
	local target = data:toPlayer()
	self.room:setTag("nosfengpo",sgs.QVariant(target:objectName()))
	return true
end
sgs.ai_skill_choice["nosfengpo"] = function(self, choices)
	local items = choices:split("+")
    local name = self.room:getTag("nosfengpo"):toString()
	
	if name == "" then return items[1] end
	local target = nil
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if p:objectName() == name then
			target = p
			break
		end
	end
	if target == nil then return items[1] end
	if #items == 1 then
        return items[1]
	else
		if self:isEnemy(target) and getCardsNum("Jink", target, self.player) == 0 then
			return "nosfengpo2"
		else
			return "nosfengpo1"
		end
    end
    return items[1]
end

sgs.ai_skill_invoke.twbaobian = function(self, data)
	local to = data:toPlayer()
	if to and self:isFriend(to) and to:getKingdom() == self.player:getKingdom() then return true end
	if to and self:isEnemy(to) and to:getKingdom() ~= self.player:getKingdom() then return true end
	return sgs.ai_skill_invoke["yishi"]
end

sgs.ai_skill_invoke.tijin = sgs.ai_skill_invoke.xiaolian

sgs.ai_skill_invoke.xiaolian = function(self, data)
	local to = data:toCardUse().to:first()
	if to and self:isFriend(to) and self:isWeak(to) and not self:isWeak(self.player) then
		return true
	end
	return false
end

