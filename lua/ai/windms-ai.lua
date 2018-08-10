sgs.ai_skill_use["@@shernsuh1"]=function(self,prompt)
	self:updatePlayers()
	self:sort(self.enemies,"defense")
	if self.player:containsTrick("lightning") and self.player:getCards("j"):length()==1
	  and self:hasWizard(self.friends) and not self:hasWizard(self.enemies, true) then
		return "."
	end

	if self:needBear() then return "." end

	local selfSub = self.player:getHp() - self.player:getHandcardNum()
	local selfDef = sgs.getDefense(self.player)

	for _,enemy in ipairs(self.enemies) do
		local def = sgs.getDefenseSlash(enemy, self)
		local slash = sgs.Sanguosha:cloneCard("slash")
		local eff = self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)

		if not self.player:canSlash(enemy, slash, false) then
		elseif self:slashProhibit(nil, enemy) then
		elseif def < 6 and eff then return "@ShernsuhCard=.->"..enemy:objectName()

		elseif selfSub >= 2 then return "."
		elseif selfDef < 6 then return "." end
	end

	for _,enemy in ipairs(self.enemies) do
		local def=sgs.getDefense(enemy)
		local slash = sgs.Sanguosha:cloneCard("slash")
		local eff = self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)

		if not self.player:canSlash(enemy, slash, false) then
		elseif self:slashProhibit(nil, enemy) then
		elseif eff and def < 8 then return "@ShernsuhCard=.->"..enemy:objectName()
		else return "." end
	end
	return "."
end

sgs.ai_get_cardType = function(card)
	if card:isKindOf("Weapon") then return 1 end
	if card:isKindOf("Armor") then return 2 end
	if card:isKindOf("DefensiveHorse") then return 3 end
	if card:isKindOf("OffensiveHorse")then return 4 end
end

sgs.ai_skill_use["@@shernsuh2"] = function(self, prompt, method)
	self:updatePlayers()
	self:sort(self.enemies, "defenseSlash")

	local selfSub = self.player:getHp() - self.player:getHandcardNum()
	local selfDef = sgs.getDefense(self.player)

	local cards = self.player:getCards("he")
	cards = sgs.QList2Table(cards)

	local eCard
	local hasCard = { 0, 0, 0, 0 }

	if self:needToThrowArmor() and not self.player:isCardLimited(self.player:getArmor(), method) then
		eCard = self.player:getArmor()
	end

	if not eCard then
		for _, card in ipairs(cards) do
			if card:isKindOf("EquipCard") then
				hasCard[sgs.ai_get_cardType(card)] = hasCard[sgs.ai_get_cardType(card)] + 1
			end
		end

		for _, card in ipairs(cards) do
			if card:isKindOf("EquipCard") and hasCard[sgs.ai_get_cardType(card)] > 1 then
				eCard = card
				break
			end
		end

		if not eCard then
			for _, card in ipairs(cards) do
				if card:isKindOf("EquipCard") and sgs.ai_get_cardType(card) > 3 and not self.player:isCardLimited(card, method) then
					eCard = card
					break
				end
			end
		end
		if not eCard then
			for _, card in ipairs(cards) do
				if card:isKindOf("EquipCard") and not card:isKindOf("Armor") and not self.player:isCardLimited(card, method) then
					eCard = card
					break
				end
			end
		end
	end

	if not eCard then return "." end

	local effectslash, best_target, target, throw_weapon
	local defense = 6
	local weapon = self.player:getWeapon()
	if weapon and eCard:getId() == weapon:getId() and (eCard:isKindOf("fan") or eCard:isKindOf("QinggangSword")) then throw_weapon = true end

	for _, enemy in ipairs(self.enemies) do
		local def = sgs.getDefense(enemy)
		local slash = sgs.Sanguosha:cloneCard("slash")
		local eff = self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self)

		if not self.player:canSlash(enemy, slash, false) then
		elseif throw_weapon and enemy:hasArmorEffect("vine") and not self.player:hasSkill("zonghuo") then
		elseif self:slashProhibit(nil, enemy) then
		elseif eff then
			if enemy:getHp() == 1 and getCardsNum("Jink", enemy) == 0 then best_target = enemy break end
			if def < defense then
				best_target = enemy
				defense = def
			end
			target = enemy
		end
		if selfSub < 0 then return "." end
	end

	if best_target then return "@ShernsuhCard="..eCard:getEffectiveId().."->"..best_target:objectName() end
	if target then return "@ShernsuhCard="..eCard:getEffectiveId().."->"..target:objectName() end

	return "."
end

