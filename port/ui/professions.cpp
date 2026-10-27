// professions.cpp - see professions.h.
#include <stdio.h>
#include "professions.h"

bool cg_prof_rule(int i, const CgCtx &c)
{
    switch (i) {                                  /* (s037 18c8:22be, as decompiled: src/wip/s037/s037_18c8.c) */
    case 0:
        if (!(c.taken[0] & 1) && c.age < 0x1e)
            return true;
        else if (!(c.taken[0] & 1) && c.age < 0x1a && c.attr[1] >= 0x19)
            return true;
        return false;
    case 1:
        if ((c.taken[0] & 1) || (c.taken[2] & 1) || (c.taken[3] & 1) || (c.taken[5] & 1)
            || (c.taken[20] & 1) || (c.taken[36] & 1) || (c.taken[1] & 1))
            return true;
        return false;
    case 2:
        if ((c.taken[1] & 1) || (c.taken[2] & 1) || (c.taken[5] & 1) || (c.taken[3] & 1))
            return true;
        return false;
    case 3:
        if (c.attr[4] >= 0x14 && c.attr[3] >= 0x14 && c.attr[5] >= 0x14 && ((c.taken[6] & 1)
            || (c.taken[2] & 1) || (c.taken[7] & 1) || (c.taken[5] & 1) || (c.taken[9] & 1)
            || (c.taken[20] & 1) || (c.taken[3] & 1)))
            return true;
        return false;
    case 4:
        if ((c.taken[6] & 1) || (c.taken[4] & 1) || c.child == 0)
            return true;
        return false;
    case 5:
        if (c.skill[9] > 0xf && ((c.age < 0x14 && c.child == 0) || (c.taken[7] & 1)
            || (c.taken[5] & 1) || c.last == 4 || c.last == 6 || c.last == 9 || c.last == 0xf
            || c.last == 3))
            return true;
        return false;
    case 6:
        if ((c.age < 0x14 && (c.child == 0 || c.child == 1)) || (c.taken[7] & 1)
            || (c.taken[6] & 1) || c.last == 5 || c.last == 8 || c.last == 9 || c.last == 0xf
            || c.last == 0x14 || c.last == 3)
            return true;
        return false;
    case 7:
        if (c.last == 7 || ((c.prev == 4 || c.prev == 6 || c.prev == 9 || c.prev == 0xf)
            && (c.last == 4 || c.last == 6 || c.last == 9 || c.last == 0xf)))
            return true;
        return false;
    case 8:
        if (c.female != 1 && c.attr[4] >= 0x14 && c.attr[3] >= 0x14 && c.attr[5] >= 0x14
            && ((c.taken[7] & 1) || (c.taken[9] & 1) || (c.taken[15] & 1) || (c.taken[8] & 1)
            || c.last == 6 || c.last == 0xe || c.last == 0x14 || c.last == 0x16
            || c.last == 0x18 || c.last == 4 || ((c.prev == 0xc || c.prev == 0xd
            || c.prev == 0x15 || c.prev == 0xa) && (c.last == 0xc || c.last == 0xd
            || c.last == 0x15 || c.last == 0xa))))
            return true;
        return false;
    case 9:
        if (c.female != 1 && c.attr[4] >= 0x19 && c.attr[3] >= 0x19 && c.attr[5] >= 0x19
            && (c.last == 9 || c.last == 0xf || ((c.prev == 7 || c.prev == 8 || c.prev == 6)
            && (c.last == 7 || c.last == 8 || c.last == 6))))
            return true;
        return false;
    case 10:
        if (c.female != 1 && (c.last == 8 || c.last == 0xa || c.last == 0xb || c.last == 0xd
            || c.last == 0xe || c.last == 0xf))
            return true;
        return false;
    case 11:
        if (c.skill[9] >= 0xf)
            return true;
        return false;
    case 12:
        if ((c.age < 0x14 && c.child != 4 && c.child != 5) || (c.attr[4] >= 0xc
            && !(c.taken[13] & 1) && !(c.taken[14] & 1) && !(c.taken[15] & 1)
            && !(c.taken[8] & 1) && !(c.taken[9] & 1) && !(c.taken[10] & 1)
            && !(c.taken[15] & 1) && !(c.taken[8] & 1) && !(c.taken[22] & 1)
            && !(c.taken[23] & 1) && !(c.taken[24] & 1) && !(c.taken[25] & 1)))
            return true;
        return false;
    case 13:
        if (c.age < 0x14 || (!(c.taken[13] & 1) && !(c.taken[4] & 1) && !(c.taken[6] & 1)
            && !(c.taken[7] & 1) && !(c.taken[14] & 1) && !(c.taken[15] & 1)
            && !(c.taken[8] & 1) && !(c.taken[9] & 1) && !(c.taken[22] & 1)
            && !(c.taken[23] & 1) && !(c.taken[24] & 1) && !(c.taken[25] & 1)
            && !(c.taken[10] & 1)))
            return true;
        return false;
    case 14:
        if (c.attr[4] >= 0xf && c.attr[3] >= 0xf && c.attr[5] >= 0xf && c.skill[8] >= 5
            && ((c.taken[4] & 1) || (c.taken[6] & 1) || (c.taken[13] & 1) || (c.taken[7] & 1)
            || (c.taken[14] & 1) || (c.taken[15] & 1) || (c.taken[8] & 1) || (c.taken[9] & 1)
            || (c.taken[22] & 1) || (c.taken[23] & 1) || (c.taken[24] & 1) || (c.taken[25] & 1)
            || (c.taken[10] & 1) || (c.taken[21] & 1)))
            return true;
        return false;
    case 15:
        if (c.attr[4] >= 0x14 && c.attr[3] >= 0x14 && c.attr[5] >= 0x14 && c.skill[8] >= 0xf
            && c.attr[4] >= 0xc && (c.last == 4 || c.last == 6 || c.last == 8 || c.last == 7
            || c.last == 9 || c.last == 0xf || ((c.prev == 0xe || c.prev == 0x18)
            && (c.last == 0xe || c.last == 0x18))))
            return true;
        return false;
    case 16:
        if ((c.age < 0x14 && (c.child == 4 || c.child == 5)) || (c.last != 4 && c.last != 3
            && c.last != 5 && c.last != 6 && c.last != 8 && c.last != 0x12 && c.last != 0x18
            && c.last != 0x19 && c.last != 0x1c && (!c.taken[7] & 1) && (!c.taken[15] & 1)
            && (!c.taken[9] & 1) && (!c.taken[19] & 1) && (!c.taken[20] & 1)
            && (!c.taken[26] & 1) && (!c.taken[29] & 1)))
            return true;
        return false;
    case 17:
        if ((c.age < 0x14 && c.child != 4 && c.child != 5) || (c.skill[10] >= 5
            && c.attr[4] >= 0xc && ((c.taken[3] & 1) || (c.taken[4] & 1) || (c.taken[6] & 1)
            || (c.taken[8] & 1) || (c.taken[9] & 1) || (c.taken[14] & 1) || (c.taken[16] & 1)
            || (c.taken[15] & 1) || (c.taken[18] & 1) || (c.taken[22] & 1) || (c.taken[23] & 1)
            || (c.taken[24] & 1) || (c.taken[25] & 1) || (c.taken[28] & 1) || (c.taken[29] & 1)
            || (c.taken[32] & 1) || (c.taken[17] & 1))))
            return true;
        return false;
    case 18:
        if ((c.age < 0x14 && (c.child == 0 || c.child == 1)) || (c.skill[10] >= 0xa
            && c.attr[4] >= 0xf && ((c.taken[17] & 1) || (c.taken[4] & 1) || (c.taken[7] & 1)
            || (c.taken[19] & 1) || (c.taken[18] & 1) || (c.taken[24] & 1) || (c.taken[26] & 1)
            || (c.taken[29] & 1))))
            return true;
        return false;
    case 19:
        if (c.skill[10] >= 0xa && c.attr[4] >= 0x14 && (c.last == 7 || c.last == 9
            || c.last == 0x12 || c.last == 0x13))
            return true;
        return false;
    case 20:
        if ((c.taken[34] & 1) && ((c.taken[4] & 1) || (c.taken[2] & 1) || (c.taken[3] & 1)
            || (c.taken[5] & 1) || (c.taken[7] & 1) || (c.taken[8] & 1) || (c.taken[15] & 1)
            || (c.taken[9] & 1) || (c.taken[19] & 1) || (c.taken[24] & 1) || (c.taken[29] & 1)
            || (c.taken[26] & 1) || (c.taken[20] & 1)))
            return true;
        return false;
    case 21:
        if (c.age < 0x14 || (c.skill[12] >= 6 && c.attr[4] >= 0xc && (c.last == 0
            || c.last == 2 || c.last == 1 || c.last == 0xa || c.last == 0xb || c.last == 0xc
            || c.last == 0xd || c.last == 0xe || c.last == 0x10 || c.last == 0x11
            || c.last == 0x12 || c.last == 0x15 || c.last == 0x1c || c.last == 0x1b
            || c.last == 4 || c.last == 0x20 || c.last == 0x21)))
            return true;
        return false;
    case 22:
        if ((c.age < 0x14 && c.child == 1) || (c.skill[12] >= 0xf && c.attr[4] >= 0xc
            && ((c.taken[4] & 1) || (c.taken[6] & 1) || (c.taken[3] & 1) || (c.taken[5] & 1)
            || (c.taken[20] & 1) || (c.taken[8] & 1) || (c.taken[15] & 1) || (c.taken[9] & 1)
            || (c.taken[19] & 1) || (c.taken[24] & 1) || (c.taken[21] & 1) || (c.taken[22] & 1)
            || (c.taken[25] & 1) || (c.taken[26] & 1) || ((c.prev == 0xc || c.prev == 0xe
            || c.prev == 0x12) && (c.last == 0xc || c.last == 0xe || c.last == 0x12)))))
            return true;
        return false;
    case 23:
        if (c.skill[13] >= 0xf && ((c.taken[21] & 1) || (c.taken[22] & 1) || (c.taken[24] & 1)
            || (c.taken[25] & 1) || (c.taken[26] & 1) || (c.taken[23] & 1)))
            return true;
        return false;
    case 24:
        if (c.skill[12] >= 0x14 && ((c.taken[9] & 1) || (c.taken[22] & 1) || (c.taken[23] & 1)
            || (c.taken[25] & 1) || (c.taken[26] & 1) || (c.taken[15] & 1) || (c.taken[24] & 1)))
            return true;
        return false;
    case 25:
        if (c.attr[4] >= 0x1e && ((c.taken[8] & 1) || (c.taken[9] & 1) || (c.taken[15] & 1)
            || (c.taken[21] & 1) || (c.taken[24] & 1) || (c.taken[22] & 1) || (c.taken[25] & 1)
            || (c.taken[26] & 1) || ((c.prev == 0xc || c.prev == 0xe || c.prev == 0xa
            || c.prev == 0x17) && (c.last == 0xc || c.last == 0xe || c.last == 0xa
            || c.last == 0x17))))
            return true;
        return false;
    case 26:
        if (c.attr[4] >= 0x23 && (c.taken[25] & 1))
            return true;
        return false;
    case 27:
        if ((c.age < 0x14 && c.child != 0) || (c.age >= 0x14 && !(c.taken[27] & 1)
            && !(c.taken[7] & 1) && !(c.taken[9] & 1) && !(c.taken[15] & 1)
            && !(c.taken[19] & 1) && !(c.taken[26] & 1) && !(c.taken[28] & 1)
            && !(c.taken[29] & 1) && c.last != 3 && c.last != 4 && c.last != 5 && c.last != 6
            && c.last != 8 && c.last != 0x14 && c.last != 0x16 && c.last != 0x12
            && c.last != 0x17 && c.last != 0x18 && c.last != 0x19))
            return true;
        return false;
    case 28:
        if ((c.age < 0x14 && (c.child == 2 || c.child == 3)) || (c.taken[23] & 1)
            || (c.taken[25] & 1) || (c.taken[26] & 1) || (c.taken[28] & 1) || (c.taken[29] & 1)
            || (c.taken[27] & 1))
            return true;
        return false;
    case 29:
        if (c.attr[4] >= 0xc && ((c.taken[19] & 1) || (c.taken[26] & 1) || (c.taken[28] & 1)))
            return true;
        return false;
    case 30:
        if ((c.age < 0x14 && c.child != 0) || (c.age >= 0x14 && !(c.taken[7] & 1)
            && !(c.taken[9] & 1) && !(c.taken[15] & 1) && !(c.taken[19] & 1)
            && !(c.taken[26] & 1) && !(c.taken[29] & 1) && c.last != 3 && c.last != 4
            && c.last != 5 && c.last != 6 && c.last != 8 && c.last != 0x14 && c.last != 0x18
            && c.last != 0x12 && c.last != 0x19))
            return true;
        return false;
    case 31:
        if ((c.age < 0x14 && c.child != 0) || (c.age >= 0x14 && !(c.taken[7] & 1)
            && !(c.taken[9] & 1) && !(c.taken[15] & 1) && !(c.taken[19] & 1)
            && !(c.taken[26] & 1) && !(c.taken[28] & 1) && !(c.taken[29] & 1) && c.last != 3
            && c.last != 4 && c.last != 5 && c.last != 6 && c.last != 8 && c.last != 0x14
            && c.last != 0x18 && c.last != 0x12 && c.last != 0x19))
            return true;
        return false;
    case 32:
        if ((c.age < 0x14 && (c.child == 0 || c.child == 1 || c.child == 2))
            || (c.attr[4] >= 0x19 && c.skill[16] >= 0xf && ((c.taken[1] & 1)
            || (c.taken[2] & 1) || (c.taken[3] & 1) || (c.taken[4] & 1) || (c.taken[8] & 1)
            || (c.taken[10] & 1) || (c.taken[14] & 1) || (c.taken[16] & 1) || (c.taken[17] & 1)
            || (c.taken[18] & 1) || (c.taken[21] & 1) || (c.taken[22] & 1) || (c.taken[24] & 1)
            || (c.taken[25] & 1) || (c.taken[28] & 1) || (c.taken[30] & 1) || (c.taken[33] & 1)
            || (c.taken[31] & 1))))
            return true;
        return false;
    case 33:
        if ((c.age < 0x14 && (c.child != 3 || c.child != 5)) || (c.skill[16] >= 0xa
            && ((c.taken[1] & 1) || (c.taken[2] & 1) || (c.taken[8] & 1) || (c.taken[10] & 1)
            || (c.taken[35] & 1) || (c.taken[16] & 1) || (c.taken[17] & 1) || (c.taken[18] & 1)
            || (c.taken[21] & 1) || (c.taken[22] & 1) || (c.taken[24] & 1) || (c.taken[25] & 1)
            || (c.taken[28] & 1) || (c.taken[30] & 1) || (c.taken[33] & 1) || (c.taken[31] & 1)
            || (c.taken[36] & 1))))
            return true;
        return false;
    case 34:
        if ((c.age < 0x14 && (c.child != 0 || c.child != 1)) || (!(c.taken[7] & 1)
            && !(c.taken[9] & 1) && !(c.taken[15] & 1) && !(c.taken[19] & 1)
            && !(c.taken[26] & 1) && !(c.taken[29] & 1) && c.last != 3 && c.last != 4
            && c.last != 5 && c.last != 6 && c.last != 8 && c.last != 0x1c && c.last != 0x18
            && c.last != 0x12 && c.last != 0x19))
            return true;
        return false;
    case 35:
        if ((c.age < 0x14 && (c.child == 0 || c.child == 3 || c.child == 5))
            || (c.skill[18] >= 0xf && (c.last != 7 || c.last != 0xf || c.last != 0x1a
            || c.last != 0x13 || c.last != 0x1d) && ((c.taken[0] & 1) || (c.taken[1] & 1)
            || (c.taken[2] & 1) || (c.taken[3] & 1) || (c.taken[5] & 1) || (c.taken[10] & 1)
            || (c.taken[16] & 1) || (c.taken[11] & 1) || (c.taken[20] & 1) || (c.taken[18] & 1)
            || (c.taken[34] & 1) || (c.taken[35] & 1) || (c.taken[36] & 1))))
            return true;
        return false;
    case 36:
        if (c.age < 0x14 || (c.taken[0] & 1) || (c.taken[1] & 1) || (c.taken[2] & 1)
            || (c.taken[3] & 1) || (c.taken[5] & 1) || (c.taken[33] & 1) || (c.taken[36] & 1)
            || (c.taken[35] & 1) || c.last == 0xa || c.last == 0xb || c.last == 0x14
            || c.last == 0x10 || c.last == 0x1e || c.last == 0x1f || c.last == 0x20
            || c.last == 0x22)
            return true;
        return false;
    }
    return false;
}

