sgs.ai_skill_invoke.shangshyh = function(self, data)
	if self.player:getLostHp() == 1 then return sgs.ai_skill_invoke.noslianying(self, data) end
	return true
end

sgs.ai_skill_invoke.tzyhshour = function(self, data)
	if self:needBear() then return true end
	if self.player:isSkipped(sgs.Player_Play) then return true end

	local chance_value = 1
	local peach_num = self:getCardsNum("Peach")
	local can_save_card_num = self:getOverflow(self.player, true) - self.player:getHandcardNum()

	if self.player:getHp() <= 2 and self.player:getHp() < getBestHp(self.player) then chance_value = chance_value + 1 end
	if self.player:hasSkills("nosrende|rende") and self:findFriendsByType(sgs.Friend_Draw) then chance_value = chance_value - 1 end
	if self.player:hasSkill("qingnang") then
		for _, friend in ipairs(self.friends) do
			if friend:isWounded() then chance_value = chance_value - 1 break end
		end
	end
	if self.player:hasSkill("jieyin") then
		for _, friend in ipairs(self.friends) do
			if friend:isWounded() and friend:isMale() then chance_value = chance_value - 1 break end
		end
	end

	return self:ImitateResult_DrawNCards(self.player, self.player:getVisibleSkillList(true)) - can_save_card_num + peach_num  <= chance_value
end
sgs.ai_skill_invoke.qianxigz = function(self, data)
	if self.player:hasFlag("AI_doNotInvoke_qianxigz") then
		self.player:setFlags("-AI_doNotInvoke_qianxigz")
		return
	end
	if self.player:getPile("incantation"):length() > 0 then
		local card = sgs.Sanguosha:getCard(self.player:getPile("incantation"):first())
		if not self.player:getJudgingArea():isEmpty() and not self.player:containsTrick("YanxiaoCard") and not self:hasWizard(self.enemies, true) then
			local trick = self.player:getJudgingArea():last()
			if trick:isKindOf("Indulgence") then
				if card:getSuit() == sgs.Card_Heart or (self.player:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade) then return false end
			elseif trick:isKindOf("SupplyShortage") then
				if card:getSuit() == sgs.Card_Club then return false end
			end
		end
		local zhangbao = self.room:findPlayerBySkillName("yingbing")
		if zhangbao and self:isEnemy(zhangbao) and not zhangbao:hasSkill("manjuan")
			and (card:isRed() or (self.player:hasSkill("hongyan") and card:getSuit() == sgs.Card_Spade)) then return false end
	end
	for _, p in ipairs(self.enemies) do
		if self.player:distanceTo(p) == 1 and not p:isKongcheng() then
			return true
		end
	end
	return false
end

sgs.ai_skill_playerchosen.qianxigz = function(self, targets)
	local enemies = {}
	local slash = self:getCard("Slash") or sgs.Sanguosha:cloneCard("slash")
	local isRed = (self.player:getTag("qianxigz"):toString() == "red")

	for _, target in sgs.qlist(targets) do
		if self:isEnemy(target) and not target:isKongcheng() then
			table.insert(enemies, target)
		end
	end

	if #enemies == 1 then
		return enemies[1]
	else
		self:sort(enemies, "defense")
		if not isRed then
			for _, enemy in ipairs(enemies) do
				if enemy:hasSkill("qingguo") and self:slashIsEffective(slash, enemy) then return enemy end
			end
			for _, enemy in ipairs(enemies) do
				if enemy:hasSkill("kanpo") then return enemy end
			end
		else
			for _, enemy in ipairs(enemies) do
				if getKnownCard(enemy, self.player, "Jink", false, "h") > 0 and self:slashIsEffective(slash, enemy) and sgs.isGoodTarget(enemy, self.enemies, self) then return enemy end
			end
			for _, enemy in ipairs(enemies) do
				if getKnownCard(enemy, self.player, "Peach", true, "h") > 0 or enemy:hasSkill("jijiu") then return enemy end
			end
			for _, enemy in ipairs(enemies) do
				if getKnownCard(enemy, self.player, "Jink", false, "h") > 0 and self:slashIsEffective(slash, enemy) then return enemy end
			end
		end
		for _, enemy in ipairs(enemies) do
			if enemy:hasSkill("longhun") then return enemy end
		end
		return enemies[1]
	end
	return targets:first()