sgs.ai_cardneed.shernsuh = function(to, card, self)
	return card:getTypeId() == sgs.Card_TypeEquip and getKnownCard(to, self.player, "EquipCard", false) < 2
end

sgs.ai_card_intention.ShernsuhCard = sgs.ai_card_intention.Slash

sgs.shernsuh_keep_value = sgs.xiaoji_keep_value

function sgs.ai_skill_invoke.jiuhshoou(self, data)
	if not self.player:faceUp() then return true end
	for _, friend in ipairs(self.friends) do
		if friend:hasSkills("fangzhu|jilve") and not self:isWeak(friend) then return true end
		if friend:hasSkill("junxing") and friend:faceUp() and not self:willSkipPlayPhase(friend)
			and not (friend:isKongcheng() and self:willSkipDrawPhase(friend)) then
			return true
		end
	end
	if not self.player:hasSkill("jieewei") then return false end
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if card:getTypeId() == sgs.Card_TypeTrick and not card:isKindOf("Nullification") then
			local dummy_use = { isDummy = true }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then return true end
		elseif card:getTypeId() == sgs.Card_TypeEquip then
			local dummy_use = { isDummy = true }
			self:useEquipCard(card, dummy_use)
			if dummy_use.card then return true end
		end
	end
	local Rate = math.random() + self.player:getCardCount()/10 + self.player:getHp()/10
	if Rate > 1.1 then return true end
	return false
end

sgs.ai_skill_invoke.jieewei = true

sgs.ai_skill_use["TrickCard+^Nullification,EquipCard|.|.|hand"] = function(self, prompt, method)
	local cards = sgs.QList2Table(self.player:getHandcards())
	self:sortByUseValue(cards)
	for _, card in ipairs(cards) do
		if card:getTypeId() == sgs.Card_TypeTrick and not card:isKindOf("Nullification") and (not card:isKindOf("IronChain") or not dummy_use.to:isEmpty()) then
			local dummy_use = { isDummy = true, to = sgs.SPlayerList() }
			self:useTrickCard(card, dummy_use)
			if dummy_use.card then
				self.jieewei_type = sgs.Card_TypeTrick
				if dummy_use.to:isEmpty() then
					return dummy_use.card:toString()
				else
					local target_objectname = {}
					for _, p in sgs.qlist(dummy_use.to) do
						table.insert(target_objectname, p:objectName())
					end
					return dummy_use.card:toString() .. "->" .. table.concat(target_objectname, "+")
				end
			end
		elseif card:getTypeId() == sgs.Card_TypeEquip then
			local dummy_use = { isDummy = true }
			self:useEquipCard(card, dummy_use)
			if dummy_use.card then
				self.jieewei_type = sgs.Card_TypeEquip
				return dummy_use.card:toString()
			end
		end
	end
	return "."
end

sgs.ai_skill_playerchosen.jieewei = function(self, targets)
	if self.jieewei_type == sgs.Card_TypeTrick then return self:findPlayerToDiscard("j", true, true, targets)
	elseif self.jieewei_type == sgs.Card_TypeEquip then return self:findPlayerToDiscard("e", true, true, targets) end
end

sgs.ai_skill_invoke.liehgong = function(self, data)
	local target = data:toPlayer()
	return not self:isFriend(target)
end

function SmartAI:canLiehgong(to, from)
	from = from or self.room:getCurrent()
	to = to or self.player
	if not from then return false end
	if from:hasSkill("liehgong") and from:getPhase() == sgs.Player_Play and (to:getHandcardNum() >= from:getHp() or to:getHandcardNum() <= from:getAttackRange()) then return true end
	if from:hasSkill("kofliehgong") and from:getPhase() == sgs.Player_Play and to:getHandcardNum() >= from:getHp() then return true end
	return false
end