std::vector<int> cg_prof_offered(const CgCtx &c)
{
    std::vector<int> v;
    for (int i = 0; i < 37; i++) { if (cg_prof_rule(i, c)) v.push_back(i); if (v.size() > 14) break; }
    return v;
}

const char *cg_prof_needs(int p)
{
    /* (in words, from cg_prof_rule; "first" is the first profession, straight after the childhood;
       "once" anywhere before, "just now" the one just done) */
    static const char *N[37] = {
        /* Recruit */ "从未招募过，且年龄未满 30 岁。",
        /* Soldier */ "曾经是新兵、士兵、老兵、队长、骑士、Schulz 或强盗。",
        /* Veteran */ "曾经是士兵、老兵、队长或骑士。",
        /* Captain */ "{智力：20} {感知：20} {魅力：20}；曾经是老兵、队长、骑士、宫廷侍臣、庄园主、主教或Schulz。",
        /* Noble Heir */ "在贵族家庭长大，或曾经是贵族继承人或宫廷侍臣。",
        /* Knight */ "{美德：16}；童年在贵族家庭，或曾经是骑士、庄园主，或刚刚成为贵族继承人、宫廷侍臣、队长、主教或修道院院长/女院长。",
        /* Courtier */ "童年在贵族家庭或富裕城市家庭，或曾经是宫廷侍臣、庄园主；或者刚刚成为骑士、队长、神父、主教、修道院院长/女院长或Schulz。",
        /* Manorial Lord */ "刚才是一位领主，或者最后两种职业都是贵族继承人、宫廷侍从、主教或修道院院长/女院长。",
        /* Priest */ "一位拥有{智力:20} {感知:20} {魅力:20}的男子；曾是领主、神父、主教或修道院院长，或者刚才还是贵族继承人、宫廷侍从、僧侣、学徒、书记员或教授，又或者最后两种职业都是献身者、见习僧侣、修士或学生。",
        /* Bishop */ "一位拥有{智力:25} {感知:25} {魅力:25}的男子；刚才还是主教或修道院院长，或者最后两种职业都是宫廷侍从、领主或神父。",
        /* Friar */ "一位男子；刚才还是神父、修士、隐士、见习僧侣、僧侣或修道院院长。",
        /* Hermit */ "{Virtue:15}.",
        /* Oblate */ "除平民之子外，任何人的第一职业；否则{智力:12}，且绝不是见习僧侣、僧侣/修女、修道院院长/女院长、神父、主教、修士、书记员、医师、教授或炼金术师。",
        /* Novice */ "第一职业；或者从来不是贵族继承人、宫廷侍从、领主、神父、主教、修士、见习僧侣、僧侣/修女、修道院院长/女院长、书记员、医师、教授或炼金术师。",
        /* Monk/Nun */ "{智力:15} {感知:15} {魅力:15}，{宗教:5}；曾是贵族继承人、宫廷侍从、领主、神父、主教、修士、见习僧侣、僧侣/修女、修道院院长/女院长、学生、书记员、医师、教授或炼金术师。",
        /* Abbot */ "{智力:20} {感知:20} {魅力:20}，{宗教:15}；刚才还是贵族继承人、宫廷侍从、领主、神父、主教或修道院院长/女院长，或者最后两种职业都是僧侣/修女或教授。",
        /* Peddler */ "不仅是在成为贵族继承人、宫廷侍从、队长、骑士、神父、旅行商人、教授、炼金术师或熟练工匠之后，而且从来不是领主、主教、修道院院长/女院长、商人兼业主、学徒、首席炼金术师或首席工匠。",
        /* Local Trader */ "除平民之子外，任何人的第一职业；否则{说通用语:5}，{智力:12}，且曾是队长、贵族继承人、宫廷侍从、神父、主教、僧侣/修女、修道院院长/女院长、小贩、本地商人、旅行商人、书记员、医师、教授、炼金术师、熟练工匠、首席工匠或骗子。",
        /* Travelling Merchant */ "在贵族家庭或富裕城市家庭中长大的孩子，其第一职业；否则{说通用语:10}，{智力:15}，且曾是贵族继承人、领主、本地商人、旅行商人、商人兼业主、教授、首席炼金术师或首席工匠。",
        /* Merchant-Proprietor */ "{说通用语:10}，{智力:20}；刚才还是领主、主教、旅行商人或商人兼业主。",
        /* Schulz */ "曾经是农民，也曾是老兵、队长、骑士、贵族继承人、领主、神父、主教、修道院院长/女院长、商人兼业主、学徒、教授、首席炼金术师或首席工匠。",
        /* Student */ "第一职业；或者{读写能力:6}，{智力:12}，且刚才还是新兵、士兵、老兵、贵族继承人、修士、隐士、献身者、见习僧侣、僧侣/修女、小贩、本地商人、旅行商人、学生、学徒、熟练工匠、骗子或窃贼。",
        /* Clerk */ "在富裕城市家庭中长大的孩子，其第一职业；否则{读写能力:15}，{智力:12}，且曾是队长、贵族继承人、骑士、宫廷侍从、神父、主教、修道院院长/女院长、商人兼业主、学徒、书记员、教授、炼金术师或首席炼金术师，或者最后两种职业都是献身者、僧侣/修女或旅行商人。",
        /* Physician */ "{治疗:15}；曾是学生、书记员、医师、教授、炼金术师或首席炼金术师。",
        /* Professor */ "{读写能力:20}；曾是主教、修道院院长/女院长、书记员、医师、教授、炼金术师或首席炼金术师。",
        /* Alchemist */ "{智力:30}；曾是神父、主教、修道院院长/女院长、学生、书记员、教授、炼金术师或首席炼金术师，或者最后两种职业都是献身者、僧侣/修女、修士或医师。",
        /* Master Alchemist */ "{智力:35}；曾是炼金术师。",
        /* Apprentice */ "除贵族之子外，任何人的第一职业；此后绝不再做学徒、熟练工匠、领主、主教、修道院院长/女院长、商人兼业主或首席炼金术师，也不仅是在成为队长、贵族继承人、骑士、宫廷侍从、神父、学徒、书记员、旅行商人、医师、教授或炼金术师之后。",
        /* Journeyman */ "在城镇商人家族或乡村手工业家庭中长大的孩子，其第一职业；或者曾是学徒、熟练工匠、医师、炼金术师或首席炼金术师。",
        /* Master Craftsman */ "{智力:12}；曾是熟练工匠、商人兼业主或首席炼金术师。",
        /* Laborer */ "除贵族之子外，任何人的第一职业；此后绝不再做领主、主教、修道院院长/女院长、商人兼业主、首席炼金术师或首席工匠，也不仅是在成为队长、贵族继承人、骑士、宫廷侍从、神父、学徒、旅行商人、教授或炼金术师之后。",
        /* Vagabond */ "除贵族之子外，任何人的第一职业；此后绝不再做领主、主教、修道院院长/女院长、商人兼业主、熟练工匠、首席炼金术师或首席工匠，也不仅是在成为队长、贵族继承人、骑士、宫廷侍从、神父、学徒、旅行商人、教授或炼金术师之后。",
        /* Swindler */ "在贵族家庭、富裕城市家庭或城镇商人家族中长大的孩子，其第一职业；否则{智力:25}，{街头智慧:15}，且曾是士兵、老兵、队长、贵族继承人、神父、修士、僧侣/修女、小贩、本地商人、旅行商人、学生、书记员、教授、炼金术师、熟练工匠、劳工、流浪者或窃贼。",
        /* Thief */ "第一职业；或者{街头智慧:10}，且曾是士兵、老兵、神父、修士、小贩、本地商人、旅行商人、学生、书记员、教授、炼金术师、熟练工匠、劳工、流浪者、窃贼、猎人或强盗。",
        /* Peasant */ "第一职业；或者绝不再做领主、主教、修道院院长/女院长、商人兼业主、首席炼金术师或首席工匠，也不仅是在成为队长、贵族继承人、骑士、宫廷侍从、神父、旅行商人、教授、炼金术师或熟练工匠之后。",
        /* Hunter */ "在贵族家庭、乡村手工业家庭或农村平民中长大的孩子，其第一职业；否则{木头智慧:15}，且曾是新兵、士兵、老兵、队长、骑士、修士、隐士、小贩、旅行商人、学徒、农民、猎人或强盗。",
        /* Bandit */ "第一职业；或者曾是新兵、士兵、老兵、队长、骑士、窃贼、猎人或强盗，或者刚才还是修士、隐士、小贩、学徒、劳工、流浪者、骗子或农民。",
    };
    return p >= 0 && p < 37 ? N[p] : "";
}