end

sgs.ai_playerchosen_intention.qianxigz = 80

sgs.ai_skill_choice.mihjih_draw = function(self, choices)
	return "" .. self.player:getLostHp()
end

sgs.ai_skill_invoke.mihjih = function(self, data)
	if #self.friends_noself == 0 then return false end
	for _, friend in ipairs(self.friends_noself) do
		if not friend:hasSkill("manjuan") and not self:isLihunTarget(friend) then return true end
	end
	return false
end

sgs.ai_skill_askforyiji.mihjih = function(self, card_ids)
	local available_friends = {}
	for _, friend in ipairs(self.friends_noself) do
		if not friend:hasSkill("manjuan") and not self:isLihunTarget(friend) then table.insert(available_friends, friend) end
	end

	local toGive, allcards = {}, {}
	local keep
	for _, id in ipairs(card_ids) do
		local card = sgs.Sanguosha:getCard(id)
		if not keep and (isCard("Jink", card, self.player) or isCard("Analeptic", card, self.player)) then
			keep = true
		else
			table.insert(toGive, card)
		end
		table.insert(allcards, card)
	end

	local cards = #toGive > 0 and toGive or allcards
	self:sortByKeepValue(cards, true)
	local id = cards[1]:getId()

	local card, friend = self:getCardNeedPlayer(cards)
	if card and friend and table.contains(available_friends, friend) then return friend, card:getId() end

	if #available_friends > 0 then
		self:sort(available_friends, "handcard")
		for _, afriend in ipairs(available_friends) do
			if not self:needKongcheng(afriend, true) then
				return afriend, id
			end
		end
		self:sort(available_friends, "defense")
		return available_friends[1], id
	end
	return nil, -1
end

sgs.ai_skill_playerchosen.zhongyoong = function(self, targetlist)
	self:sort(self.friends)
	if self:getCardsNum("Slash") > 0 then
		for _, friend in ipairs(self.friends) do
			if not targetlist:contains(friend) or friend:objectName() == self.player:objectName() then continue end
			if getCardsNum("Jink", friend, self.player) < 1 or sgs.card_lack[friend:objectName()]["Jink"] == 1 then
				return friend
			end
		end
		if self:getCardsNum("Jink") == 0 and targetlist:contains(self.player) then return self.player end
	end
	local lord = self.room:getLord()
	if self:isFriend(lord) and sgs.isLordInDanger() and targetlist:contains(lord) and getCardsNum("Jink", lord, self.player) < 2 then return lord end
	if self.role == "renegade" and targetlist:contains(self.player) then return self.player end
	return self:findPlayerToDraw(true, 1) or self.friends[1]
end

sgs.ai_skill_invoke.tsydyi = true

sgs.ai_skill_use["@@tsydyi"] = function(self)
	local current = self.room:getCurrent()
	local slash = sgs.Sanguosha:cloneCard("slash")
	if self:isEnemy(current) then
		if (getCardsNum("Slash", current, self.player) >= 1 or self.player:getPile("lock"):length() > 2)
		and not (current:hasWeapon("crossbow") or current:hasSkill("paoxiao")) then
			for _, player in sgs.qlist(self.room:getOtherPlayers(current)) do
				if self:isFriend(player) and player:distanceTo(current) <= current:getAttackRange()
				and self:slashIsEffective(slash, player) and (self:isWeak(player) or self.player:getPile("lock"):length() > 1) then
					return "@TsydyiCard=" .. self.player:getPile("lock"):first()
				end
			end
		end
	end
	return "."