function SmartAI:findLeirjiTarget(player, leirji_value, slasher, latest_version)
	if not latest_version then
		return self:findLeirjiTarget(player, leirji_value, slasher, 1) or self:findLeirjiTarget(player, leirji_value, slasher, -1)
	end
	if not player:hasSkill(latest_version == 1 and "leirji" or "nosleirji" or "olleirji") then return nil end
	if slasher then
		if not self:slashIsEffective(sgs.Sanguosha:cloneCard("slash"), player, slasher, slasher:hasWeapon("qinggang_sword")) then return nil end
		if slasher:hasSkill("liehgong") and slasher:getPhase() == sgs.Player_Play and self:isEnemy(player, slasher)
			and (player:getHandcardNum() >= slasher:getHp() or player:getHandcardNum() <= slasher:getAttackRange()) then
			return nil
		end
		if slasher:hasSkill("kofliehgong") and slasher:getPhase() == sgs.Player_Play
			and self:isEnemy(player, slasher) and player:getHandcardNum() >= slasher:getHp() then
			return nil
		end
		if not latest_version then
			if not self:hasSuit("spade", true, player) and player:getHandcardNum() < 3 then return nil end
		else
			if not self:hasSuit("black", true, player) and player:getHandcardNum() < 2 then return nil end
		end
		if not (getKnownCard(player, self.player, "Jink", true) > 0
				or (getCardsNum("Jink", player, self.player) >= 1 and sgs.card_lack[player:objectName()]["Jink"] ~= 1 and player:getHandcardNum() >= 4)
				or (not self:isWeak(player) and self:hasEightDiagramEffect(player) and not slasher:hasWeapon("qinggang_sword") and sgs.card_lack[player:objectName()]["Jink"] ~= 1)) then
			return nil
		end
	end
	local getCmpValue = function(enemy)
		local value = 0
		if not self:damageIsEffective(enemy, sgs.DamageStruct_Thunder, player) then return 99 end
		if enemy:hasSkill("hongyan") then
			if latest_version == -1 then return 99
			elseif not self:hasSuit("club", true, player) and player:getHandcardNum() < 3 then value = value + 80
			else value = value + 70 end
		end
		if self:cantbeHurt(enemy, player, latest_version == 1 and 1 or 2) or self:objectiveLevel(enemy) < 3
			or (enemy:isChained() and not self:isGoodChainTarget(enemy, player, sgs.DamageStruct_Thunder, latest_version == 1 and 1 or 2)) then return 100 end
		if not sgs.isGoodTarget(enemy, self.enemies, self) then value = value + 50 end
		if not latest_version and enemy:hasArmorEffect("silver_lion") then value = value + 20 end
		if enemy:hasSkills(sgs.exclusive_skill) then value = value + 10 end
		if enemy:hasSkills(sgs.masochism_skill) then value = value + 5 end
		if enemy:isChained() and self:isGoodChainTarget(enemy, player, sgs.DamageStruct_Thunder, latest_version == 1 and 1 or 2) and #(self:getChainedEnemies(player)) > 1 then value = value - 25 end
		if enemy:isLord() then value = value - 5 end
		value = value + enemy:getHp() + sgs.getDefenseSlash(enemy, self) * 0.01
		if latest_version and player:isWounded() and not self:needToLoseHp(player) then value = value + 15 end
		return value
	end

	local cmp = function(a, b)
		return getCmpValue(a) < getCmpValue(b)
	end

	local enemies = self:getEnemies(player)
	table.sort(enemies, cmp)
	for _,enemy in ipairs(enemies) do
		if getCmpValue(enemy) < leirji_value then return enemy end
	end
	return nil
end

sgs.ai_skill_playerchosen.leirji = function(self, targets)
	local mode = self.room:getMode()
	if mode:find("_mini_17") or mode:find("_mini_19") or mode:find("_mini_20") or mode:find("_mini_26") then
		local players = self.room:getAllPlayers();
		for _, aplayer in sgs.qlist(players) do
			if aplayer:getState() ~= "robot" then
				return aplayer
			end
		end
	end

	self:updatePlayers()
	return self:findLeirjiTarget(self.player, 100, nil, 1)
end

function SmartAI:needLeirji(to, from)
	return self:findLeirjiTarget(to, 50, from, -1)
end

sgs.ai_playerchosen_intention.leirji = 80

function sgs.ai_slash_prohibit.leirji(self, from, to, card) -- @todo: Qianxi flag name
	if self:isFriend(to) then return false end
	if to:hasFlag("QianxiTarget") and (not self:hasEightDiagramEffect(to) or self.player:hasWeapon("qinggang_sword")) then return false end
	local hcard = to:getHandcardNum()
	if self:canLiehgong(to, from) then return false end
	if from:getRole() == "rebel" and to:isLord() then
		local other_rebel
		for _, player in sgs.qlist(self.room:getOtherPlayers(from)) do
			if sgs.evaluatePlayerRole(player) == "rebel" or sgs.compareRoleEvaluation(player, "rebel", "loyalist") == "rebel" then
				other_rebel = player
				break
			end
		end
		if not other_rebel and (self:hasSkills("hongyan") or self.player:getHp() >= 4) and (self:getCardsNum("Peach") > 0  or self.player:hasSkills("hongyan|ganglie|neoganglie")) then
			return false
		end
	end

	if sgs.card_lack[to:objectName()]["Jink"] == 2 then return true end
	if getKnownCard(to, self.player, "Jink", true) >= 1 or (self:hasSuit("spade", true, to) and hcard >= 2) or hcard >= 4 then return true end
	if not from then
		from = self.room:getCurrent()
	end
	if self:hasEightDiagramEffect(to) and not IgnoreArmor(from, to) then return true end
