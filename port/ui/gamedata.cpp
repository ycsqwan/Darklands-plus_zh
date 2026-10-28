// gamedata.cpp - see gamedata.h
#include "gamedata.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <strings.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#endif

GameData gamedata;
static std::vector<std::string> formula_raw;      /* the formulae's names as DARKLAND.LST writes them (the game's own letters) */
extern "C" int gamedata_formula_raw(int f, char *out, int n)   /* (for text the game shows: a card's $Text2) */
{
    if (f < 0 || f >= (int)formula_raw.size() || n < 1) return 0;
    snprintf(out, (size_t)n, "%s", formula_raw[(size_t)f].c_str());
    return 1;
}
extern "C" const char *datamods_redirect(const char *rel, const char *game_path);
extern "C" {
#include "../runtime/cpu.h"
#include "../runtime/game.h"
}

static FILE *open_ci(const char *dir, const char *name)
{
    { const char *m = datamods_redirect(name, ""); if (m) { FILE *f = fopen(m, "rb"); if (f) return f; } }   /* a mod's file */
    DIR *d = opendir(dir);
    char path[1200];
    if (d) {
        struct dirent *e;
        while ((e = readdir(d))) {
            if (!strcasecmp(e->d_name, name)) {
                snprintf(path, sizeof path, "%s/%s", dir, e->d_name);
                closedir(d);
                return fopen(path, "rb");
            }
        }
        closedir(d);
    }
    snprintf(path, sizeof path, "%s/%s", dir, name);
    return fopen(path, "rb");
}

static std::vector<unsigned char> read_all(FILE *f)
{
    std::vector<unsigned char> v;
    if (!f) return v;
    unsigned char b[4096];
    size_t n;
    while ((n = fread(b, 1, sizeof b, f)) > 0) v.insert(v.end(), b, b + n);
    fclose(f);
    return v;
}

static int w16(const unsigned char *p) { return p[0] | (p[1] << 8); }

const char *SHOP_NAMES[9] = { "锻铁匠", "商品商人", "锻剑师", "锻甲师", "枪械匠", "弓箭匠",
                              "工艺师", "珠宝匠", "布料商" };

std::string game_text(const char *s, size_t n)
{
    std::string o;
    for (size_t i = 0; i < n && s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        /* 0xc2..0xf4 are the only legal UTF-8 lead bytes; every byte the game
           uses as a colour code (0x80..0xbf, 0xff) falls outside that window,
           so translated UTF-8 passes through while colour codes still drop. */
        if (c >= 0xc2 && c <= 0xf4) {
            int len = c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
            bool ok = (i + len <= n);
            for (int k = 1; ok && k < len; k++)
                if (((unsigned char)s[i + k] & 0xc0) != 0x80) ok = false;
            if (ok) { o.append(s + i, len); i += len - 1; continue; }
        }
        switch (s[i]) {
        case 0x1f: o += "\xc3\xa4"; break;    // a umlaut (the game's own code for it)
        case '{': o += "\xc3\xb6"; break;     // o umlaut
        case '|': o += "\xc3\xbc"; break;     // u umlaut
        case '}': o += "\xc3\xa4"; break;     // a umlaut
        case '~': o += "\xc3\x9f"; break;     // sharp s
        default: if ((unsigned char)s[i] >= 32 && (unsigned char)s[i] < 127) o += s[i]; else if (s[i] == '\n' || s[i] == '\r') o += ' ';
        }
    }
    return o;
}

static ItemKind classify(const ItemDef &d)
{
    int f = d.flags;
    if (f & 0x4000) return K_DOCUMENT;
    if (f & 0x2000) return K_HORSE;
    if (f & 0x1000) return K_RELIC;
    if (f & 0x800) return K_POTION;
    if (f & 0x400) return K_INGREDIENT;
    if ((f & 0x300) == 0x300) return K_INSTRUMENT;
    if (f & 0x200) return K_TOOL;
    if (f & 0x100) return K_GEAR;
    /* the item type numbers (LST +30) group the equipment; the flags leave crossbows, guns,
       ammunition and the soft armours unmarked */
    int t = d.type;
    if (t >= 95 && t <= 97) return K_SHIELD;
    if (t >= 67 && t <= 84) return K_ARMOR;
    if ((t >= 23 && t <= 33) || (t >= 64 && t <= 66)) return K_MISSILE;
    if (t >= 0 && t <= 22 && (f & 0x0f)) return K_WEAPON;
    if (f & 0x80) return K_SHIELD;
    if (f & 0x40) return K_ARMOR;
    if (f & 0x30) return K_MISSILE;
    if (f & 0x0f) return K_WEAPON;
    if (d.flags2 & 0x80) return K_QUEST;
    return K_OTHER;
}

const char *kind_name(ItemKind k)
{
    static const char *n[] = { "武器", "投射武器", "盔甲", "盾牌", "装备", "工具", "乐器",
                               "炼金术原料", "药剂", "圣物", "马匹", "文件", "奇珍异宝", "其他" };
    return n[k];
}
const char *kind_name_one(ItemKind k)
{
    static const char *n[] = { "武器", "投射武器", "盔甲", "盾牌", "装备", "工具", "乐器",
                               "炼金术原料", "药剂", "遗物", "马匹", "文件", "好奇心", "物品" };
    return n[k];
}