end

sgs.ai_skill_playerchosen.youdi = function(self, targets)
	self.youdi_obtain_to_friend = false
	local throw_armor = self:needToThrowArmor()
	if throw_armor and #self.friends_noself > 0 and self.player:getCardCount(true) > 1 then
		for _, friend in ipairs(self.friends_noself) do
			if friend:canDiscard(self.player, self.player:getArmor():getEffectiveId())
				and (self:needToThrowArmor(friend) or (self:needKongcheng(friend) and friend:getHandcardNum() == 1)
					or friend:getHandcardNum() <= self:getLeastHandcardNum(friend)) then
				return friend
			end
		end
	end

	local valuable, dangerous = self:getValuableCard(self.player), self:getDangerousCard(self.player)
	local slash_ratio = 0
	if not self.player:isKongcheng() then
		local slash_count = 0
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if c:isKindOf("Slash") then slash_count = slash_count + 1 end
		end
		slash_ratio = slash_count / self.player:getHandcardNum()
	end
	if not valuable and not dangerous and slash_ratio > 0.45 then return nil end

	self:sort(self.enemies, "defense")
	self.enemies = sgs.reverse(self.enemies)
	for _, enemy in ipairs(self.enemies) do
		if enemy:canDiscard(self.player, "he") and not self:doNotDiscard(enemy, "he") then
			if (valuable and enemy:canDiscard(self.player, valuable)) or (dangerous and enemy:canDiscard(self.player, dangerous)) then
				if (self:getValuableCard(enemy) or self:getDangerousCard(enemy)) and sgs.getDefense(enemy) > 8 then return enemy end
			elseif not enemy:isNude() then return enemy
			end
		end
	end
end

sgs.ai_choicemade_filter.cardChosen.youdi_obtain = sgs.ai_choicemade_filter.cardChosen.snatch
local dinqpiin_skill = {}
dinqpiin_skill.name = "dinqpiin"
table.insert(sgs.ai_skills, dinqpiin_skill)
dinqpiin_skill.getTurnUseCard = function(self, inclusive)
	sgs.ai_use_priority.DinqpiinCard = 0
	if not self.player:canDiscard(self.player, "h") or self.player:getMark("dinqpiin") == 0xE then return false end
	for _, p in sgs.qlist(self.room:getAlivePlayers()) do
		if not p:hasFlag("dinqpiin") and p:isWounded() then
			if not self:toTurnOver(self.player) then sgs.ai_use_priority.DinqpiinCard = 8.9 end
			return sgs.Card_Parse("@DinqpiinCard=.")
		end
	end
end
sgs.ai_skill_use_func.DinqpiinCard = function(card, use, self)
	local cards = {}
	local cardType = {}
	for _, card in sgs.qlist(self.player:getHandcards()) do
		if bit32.band(self.player:getMark("dinqpiin"), bit32.lshift(1, card:getTypeId())) == 0 then
			table.insert(cards, card)
			if not table.contains(cardType, card:getTypeId()) then table.insert(cardType, card:getTypeId()) end
		end
	end
	for _, id in sgs.qlist(self.player:getPile("wooden_ox")) do
		local card = sgs.Sanguosha:getCard(id)
		if bit32.band(self.player:getMark("dinqpiin"), bit32.lshift(1, card:getTypeId())) == 0 then
			table.insert(cards, card)
			if not table.contains(cardType, card:getTypeId()) then table.insert(cardType, card:getTypeId()) end
		end
	end
	if #cards == 0 then return end
	self:sortByUseValue(cards, true)
	if self:isValuableCard(cards[1]) then return end

	if #cardType > 1 or not self:toTurnOver(self.player) then
		self:sort(self.friends)
		for _, friend in ipairs(self.friends) do
			if not friend:hasFlag("dinqpiin") and friend:isWounded() then
				use.card = sgs.Card_Parse("@DinqpiinCard=" .. cards[1]:getEffectiveId())
				if use.to then use.to:append(friend) end
				return
			end
		end
	end