end

sgs.ai_skill_use["@@tianshiang"] = function(self, data, method)
	if not method then method = sgs.Card_MethodDiscard end
	local friend_lost_hp = 10
	local friend_hp = 0
	local card_id
	local target
	local cant_use_skill
	local dmg

	if data == "@tianshiang-card" then
		dmg = self.player:getTag("TianshiangDamage"):toDamage()
	else
		dmg = data
	end

	if not dmg then self.room:writeToConsole(debug.traceback()) return "." end

	local cards = self.player:getCards("h")
	cards = sgs.QList2Table(cards)
	self:sortByUseValue(cards, true)
	for _, card in ipairs(cards) do
		if not self.player:isCardLimited(card, method) and card:getSuit() == sgs.Card_Heart and not card:isKindOf("Peach") then
			card_id = card:getId()
			break
		end
	end
	if not card_id then return "." end

	self:sort(self.enemies, "hp")

	for _, enemy in ipairs(self.enemies) do
		if (enemy:getHp() <= dmg.damage and enemy:isAlive() and enemy:getLostHp() + dmg.damage < 3) then
			if (enemy:getHandcardNum() <= 2 or enemy:hasSkills("guose|leirji|ganglie|enyuan|qingguo|wuyan|kongcheng") or enemy:containsTrick("indulgence"))
				and self:canAttack(enemy, dmg.from or self.room:getCurrent(), dmg.nature)
				and not (dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and enemy:hasSkill("wuyan")) then
				return "@TianshiangCard=" .. card_id .. "->" .. enemy:objectName()
			end
		end
	end

	for _, friend in ipairs(self.friends_noself) do
		if (friend:getLostHp() + dmg.damage > 1 and friend:isAlive()) then
			if friend:isChained() and dmg.nature ~= sgs.DamageStruct_Normal and not self:isGoodChainTarget(friend, dmg.from, dmg.nature, dmg.damage, dmg.card) then
			elseif friend:getHp() >= 2 and dmg.damage < 2
					and (friend:hasSkills("yiji|buqu|nosbuqu|shuangxiong|zaiqi|yinghun|jianxiong|fangzhu")
						or self:getDamagedEffects(friend, dmg.from or self.room:getCurrent())
						or self:needToLoseHp(friend)
						or (friend:getHandcardNum() < 3 and (friend:hasSkill("nosrende") or (friend:hasSkill("rende") and not friend:hasUsed("RendeCard"))))) then
				return "@TianshiangCard=" .. card_id .. "->" .. friend:objectName()
				elseif dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and friend:hasSkill("wuyan") and friend:getLostHp() > 1 then
					return "@TianshiangCard=" .. card_id .. "->" .. friend:objectName()
			elseif hasBuquEffect(friend) then return "@TianshiangCard=" .. card_id .. "->" .. friend:objectName() end
		end
	end

	for _, enemy in ipairs(self.enemies) do
		if (enemy:getLostHp() <= 1 or dmg.damage > 1) and enemy:isAlive() and enemy:getLostHp() + dmg.damage < 4 then
			if (enemy:getHandcardNum() <= 2)
				or enemy:containsTrick("indulgence") or enemy:hasSkills("guose|leirji|vsganglie|ganglie|enyuan|qingguo|wuyan|kongcheng")
				and self:canAttack(enemy, (dmg.from or self.room:getCurrent()), dmg.nature)
				and not (dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and enemy:hasSkill("wuyan")) then
				return "@TianshiangCard=" .. card_id .. "->" .. enemy:objectName() end
		end
	end

	for i = #self.enemies, 1, -1 do
		local enemy = self.enemies[i]
		if not enemy:isWounded() and not self:hasSkills(sgs.masochism_skill, enemy) and enemy:isAlive()
			and self:canAttack(enemy, dmg.from or self.room:getCurrent(), dmg.nature)
			and (not (dmg.card and dmg.card:getTypeId() == sgs.Card_TypeTrick and enemy:hasSkill("wuyan") and enemy:getLostHp() > 0) or self:isWeak()) then
			return "@TianshiangCard=" .. card_id .. "->" .. enemy:objectName()
		end
	end

	return "."
end