/* the lives of the saints, as DARKLAND.SNT holds them (indexed like gamedata.saints) */
static const char *const SAINT_TEXT_ZH[136] = {
    "作为士兵与屠夫的守护圣人，他曾是一名异教徒军官，后皈依基督教，并在临终前遭受了极其残酷的折磨。他能适度提升魅力与耐力（仅治疗效果），并大幅增强所有武器技能。",
    "钟声与护士的守护圣人，她在罗马迫害期间遭受了诸多酷刑。艺术作品中的描绘促使人们在她的圣日为面包祈福。她能适度提升魅力与耐力（仅限治疗）；女性则可大幅提升耐力。",
    "一位象征贞洁与纯真的伟大基督徒，阿格尼丝除非队伍中至少有一名女性，否则不会提供任何加成。她能大幅提升当地的声望与魅力。",
    "一位爱尔兰圣人（亦称马多克），以其神奇的斋戒与祈祷而闻名，艾丹还以仁慈与对动物的热爱著称。他能大幅提升木系属性，并可能使队伍与野生动物交流或操控它们。",
    "学习的守护圣人，他极大地提升了炼金术，大幅提高了说拉丁语和读写能力，并为智力与感知带来适度的提升。",
    "作为初等教育与高等教育的热忱信徒，阿尔昆于781年被任命为查理曼的大臣。他极大地提升了拉丁语和读写能力，并适度提高了智力。此外，他还使贵族对本派更加友好。",
    "乞丐的守护圣人阿莱克西斯以其极端贫困的誓言而闻名，他在自己的家中以乞丐的身份默默度过了十七年。他能大幅提升当地声望，并适度提升魅力。此外，若队伍陷入贫困，他还会赐予他们金钱。",
    "苏格兰、俄罗斯及渔民的守护圣人安德鲁是十二使徒之一。他能适度提升耐力与魅力，并为苏格兰或俄罗斯的地点声望带来可观的加成。",
    "罗马军队中的百夫长安德鲁呼求基督，蒙受了胜利。他与部下皈依了信仰，随后被捕并被杀害。他在使用刃器、冲击武器、长杆武器、投掷武器以及弓箭时，技能大幅提升。",
    "皮肤病、家畜与篮匠的守护圣人安东尼，以其禁欲主义和对上帝的虔诚而闻名。他能小幅提升力量与耐力（仅限治疗效果），并增强魅力。他还大幅提高通用语与拉丁语的使用能力，以及读写能力。此外，人们也可祈求他驱逐恶魔。",
    "贫困者、葡萄牙、遗失物品与丰收的守护圣人——帕多瓦的安东尼以其激情澎湃的布道和强大的皈依能力而闻名。他能小幅提升力量，适度提升感知，显著增强冲击武器与拉丁语能力，并大幅提高魅力与通用语言能力。",
    "出身高贵的阿努尔夫是一名战士，在7世纪初的国家事务中颇具影响力。他能小幅提升力量和智力，适度提升读写、拉丁语、街头智慧、骑术和感知，并大幅提高所有武器技能。",
    "以其教义与著述闻名；因其为罗马皇帝所撰的《基督教辩护》而被称为“护教者”。在战斗中，他能令敌人退却或逃离。此外，他还可治愈少量流失的力量与相当数量的流失的耐力。",
    "枪手、矿工、建筑工人和炮兵的守护圣人。她被亲生父亲出卖，为信仰而成为贞洁殉道者。她极大地提升机巧，并大幅增强机械飞弹技能。此外，她还能在一段时间内使伤口不再致死（力量永不低于1）。",
    "也被称为巴蒂尔德；她曾是法国国王克洛维治下，被赐予皇宫市长的一名女奴。649年，他与她成婚；在其去世后，她以摄政身份执政，并推行了许多伟大的事业。她能小幅提升智力与感知，还可被召唤以解救队伍于囚禁之中。",
    "一位以摧毁异教崇拜之物——雷神之橡树而闻名的成功教师与传道者。他能适度提升魅力、所有武器技能以及通用语言能力。他可被召唤来摧毁异教圣地、净化受污染之地，并保护队伍免受异教与撒旦系生物的侵害。",
    "作为哲学家、传教士、学生、图书馆员和少女的守护圣人，凯瑟琳是十四位圣助之一，以其信仰与皈依而闻名。她能小幅提升智力，大幅提高魅力，并显著增强通用语言能力。",
    "作为意大利的守护圣人，也是她那个时代最强大的基督教神秘主义者之一，凯瑟琳以虔诚地救治病患而闻名。她能为感知和魅力带来良好但不稳定的提升，并大幅提高说通用语的能力；此外，她还能帮助施法者察觉邪恶或做预言之梦。",
    "音乐的守护圣人，她曾被迫结婚，却感化了丈夫。因信仰遭谴责，她奇迹般地免于窒息而死的刑罚。她能小幅提升魅力，并在持有乐器时大幅提高当地声望。她还能救众人于窒息之危。",
    "诺森布里亚的前国王塞奥伍尔夫并非一位杰出的领袖，尽管他因虔诚而备受赞誉。他的陵墓曾发生过许多神迹。他能小幅提升智力与感知，并适度提升力量、拉丁语以及所有武器技能。",
    "智慧的十位女儿之一，当古罗马人将她投入熔炉时，她毫发无损地走了出来。她既能提升所有人的火焰抗性，又能大幅提高一名选定之人的火焰抗性；此外，她还能治愈少量的耐力。",
    "在21岁时，克里斯蒂娜显然因癫痫发作而去世。在她的弥撒期间，她突然苏醒并飞到了教堂的屋顶上。许多人将许多悬浮现象归因于她。她能为魅力带来小幅但不稳定的提升，并为敏捷带来适度的提升；有时她还能带领队伍摆脱困境。",
    "旅行者与朝圣者的守护圣人，曾带领旅人渡过河流。他为基督忍受了无数酷刑与袭击。他能提升旅行速度，恢复少量力量与适度的耐力，并大幅提高弓箭类武器与街头智慧的属性，同时显著增强木工与骑术的属性。",
    "克拉拉以极度虔诚与严守清贫之誓而闻名，她在方济各会的传播及阿西西的保卫中发挥了重要作用。她能够恢复力量与耐力的损耗，并使撒旦的敌人在战斗中溃败。",
    "她于492年嫁给了法兰克国王克洛维，并因在战役中拯救了他的军队之一而备受赞誉。丈夫去世后，她毕生致力于救助病患。她能适度提升魅力，并大幅提升治疗效果；同时还能小幅恢复力量并大幅恢复耐力。",
    "爱尔兰克洛因的守护圣人科尔曼是一位诗人，后来成为卡谢尔的王室吟游诗人。他能为魅力和拉丁语带来适度的提升，为读写能力带来不错的提升，而为通用语演讲则带来非常显著的提升。",
    "作为爱尔兰的一位权势人物，科伦巴为赎罪而前往苏格兰的皮克特人中开展传教工作，此前他曾引发两座修道院之间的争斗。他能小幅提升魅力，适度提升耐力和力量，并大幅提高所有近战武器技能。此外，他还会使感知暂时小幅下降。",
    "理发师与医师的守护圣人科斯马斯是达米安的孪生兄弟；两人在生前与死后都行了许多神迹。科斯马斯能小幅提升感知，大幅提升治疗效果；他还可少量恢复力量与耐力，并传授有关圣达米安的知识。",
    "守护圣人（与其兄弟克里斯皮尼亚努斯一同）是鞋匠、皮革工匠和制鞋工的守护者。克里斯皮尼亚努斯本是一名制鞋匠，曾使许多人皈依。所有非金属护甲的品质均可得到提升。",
    "一名试图以魔法手段赢得一名基督教少女芳心的术士与黑魔法师。她的信仰比他的魔法更为强大；他弃绝了邪恶之道，皈依了正道。他能小幅提升智力与炼金术，并使撒旦系魔法有50%的几率失效。",
    "理发师与医师的守护圣人达米安是科斯马的孪生兄弟；两人在生前与死后都行了许多神迹。达米安能恢复所有已丧失的耐力，并补足相当一部分已丧失的力量；此外，他还使你自动掌握圣科斯马的知识。",
    "威尔士的守护圣人，也是一位曾为英格兰男爵、后又成为苏格兰国王的贵族；他曾为争夺英格兰王位而战，但未获成功。他以慈善、虔诚与公正著称。可使魅力与智力小幅提升，并适度提高感知、武器技能及骑术。",
    "巴黎与法国的守护圣人，因成功使众人皈依而被斩首。他的追随者从塞纳河中救出了他的遗体。能恢复大量流失的力量，并少量恢复耐力；显著提升魅力与通用语能力，还能提升巴黎与法国当地的声望。",
    "在早年，他曾是一名与亚瑟王并肩作战的士兵；后来，他的木制雕像被赋予了诸多神迹。它能小幅提升力量，治愈少量耐力，并大幅增强所有武器技能。",
    "摩纳哥与科西嘉的守护圣人德沃塔是一位年轻的基督徒女孩，她因信仰而遭受酷刑致死。她能够恢复所有已损失的耐力和部分力量，并帮助队伍抵御或逃脱酷刑。她的魅力显著提升，还可能提高科西嘉与摩纳哥当地的声望。",
    "盗贼与被定罪之人的守护者迪斯马斯，据信是与基督一同被钉在十字架上的那名盗贼。他能为全队大幅提升敏捷、木艺与潜行，同时大幅增强街头智慧与巧思。他还提高魅力与通用语能力，并可能使囚禁中的逃脱尝试得以成功。",
    "作为天文学家的守护圣人和多明我会的创始人，多明我还在西班牙广泛游历。他能大幅提升智力、通用语、拉丁语以及读写能力。此外，他还能使宗教裁判所的成员对队伍表现出尊重与敬意。",
    "普鲁士的守护圣人，一位温和谦逊的朝圣者，曾驯服了一位暴戾的父亲——但泽的剑匠阿尔布雷希特。她能感知邪恶，尤其是在德国人之中，并大幅提升魅力与治疗能力，同时适度提升感知。她甚至可能为有需要的人赐予一把神奇的长剑。",
    "牧羊人的守护圣人，他本人也是一位朝圣者、牧羊人和隐士。他能适度提升感知，显著提升治疗效果，并大幅提高木艺技能。此外，他还能够提升队伍的移动速度，但不会增加遭遇战的频率。",
    "锻造师、盔甲匠、金匠、锁匠和珠宝匠的守护圣人邓斯坦，能适度提升感知、武器技能、炼金术和街头智慧，并大幅提高机巧。此外，队伍中所有金属盔甲的品质也可得到提升。",
    "癫痫患者、精神疾病患者以及被恶魔附身者的守护圣人迪姆芙娜，因拒绝了父亲的乱伦之念而被其杀害。她能显著提升耐力，并使敏捷与治疗能力得到提升，同时大幅增强隐匿能力；召唤她可治愈多种疾患，甚至令撒旦系生物闻风而逃。",
    "在威廉之前的一位英格兰国王，他创建了威斯敏斯特大教堂。当他被召唤时，他会确保所有队员的某项属性至少达到20，但会付出巨大的财富代价。他还能帮助发现贵族中的邪恶，适度提升智力、感知、所有武器技能以及骑术，并治愈少量耐力。",
    "金属工匠与金匠的守护圣人埃利吉乌斯以其精美的手工艺和慷慨的慈善捐赠而闻名。他能大幅提升工艺，适度提升炼金术，并可能强化刀剑、锤子、狼牙棒、长戟、弩以及手枪。",
    "抵御地震的守护圣人埃米迪乌斯曾是一名异教骑士，后来皈依了基督教，并成功地使他人也皈依。他能大幅提升魅力与通用语言能力，还能让队伍在特定区域抵御地震，或召唤地震来摧毁异教或撒旦的据点。",
    "一位旅行者、十字军战士兼政治家，恩格尔贝特曾举兵对抗奥托四世，并因此被逐出教会；在加入阿尔比十字军后，这一禁令才得以解除。他能适度提升魅力与通用语能力，大幅提高木艺与骑术，并且还使队伍能够像在陆地上一样沿河航行。",
    "水手的守护圣人，也是十四位圣助之一，他亦被称为圣埃尔莫，并曾被天使救下，免于被活活烧死。他适度提升敏捷与感知，并使水手与海员变得友善。此外，他还增强队伍对雷电与火焰的防御。",
    "瑞典的守护圣人，也是瑞典国王，直到被叛乱的贵族处以斩首。他能小幅提升魅力，适度提升力量、骑术和通用语言能力，并大幅提高武器技能。此外，他还能够让贵族（尤其是邪恶贵族）暴露行踪，并提升他在瑞典当地的声望。",
    "猎人与逃脱的守护圣人，也是十四位圣助之一，尤斯塔修斯在目睹一头鹿的双角间显现出基督之印后皈依了信仰。他能为骑术带来不错的加成，为除冲击与链锤之外的所有武器技能提供大幅加成，并为木艺带来惊人的提升。他还可在林间疾行。",
    "一位主教的助手，在一次迫害中主教逃亡后，他代为主教被捕。费利克斯被天使从牢狱中解救出来。他能为敏捷带来适度的提升，为木系带来不错的提升，同时还能大幅提升潜行能力。若被囚禁，角色一生中仅能奇迹般地逃脱一次。",
    "传教士、隐士与修道院的创始人，在芬巴尔去世后，整整两周日不落。他能小幅提升魅力与感知，并大幅提高拉丁语能力；每当他被召唤时，队伍便能获得一整天的光明。",
    "以与圣科伦巴的宿怨以及种种离奇神迹（包括移山填海）而闻名。他能小幅提升街头智慧，并适度提升魅力、敏捷、通用语和读写能力。在召唤他的期间，队伍可在水上行走，仿佛行于陆地一般，持续一整天。",
    "波兰、奥地利以及那些身处水患之中的民众的守护圣人弗洛里安，被绑上巨石投入河中而殉道。他能适度提升所有武器技能与骑术，并可能提高他在波兰和奥地利的当地声望。此外，他还使队伍能够在水面上行走，如同在陆地上一般。",
    "一位曾目睹异象的富有的贵族，舍弃了财富，成为当时最具影响力的基督教导师。他能适度提升耐力与感知，显著提升魅力与通用语言能力，并极大提升美德。当召唤他时，队伍可能会损失10%的现金。",
    "上帝的天使，曾将基督降生的消息带给玛利亚。他能大幅提升智力和感知，并能为队伍传递神谕，帮助他们辨明真实动机、获取额外情报。在战斗中，他会吹响号角，令敌人惊恐而逃。",
    "作为灾难、干旱、降雨和发热的守护圣人，她以多次拯救巴黎于水火之中而闻名。她能恢复少量的力量与适度的耐力，并在队伍发起攻击之前阻止敌人主动开战；若在战斗中被召唤，敌人甚至可能仓皇逃窜。",
    "骑士与十字军的守护圣人。因手持长矛屠龙并救下一位少女而闻名。他能提升武器与盔甲的品质，为敏捷带来小幅加成，为力量带来显著加成，并大幅强化所有武器技能与骑术。",
    "一位曾为士兵、照料病患，并为年轻时的罪过忏悔了七年的圣人。他能小幅提升魅力，大幅提高所有武器技能，并显著增强治疗效果。此外，他还能够有效恢复力量与耐力。",
    "旅行者与园丁的守护圣人，她以好客与异象闻名。她能小幅提升骑术，大幅提升木艺与拉丁语，并极大提升读写能力。她还能让人瞬间前往最近的城镇或城市，并带来关于未来的预言之境。",
    "残疾者与乞丐的守护圣人，也是动物的朋友。他能小幅提升街头智慧，大幅提高潜行能力，并极大增强林间智慧；必要时还能提供金钱援助，且能治愈少量力量与大量耐力。不过，他会使敏捷与武器技能有所下降。",
    "一位曾是黑魔法师的人，他弃绝了黑魔法，并遭受了恶魔般的袭击，直到被圣母的异象所拯救。他能小幅提升智力，大幅提高工艺，显著增强感知，并极大提升炼金术。召唤贾尔斯会带来一道伤痕，以及一项美德的损失。",
    "一位严于律己的纪律大师，曾周游各地，对多家修道院进行改革。他凭借自身的纪律与魄力，在教会中步步高升。他能大幅提升读写能力和拉丁语水平，恢复相当可观的耐力，并且能够预防雪崩、暴风雪等旅行灾难。",
    "他在修道院中长大，其严苛的纪律与对神职人员独身的坚持备受争议。他能为“巧匠”带来不错的加成，大幅提高“通用语言”技能，治愈少量“耐力”，并可能感知到僧侣与神职人员中的邪恶气息。若在战斗前被召唤，他还能阻止敌人发动攻击。",
    "温德人的王子，活跃于战争与传教之中，他大幅提升所有武器技能，并提高在温德地区德国的声望（包括不来梅、汉堡、吕讷堡、吕贝克、维斯马尔、罗斯托克和石勒苏益格）。",
    "绝望境地、地震与洪水的守护圣人。他能小幅提升魅力，大幅提高炼金术与通用语言能力，并显著增强工艺技能。他的神恩或许能在非常规情境中助你一臂之力，还能暂时提升你的运气。",
    "西里西亚的守护圣人，她嫁给了西里西亚公爵，在战争、政治以及救济贫民方面都十分活跃。她在西里西亚为治疗和当地声望提供大幅加成。她还能治愈少量的力量与耐力。在面对贵族时，他们往往会透露一些信息。",
    "一位曾为神圣罗马帝国皇帝的圣人，以卓越的能力与虔诚著称。他能小幅提升智力，并大幅增强所有武器技能；在与任何高等贵族打交道时，还能提升当地声望。",
    "一位致力于帮助穷人的积极和平使者，他因通过祈祷降雨而结束了严重的旱灾而闻名。他能小幅提升力量，并在与农民打交道时大幅或显著提升魅力。此外，他还能够感知农民和农夫体内的邪恶。",
    "他生来失明，却仍创建了一座修道院，并一直坚守至终，因其神迹与圣洁而备受尊崇。他能大幅提升感知，并能察觉人心中的邪恶。",
    "一位创立修道院、以其异象闻名的杰出修女，因此被冠以“莱茵河的西比尔”之名。圣希尔德加德能带来有益的异象，尤其是在靠近莱茵河时。她还能大幅提升感知，并在某些情况下提升你的本地声望。",
    "作为猎人的守护圣人，他在一只雄鹿的角间看到了一个十字架的异象。他能大幅提高长矛、投掷武器和弓箭技能，显著提升机械飞弹武器的性能，极大地增强潜行能力，并大幅提升木艺水平。他还能够在林地遭遇中为队伍提供帮助。",
    "一位曾一度被委以守护圣杯重任的战士，后来成为僧侣。有人说，加拉哈德爵士便是以他为原型。他能小幅提升骑术，大幅提高武器技能与通用语言能力，并显著增强魅力。",
    "作为农民与马德里的守护圣人，他曾是某位西班牙贵族的雇工，在那里过着虔诚而清贫的生活。他能为“说通用语”带来小幅提升，为“魅力”带来适度提升，为“街头智慧”带来良好提升。他还可能提升马德里与西班牙当地的声望，并增强“魅力”。",
    "她也被称为德伊尔德雷和米达，曾行过许多治愈的神迹，包括让一颗被砍下的头颅与身体重新结合。她还拥有适度的魅力值和极佳的治疗能力；她能够恢复角色全部流失的力量，并恢复大量流失的耐力。",
    "西班牙的守护圣人，也是首位被希律·亚基帕处死的使徒。能小幅提升力量、耐力和魅力，并大幅提高美德。同时会使智力暂时下降一点，还能提升西班牙当地的声望。",
    "一位被戴克里先投入野兽之口的主教；那些野兽竟拒绝攻击。雅努阿里乌斯的血液盛于一只小瓶，据说每逢他的瞻礼日便会液化。祈求雅努阿里乌斯，便可使野兽停止攻击；若与他的圣物一同使用，任何敌人都无法抵挡雅努阿里乌斯。",
    "作为难产妇女的守护圣人，他也被称为约翰·思温，以其虔诚与诚实而闻名。他能小幅提升力量和耐力，仅对女性有效，并且能够同时治疗这两种属性，对女性的效果尤胜于男性。",
    "传道者的守护圣人，他以布道闻名，因此得名“金口”，意为“金口”。他能小幅提升魅力，并大幅提高说通用语和拉丁语的能力，但会略微降低感知。由于厌恶奢华，他在节庆日时可能会无视祈祷。",
    "也被称为约翰·斯科拉斯提库斯，以著有《天堂之梯》而闻名，该书阐述了达到宗教完美所需的三十个步骤。每拥有五点美德，便能小幅提升所有技能，并额外增加一项技能的等级。",
    "文策斯劳斯四世国王的宿敌，最终被他折磨致死。小幅提升街头智慧，适度提升魅力与智力，并大幅提高通用语能力。还能提升布拉格与波希米亚的当地声望。",
    "玛利亚的丈夫、耶稣的“父亲”约瑟，同时也是木匠、木工、旅行者和寻房者的守护圣人。他在街头智慧方面有不错的加成，在工艺方面有极佳的加成，而在城内则能为魅力带来适度的加成。",
    "绝望境地的守护圣人，也是十二使徒之一。他能小幅提升各项属性，并适度提升各项技能。在绝望或极度危急的境况下，他还可能提供特殊援助。",
    "作为旅行者、旅店老板和船夫的守护圣人，他因罪行受到惩罚，却在将自己的床铺让给一名麻风病人时获得了宽恕。他能适度提升骑术，并大幅提高林地知识；此外，他还可使人们在桥梁、浅滩和渡口上自由通行。",
    "他是爱尔兰国王之子，以善于感化信徒和拥有神奇法力而闻名。他能小幅提升骑术，适度提升魅力，显著提升治疗能力，并大幅提高说通用语的技能。此外，他还能够恢复少量力量和大量耐力。",
    "一位匈牙利国王，以其对该国的热忱捍卫、虔诚与勇武而闻名。他能小幅提升力量与敏捷，适度提升通用语能力，并大幅提高骑术及所有武器技能。",
    "他在罗马殉道，以如此超然的镇定承受酷刑，以至于被认为终结了该城的异教信仰。他能大幅提升魅力、街头智慧和通用语言技能；还能为角色恢复所有已耗尽的耐力，并帮助队伍抵御或逃脱酷刑。",
    "一位耶稣的朋友，在墓中躺了四天后复活。召唤拉撒路可以恢复相当多的已损失耐力，以及角色全部已损失的力量，除非该角色已经死亡。",
    "在十字架刑上认出基督是上帝之子的罗马百夫长。他后来皈依了基督教，并在殉道后，下令处死他的总督也随之皈依。他能适度提升所有武器技能，并可提高一名角色武器的品质。",
    "“圣露西亚”，因双眼被挖出后又奇迹般复原而闻名；她的名字意为“光明”。她能小幅提升感知，可将一名角色的力量与耐力恢复至满值，并提供一天的光明。",
    "医生与画家的守护圣人，第三部福音书与使徒行传的作者。他本人也是一名医生，能极大提升治疗效果，并且还能治愈相当多的力量值和大量耐力值。",
    "在1202年，她以成为本笃会修女的愿景为指引，经历了许多神奇的狂喜，最终皈依了西多会。她的感知与美德各提升了一点，魅力则提升了相当多。她有时还能让他人悬浮于障碍之上。在祈祷中分享自己的狂喜会消耗耐力。",
    "分娩与死亡的守护圣人，她曾被一条龙（实际上是魔鬼）吞下，却毫发无损地重生。她能恢复耐力和力量（对女性的效果比男性更强）。此外，她还能小幅提升女性的各项属性与技能，并增强某人的护甲的火焰抗性。",
    "一位曾是贵族的女性，当她的丈夫遇害时，她舍弃了尘世的一切，一心投身于救助病患与贫苦之人。她能为通用语带来不错的加成，为治疗带来极高的加成。此外，她还能恢复大量的力量与全部的耐力。",
    "他是公证人、作家、埃及和威尼斯的守护圣人，也是十二使徒之一。他曾一度面临被俘的威胁，却从斗篷里钻出来，赤身裸体地沿街逃窜。他能大幅提升敏捷，并大幅提高读写能力。",
    "士兵、骑兵和鹅的守护圣人。他能感知某些人心中的邪恶，只要角色身着部分盔甲，就能帮助治愈伤势，并使魅力提升一点，感知提升不少，而说通用语和骑术则大幅提升。",
    "银行家与簿记员的守护圣人，十二使徒之一，也是第一福音书的作者。他能为智力带来适度提升，为拉丁语带来大幅提升，为读写能力带来极大提升。此外，他还能够提升与富格尔家族和美第奇家族的当地声望。",
    "剑匠、士兵、军队、皮埃蒙特、萨伏依和撒丁岛的守护圣人莫里斯是一位基督教军团士兵，他拒绝向诸神献祭，并与他的军团一同殉道。他能为炼金术带来不错的加成，也为刃器带来极为显著的加成。他还可能提升所持任何刃器的品质。",
    "战斗、布鲁塞尔和德国的守护圣人，也是天界军团的三位统帅之一。他能小幅提升所有武器技能，显著提升魅力，并大幅增强治疗效果；在战斗中，他还能强化队伍的武器与护甲。此外，他还能够提升德国和布鲁塞尔的当地声望。",
    "一位著名的修女兼修道院的创始人，因其异象与悬浮之力而备受尊崇。她能大幅提升魅力与治疗能力。圣米尔伯加有时甚至愿意将所有人托举至障碍物之上或绕过障碍物。",
    "一名曾为奴隶、因性情凶残而获释的亡命之徒。他不知何故皈依了信仰，以严酷的苦行著称。他使木艺与街头智慧大幅提升，各类武器技能大幅增强，潜行能力更是显著提高。",
    "风暴肆虐的水手、俄罗斯、洛林、希腊、西西里和阿普利亚的守护圣人；他的慷慨催生了圣诞老人的传说。他为敏捷带来适度提升，为魅力带来良好提升，为骑术带来巨大提升。他在其庇护的地区影响当地声望，并使船只与小艇免受风暴侵袭。",
    "作为盲人与阿尔萨斯的守护圣人，她是一位阿尔萨斯领主的失明之女。她在十二岁时首次受洗，视力得以恢复。她能为感知属性带来适度加成，并提升她在阿尔萨斯地区的声望。此外，她还能带来光明。",
    "他在世俗与教会事务中均活跃，曾随国王埃塞尔斯坦参加布鲁南伯赫之战。他使魅力小幅提升，所有武器技能与通用语言能力小幅提升，而感知则大幅提升。",
    "挪威的守护圣人奥拉夫年轻时曾是一名海盗，后来登基成为挪威国王。他为统一挪威并使其基督教化所作出的努力，使他成为该国伟大的英雄之一。能大幅提升所有武器技能，并恢复大量耐力。",
    "作为医疗行业的守护圣人，也是十四位圣助之一，他曾是一名医生，在临终前历经酷刑仍幸存。他能大幅提升炼金术与治疗能力；使身披重甲的队员恢复流失的力量与耐力；在战斗中，动物与火焰对队伍的影响也会减弱。",
    "爱尔兰的守护圣人，他以神迹般的方式使大多数爱尔兰酋长和德鲁伊皈依。他大幅提升力量、魅力、近战武器、长柄武器和冲击武器的属性，并精通通用语及读写能力，同时极大地提升拉丁语水平。在战斗中，他还能强化护甲以抵御远程武器的攻击。",
    "马耳他与希腊的守护圣人，他曾是一位行过诸多神迹的使徒。能小幅提升耐力，适度提升魅力，显著提升拉丁语能力，并大幅提高通用语口语与读写能力。他还可治愈一点力量值。能够提供逃脱技能，并提升他在希腊与马耳他的当地声望。",
    "保罗天真而幼稚，六十岁时成为了一名弟子，在那里他展现出读心与治愈的神力。他能恢复已丧失的力量与耐力，并使队伍得以洞悉他人的动机；被召唤时，会使角色的智力暂时降低一半。",
    "出身高贵的佩尔佩图阿因信仰而被捕；被判处于公共竞技中处死，当野兽拒绝攻击她时，她最终被剑所杀。能大幅提升魅力，并能使野生动物停止攻击、四散逃窜。",
    "渔夫的守护圣人，也是一位使徒；他以无视危险著称，曾被天使从监狱中救出。大幅提升力量、通用语、拉丁语和治疗能力，适度提升魅力，并大幅提高近战武器的威力。还能协助越狱；但会使角色的感知暂时降低一半。",
    "一位在破像运动时期遭受迫害的亚美尼亚僧侣；由于其神迹，他还被指控为巫师。他能适度提升智力与感知，显著提升骑术与炼金术，以及极大地提升潜行能力。然而，队伍会损失少量当地声望。",
    "一位虔诚的男子，年届八十岁时被下令活活烧死，但烈火竟未能伤及他；于是改用长矛刺杀。每当他被召唤时，无论你为谁祈祷，全队对火焰的抗性都会提升。",
    "盲人、旅人与平安旅途以及眼疾的守护圣人；拉斐尔是三大天使之一。他能适度提升敏捷与感知，同时大幅增强治疗效果。他能恢复所有流失的力量与耐力，并且能够摧毁并净化异教或撒旦崇拜的场所。",
    "律师的守护圣人，以皈依与布道闻名。他能大幅提升智力，并显著提升读写能力。还能让队伍在一天内如同在陆地上般行走在水面上，并提高与多明我会的当地声望。",
    "一位热衷于前往黑暗之地传教的杰出学者与诗人；他在试图皈依摩尔人时饱受屈辱与险境。能大幅提升智力，并极大提升炼金术与读写能力。若召唤者的力量与耐力极低，则会提升至“9”。",
    "石匠的守护圣人，他因作品遭嫉妒的石匠谋害。祈祷或许能让人看见隐秘之门，并使人在石壁上直攀而上。他适度提升炼金术，大幅提升工艺术，并治愈相当多的耐力，但会适度降低魅力。",
    "作为瘟疫与囚犯的守护圣人，他竭力救助瘟疫患者，并且自己也从疾病中痊愈。他曾因被指控为间谍而入狱，最终死于狱中。他的技艺可获得适度提升，治疗效果亦颇为可观。他能解除某一区域内的瘟疫，并治愈少量的力量与耐力。",
    "一名哥特人皈依了基督教，他在一次迫害中公开宣扬自己的信仰，却毫发无损。他经受了多次酷刑，最终在拒绝获释的提议后被投入水中淹死。大幅提升魅力，并恢复所有已损失的力量与耐力。",
    "弓箭手、运动员和士兵的守护圣人，他曾被箭射中处决，却幸存下来。能大幅提升骑术与敏捷，对弓类武器有极佳加成，对其他各类武器技能也有适度提升。他还能治疗少量力量与耐力，并解除范围内的瘟疫。",
    "克拉科夫与波兰的守护圣人，一位备受尊崇的谋士，被博莱斯瓦夫国王所杀害。他能适度提升魅力，并大幅提高通用语与拉丁语的使用能力。还能恢复少量力量与适量的耐力，并提升波兰及克拉科夫当地的声望。",
    "匈牙利的守护圣人，马扎尔人的统治者，后来成为匈牙利国王。他能适度提升智力、魅力和通用语能力，并大幅提高骑术及所有武器技能。他还能够提升他在匈牙利的当地声望。",
    "一位前往德国传教的传教士，凭借其雄辩的口才与炽热的热忱感化了众多信徒。他能大幅提升魅力，并大幅提高说通用语的能力。此外，他还能够略微恢复力量，并适度提升耐力。",
    "一位曾在迫害中历经重重酷刑的前罗马士兵。他被投入竞技场，却未遭野兽攻击。他能小幅提升魅力，并治愈少量力量与较多耐力。此外，他还能令野生动物在战斗中四散逃窜。",
    "因其对病患与贫民的善举而被誉为仁慈之人，他的刽子手也因怜悯他而与他一同遇害。极大地提升治疗效果，并能治愈少量的力量与适度的耐力。",
    "作为罗马军队的一名新兵，他拒绝参加异教仪式，最终被活活烧死在火炉中。他的魅力会小幅提升，所有武器技能都会大幅提升，而骑术则会得到极大提升。",
    "他是那位直到亲眼目睹基督复活才相信的使徒，因此我们有了“多疑的托马斯”这一说法。他能小幅提升普通语言、力量、耐力和木系技能，并适度提升治疗与工艺技能。",
    "大学与学者的守护圣人，其著作如今已成为基督教神学的基石。为智力与拉丁语提供显著加成，为读写能力提供大幅加成，为宗教信仰提供极高的加成。同时提升修道院与大学的当地声望。",
    "作为恋人的守护圣人，他曾是罗马的一位教士，以虔诚与治愈之术闻名。他能小幅提升潜行与通用语言能力，并大幅提高魅力与治疗效果。此外，他还可恢复少量力量与耐力。最后，施展其神恩时，或许能赢得他人的善意与友谊。",
    "被判处死刑的罗马士兵；他感化了自己的卫兵，而这些卫兵随后也被判与他同死。适度提升魅力与所有武器技能，并大幅提升通用语言能力。还能恢复全部流失的耐力以及少量流失的力量。",
    "舞蹈家、演员、喜剧演员和癫痫患者的守护圣人，也是十四位圣助之一。他能大幅提升魅力和敏捷，并且能够治愈癫痫发作、驱逐恶魔，还能将队伍传送至最近的城镇或城市。不过，施法者可能会损失部分耐力。",
    "波希米亚的守护圣人；他以基督教义与严苛的统治招致诸多敌对势力。最终被其兄弟所领导的叛乱派系杀害。为力量、感知、智力、所有武器技能以及骑术提供适度加成。同时提升他在贵族及波希米亚地区的声望。",
    "威尔弗里德周游四方，致力于在凯尔特地区确立罗马的统治秩序。他参与了教会内部的权力斗争，并主持兴建了许多修道院。他的魅力与机巧属性会得到适度提升，而通用语言能力则会大幅提升。此外，他还能够恢复少量耐力，并使队伍的行进速度提高一倍。",
    "作为西欧的一名积极传教士，他曾多次与那些一心要置他于死地的异教徒发生冲突，却都幸存下来。能小幅提升敏捷，并显著提升木艺和骑术。若在战斗前或战斗中被召唤，对手可能会因惊讶而在短时间内不敢攻击你。",
    "一位伟大的流浪者与朝圣者，他撰写了第一本英文旅行记。为木系技能提供适度加成，为魅力提供良好加成，为通用语言能力提供极佳加成。",
    "作为一名传教士，他在皈依弗里斯兰人方面极为成功，以至于被誉为“弗里斯人的使徒”。他能为木艺带来适度的提升，为魅力带来不错的提升，为通用语言带来极佳的提升。他还能够提升德国北部的当地声望。",
    "一位修道院学校的校长，以其在贫困人群及贵族（包括皇帝）中的工作而闻名。他能适度提升智力、魅力、木艺和骑术，并大幅提高通用语的使用能力。此外，他还能够提升村庄及贵族群体中的本地声望。",
    "仆人们的守护圣人，她以耐心、热忱与圣洁而闻名。她在同一个家庭中侍奉了46年。适度提升感知与街头智慧，并恢复一点力量和少量耐力。她还能从仆人们那里获取情报。",
};