end

sgs.ai_use_priority.DinqpiinCard = 0
sgs.ai_card_intention.DinqpiinCard = -10

sgs.ai_skill_playerchosen.myngjiann = function(self, targets)
    if (sgs.ai_skill_invoke.fangquan(self) or self:needKongcheng(self.player)) then
        local cards = sgs.QList2Table(self.player:getHandcards())
        self:sortByKeepValue(cards)
        if sgs.current_mode_players.rebel == 0 then
            local lord = self.room:getLord()
            if lord and self:isFriend(lord) and lord:objectName() ~= self.player:objectName() then
                return lord
            end
        end

        local AssistTarget = self:AssistTarget()
        if AssistTarget and not self:willSkipPlayPhase(AssistTarget) then
            return AssistTarget
        end

        self:sort(self.friends_noself, "chaofeng")
        return self.friends_noself[1]
    end
    return nil
end

sgs.ai_playerchosen_intention.myngjiann = -80

sgs.ai_skill_invoke["taoxi"] = function(self, data)
    local to = data:toPlayer()
    if to and self:isEnemy(to) then

        if self.player:hasSkill("lihun") and to:isMale() and not self.player:hasUsed("LihunCard") then
            local callback = lihun_skill.getTurnUseCard
            if type(callback) == "function" then
                local skillcard = callback(self)
                if skillcard then
                    local dummy_use = {
                        isDummy = true,
                        to = sgs.SPlayerList(),
                    }
                    self:useSkillCard(skillcard, dummy_use)
                    if dummy_use.card then
                        for _,p in sgs.qlist(dummy_use.to) do
                            if p:objectName() == to:objectName() then
                                return true
                            end
                        end
                    end
                end
            end
        end

        if self.player:hasSkill("dimeng") and not self.player:hasUsed("DimengCard") then
            local callback = dimeng_skill.getTurnUseCard
            if type(callback) == "function" then
                local skillcard = callback(self)
                if skillcard then
                    local dummy_use = {
                        isDummy = true,
                        to = sgs.SPlayerList(),
                    }
                    self:useSkillCard(skillcard, dummy_use)
                    if dummy_use.card then
                        for _,p in sgs.qlist(dummy_use.to) do
                            if p:objectName() == to:objectName() then
                                return true
                            end
                        end
                    end
                end
            end
        end

        local dismantlement_count = 0
        if to:hasSkill("noswuyan") or to:getMark("@late") > 0 then
            if self.player:hasSkill("yinling") then
                local black = self:getSuitNum("spade|club", true)
                local num = 4 - self.player:getPile("brocade"):length()
                dismantlement_count = dismantlement_count + math.max(0, math.min(black, num))
            end
        else
            dismantlement_count = dismantlement_count + self:getCardsNum("Dismantlement")
            if self.player:distanceTo(to) == 1 or self:hasSkills("qicai|nosqicai") then
                dismantlement_count = dismantlement_count + self:getCardsNum("Snatch")
            end
        end

        local handcards = to:getHandcards()
        if dismantlement_count >= handcards:length() then
            return true
        end

        local can_use, cant_use = {}, {}
        for _,c in sgs.qlist(handcards) do
            if self.player:isCardLimited(c, sgs.Card_MethodUse, false) then
                table.insert(cant_use, c)
            else
                table.insert(can_use, c)
            end
        end

        if #can_use == 0 and dismantlement_count == 0 then
            return false
        end

        if self:needToLoseHp() then
            --Infact, needToLoseHp now doesn't mean self.player still needToLoseHp when this turn end.
            --So this part may make us upset sometimes... Waiting for more details.
            return true
        end

        local knowns, unknowns = {}, {}
        local flag = string.format("visible_%s_%s", self.player:objectName(), to:objectName())
        for _,c in sgs.qlist(handcards) do
            if c:hasFlag("visible") or c:hasFlag(flag) then
                table.insert(knowns, c)
            else
                table.insert(unknowns, c)
            end
        end

        if #knowns > 0 then --Now I begin to lose control... Need more help.
            local can_use_record = {}
            for _,c in ipairs(can_use) do
                can_use_record[c:getId()] = true
            end

            local can_use_count = 0
            local to_can_use_count = 0
            local function can_use_check(user, to_use)
                if to_use:isKindOf("EquipCard") then
                    return not user:isProhibited(user, to_use)
                elseif to_use:isKindOf("BasicCard") then
                    if to_use:isKindOf("Jink") then
                        return false
                    elseif to_use:isKindOf("Peach") then
                        if user:hasFlag("Global_PreventPeach") then
                            return false
                        elseif user:getLostHp() == 0 then
                            return false
                        elseif user:isProhibited(user, to_use) then
                            return false
                        end
                        return true
                    elseif to_use:isKindOf("Slash") then
                        if to_use:isAvailable(user) then
                            local others = self.room:getOtherPlayers(user)
                            for _,p in sgs.qlist(others) do
                                if user:canSlash(p, to_use) then
                                    return true
                                end
                            end
                        end
                        return false
                    elseif to_use:isKindOf("Analeptic") then
                        if to_use:isAvailable(user) then
                            return not user:isProhibited(user, to_use)
                        end
                    end
                elseif to_use:isKindOf("TrickCard") then
                    if to_use:isKindOf("Nullification") then
                        return false
                    elseif to_use:isKindOf("DelayedTrick") then
                        if user:containsTrick(to_use:objectName()) then
                            return false
                        elseif user:isProhibited(user, to_use) then
                            return false
                        end
                        return true
                    elseif to_use:isKindOf("Collateral") then
                        local others = self.room:getOtherPlayers(user)
                        local selected = sgs.PlayerList()
                        for _,p in sgs.qlist(others) do
                            if to_use:targetFilter(selected, p, user) then
                                local victims = self.room:getOtherPlayers(p)
                                for _,p2 in sgs.qlist(victims) do
                                    if p:canSlash(p2) then
                                        return true
                                    end
                                end
                            end
                        end
                    elseif to_use:isKindOf("ExNihilo") then
                        return not user:isProhibited(user, to_use)
                    else
                        local others = self.room:getOtherPlayers(user)
                        for _,p in sgs.qlist(others) do
                            if not user:isProhibited(p, to_use) then
                                return true
                            end
                        end
                        if to_use:isKindOf("GlobalEffect") and not user:isProhibited(user, to_use) then
                            return true
                        end
                    end
                end
                return false
            end
            for _,c in ipairs(knowns) do
                if can_use_record[c:getId()] and can_use_check(self.player, c) then
                    can_use_count = can_use_count + 1
                end
            end

            local to_friends = self:getFriends(to)
            local to_has_weak_friend = false
            local to_is_weak = self:isWeak(to)
            for _,friend in ipairs(to_friends) do
                if self:isEnemy(friend) and self:isWeak(friend) then
                    to_has_weak_friend = true
                    break
                end
            end

            local my_trick, my_slash, my_aa, my_duel, my_sa = nil, nil, nil, nil, nil
            local use = self.player:getTag("taoxi_carduse"):toCardUse()
            local ucard = use.card
            if ucard:isKindOf("TrickCard") then
                my_trick = 1
                if ucard:isKindOf("Duel") then
                    my_duel = 1
                elseif ucard:isKindOf("ArcheryAttack") then
                    my_aa = 1
                elseif ucard:isKindOf("SavageAssault") then
                    my_sa = 1
                end
            elseif ucard:isKindOf("Slash") then
                my_slash = 1
            end
            
            for _,c in ipairs(knowns) do
                if isCard("Nullification", c, to) then
                    my_trick = my_trick or ( self:getCardsNum("TrickCard") - self:getCardsNum("DelayedTrick") )
                    if my_trick > 0 then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                end
                if isCard("Jink", c, to) then
                    my_slash = my_slash or self:getCardsNum("Slash")
                    if my_slash > 0 then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                    my_aa = my_aa or self:getCardsNum("ArcheryAttack")
                    if my_aa > 0 then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                end
                if isCard("Peach", c, to) then
                    if to_has_weak_friend then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                end
                if isCard("Analeptic", c, to) then
                    if to:getHp() <= 1 and to_is_weak then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                end
                if isCard("Slash", c, to) then
                    my_duel = my_duel or self:getCardsNum("Duel")
                    if my_duel > 0 then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                    my_sa = my_sa or self:getCardsNum("SavageAssault")
                    if my_sa > 0 then
                        to_can_use_count = to_can_use_count + 1
                        continue
                    end
                end
            end

            if can_use_count >= to_can_use_count + #unknowns then
                return true
            elseif can_use_count > 0 and ( can_use_count + 0.01 ) / ( to_can_use_count + 0.01 ) >= 0.5 then
                return true
            end
        end

        if self:getCardsNum("Peach") > 0 then
            return true
        end
    end
    return false