std::vector<int> cg_prof_leads_to(int p)
{
    std::vector<int> v;
    if (p < 0 || p >= 37) return v;
    CgCtx a; a.age = 20;                                      /* (a later profession, every attribute and skill high: only the past counts) */
    for (int k = 0; k < 6; k++) a.attr[k] = 99;
    for (int k = 0; k < 19; k++) a.skill[k] = 99;
    CgCtx b = a; b.taken[p] = 1; b.last = b.prev = p;          /* (p once, and the last two terms) */
    for (int q = 0; q < 37; q++) if (q != p && cg_prof_rule(q, b) && !cg_prof_rule(q, a)) v.push_back(q);
    return v;
}

static std::string short_of(int p, const CgCtx &c, const CgCtx &past);

std::string cg_prof_short(int p, const CgCtx &c) { return short_of(p, c, c); }

std::string cg_prof_why(int p, const CgCtx &c)
{
    if (cg_prof_rule(p, c)) return "";
    CgCtx hi = c;
    for (int k = 0; k < 6; k++) hi.attr[k] = 99;
    for (int k = 0; k < 19; k++) hi.skill[k] = 99;
    std::string r;
    CgCtx m = hi; m.female = 0;
    if (c.female && !cg_prof_rule(p, hi) && cg_prof_rule(p, m)) { r = "仅限男性"; hi.female = 0; }
    CgCtx past = c; past.female = hi.female;
    if (!cg_prof_rule(p, hi)) {                               /* the past is not right: some past would be */
        int q = -1;
        for (int k = 0; k < 37 && q < 0; k++) { CgCtx t = hi; t.taken[k] = 1; t.last = t.prev = k; if (cg_prof_rule(p, t)) q = k; }
        r += r.empty() ? "不适用于该角色的过往" : "，也不适用于这段过往";
        if (q < 0) return r + ".";
        past.taken[q] = 1; past.last = past.prev = q;
    }
    std::string s = short_of(p, c, past);
    if (!s.empty()) r += (r.empty() ? "除了" : "；还缺少") + s;
    return r + ".";
}