/* formula names as displayed (gamedata.formula_names) */
static const char *const FORMULA_ZH[66] = {
    "拉齐的毒气",
    "克雷莫纳的杰拉德的毒气",
    "佩特鲁斯的毒气",
    "所罗门的灼眼",
    "盖伦的灼眼",
    "弗拉梅尔的灼眼",
    "东方黑云",
    "鲁菲努斯的黑云",
    "斯科特的黑云",
    "拉齐的跳蚤粉",
    "弗拉梅尔的跳蚤粉",
    "理查德的跳蚤粉",
    "所罗门的蚀水",
    "阿尔弗雷德的蚀水",
    "克雷莫纳的杰拉德的蚀水",
    "斯马尔格登的死亡之息",
    "肯迪的死亡之息",
    "西蒙的死亡之息",
    "非洲阳爆",
    "卢尔的阳爆",
    "埃利亚斯兄弟的阳爆",
    "塔米米的雷霆",
    "迈蒙尼德的雷霆",
    "维拉诺瓦的阿尔纳尔德的雷霆",
    "贾比尔的阿拉伯火药",
    "扎迪思的阿拉伯火药",
    "休的阿拉伯火药",
    "西那的石焦油",
    "阿尔伯特的石焦油",
    "罗伯特的石焦油",
    "哈扬的致命利刃",
    "文森特的致命利刃",
    "罗杰的致命利刃",
    "马吉里蒂的利锋",
    "约翰的利锋",
    "佩特鲁斯的利锋",
    "伊本·鲁世德的巨力",
    "吉勒达基的巨力",
    "阿尔伯特的巨力",
    "伊本·乌迈尔的真射",
    "比特鲁吉的真射",
    "道斯滕的约翰的真射",
    "伊本·叶齐德的坚甲",
    "埃利亚斯兄弟的坚甲",
    "伦纳德的坚甲",
    "莫里埃努斯的变形",
    "阿维森纳的变形",
    "尼古拉的变形",
    "阿尔法·拉斯的真视",
    "鲁菲努斯的真视",
    "伦纳德的真视",
    "西那的新风",
    "斯科特的新风",
    "鲁佩斯克的约翰的新风",
    "贾比尔的铁臂",
    "凯特内的罗伯特的铁臂",
    "休的铁臂",
    "哈扬的疾行",
    "道斯滕的约翰的疾行",
    "理查德的疾行",
    "盖伦的优雅精华",
    "斯科特的优雅精华",
    "阿维森纳的优雅精华",
    "贾比尔的火墙",
    "迈蒙尼德的火墙",
    "卢尔的火墙",
};