end
 
local zhenshan_skill = {}
zhenshan_skill.name = "zhenshan"
table.insert(sgs.ai_skills, zhenshan_skill)
zhenshan_skill.getTurnUseCard = function(self)
    if self.player:hasUsed("ZhenshanCard") or not ZhenshanTarget(self) then
        return nil
    elseif self.player:getLostHp() > 0 then
        return sgs.Card_Parse("@ZhenshanCard=.:peach")
    elseif sgs.Slash_IsAvailable(self.player) then
        return sgs.Card_Parse("@ZhenshanCard=.:slash")
    end
end
 
sgs.ai_skill_use_func.ZhenshanCard = function(card, use, self)
    local target = ZhenshanTarget(self)
    if not target then return end
    local pattern = nil
    local use_peach, use_slash = false, false
     
    if self.player:getLostHp() > 0 and self:isWeak() then
        local peach = sgs.Sanguosha:cloneCard("peach")
        peach:deleteLater()
        local dummy_use = {
        isDummy = true,
        }
        self:useBasicCard(peach, dummy_use)
        if dummy_use.card then
            use_peach = true
            pattern = "peach"
        end
    end
     
     
    if not use_peach then
        if sgs.Slash_IsAvailable(self.player) then
            local slash = sgs.Sanguosha:cloneCard("slash")
            slash:deleteLater()
            local dummy_use = {
                isDummy = true,
            }
            self:useBasicCard(slash, dummy_use)
            if dummy_use.card then
                use_slash = true
                pattern = "slash"
            end
        end
    end 
     
    if not use_slash then
        if not use_peach and self.player:getLostHp() > 0 then
            local peach = sgs.Sanguosha:cloneCard("Peach")
            peach:deleteLater()
            local dummy_use = {
                isDummy = true,
            }
            self:useBasicCard(peach, dummy_use)
            if dummy_use.card then
                use_peach = true
                pattern = "peach"
            end
        end
    end
     
    if pattern then
        local card_str = "@ZhenshanCard=.:"..pattern
        local acard = sgs.Card_Parse(card_str)
        use.card = acard
    end