sgs.ai_card_intention.TianshiangCard = function(self, card, from, tos)
	local to = tos[1]
	if self:getDamagedEffects(to) or self:needToLoseHp(to) then return end
	local intention = 10
	if hasBuquEffect(to) then intention = 0
	elseif (to:getHp() >= 2 and to:hasSkills("yiji|shuangxiong|zaiqi|yinghun|jianxiong|fangzhu"))
		or (to:getHandcardNum() < 3 and (to:hasSkill("nosrende") or (to:hasSkill("rende") and not to:hasUsed("RendeCard")))) then
		intention = 0
	end
	sgs.updateIntention(from, to, intention)
end

function sgs.ai_slash_prohibit.tianshiang(self, from, to)
	if from:hasSkill("jueqing") or (from:hasSkill("nosqianxi") and from:distanceTo(to) == 1) then return false end
	if from:hasFlag("NosJiefanUsed") then return false end
	if self:isFriend(to, from) then return false end
	return self:cantbeHurt(to, from)
end

sgs.tianshiang_suit_value = {
	heart = 4.9
}

function sgs.ai_cardneed.tianshiang(to, card, self)
	return (card:getSuit() == sgs.Card_Heart or (to:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade))
		and (getKnownCard(to, self.player, "heart", false) + getKnownCard(to, self.player, "spade", false)) < 2
end


function sgs.ai_cardneed.kwangguu(to, card, self)
	return card:isKindOf("OffensiveHorse") and not (to:getOffensiveHorse() or getKnownCard(to, self.player, "OffensiveHorse", false) > 0)
end


duannlyang_skill={}
duannlyang_skill.name="duannlyang"
table.insert(sgs.ai_skills,duannlyang_skill)
duannlyang_skill.getTurnUseCard=function(self)
	local cards = self.player:getCards("he")
	cards=sgs.QList2Table(cards)

	local card

	self:sortByUseValue(cards,true)

	for _,acard in ipairs(cards)  do
		if (acard:isBlack()) and (acard:isKindOf("BasicCard") or acard:isKindOf("EquipCard")) and (self:getDynamicUsePriority(acard)<sgs.ai_use_value.SupplyShortage)then
			card = acard
			break
		end
	end

	if not card then return nil end
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	local card_str = ("supply_shortage:duannlyang[%s:%s]=%d"):format(suit, number, card_id)
	local skillcard = sgs.Card_Parse(card_str)

	assert(skillcard)

	return skillcard
end

sgs.ai_cardneed.duannlyang = function(to, card, self)
	return card:isBlack() and card:getTypeId() ~= sgs.Card_TypeTrick and getKnownCard(to, self.player, "black", false) < 2
end

sgs.duannlyang_suit_value = {
	spade = 3.9,
	club = 3.9
}

sgs.ai_skill_invoke.conqueror= function(self, data)
	local target = data:toPlayer()
	if self:isFriend(target) and not self:needToThrowArmor(target) then
	return false end
return true
end

sgs.ai_skill_choice.conqueror = function(self, choices, data)
	local target = data:toPlayer()
	if (self:isFriend(target) and not self:needToThrowArmor(target)) or (self:isEnemy(target) and target:getEquips():length() == 0) then
	return "EquipCard" end
	local choice = {}
	table.insert(choice, "EquipCard")
	table.insert(choice, "TrickCard")
	table.insert(choice, "BasicCard")
	if (self:isEnemy(target) and not self:needToThrowArmor(target)) or (self:isFriend(target) and target:getEquips():length() == 0) then
		table.removeOne(choice, "EquipCard")
		if #choice == 1 then return choice[1] end
	end
	if (self:isEnemy(target) and target:getHandcardNum() < 2) then
		table.removeOne(choice, "BasicCard")
		if #choice == 1 then return choice[1] end
	end
	if (self:isEnemy(target) and target:getHandcardNum() > 3) then
		table.removeOne(choice, "TrickCard")
		if #choice == 1 then return choice[1] end
	end
	return choice[math.random(1, #choice)]
end

sgs.ai_skill_cardask["@conqueror"] = function(self, data)
	local has_card
	local cards = sgs.QList2Table(self.player:getCards("he"))
	self:sortByUseValue(cards, true)
	for _,cd in ipairs(cards) do
		if self:getArmor("SilverLion") and card:isKindOf("SilverLion") then
			has_card = cd
			break
		end
		if not cd:isKindOf("Peach") and not card:isKindOf("Analeptic") and not (self:getArmor() and cd:objectName() == self.player:getArmor():objectName()) then
			has_card = cd
			break
		end
	end
	if has_card then
		return "$" .. has_card:getEffectiveId()
	else
		return ".."
	end
end