/* formula effects (gamedata.formulas[i].text) */
static const char *const FORMULA_DESC_ZH[66] = {
    "一大团臭味令人作呕的云雾；敏捷与战斗技能下降",
    "一大团臭味令人作呕的云雾；敏捷与战斗技能下降",
    "一大团臭味令人作呕的云雾；敏捷与战斗技能下降",
    "一瓶致盲粉末；被击中者的技能大幅下降。",
    "一瓶致盲粉末；被击中者的技能大幅下降。",
    "一瓶致盲粉末；被击中者的技能大幅下降。",
    "一团浓密的黑烟；遮蔽视线，提升隐匿并降低感知。",
    "一团浓密的黑烟；遮蔽视线，提升隐匿并降低感知。",
    "一团浓密的黑烟；遮蔽视线，提升隐匿并降低感知。",
    "一团痒粉；按目标护甲比例降低其技能。",
    "一团痒粉；按目标护甲比例降低其技能。",
    "一团痒粉；按目标护甲比例降低其技能。",
    "油腻的酸性液体，能腐蚀金属——甚至护甲。制作危险",
    "油腻的酸性液体，能腐蚀金属——甚至护甲。制作危险",
    "油腻的酸性液体，能腐蚀金属——甚至护甲。制作危险",
    "一团有毒的致命雾气，护甲无法抵挡。",
    "一团有毒的致命雾气，护甲无法抵挡。",
    "一团有毒的致命雾气，护甲无法抵挡。",
    "一阵致盲的强光爆闪；震晕附近的任何人。",
    "一阵致盲的强光爆闪；震晕附近的任何人。",
    "一阵致盲的强光爆闪；震晕附近的任何人。",
    "一只密封的小瓶，撞击时会猛烈爆炸。",
    "一只密封的小瓶，撞击时会猛烈爆炸。",
    "一只密封的小瓶，撞击时会猛烈爆炸。",
    "一种黏稠的燃烧物质，会附着在血肉上并渗入护甲。",
    "一种黏稠的燃烧物质，会附着在血肉上并渗入护甲。",
    "一种黏稠的燃烧物质，会附着在血肉上并渗入护甲。",
    "一滩黏稠的污泥；会减慢任何跨过它的人。",
    "一滩黏稠的污泥；会减慢任何跨过它的人。",
    "一滩黏稠的污泥；会减慢任何跨过它的人。",
    "涂在非打击类武器上，以增加其伤害。",
    "涂在非打击类武器上，以增加其伤害。",
    "涂在非打击类武器上，以增加其伤害。",
    "涂在刀剑与长杆武器上，增强战斗效能。",
    "涂在刀剑与长杆武器上，增强战斗效能。",
    "涂在刀剑与长杆武器上，增强战斗效能。",
    "涂在武器上，提升其品质。",
    "涂在武器上，提升其品质。",
    "涂在武器上，提升其品质。",
    "涂在投掷与射击武器上，提高命中精度。",
    "涂在投掷与射击武器上，提高命中精度。",
    "涂在投掷与射击武器上，提高命中精度。",
    "涂在护甲上，增加其厚度与品质。",
    "涂在护甲上，增加其厚度与品质。",
    "涂在护甲上，增加其厚度与品质。",
    "一种净化溶液，有时似乎能变出更多黄金。",
    "一种净化溶液，有时似乎能变出更多黄金。",
    "一种净化溶液，有时似乎能变出更多黄金。",
    "一种内服药剂；提升使用者的感知。",
    "一种内服药剂；提升使用者的感知。",
    "一种内服药剂；提升使用者的感知。",
    "一种治疗药剂，恢复失去的耐力。",
    "一种治疗药剂，恢复失去的耐力。",
    "一种治疗药剂，恢复失去的耐力。",
    "一种药剂，可把力量提升到正常上限之上。",
    "一种药剂，可把力量提升到正常上限之上。",
    "一种药剂，可把力量提升到正常上限之上。",
    "一种饮剂，可把敏捷提升到正常上限之上。",
    "一种饮剂，可把敏捷提升到正常上限之上。",
    "一种饮剂，可把敏捷提升到正常上限之上。",
    "一种治疗药剂，恢复耐力与力量。",
    "一种治疗药剂，恢复耐力与力量。",
    "一种治疗药剂，恢复耐力与力量。",
    "一种饮剂，大幅提升使用者对火系魔法的抗性。",
    "一种饮剂，大幅提升使用者对火系魔法的抗性。",
    "一种饮剂，大幅提升使用者对火系魔法的抗性。",
};