end
 
sgs.ai_cardsview_valuable.zhenshan = function(self, class_name, player) 
    if not ZhenshanTarget(self) then return end
    local pattern = nil
    if class_name == "Slash" then
        pattern = "slash"
    elseif class_name == "Jink" then
        pattern = "jink"
    elseif class_name == "Peach" then
        pattern = "peach"
    end
    if pattern then
        local card_str = "@ZhenshanCard=.:"..pattern
        return card_str
    end
end
 
sgs.ai_skill_playerchosen.zhenshan = function(self, targets)
    return ZhenshanTarget(self)
end
 
sgs.ai_skill_invoke.zhenshan = function(self, data)
   if ZhenshanTarget(self) then return true end
   return false
end

angwo_skill = {name = "angwo"}
table.insert(sgs.ai_skills, angwo_skill)
angwo_skill.getTurnUseCard = function(self)
    if (not self.player:hasUsed("AngwoCard")) then
        return sgs.Card_Parse("@AngwoCard=.")
    end

    return nil
end

sgs.ai_skill_use_func.AngwoCard = function(card, use, self)
    local l = {}
    function calculateMinus(player, range)
        if range <= 0 then return 0 end
        local n = 0
        for _, p in sgs.qlist(self.room:getAlivePlayers()) do
            if player:inMyAttackRange(p) and not player:inMyAttackRange(p, range) then n = n + 1 end
        end
        return n
    end

    function filluse(to, id)
        use.card = card
        if (use.to) then use.to:append(to) end
        self.angwoid = id
    end

    for _, p in ipairs(self.enemies) do
        if (p:getWeapon()) then
            local weaponrange = p:getWeapon():getRealCard():toWeapon():getRange()
            local n = calculateMinus(p, weaponrange - 1)
            table.insert(l, {player = p, id = p:getWeapon():getEffectiveId(), minus = n})
        end
        if (p:getOffensiveHorse()) then
            local n = calculateMinus(p, 1)
            table.insert(l, {player = p, id = p:getOffensiveHorse():getEffectiveId(), minus = n})
        end
    end

    if #l > 0 then
        function sortByMinus(a, b)
            return a.minus > b.minus
        end

        table.sort(l, sortByMinus)
        if l[1].minus > 0 then
            filluse(l[1].player, l[1].id)
            return
        end
    end

    for _, p in ipairs(self.enemies) do
        if (p:getTreasure()) then
            filluse(p, p:getTreasure():getEffectiveId())
            return
        end
    end

    for _, p in ipairs(self.friends_noself) do
        if (self:needToThrowArmor(p) and p:getArmor()) then
            filluse(p, p:getArmor():getEffectiveId())
            return
        end
    end

    self:sort(self.enemies, "threat")
    for _, p in ipairs(self.enemies) do
        if (p:getArmor() and not p:getArmor():isKindOf("GaleShell")) then
            filluse(p, p:getArmor():getEffectiveId())
            return
        end
    end

    for _, p in ipairs(self.enemies) do
        if (p:getDefensiveHorse()) then
            filluse(p, p:getDefensiveHorse():getEffectiveId())
            return
        end
    end

    for _, p in ipairs(self.friends) do
        if (p:hasSkill("xiaoji") or p:hasSkill("xuanfeng") or p:hasSkill("nosxuanfeng")) then
            filluse(p, p:getCards("e"):first():getEffectiveId())
            return
        end
    end
end

sgs.ai_use_priority.AngwoCard = sgs.ai_use_priority.ExNihilo + 0.01

sgs.ai_skill_cardchosen.angwo = function(self)
    return self.angwoid
end

zhanjue_skill = {name = "zhanjue"}
table.insert(sgs.ai_skills, zhanjue_skill)
zhanjue_skill.getTurnUseCard = function(self)
    if (self.player:getMark("zhanjuedraw") >= 2) then return nil end

    if (self.player:isKongcheng()) then return nil end

	if self.player:getHandcardNum() > 1 then
		for _, c in sgs.qlist(self.player:getHandcards()) do
			if willUse(self, c:getClassName()) then return nil end
		end
	end
    local duel = sgs.Sanguosha:cloneCard("duel", sgs.Card_SuitToBeDecided, -1)
    duel:addSubcards(self.player:getHandcards())
    duel:setSkillName("zhanjue")
	return duel
end
sgs.ai_use_priority.zhanjue_skill = sgs.ai_use_priority.ExNihilo - 0.1
