-- translation for WisdomPackage

local t = {


-- effects
	["cv:wisxuyou"] = "庞小鸡",
	["$juao1"] = "用吾之计，敌可轻取~",
	["$juao2"] = "阿瞒，卿不得我，不得冀州也。", -- 对曹操
	["$tanlan1"] = "汝等小计，何足道哉", -- 发动拼点
	["$tanlan2"] = "匹夫尔敢如此", -- 拼点失败;拼点成功触发恃才
	["$shicai2"] = "真是不自量力！",
	["$shicai"] = "莽夫，无可救药啊。", -- 贪婪触发
	["~wis_xuyou"] = "汝等果然是无可救药~啊~",

	["cv:wisjiangwei"] = "Jr. Wakaran",
	["$yicai1"] = "系从尚父出，术奉武侯来。",
	["$yicai2"] = "天水麒麟儿，异才敌千军。",
	["$beifa1"] = "北伐兴蜀汉，继志越祁山",
	["$beifa2"]= "哀侯悲愤填心胸，九伐中原亦无悔。",
	["~wis_jiangwei"] = "终究还是回天乏术吗？",

	["cv:wisjiangwan"] = "喵小林",
	["$houyuan"] = "汝等力战，吾定当稳固后方。",
	["$chouliang"] = "息民筹粮，伺机反攻。",
	["~wis_jiangwan"] = "蜀中疲敝，无力辅政矣",

	["cv:wissunce"] = "裤衩",
	["$bawang1"] = "匹夫，可敢与我一较高下？", -- 提出拼点
	["$bawang2"] = "虎踞鹰扬，霸王之武。", -- 胜利
	["$weidai1"] = "吾之将士，借我一臂之力！", -- 回合内发动
	["$weidai2"] = "我江东子弟何在？", -- 濒死时发动
	["~wis_sunce"] = "百战之躯，竟遭暗算……",

	["cv:wiszhangzhao"] = "喵小林",
	["$longluo1"] = "吾当助汝，共筑功勋。",
	["$longluo2"] = "江东多才俊，吾助主揽之。", -- 对孙权+孙策
	["$fuzuo1"] = "尽诚匡弼，成君之业。",
	["$fuzuo2"] = "此言，望吾主慎之重之。", -- 对孙权+孙策
	["$jincui1"] = "从吾之谏，功业可成。", -- 摸牌
	["$jincui2"] = "此人贼心昭著，当趋逐之。", -- 弃牌
	["~wis_zhangzhao"] = "尽力辅佐，吾主为何……",

	["cv:wishuaxiong"] = "极光星逝",
	["$badao"] = "三合之内，吾必斩汝！",
	["$wenjiu1"] = "有末将在，何需温侯出马？", -- 触发黑色杀
	["$wenjiu2"] = "好快……", -- 触发红色杀（凄惨点）
	["~wis_huaxiong"] = "来将何人……啊……",

	["cv:wistianfeng"] = "喵小林",
	["$gushou1"] = "外结英雄，内修农战。", -- 失去闪 桃 酒
	["$gushou2"] = "奇兵迭出，扰敌疲兵。", -- 失去杀
	["$shipo1"] = "此中有诈，吾当出计破之。",
	["$shipo2"] = "休要蒙蔽我主！", -- 对主公袁绍
	["$yuwen"] = "吾当自死，不劳明公动手。",
	["~wis_tianfeng"] = "不识其主，虽死何惜。",

	["cv:wisshuijing"] = "喵小林",
	["$shouye1"] = "授汝等之策，自可平定天下。", -- 用红桃
	["$shouye2"] = "为天下太平，还望汝等尽力。", -- 用方片
	["$jiehuo"] = "桃李满天下，吾可归隐矣。",
	["$shien1"] = "吾师教诲，终身不忘。",
	["$shien2"] = "龙凤之才，全赖吾师。", -- 龙凤发动师恩
	["~wis_shuijing"] = "儒生俗士，终究难平天下吗？",
}

local generals = {"wisxuyou", "wisjiangwei", "wisjiangwan", "wissunce", "wiszhangzhao", "wishuaxiong", "wistianfeng", "wisshuijing"}

for _, general in ipairs(generals) do
	t["designer:" .. general] = t["designer:wisdoms"]
end

return t