void gamedata_load(const char *dir)
{
    std::vector<unsigned char> lst = read_all(open_ci(dir, "DARKLAND.LST"));
    if (lst.size() > 3) {
        int n = lst[0];
        for (int i = 0; i < n && 3 + (i + 1) * 46 <= (int)lst.size(); i++) {
            const unsigned char *r = &lst[3 + i * 46];
            ItemDef d;
            d.game_name = d.name = d.plain = std::string((const char *)r, strnlen((const char *)r, 20));
            d.shortname = std::string((const char *)r + 20, strnlen((const char *)r + 20, 10));
            d.type = w16(r + 30); d.flags = w16(r + 32); d.flags2 = w16(r + 34);
            d.weight = r[37]; d.quality = r[38]; d.value = w16(r + 44);
            d.flags5 = r[36]; d.rarity = r[39];
            d.mineral = (d.flags2 & 0x1000) != 0;
            d.kind = classify(d);
            gamedata.items.push_back(d);
        }
        /* then saint full names, short names, formula full names, short names */
        size_t pos = 3 + (size_t)n * 46;
        int ns = lst[1], nf = lst[2];
        auto cstr = [&](void) { std::string r; while (pos < lst.size() && lst[pos]) r += (char)lst[pos++]; pos++; return r; };
        for (int i = 0; i < ns && pos < lst.size(); i++) { Saint t; t.game_name = t.name = game_text(cstr().c_str(), 64); gamedata.saints.push_back(t); }
        for (int i = 0; i < ns && pos < lst.size(); i++) gamedata.saints[i].shortname = game_text(cstr().c_str(), 64);
        formula_raw.clear();
        for (int i = 0; i < nf && pos < lst.size(); i++) { std::string r = cstr(); formula_raw.push_back(r); gamedata.formula_game_names.push_back(game_text(r.c_str(), 64)); }
        gamedata.formula_names = gamedata.formula_game_names;
    }
    std::vector<unsigned char> snt = read_all(open_ci(dir, "DARKLAND.SNT"));
    if (snt.size() > 1)
        for (int i = 0; i < snt[0] && i < (int)gamedata.saints.size() && 1 + (size_t)(i + 1) * 360 <= snt.size(); i++)
            gamedata.saints[i].text = game_text((const char *)&snt[1 + (size_t)i * 360], 360);
    for (int i = 0; i < (int)gamedata.saints.size() && i < 136; i++)
        if (SAINT_TEXT_ZH[i][0]) gamedata.saints[i].text = SAINT_TEXT_ZH[i];
    std::vector<unsigned char> alc = read_all(open_ci(dir, "DARKLAND.ALC"));
    if (alc.size() > 1) {
        int n = alc[0];
        for (int i = 0; i < n && 1 + (i + 1) * 0x68 <= (int)alc.size(); i++) {
            const unsigned char *r = &alc[1 + i * 0x68];
            Formula f;
            f.text = std::string((const char *)r, strnlen((const char *)r, 0x50));
            f.mystic = w16(r + 0x50);
            f.risk = w16(r + 0x52);
            for (int k = 0; k < 5; k++) {
                int c = r[0x54 + 4 * k], it = w16(r + 0x56 + 4 * k);
                if (c > 0) f.ingr.push_back({ c, it });
            }
            gamedata.formulas.push_back(f);
        }
        for (int i = 0; i < (int)gamedata.formulas.size() && i < 66; i++)
            if (FORMULA_DESC_ZH[i][0]) gamedata.formulas[i].text = FORMULA_DESC_ZH[i];
    }
    std::vector<unsigned char> cty = read_all(open_ci(dir, "DARKLAND.CTY"));
    if (cty.size() > 1) {
        int n = cty[0];
        for (int i = 0; i < n && 1 + (i + 1) * 0x26e <= (int)cty.size(); i++) {
            const unsigned char *r = &cty[1 + i * 0x26e];
            City c;
            c.shortname = game_text((const char *)r, 0x20);
            c.name = game_text((const char *)r + 0x20, 0x20);
            c.size = w16(r + 0x40); c.type = w16(r + 0x58); c.buildings = w16(r + 0x5e);
            for (int k = 0; k < 9; k++) c.shopq[k] = r[0x62 + k];
            c.x = w16(r + 0x42); c.y = w16(r + 0x44);
            c.seed = w16(r + 0x56);
            for (int k = 0; k < 5; k++) c.docks[k] = (short)w16(r + 0x4a + 2 * k);
            gamedata.cities.push_back(c);
        }
    }
    gamedata.loaded = !gamedata.items.empty();
    gamedata_names();
}