/* the attributes and skills c lacks for p, its past taken as `past` */
static std::string short_of(int p, const CgCtx &c, const CgCtx &past)
{
    static const struct { bool skill; int k; const char *name; } S[] = {
        { false, 1, "力量" }, { false, 3, "感知" }, { false, 4, "智力" }, { false, 5, "魅力" },
        { true, 8, "宗教" }, { true, 9, "美德" }, { true, 10, "通用语言" }, { true, 12, "读写能力" },
        { true, 13, "治疗" }, { true, 16, "精通街头" }, { true, 18, "精通林地" } };
    const int NS = (int)(sizeof S / sizeof S[0]);
    auto val = [&](const CgCtx &x, int i) -> signed char { return S[i].skill ? x.skill[S[i].k] : x.attr[S[i].k]; };
    auto set = [&](CgCtx &x, int i, int v) { if (S[i].skill) x.skill[S[i].k] = (signed char)v; else x.attr[S[i].k] = (signed char)v; };
    CgCtx hi = past;
    for (int i = 0; i < NS; i++) if (val(hi, i) < 99) set(hi, i, 99);
    if (!cg_prof_rule(p, hi)) return "";                      /* (the past, or being a woman, decides) */
    std::string r;
    for (int i = 0; i < NS; i++) {
        CgCtx t = hi; set(t, i, val(c, i));
        if (cg_prof_rule(p, t)) continue;                     /* (enough of this one already) */
        int need = val(c, i) + 1;
        while (need < 99) { set(t, i, need); if (cg_prof_rule(p, t)) break; need++; }
        char b[64]; snprintf(b, sizeof b, "%s%s %d（已有%d）", r.empty() ? "" : ", ", S[i].name, need, (int)val(c, i));
        r += b;
    }
    return r;
}
