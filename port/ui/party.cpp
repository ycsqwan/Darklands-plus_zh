// party.cpp - a short word on what each attribute and skill is for (the character sheet's Skills
// page shows it under the pointer, with the whole party's values).
static const char *ATTR_HELP[7] = {
    "生命值与耐力。伤势会消耗它们；当生命值为0时，角色会昏迷。休息可使其恢复。",
    "身体力量。武器与盔甲需要最低{力量}（商店的工具提示中显示），以及{耐力}和"
    "{力量}共同决定角色能携带的重量。严重伤势也会消耗力量；当力量为0时，角色会死亡。",
    "速度与灵巧，包括身体与手部的敏捷度。",
    "警觉：察觉事物、人物与危险。",
    "智慧与学识。",
    "风采：他人对角色的印象。",
    "圣人的善意。呼唤圣人会消耗它；在旅店祈祷并等待时间，即可恢复。",
};
static const char *SKILL_HELP[19] = {
    0, 0, 0, 0, 0, 0, 0,
    "根据角色所知的配方调制药剂：炼金术水平越高，失败几率越低（角色面板的“配方”标签显示每位成员的成功率）。",
    "对信仰及其圣人的了解：学习圣人知识，与教会打交道。",
    "道德纯洁度，受圣人与教会重视。罪恶行为会降低此值。",
    "用普通人的语言与他们交谈：询问、说服、虚张声势。",
    "教会与学者的语言：神父、修士、大学。",
    "读写能力：信件、书籍与配方。",
    "处理伤口与疾病。",
    "机械技能：锁具、陷阱与装置。",
    "不被发现、不被听见地移动与隐藏。",
    "城市街道及其地下世界的种种门道。",
    "驾驭马匹。",
    "野外生活：寻找方向、狩猎、辨认踪迹。",
};

const char *ui_attr_help(int a) { return a >= 0 && a < 7 ? ATTR_HELP[a] : ""; }       /* (the character sheet's strip) */
const char *ui_skill_help(int k) { return k >= 0 && k < 19 && SKILL_HELP[k] ? SKILL_HELP[k] : ""; }