/* ---------------------------------------------------------------- the names in full
   Only what is certain is written out: the alchemists whose names the game gives in full in
   another formula (Petrus Bonus, Nicolas Flammel, Rob de Ketene, Michael Scot, John of
   Rupescissa) or who are the famous ones of that name (Albertus Magnus, Raimundus Lullus, Arnald
   of Villanova, Gerard of Cremona); "Rufinus of G", "Alfred of S", "Simon C", "Vincent Bs",
   "Roger of H", "Leonard of M" and "Nicolaus of A" stay as the game has them. */
static const char *const ITEM_FULL[][2] = {
    {"Thorn of the Crown", "王冠之刺"},
    {"S.Arnulf GreatSwrd", "圣阿努尔夫巨剑"},
    {"S.George GreatSwrd", "圣乔治巨剑"},
    {"Staff of S.Patrick", "圣帕特里克之杖"},
    {"S.Kessog Medallion", "圣凯索格徽章"},
    {"S.Swithbert's Foot", "圣斯威特伯特之足"},
    {"S.Thealeaus' Spoon", "圣西劳斯之勺"},
    {"Spar of S.Erasmus", "圣伊拉斯谟之矛"},
    {"Spear of Longinus", "朗基努斯之枪"},
    {"S.Emygdius Finger", "圣埃米迪乌斯之指"},
    {"S.Odo's Testament", "圣奥多遗言"},
    {"S.Raphael's Water", "圣拉斐尔之水"},
    {"S.Willehad's Shoe", "圣威利哈德之鞋"},
    {"East. Black Bean", "东方黑豆"},
    {"Letter of Credit", "信用证"},
    {"Residency Permit", "居留许可"},
    {"Lantern of Light", "光明之灯"},
    {"S.Catherine Pain", "圣凯瑟琳之痛"},
    {"S.Olaf Battleaxe", "圣奥拉夫战斧"},
    {"S.Dunstan Hammer", "圣邓斯坦之锤"},
    {"S.Gabriel's Horn", "圣加百列之角"},
    {"Military Hammer", "军用锤"},
    {"Phleqmatic Base", "黏液基质"},
    {"Breath of Death", "死亡之息"},
    {"Essence o'Grace", "优雅精华"},
    {"Sworn Statement", "宣誓书"},
    {"Globe with Fish", "鱼形地球仪"},
    {"S.Edward's Ring", "圣爱德华之戒"},
    {"Throwing Knife", "飞刀"},
    {"Two-hand Sword", "双手剑"},
    {"Two-hand Flail", "双手链锤"},
    {"Military Flail", "军用链锤"},
    {"Grappling Hook", "抓钩"},
    {"White Cinnabar", "白朱砂"},
    {"Transformation", "变形"},
    {"Birth Confess.", "初生宣誓"},
    {"Evil Medallion", "邪徽"},
    {"Hermit's Habit", "隐士长袍"},
    {"History Scroll", "历史卷轴"},
    {"Tusk of a Boar", "野猪獠牙"},
    {"S.Mary's Tears", "圣玛丽之泪"},
    {"S.Hubert's Bow", "圣休伯特之弓"},
    {"S.Ita's Needle", "圣伊塔之针"},
    {"S.Odilia's Oil", "圣奥迪利亚之油"},
    {"Composite Bow", "复合弓"},
    {"Brass Handgun", "铜手铳"},
    {"Medium Shield", "中盾"},
    {"Average Horse", "普通马"},
    {"Choleric Base", "胆汁基质"},
    {"Sanguine Base", "多血基质"},
    {"Noxious Aroma", "毒气"},
    {"Secret Ledger", "秘密账簿"},
    {"Illum. Gospel", "彩绘福音书"},
    {"Ancient Crown", "古代王冠"},
    {"Magic Balance", "魔法天平"},
    {"Great Hammer", "巨锤"},
    {"Giant Cudgel", "巨棍"},
    {"Quarterstaff", "长棍"},
    {"Throwing Axe", "飞斧"},
    {"Iron Handgun", "铁手铳"},
    {"Studded Lthr", "铆钉皮甲"},
    {"Small Shield", "小盾"},
    {"Large Shield", "大盾"},
    {"Superb Horse", "良驹"},
    {"Melanc. Base", "忧郁基质"},
    {"Arabian Fire", "阿拉伯火药"},
    {"Deadly Blade", "致命利刃"},
    {"Love Letters", "情书"},
    {"Birth Record", "出生记录"},
    {"Treason Plan", "叛国计划"},
    {"Counterfeits", "伪币模"},
    {"Seed of Life", "生命之种"},
    {"Sword of War", "战争之剑"},
    {"Short Sword", "短剑"},
    {"Short Spear", "短矛"},
    {"Plate Armor", "板甲"},
    {"Cuirbouilli", "硬化皮甲"},
    {"Marsh Vapor", "沼气"},
    {"Pitchblende", "沥青铀矿"},
    {"Black Cloud", "黑云"},
    {"Eater Water", "蚀水"},
    {"Thunderbolt", "雷霆"},
    {"Great Power", "巨力"},
    {"Silver Mace", "银钉头锤"},
    {"Magic Honey", "魔法蜂蜜"},
    {"Long Sword", "长剑"},
    {"Battle Axe", "战斧"},
    {"Long Spear", "长矛"},
    {"Cuirboulli", "硬化皮甲"},
    {"Brigandine", "板链甲"},
    {"Iron Spike", "铁钉"},
    {"Fast Horse", "快马"},
    {"Pack Horse", "驮马"},
    {"Zincblende", "闪锌矿"},
    {"Solanaceae", "茄科植物"},
    {"Aqua Regia", "王水"},
    {"Mondragora", "曼德拉草"},
    {"Strongedge", "利锋"},
    {"Trueflight", "真射"},
    {"Cheap Copy", "廉价抄本"},
    {"Confession", "忏悔录"},
    {"Prayerbook", "祈祷书"},
    {"Hyoscyamus", "天仙子"},
    {"Quest Item", "任务物品"},
    {"Field Axe", "野战斧"},
    {"Short Bow", "短弓"},
    {"Chainmail", "锁子甲"},
    {"Pure Gold", "纯金"},
    {"Brimstone", "硫磺"},
    {"Stone-tar", "石焦油"},
    {"Hardarmor", "坚甲"},
    {"Truesight", "真视"},
    {"Quickmove", "疾行"},
    {"Debt Note", "借据"},
    {"Hand Axe", "手斧"},
    {"Falchion", "弯刀"},
    {"Long Bow", "长弓"},
    {"Arbalest", "重弩"},
    {"Crossbow", "弩"},
    {"Clothing", "布衣"},
    {"Lockpick", "撬锁工具"},
    {"Manganes", "锰"},
    {"Antimoni", "锑"},
    {"Orpiment", "雌黄"},
    {"Camomile", "洋甘菊"},
    {"Fleadust", "跳蚤粉"},
    {"Sunburst", "阳爆"},
    {"New-wind", "新风"},
    {"Firewall", "火墙"},
    {"Tarnhelm", "隐形盔"},
    {"Wolfskin", "狼皮"},
    {"Gold Cup", "金杯"},
    {"Poniard", "尖匕首"},
    {"Halberd", "戟"},
    {"Javelin", "标枪"},
    {"Quarrel", "弩矢"},
    {"Leather", "皮甲"},
    {"Lantern", "灯笼"},
    {"Eyeburn", "灼眼"},
    {"Ironarm", "铁臂"},
    {"Dagger", "匕首"},
    {"Padded", "软甲"},
    {"Candle", "蜡烛"},
    {"Naptha", "石脑油"},
    {"Zinken", "锌"},
    {"Arrow", "箭"},
    {"Scale", "鳞甲"},
    {"Torch", "火把"},
    {"Clock", "钟表"},
    {"Flute", "长笛"},
    {"Nikel", "镍"},
    {"Maul", "大锤"},
    {"Mace", "钉头锤"},
    {"Club", "棍棒"},
    {"Pike", "长枪"},
    {"Dart", "飞镖"},
    {"Ball", "弹丸"},
    {"Rope", "绳索"},
    {"Harp", "竖琴"},
    {"Mule", "骡子"},
    {"Alum", "明矾"},
    {"Bone", "骸骨"},
    {"Gum", "树胶"},
    {"Key", "钥匙"},
};
static const char *const SAINT_FULL[][2] = {
    {"St.Christina, Astonish.", "圣克里斯蒂娜（惊异者）"},
    {"St.Gertrude o'Nivelles", "圣尼维勒的格特鲁德"},
    {"St.Gregory Thaumaturg.", "圣格列高利（神术师）"},
    {"St.John of Bridlington", "圣布里德灵顿的约翰"},
    {"St.Margaret of Cortona", "圣科托纳的玛格丽特"},
    {"St.Thalelaeus,Merciful", "圣塔莱劳斯（仁慈者）"},
    {"St.Victor o'Marseilles", "圣马赛的维克多"},
    {"St.Andrew the Tribune", "圣安德烈（百夫长）"},
    {"St.Catherine of Siena", "圣锡耶纳的凯瑟琳"},
    {"St.Julian, Hospitaler", "圣朱利安（接待者）"},
    {"St.Thomas the Apostle", "圣多默（宗徒）"},
    {"St.Dorothy of Montau", "圣蒙陶的多萝西"},
    {"St.Edward, Confessor", "圣爱德华（忏悔者）"},
    {"St.Giles of Portugal", "圣葡萄牙的吉尔"},
    {"St.Albert the Great", "大圣阿尔伯特"},
    {"St.Anthony of Padua", "圣帕多瓦的安东尼"},
    {"St.Colman of Cloyne", "圣克洛因的科尔曼"},
    {"St.John Nepolmuchen", "圣约翰·涅波尔穆琴"},
    {"St.Raymond Penafort", "圣雷蒙德·佩尼亚福特"},
    {"St.John Chrysostom", "圣约翰（金口）"},
    {"St.Martin of Tours", "圣图尔的马丁"},
    {"St.Moses the Black", "圣黑人摩西"},
    {"St.Paul the Simple", "圣保罗（单纯者）"},
    {"St.Peter of Atroa", "圣阿特拉的彼得"},
    {"St.Sabas the Goth", "圣西巴斯的哥特人"},
    {"St.Thomas Aquinas", "圣托马斯·阿奎那"},
    {"St.Derfel Gadarn", "圣德费尔·加达恩"},
    {"St.Felix of Nola", "圣诺拉的费利克斯"},
    {"St.John Climacus", "圣约翰·克利马科斯"},
    {"St.Theodore Tiro", "圣西奥多（提罗）"},
    {"St.Apollinarius", "圣阿波利纳里斯"},
    {"St.Raymond Lull", "圣雷蒙德·卢尔"},
    {"St.Christopher", "圣克里斯托弗"},
    {"St.Williborord", "圣威利博罗德"},
    {"St.Gottschalk", "圣戈特沙尔克"},
    {"St.Lasdislaus", "圣拉斯迪斯劳斯"},
    {"St.Stanislaus", "圣斯坦尼斯劳斯"},
    {"St.Wenceslaus", "圣瓦茨拉夫"},
    {"St.Bathildis", "圣巴蒂尔德"},
    {"St.Catherine", "圣凯瑟琳"},
    {"St.Engelbert", "圣恩格尔贝特"},
    {"St.Genevieve", "圣热纳维耶芙"},
    {"St.Hildegard", "圣希尔德加德"},
    {"St.Januarius", "圣雅努阿里乌斯"},
    {"St.Longinius", "圣朗吉努斯"},
    {"St.Lutgardis", "圣卢特加迪斯"},
    {"St.Pantaleon", "圣潘塔莱翁"},
    {"St.Sebastian", "圣塞巴斯蒂安"},
    {"St.Swithbert", "圣斯威特伯特"},
    {"St.Valentine", "圣瓦伦丁"},
    {"St.Willebald", "圣威勒巴尔德"},
    {"St.Boniface", "圣博尼法斯"},
    {"St.Ceolwulf", "圣塞尔武尔夫"},
    {"St.Clotilda", "圣克洛蒂尔德"},
    {"St.Godehard", "圣戈德哈德"},
    {"St.Heribert", "圣赫里伯特"},
    {"St.Lawrence", "圣劳伦斯"},
    {"St.Margaret", "圣玛格丽特"},
    {"St.Milburga", "圣米尔布尔加"},
    {"St.Nicholas", "圣尼古拉"},
    {"St.Perpetua", "圣佩尔佩图阿"},
    {"St.Polycarp", "圣波利卡普"},
    {"St.Tarachus", "圣塔拉库斯"},
    {"St.Willehad", "圣威利哈德"},
    {"St.Wolfgang", "圣沃尔夫冈"},
    {"St.Anthony", "圣安东尼"},
    {"St.Barbara", "圣芭芭拉"},
    {"St.Cecilia", "圣塞西莉亚"},
    {"St.Charity", "圣慈善"},
    {"St.Columba", "圣科伦巴"},
    {"St.Crispin", "圣克里斯平"},
    {"St.Cyprian", "圣西普里安"},
    {"St.Dominic", "圣多米尼克"},
    {"St.Dunstan", "圣邓斯坦"},
    {"St.Dymphna", "圣丁姆芙娜"},
    {"St.Eligius", "圣埃利吉乌斯"},
    {"St.Emydius", "圣埃米迪乌斯"},
    {"St.Erasmus", "圣伊拉斯谟"},
    {"St.Eustace", "圣尤斯塔斯"},
    {"St.Finnian", "圣芬尼安"},
    {"St.Florian", "圣弗洛里安"},
    {"St.Francis", "圣方济各"},
    {"St.Gabriel", "圣加百列"},
    {"St.Godfrey", "圣戈弗雷"},
    {"St.Isidore", "圣伊西多尔"},
    {"St.Lazarus", "圣拉撒路"},
    {"St.Matthew", "圣马太"},
    {"St.Maurice", "圣莫里斯"},
    {"St.Michael", "圣弥额尔"},
    {"St.Patrick", "圣帕特里克"},
    {"St.Raphael", "圣拉斐尔"},
    {"St.Reinold", "圣莱诺尔德"},
    {"St.Stephen", "圣斯德望"},
    {"St.Wilfrid", "圣威尔弗里德"},
    {"St.Adrian", "圣阿德里安"},
    {"St.Agatha", "圣阿加莎"},
    {"St.Alcuin", "圣阿尔昆"},
    {"St.Alexis", "圣阿莱克西斯"},
    {"St.Andrew", "圣安德烈"},
    {"St.Arnulf", "圣阿努尔夫"},
    {"St.Cosmas", "圣科斯马斯"},
    {"St.Damian", "圣达米安"},
    {"St.Devota", "圣德沃塔"},
    {"St.Dismas", "圣迪斯马斯"},
    {"St.Finbar", "圣芬巴尔"},
    {"St.George", "圣乔治"},
    {"St.Gerlac", "圣格拉赫"},
    {"St.Hedwig", "圣海德薇"},
    {"St.Hubert", "圣休伯特"},
    {"St.Illtyd", "圣伊尔蒂德"},
    {"St.Joseph", "圣约瑟"},
    {"St.Kessog", "圣凯索格"},
    {"St.Odilia", "圣奥迪利亚"},
    {"St.Agnes", "圣阿格尼丝"},
    {"St.Aidan", "圣艾丹"},
    {"St.Clare", "圣克莱尔"},
    {"St.David", "圣大卫"},
    {"St.Denis", "圣德尼"},
    {"St.Drogo", "圣德罗戈"},
    {"St.Giles", "圣吉尔"},
    {"St.Henry", "圣亨利"},
    {"St.Herve", "圣埃尔韦"},
    {"St.James", "圣雅各"},
    {"St.Peter", "圣伯多禄"},
    {"St.Vitus", "圣维图斯"},
    {"St.Eric", "圣埃里克"},
    {"St.Jude", "圣犹达"},
    {"St.Lucy", "圣露西"},
    {"St.Luke", "圣路加"},
    {"St.Mark", "圣马可"},
    {"St.Olaf", "圣奥拉夫"},
    {"St.Paul", "圣保禄"},
    {"St.Roch", "圣罗克"},
    {"St.Zita", "圣齐塔"},
    {"St.Ita", "圣伊塔"},
    {"St.Odo", "圣奥多"},
};
static const char *const FORMULA_PARTS[][2] = {
    {"S.Andrew Tr", "圣安德烈（百夫长）"},
    {"S.Anthny Pa", "圣帕多瓦的安东尼"},
    {"S.Catherine", "圣凯瑟琳"},
    {"S.Cathrn Si", "圣锡耶纳的凯瑟琳"},
    {"S.Christina", "圣克里斯蒂娜（惊愕者）"},
    {"S.Christphr", "圣克里斯托弗"},
    {"S.Colman Cl", "圣克洛因的科尔曼"},
    {"S.Derfel Ga", "圣德费尔·加达恩"},
    {"S.Dorthy Mn", "圣蒙陶的多萝西"},
    {"S.Edward Cf", "圣爱德华（忏悔者）"},
    {"S.Engelbert", "圣恩格尔贝特"},
    {"S.FelixNola", "圣诺拉的费利克斯"},
    {"S.Genevieve", "圣热纳维耶芙"},
    {"S.Gertrd Ni", "圣尼维勒的格特鲁德"},
    {"S.Giles Por", "圣葡萄牙的吉尔"},
    {"S.Gottschlk", "圣戈特沙尔克"},
    {"S.Gregory T", "圣格列高利（行奇迹者）"},
    {"S.Apollnrs", "圣阿波利纳里斯"},
    {"S.Bathilds", "圣巴蒂尔德"},
    {"S.Boniface", "圣博尼法斯"},
    {"S.Ceolwulf", "圣塞尔武尔夫"},
    {"S.Clotilda", "圣克洛蒂尔德"},
    {"S.Godehard", "圣戈德哈德"},
    {"S.Anthony", "圣安东尼"},
    {"S.Barbara", "圣芭芭拉"},
    {"S.Cecilia", "圣塞西莉亚"},
    {"S.Charity", "圣慈善"},
    {"S.Columba", "圣科伦巴"},
    {"S.Crispin", "圣克里斯平"},
    {"S.Cyprian", "圣西普里安"},
    {"S.Dominic", "圣多米尼克"},
    {"S.Dunstan", "圣邓斯坦"},
    {"S.Dymphna", "圣丁姆芙娜"},
    {"S.Eligius", "圣埃利吉乌斯"},
    {"S.Emydius", "圣埃米迪乌斯"},
    {"S.Erasmus", "圣伊拉斯谟"},
    {"S.Eustace", "圣尤斯塔斯"},
    {"S.Finnian", "圣芬尼安"},
    {"S.Florian", "圣弗洛里安"},
    {"S.Francis", "圣方济各"},
    {"S.Gabriel", "圣加百列"},
    {"S.Godfrey", "圣戈弗雷"},
    {"S.Adrian", "圣阿德里安"},
    {"S.Agatha", "圣阿加莎"},
    {"S.Albert", "大圣阿尔伯特"},
    {"S.Alcuin", "圣阿尔昆"},
    {"S.Alexis", "圣阿莱克西斯"},
    {"S.Andrew", "圣安德烈"},
    {"S.Arnulf", "圣阿努尔夫"},
    {"S.Cosmas", "圣科斯马斯"},
    {"S.Damian", "圣达米安"},
    {"S.Devota", "圣德沃塔"},
    {"S.Dismas", "圣迪斯马斯"},
    {"S.Finbar", "圣芬巴尔"},
    {"S.George", "圣乔治"},
    {"S.Gerlac", "圣格拉赫"},
    {"S.Hedwig", "圣海德薇"},
    {"S.Agnes", "圣阿格尼丝"},
    {"S.Aidan", "圣艾丹"},
    {"S.Clare", "圣克莱尔"},
    {"S.David", "圣大卫"},
    {"S.Denis", "圣德尼"},
    {"S.Drogo", "圣德罗戈"},
    {"S.Giles", "圣吉尔"},
    {"S.Henry", "圣亨利"},
    {"S.Eric", "圣埃里克"},
};
extern "C" int config_get_int(const char *key, int def);

static std::string saint_spaced(const std::string &n)        /* "St.Jude" -> "St. Jude" */
{
    if (n.size() > 3 && n.compare(0, 3, "St.") == 0 && n[3] != ' ') return "St. " + n.substr(3);
    return n;
}

void gamedata_names(void)
{
    bool full = config_get_int("display.full_names", 1) != 0, worn = config_get_int("display.armour_worn", 1) != 0;
    for (ItemDef &d : gamedata.items) {
        std::string n = d.game_name, slot;
        bool armour = d.kind == K_ARMOR && n.size() > 2 && (n[0] == 'V' || n[0] == 'L') && n[1] == ':';
        if (armour) { slot = n.substr(0, 2); n = n.substr(2); }
        if (full) for (const auto &m : ITEM_FULL) if (n == m[0]) { n = m[1]; break; }
        d.plain = n;
        if (!armour) d.name = n;
        else if (full && worn) d.name = n + (slot[0] == 'V' ? "（上身）" : "（下身）");
        else d.name = slot + n;
    }
    for (Saint &t : gamedata.saints) {
        t.name = t.game_name;
        if (!full) continue;
        for (const auto &m : SAINT_FULL) if (t.game_name == m[0]) { t.name = m[1]; break; }
        t.name = saint_spaced(t.name);
    }
    gamedata.formula_names = gamedata.formula_game_names;
    if (full) {
        for (int i = 0; i < (int)gamedata.formula_names.size() && i < 66; i++)
            if (FORMULA_ZH[i][0]) gamedata.formula_names[i] = FORMULA_ZH[i];
        for (std::string &f : gamedata.formula_names)
            for (const auto &m : FORMULA_PARTS) { size_t at = f.find(m[0]); if (at != std::string::npos) f.replace(at, strlen(m[0]), m[1]); }
    }
}

std::string gamedata_spell_text(const std::string &t)
{
    if (!config_get_int("display.full_names", 1)) return t;
    std::string r = t;
    auto swap = [&](const char *from, const char *to) {
        size_t at = 0, fl = strlen(from), tl = strlen(to);
        while ((at = r.find(from, at)) != std::string::npos) { r.replace(at, fl, to); at += tl; }
    };
    for (const auto &m : ITEM_FULL) swap(m[0], m[1]);
    for (const auto &m : SAINT_FULL) swap(m[0], m[1]);
    for (const auto &m : FORMULA_PARTS) swap(m[0], m[1]);
    for (size_t i = 0; i + 3 < r.size(); i++)                  /* "St.Jude" -> "St. Jude" */
        if (r.compare(i, 3, "St.") == 0 && r[i + 3] >= 'A' && r[i + 3] <= 'Z' && (i == 0 || !isalpha((unsigned char)r[i - 1]))) r.insert(i + 3, " ");
    return r;
}

std::string loc_display_name(int loc)
{
    if (loc >= 0 && loc < (int)gamedata.cities.size() && !gamedata.cities[loc].name.empty()) return gamedata.cities[loc].name;
    u32 r = loc >= 0 ? g_location(loc) : 0;
    if (!r) return "";
    char b[24]; g_str((u16)(r >> 16), (u16)((u16)r + 0x26), b, sizeof b);
    static const struct { const char *cut, *full; } CUT[] = { { "雷克林斯恩", "雷克林斯豪森" }, { "马里恩韦尔德", "马里恩韦德尔" } };
    for (const auto &c : CUT) if (!strcmp(b, c.cut)) return c.full;
    return game_text(b, sizeof b);
}

/* a town's number for its cards: loc_query 0x21, the city's +0x56 + party_seed (16-bit, as the game) */
int town_seed(int loc)
{
    if (loc < 0 || loc >= (int)gamedata.cities.size()) return 0;
    return (u16)(gamedata.cities[loc].seed + g_rw(G_PARTY_SEED));
}
/* the quality an alchemist's ($Alche00, s066 190c:04c9: (seed + 4) mod 25 + 1, unsigned) or a university's
   ($Unive00, s056 18c0:0137: ((seed + 7) & 15) + 14) better Philosopher's Stone has in town loc: the same
   all game; either card sells it only when it is better than the party's, and says so only after its roll */
extern "C" int town_stone_offer(int loc, int university)
{
    if (loc < 0 || loc >= (int)gamedata.cities.size()) return 0;
    unsigned s = (unsigned)town_seed(loc);
    return university ? (int)(((s + 7) & 15) + 14) : (int)((u16)(s + 4) % 25 + 1);
}
