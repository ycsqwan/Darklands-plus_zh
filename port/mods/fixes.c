/* fixes.c - fixes for bugs that are still in the last official version (483.07). Each can
 * be switched off (F12 > Settings > Fixes); "play the original" switches them all off.
 *
 *  blessing  Saint blessings (and potions) that raise an attribute record how much they
 *            added before the attribute is capped (raise_attr_temp, s029 0e76:4c04: the cap
 *            is max + the LARGEST running boost, add_attr also clamps at 99). When they run
 *            out the full recorded amount is taken off again, so a second blessing on the
 *            same attribute, or one near 99, costs points for good (Int never comes back).
 *            Fixed by recording what was really added. And when a boost runs out (s029
 *            0e76:448a) it is taken off whatever the attribute is by then: endurance a night's
 *            rest has put back to its maximum, or strength wounds have taken, lost the boost
 *            again (a member at 3 strength whose +4 ran out died). Now only what is still above
 *            the maximum (and the other boosts and lowerings running) goes. Saints that lower an
 *            attribute for a while (Perception, Intelligence, Agility, Charisma: a negative d) are
 *            recorded as negative and given back when they run out, never past the maximum.
 *  fireward  St. Polycarp, St. Charity, Firewall ... set the high nibble of the member's
 *            fire-protection byte (party +0x41 = combatant +0x55). hit_damage (s014
 *            18bf:0778) adds that byte (& 0x7f) to the armour, so bits 4-6 turn into up to
 *            +64 armour that never wears off: the "invulnerable after Polycarp + Firewall"
 *            bug. Fixed by hiding those bits from the armour sum.
 *  fullpack  A member holds at most 64 item records; add_item_rec_impl silently drops what does
 *            not fit (quest items included). It now goes to another member with room.
 *  nullitem  Items with a code past the item list (>= 200) show as "(null)" (a NULL name
 *            pointer) and can be neither used nor sold; empty stacks (count 0) appear from
 *            the inn's ingredient refund. add_item_rec_impl (s031 18e7:01b4) now refuses both.
 *  castle    In a castle battle only the death of the boss ends the fight (D_9084), and that
 *            flag is not saved with the battle: after a save and reload mid-battle the knight
 *            is already dead and leaving always counts as fleeing ("battle not ending").
 *            combat_over (s014 0e76:08fe) now also looks at the boss's dead flag.
 *  bounty    A merchant's robber-knight bounty: $Merch03 (s092 188f:0222) hands $Raubi01
 *            (s080 1870:000c) the castle as the quest giver (saved_card2), so the reward event
 *            is not found; the card then reads and frees events[-1] (memory corruption) and the
 *            reward can be claimed again and again. The giver is now corrected on the way in.
 *  innsave   The lodging screen (s036 1462:0082) disables the Save and Load menu items (DS:9920,
 *            9934 = 2) and re-enables them on most exits but not all (a click on an empty
 *            party slot returns at once): Save/Load stayed grey until a character sheet was
 *            opened. They are enabled again on the way out.
 *  colours   The "neon hair" bug: characters' colours (and figures) change after encounters.
 *            They are now watched and put back (fixes_frame).
 *  copyprot  The manual check (s041 copy_protection, once when the party first leaves a town:
 *            pick the alchemical symbol for a substance) is skipped.
 *  alchemy   Brewing takes the ingredients with s033 1462:1630, which skips the stack that
 *            slides up when one is used up and forgets what it could not take: most batches
 *            cost only part of their ingredients. The remainder is now taken too.
 *  potions   Deadly Blade (+3..5 damage), Strongedge (+3..5 penetration) and Greatpower
 *            (14/22/30 added to the whole byte) change the member's bonus byte, party +0x34
 *            (= combatant +0x48: high nibble damage, low nibble penetration), in use_potion
 *            (s031 1462:43b2), and record a kind 0x4a event of about a day. Its expiry (s029
 *            0e76:448a) only undoes kinds 0x48 and 0x49, so the bonus stays for good; the
 *            nibbles wrap (a fourth Deadly Blade gives +1 damage, Greatpower's 14 spills into
 *            the damage nibble), and hit_damage's penetration (byte + weapon) & 15 wraps too:
 *            "after Greatpower my bows only do 3-5 damage". The byte is now the sum of the
 *            potions still running, each nibble capped at 15, and penetration stops at 15.
 *  bestweapon member_best_skill (s029 0e76:01c0) sets best_skill for every skill it looks at
 *            (missing braces), so it always ends as 6 (Missile Devices); the practice of a
 *            card (s053 1838:074e: practice_skill(leader, best_skill ...)) then trained that.
 *            best_skill is set to the first of the highest weapon skills afterwards.
 *  offmap    One location of DARKLAND.LOC (record 373, a cave south-east of Basel) lies at y 931,
 *            one row below the 931-row map: quests that send the party there cannot be finished
 *            ("quests send you off the map"). It is moved onto the last row the party can step on,
 *            929: a step keeps the party's marker above the screen's last rows (s030 1462:0c44), so
 *            with the view at the map's bottom the party never stands on row 930 by walking, and a
 *            place is entered by stepping onto it. (Until 1.3-RC31 it went to 930: still out of reach.)
 *  itemcount When robbers take the party's things (lose_items, s031 18e7:0ab8: $Raubi02's
 *            surrender and others), the loop over a member's items starts at the count, one
 *            past the last item, and remove_item_rec (18e7:0406) lowers the count even for an
 *            item that is not there: one item too many is taken, and a member who had nothing
 *            left ends with -1 items (party +0x6a is a signed byte). When that member falls
 *            in battle, his pile is made (s014 0e76:2324) with _fmemcpy(DS:a895, items,
 *            count * 6): 65530 bytes over the whole data segment, stack included, and the
 *            game dies ("left through an unmatched return"). remove_item_rec and the battle's
 *            use_item_idx (s014 18bf:0fa4, the same flaw) now ignore an index past the
 *            items; counts are also kept within 0..64 (16 for enemies), so saves that carry
 *            -1 mend themselves.
 *  testcard  Card 0 is the developers' test card ($Initi00), whose picture (pics\xhouse3.pic) the game
 *            does not have: a card that goes back to where it came from (prev_screen) went there
 *            when that was 0, and the game stopped. It now goes to the map or the town.
 *  teachers  The lodging screen (s036 1462:0082) lists the town's teachers (kind 0x28 events) into
 *            DS:8a38 with loops that walk every kind 0x28 event (event_next) but stop after as
 *            many as this town has (s029 0e76:37e2) and number them all: when another town's
 *            teachers come first in the table (two towns with teachers within their week), the
 *            list is empty and DS:8a38 stale, Train (T) finds no teacher (event_find = -1) and
 *            reads events[-1]: a cost of -4096 pfennigs ("Train or Study (-4096pfs)") and a
 *            total that adds 4096; the day itself (camp_spend_day) finds no teacher either, so
 *            nothing is learnt and the member is set back to relaxing. While that screen runs,
 *            event_next(0x28, i) now returns only this town's teachers that have begun.
 *  armourwear Identical things share one record (add_item_rec merges code + type + quality),
 *            so a worn armour and its spares are one stack. In battle, wear (acid, fire:
 *            s019 1ef8:0968) lowers the quality of that whole record, and when it is used up
 *            (1ef8:0914) consume_item is called until the record is gone: the spares carried
 *            lose quality with the worn one and are destroyed with it. Now one is taken off
 *            the stack and worn down alone, and only that one is thrown away.
 *  cardopts  Options some cards grey by mistake: $Selec01 (out over the city wall) tests the
 *            purse against the sewer-without-horses option instead of the bribe, so the bribe
 *            could be chosen without the money; $Revol14 (the crowd at the fortress) greys the
 *            climb with a rope when the party has a rope; $CityW02 (over the wall after the
 *            guards) counts only the last member's rope; Clothmakers' Street by day ($Cloth00, s053
 *            1924:0004) hides its shopfronts (opt_flag_0 = 0: the other guilds' streets show
 *            theirs, its handler 02be opens the clothmaker's shop as at night, when a woken
 *            sleeper sells for a point of reputation). Their opt_on[] put right before the
 *            option list is shown (card_options, and from the card view's own drawing of it).
 *  debtnote  A debt-note errand's papers lie in the office its giver names (below, debtnote).
 *  alchrisk  A failed brew's accident follows the formula's risk the right way round (below).
 *  bishop    A cathedral's errand is the bishop's, and its relic is paid once (below, bishop).
 *  altar     A pagan altar the party has purged is no errand's site any more (below, altar).
 *  tomb      Winning the fight in a tomb wins the errand's thing; losing it does not (below, tomb).
 *  lordreward The lord of a town without a council pays for his tasks (below, lordreward).
 *  armourday Hardarmor, St. Dunstan's and St. Patrick's armour, St. Crispin's armour quality, Firewall
 *            and the fire saints' protection last a day (below).
 *  greatpower Greatpower raises the quality of the weapon, as the manual says (below, armourday).
 *  standings A saved game keeps the party's standing with the three companies (below, standings).
 *  battlesaints A saint's boosts called in a battle wear off (below, potions: the cards close).
 *  healboost Healing no longer lowers an attribute boosted above its maximum (below, healboost).
 *  teachcap  Lessons at the inn stop at the teacher's ability (below, teachcap).
 *  oilweapons Each weapon oil only with the weapons the manual names (below, oilweapons).
 *  pauls     The two St. Pauls each do what their texts say (below, pauls).
 *  saintrep  A saint's reputation goes to a real town: St. John Nepomuk's in Prag to Prag, St.
 *            Cecilia's away from any place to the nearest town (below, saintrep).
 *  nightbowyer The bowyers' street by night ($Bowye01, s065 1a29:02ac) opens the bowyer's shop
 *            (barter shop 5) through the card call s029 0e76:21aa with the smiths' item categories,
 *            0x040000FF (what $Black00, $Sword00/01 and $Armor00/01 pass), not the bowyer's
 *            0x083C0030 (by day, $Bowye00, s065 18fb): at night the bowyer sold the smiths'
 *            weapons and armour and no bows. Found by Arkana Mechanika (Darklands Restoration).
 *  confession A town church's confession ($CityC00, s057 1838:03cc) lasts 11 - reputation / 10 hours
 *            (from 0; below 0, 12 + reputation / 20): an hour at 100, none at 110, and from 120 a
 *            negative number that advance_hours (s029 1367:05c8) adds to the hour, turning the clock
 *            back (to hour -1 at midnight). Reputation passes 99 where a card sets it without the
 *            clamp of loc_reputation_adj ($Polit00 s070 1ac1:0532, +14..19; $Polit01). It now takes
 *            an hour at least. Found by Arkana Mechanika (Darklands Restoration).
 *  appointment A council's clerk ($Counc00, s054 19df:0888) or a lord's steward ($CityF00, s050 1a06) who fits
 *            the party "into the audience schedule tomorrow" makes a kind 0x3a event (until the evening, or
 *            dawn) whose expiry handler 4 (s029 0e76:4436, event_here(0x3b, 24): the appointment the
 *            audience chance reads as 99) was meant to follow; but it is made with flags 0 and the sweep
 *            (0e76:3f30) runs a handler only with flag 1: the appointment never came. It is now made
 *            when the 0x3a event ends, at its own town (the handler used wherever the party was).
 *            Found by Arkana Mechanika (Darklands Restoration).
 *  pileup    damage_queue (s014 18bf:08ca) adds a blow's Endurance (up to 42) and Strength (up to 37)
 *            to the target's queued damage, signed bytes (DS:d70c) that damage_apply (098e) deals:
 *            past 127 they wrap negative, its "more than 42" cap misses them, and the target gains
 *            what it should have lost. Now they stop at 127. Found by Arkana Mechanika (Darklands
 *            Restoration).
 *  castlefreeze The party's starting room on a level of battlefield kind 0x7c (s007 15af:1d78) is drawn with
 *            roll_dice(1, rooms) until it is not the gate room (DS:db63): with one room to draw and that
 *            the gate, for ever (a frozen game). After 50 draws of the gate room, room 0 is taken.
 *            Found by Arkana Mechanika (Darklands Restoration).
 *  university $Unive00's question about formulae (option 1, s056 18c0:0494) on failure notes the week's
 *            refusal as event_here_type(0x4e, 0, 168 h): the saints' question's (option 0). It now notes
 *            its own (type 1). Found by Arkana Mechanika (Darklands Restoration).
 *  partyitems add_item_rec_impl (s031 18e7:01b4) for "everyone" (-2: $Dunge00, footpads ...) joins the
 *            thing to a member's matching stack and then also appends it (no else): two. Each member now
 *            gets it through the one-member path. Found by Arkana Mechanika (Darklands Restoration).
 *  itemflags item_quality_flags_impl (s031 18e7:171c), the best quality of the things of a kind (LST
 *            flags) a member carries, compares item_defs[0]'s flags, not each thing's: 0 unless the
 *            first item's flags happened to match (then every thing counted). Its two callers ($WildH00,
 *            $meetw00: riding away) add the poorest rider's mount. Now each thing's own flags; and
 *            $meetw00 (s084 1838:0536) asks for flags 0x02000000, which no thing has, for the mounts'
 *            0x2000. Found by Arkana Mechanika (Darklands Restoration).
 *  membervalue Two cards take the value a party query returns for the member who has it (the query
 *            leaves that member in best_member, DS:991d): a town church's tenth ($CityC00, s057
 *            1838:067c) raises first the Divine Favor of "member" party_max_skill(Religion), a skill
 *            value (40 or so: party + 40 * 0x80 is past the party, in the game's data); a guild
 *            leader's house raised at night ($Snuck00, s071 1838:02a8) names party_min_attr(Agility)
 *            as $ChosenOneName. Within them the first such query returns the member (below,
 *            membervalue). Found by Arkana Mechanika (Darklands Restoration).
 *  cardslips Slips in single card scripts (below, cardslips), all found by Arkana Mechanika
 *            (Darklands Restoration): the Hussites' "claim friendship" (s114 18d3:041c) succeeds
 *            when rand(100) == the chance, not <= (1 in 100 whatever the chance shown); a potion
 *            thrown at footpads (s053 1838:0aa8) is never used up; the inn's stablemaster (s051
 *            18c9:05d0) asks a mounted party whether its mounts are for sale only when it has
 *            none (texts 1 and 7 swapped); a village Schulz (s077 1996:0378) says "Please accept
 *            this florin" to a full party and gives none; the Templars' writing desk (s118
 *            1a2b:02ac) looks for unknown formulae with && for &, so it skips every potion of
 *            which one formula is known, and teaches a known one when all are; the sabbat's
 *            ointment offer between waves ($Witcd40 block 4, s128 1838:03b8) has both its lines
 *            switched off (the block's options were cleared and never set); the tatzelwurm's
 *            meal (s106 19bc:03e8) takes the count of members who fled (s029 0e76:2474) for the
 *            member eaten, so the death text and the Virtue practice came whoever fled; the
 *            Cavern of Fire and Hail crossed under armour (s130 190c:033e) and the spider
 *            warehouse (s134 1838:0550, 0692) add the Endurance and Strength their texts take; the
 *            warehouse's owner ($DPost00, s134 1838:0000) is $NamedOneName, which nothing sets
 *            (whoever was named last, or nothing: "I am . Are ye hardy adventurers?"), and so is the
 *            dwarf king met again with his red hat ($MineB03, s101 18bd:0000; only $MineB00 and B02
 *            name him: name_to_slot (7, vec_219d (), 0)); a robber
 *            knight's bounty ($Raubi01, s080 1870:000c) tells by a coin toss whether the Hanse's
 *            guide leaves (text 6) or there was none (7), and says the village's Schulz "departs
 *            from your party" (text 1) whether or not he came along; neither is let go (he stayed
 *            until his days were up). Now the texts follow who came, and he goes home. Also (below,
 *            "more"): $Selec00's second gate fight, the docks' passage hint.
 *  campguard The risk of a night at camp (s036 1462:1b80) is lowered by each member on guard: by
 *            Woodwise, by Stealth, and by one if any weapon skill is 30 or more; the test lists
 *            party +0x57, 5c, 59, 5a, 5b, 5d and leaves out Impact Weapons (+0x58). Now all seven.
 *            Found by Arkana Mechanika (Darklands Restoration).
 *  minerelic The relic deep in a mine of dwarfs, kobolds or vulcans gets its own quality and kind
 *            (the item list's), not quality 35 and item type 0x9f for any relic (below, minerelic).
 *  ownplace  A place stepped on is the place whose own tile it is, when the game took another one
 *            near it (below, ownplace).
 *  dwarfrelic The dwarf at a mine's iron door ($MineH00 text 7) tells of a relic its guards hold and
 *            hands the leader a relic there and then; it comes in the loot of the next fight won there
 *            (below, dwarfrelic).
 *  minechests A mine's floor shows a chest (piece 5 closed, 6 open, of a cell's first byte's high nibble)
 *            only where a chest is (below, minechests).
 *  carditems Things a card gives by kind (add_items, add_item, add_item_q) take their type and weight from
 *            the item list as it is, not the game's copy a battle or dungeon lays other data over
 *            (below, carditems).
 *  cityalert, dayjobs, cloudline, attrsum, lootcount, corridors, battledead, createback: below, each
 *            at its hook (Arkana Mechanika's B-55afd6, B-074e99, B-3cc78a and B-bfb04e, B-82a302,
 *            B-e080d4, B-c90740, B-f7799c, B-126118).
 *  enemygroups A battle's loader (groups_fixup, s004 0e76:1d90) drops each level's empty groups
 *            (no kind of enemy or none of it) by moving the later ones down, but tests the moved
 *            group by the empty one's count (its table is not moved): every group after the
 *            first empty one was dropped too, enemies the battle was built with. Now only the
 *            empty ones go. Found by Arkana Mechanika (Darklands Restoration).
 *  nightwall Along a city's walls at night ($CityW01, s047 1a44): the arrival looks for the best
 *            climber from member 1 on (never the first) and keeps his number (DS:5828) but not his
 *            value; the rope's chance (1a44:03e2) is DS:582A, which only a failed climb of
 *            everyone sets (the last member's value): 0 the first time, so the climber always fell.
 *            Everyone's chance (05f0) compares each member with DS:582A instead of the lowest so
 *            far: 999 (certain) the first time. Now, before each menu, the best climber of all and
 *            his value; everyone's chance is the weakest's. Found by Arkana Mechanika (Darklands
 *            Restoration).
 *  westedge  overland_open (s030 1462:00a8) puts the party's marker 8 tiles into the view; where the
 *            place left lies nearer the map's west edge (x < 8), the view stops at column 0 and the
 *            marker's x should be 0x82 + (x - 8) * 16, but it is 0x82 - (x - 8): a pixel per tile
 *            instead of 16, so the party came out at column 8, 2 to 5 tiles east of the place
 *            ("I exited Helmond and it put me to the right of it"). Nine places: Nancy, Elberg,
 *            Valkenberg, Tongeren, Gemert, Helmond, Limberg, Spa, Briey. The marker is set right
 *            before the place icons are made (1462:1c76, called only there). And the view
 *            (1462:20d2) draws column 0 as column 1 ("if (col == 0) col = 1") while the marker's
 *            tile is counted from the view's column (party_x = view + col - 1): with the view at 0,
 *            from this placement or from a scroll west (1462:0c44 clamps at 0), the party was drawn
 *            a tile east of where the game had it. The view's first column is made 1 and the marker
 *            moved back by 16; the row the same (overland_open lets it be 0, the scroll stops at 1).
 *
 * Known, and left as the game has them (no fix: they are the game's design, or never reached):
 *  - An alchemist offers formula (seed + 100 + 3k) % 22 (k 0..3): only formulae 0..21 are ever
 *    sold, the first seven potions and Thunderbolt's first grade (ui/libraries.cpp reads it so;
 *    the cause found by Arkana Mechanika).
 *  - An event "for ever" (9999 hours) ends on 31 December 1499 (s029 event_new and kin): on 1
 *    January 1500 they all end together and the quests' facts they kept are cleared. A game a
 *    hundred years long (Arkana Mechanika).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../runtime/cpu.h"
#include "../runtime/hooks.h"
#include "../runtime/game.h"
#include "../runtime/gamevars.h"
#include "../runtime/config.h"
#include "../runtime/hle.h"
#include "fixes.h"
#include "nscreen.h"
#include "gamemodel.h"
#include "saves.h"
#include "teachers.h"
#include "sainttexts.h"

Fix FIXES[] = {
    { "blessing", "祝福不再消耗良好属性点", "在原作中，对同一属性的第二次祝福，或接近99的祝福，在效果消失时会扣除相应的点数。耐力或力量的提升在耗尽后会被全额扣除，即便经过一夜休息或伤口已将其消耗殆尽（重伤的队员也可能因此丧命）。而施加于整个队伍的降低效果（如“火与冰之洞穴”）则会在第一个被提升到该数值以上的队员处停止。如今，祝福只会收回其实际增加的部分以及仍高于上限的数值，而降低效果则会作用于每位队员，并在结束时予以恢复。", 1 },
    { "fireward", "无防火护盾", "在原版中，圣人波利卡普的防火护甲以及类似的防护效果也计入护甲值（+16至+64），且不会随时间消退。现在它仅能抵御火焰伤害。", 1 },
    { "nullitem", "无“（空）”或空项", "在原版中，物品名称为“（空）”的非真实物品以及空的堆叠可能会出现在队伍的物品栏中，这些物品既不能使用也不能出售。现在已不再添加此类物品。", 1 },
    { "castle", "城堡战斗在骑士死亡时结束", "在城堡战斗中，只有骑士死亡才会结束战斗，但原版并未保存这一设定：在战斗过程中存档再读取后，即使骑士已死，离开仍会被视为逃跑。现在，读取存档后，他的死亡也会被计入。", 1 },
    { "bounty", "商人悬赏的强盗骑士只支付一次，且不再有混乱的名字", "原版查找了错误的任务发布者，导致名字混乱、内存损坏，并使奖励被反复领取。现在它能找到正确的那一个。", 1 },
    { "alchemy", "炼制会消耗所有原料", "原版在某种原料被分散到多个堆叠时只会取其中一部分；现在则会全部取走。", 1 },
    { "fullpack", "再也不会有物品因队伍已满而丢失", "一名成员最多可携带 64 种物品；在原版中，游戏把物品交给已有 64 种的成员时，物品就会直接消失（任务物品也不例外）。现在它会交给已经携带该物品的成员，否则交给携带种类最少的成员（如果哪里都没有空间，仍然会丢失）。在没人有空间时购买会白花钱，从储物室取出物品给没有空间的成员会丢失：这两种情况现均被拒绝。", 1 },
    { "innsave", "在旅店之后仍可存档与载入", "在某些情况下（例如点击空着的队伍栏位），离开旅店的住宿界面会导致存档与载入功能被禁用，直到打开角色面板为止。现在，离开后该功能已重新启用。", 1 },
    { "colors", "成员的颜色得以保留（“霓虹发色”Bug）", "在原版中，成员在遭遇战斗后，其颜色与精灵图案有时会变成刺眼的样式。现在，他们的颜色会被保留并恢复原状。", 1 },
    { "copyprot", "跳过手动检查", "当队伍首次出发时，游戏不再要求玩家从印刷手册中查找炼金术符号。", 1 },
    { "potions", "武器药水会逐渐失效，且不会损坏武器", "致命之刃、强力边缘和强大力量从未失效，其加成还会不断叠加，超出游戏所能承受的上限：使用几次药水（或一次强大力量）后，武器尤其是弓箭的攻击力几乎为零。现在它们约一天后（19 到 21 小时）失效，且同时使用多种技能时，每项加成最多叠加到 15。", 1 },
    { "mapfoot", "旅行地图底部不再跳动", "当旅行地图的视野到达陆地的最后几行（巴塞尔、苏黎世、康斯坦茨）时，队伍标记在视野滚动时会多移动五分之一，导致队伍向北跳两到四行：可能落在村庄旁边并走入其中。而且在视野再次滚动之前，队伍脚下的土地是从偏移几行的位置读取的：实际不存在的水域可能会阻止旅行。现在队伍留在原处，站在实际存在的土地上。", 1 },
    { "offmap", "土地边界之外不再出现任务目的地", "巴塞尔东南方的一处洞穴位于土地底部边缘之外，队伍无法到达，因此派往那里的任务永远无法完成。现已将该洞穴移至土地范围内，队伍可以抵达。", 1 },
    { "bestweapon", "武器训练会提升队长最擅长的武器技能", "原本卡片总是训练导弹装置，而现在，当卡片允许队长练习其最擅长的武器时，会训练队长最擅长的武器技能。", 1 },
    { "itemcount", "抢劫者只会拿走现有物品（不会再崩溃）", "当抢劫者抢走队伍物品时，游戏曾多扣了一件物品；结果导致某位成员身无分文，最终竟出现负数物品，若其随后战死，游戏还会崩溃。现在，只会拿走实际存在的物品；带有此问题的存档会自动修复。", 1 },
    { "store room", "储物室中不会丢失任何物品", "储物室可存放100种物品，但当某种物品数量超过127时（显示为负数，随即丢失），系统会拒绝继续放入，并提示原因。", 1 },
    { "testcard", "开发者测试卡不会导致崩溃", "卡片0是开发者的测试卡（“试试看……你会喜欢的！”）。原本返回原卡的卡片，在没有来源卡时会进入卡片0，而游戏因测试卡缺少图片（pics\\\\xhouse3.pic）而停止运行。现在，它已切换到陆地视图，或前往队伍所在城镇。", 1 },
    { "armorwear", "只有穿戴的盔甲才会磨损", "同款盔甲被视为同一堆，因此当正在穿戴的那件被酸或火腐蚀时，该成员所拥有的同款、同质的备用盔甲也会一同失去品质，并随之销毁。现在，只有 worn 的那件会受损。", 1 },
    { "teachers", "在旅店训练时能找到城镇的教师", "当另一座城镇也同时拥有教师时，旅店的“训练”或“学习”选项会显示无人，费用甚至高达-4096芬尼（钱包还因此膨胀），结果一天下来什么也没学到。现在，旅店会列出这座城镇的教师，并收取相应费用。", 1 },
    { "cardopts", "卡片选项不再因误操作而变灰", "翻越城墙时，贿赂可以不带现金提出（因为检查是否有钱的是通过下水道爬出的选项）；在反叛人群的堡垒前，队伍带着绳索攀爬时，相关选项会变灰；守卫只统计最后一名队员的绳索后，再攀墙时，每个后续选项都会按下方选项应有的方式执行（游戏中从未处理过的三个“放弃马匹”选项使它们步调不一致）。布商街白天除了“离开”外别无他物：其他行会街道都有的店面橱窗都被关闭，因此布商只能在夜间营业，且需付出声誉代价；如今这些店铺已恢复白日营业。", 1 },
    { "patronsaint", "大教堂、圣科斯玛或圣达米安所教授的圣人知识已被习得", "向大教堂询问其守护圣人本意是让队长了解该圣人，而圣科斯玛与圣达米安彼此传授对方的知识；但游戏并未将其传授给任何成员：无人真正学到圣人的知识，第五名成员反而被标记为知道配方。现在，圣人知识已被习得。", 1 },
    { "debtnote", "债券由商人注明", "商人派队伍去索取债券时，会标明持有者（美第奇家族或富格尔家族），但游戏却把它们藏在市场上方的另一间办公室里，因此闯入指定办公室也一无所获。现在，这些债券已被放置在指定办公室内。", 1 },
    { "alchrisk", "风险更高的炼金配方更容易爆炸，而非更不易", "每种配方都标为低、中或高风险。在旅店、村庄或藏身处失败的批次中，有一半会以事故告终，而风险等级决定是哪一种：爆炸（队伍受伤、需赔偿损失、四个月无法入住旅店）或只是一阵恶臭（一个月）。游戏把两者弄反了：低风险下四次事故有三次爆炸，高风险下两次中有一次。现在低风险下四分之一爆炸，中风险下三分之一，高风险下一半。", 1 },
    { "bishop", "来自大教堂的任务属于主教，而非商人", "大教堂的教规曾像为城镇商人发布任务一样：商人付费（且在任务进行期间不再自行发布新任务），教规每次来访都会推出新任务，而主教的遗物则可在任务完成后反复领取。现在，任务计为主教所有，其遗物作为奖励，仅发放一次。", 1 },
    { "altar", "不再向被净化的异教祭坛派遣任务", "队伍摧毁的异教祭坛已从陆地视图中消失，但任务仍可能将其指派到那里，而此类任务往往无法完成（其发起者也无法再提供新的任务）。这类祭坛不再被选择；已派出的任务则会转而前往最近仍在矗立的祭坛。", 1 },
    { "tomb", "从墓穴中取走的物品归胜利者所有", "任务从墓穴中寻找的物品会唤醒死者：赢得战斗虽未获得任何东西（尽管文字称队伍携物撤离），但输掉战斗则会得到该物品。现在，队伍获胜时即可获得该物品。", 1 },
    { "lordreward", "领主为其任务买单", "在没有议会的城镇，要塞领主会派出强盗骑士狩猎，但游戏仅在议会大厅支付此类任务的报酬，因此在这样的城镇根本无法领取。现在，领主会在接见时付款，如同议会一般。", 1 },
    { "armorday", "来自药水和圣人的盔甲与防火保护持续一天；硬甲按说明书生效", "硬甲（+1 到 +3 盔甲）、圣邓斯坦的（+1）和圣帕特里克的（+2）祝福、圣克里斯平的（每人盔甲品质 +10）以及火墙和圣波利卡普的防火保护从未失效，而且每次新的都会叠加到上一次之上。现在它们持续约一天，如说明书所述，而存档中以此方式堆叠的部分会被丢弃：只有仍在生效的才算。圣慈善、圣伊拉斯谟和圣潘塔莱翁（以及圣波利卡普）把防火保护当作盔甲给予：永久 +32 或 +48 盔甲（若开启「无防火护盾」修正则为零）；现在该成员会获得 90 的防火保护，持续一天，与圣波利卡普的相同。硬甲也按说明书生效：品质提高 20、25 或 30 点，厚度增加一级或两级（游戏此前给 1 到 3 级且不提高品质）；在已有硬甲生效时再次使用，会保留两者中更强的一个。", 1 },
    { "greatpower", "大威力可提升武器的品质", "根据说明书，大威力可使武器品质提升14、22或30点（最高可达99），而品质决定了攻击的伤害力。原版则将这些点数直接加到了成员的伤害与穿透力加成上：+14穿透力，或+1伤害与+6穿透力，或+1伤害与+14穿透力。如今它仅能提升手中武器的品质（包括弓箭与枪械），效果持续约一天。", 1 },
    { "standings", "存档会保留队伍在富格尔、美第奇与汉萨同盟中的声望", "各公司的卡片会提升或降低队伍在他们那里的声望（若队伍或城镇声望低于零，则无法接取任务），但存档并不能保留这一状态：加载后，声望将恢复到前一次游戏结束时的数值，或重启后归零。如今存档已能保存这一状态（原版仍可加载此类存档）。", 1 },
    { "battlesaints", "战斗中召唤的圣人援助会失效", "原版中，战斗中召唤的圣人通常会提升属性与技能（一般情况下），而部分圣人索取的代价（降低某项属性）却从未返还：战斗结束后便不再追踪哪些效果该消失。如今两者都已按正常机制失效，如同战斗外召唤的援助一样。", 1 },
    { "healboost", "治愈不再消耗增益", "治愈（恩典之 essence 或圣人治愈）能使受伤的属性部分恢复至最大值，但游戏也曾让超出最大值的属性部分回落至其上限：当某位成员的力量或耐力超过上限（如铁臂、新风、圣人加持）时，用于治愈的药剂往往会夺走大部分增益。如今治愈仅能提升属性，绝不会降低。", 1 },
    { "teachcap", "教师只能教授与其自身能力相符的内容", "游戏文本规定教师不得教授超出自身能力的技能（这句台词游戏从未显示），而医师则会拒绝那些知识超过他的人；然而，任何水平的教师都能将技能学到99级。如今课程止于教师的能力范围：达到上限的成员将改为休息。", 1 },
    { "oilweapons", "每种武器油仅对说明书点名的武器生效", "致命之刃对除打击武器和连枷外的每种武器生效，强力边缘对刀刃和长柄武器生效，强大力量对打击武器、连枷和手枪生效，如说明书和药水自身的标签所说。游戏把每种油都给了成员当时用来打击的任何武器，包括弓箭和拳头。", 1 },
    { "pauls", "两位圣保罗均按各自文本行事", "圣保罗使徒给予的援助与圣保罗简单者相同（全部力量与耐力恢复，智力损失一半且需维持一天，要求美德70），而圣保罗简单者则复制了圣托马斯·阿奎那的帮助。使徒自身的帮助（通用语、拉丁语、读写、魅力、耐力，少量力量恢复；美德19，按说明书所述）虽存在于游戏代码中却从未启用。如今两人各按其文本行事。", 1 },
    { "saintrep", "圣内波穆克和圣塞西莉亚的声望加成会作用于真实的城镇", "在布拉格召唤时，圣约翰·内波穆克提升的是某个无关之地（或根本不存在的地点）的声望，而非布拉格的；在远离任何城镇的地方（在陆地视图中）召唤圣塞西莉亚时，她提升的是根本不存在的地点的声望。现在内波穆克的加成作用于布拉格，塞西莉亚的加成作用于最近的城镇，与其他圣人一样。", 1 },
    { "sainttexts", "圣人按其文本行事", "对于43位圣人，游戏提供的帮助要么不足，要么偏离了圣人本身的描述：圣克里斯多福、圣德罗戈与圣威尔弗里德从未加快过队伍的行程（如今一周内旅行更快：路上天数减少），圣尤斯塔修也未穿越树林；圣尼古拉斯与圣西奥多尔提供了连枷而非骑行；圣查理蒂、圣埃拉苏斯与圣潘塔莱昂未给队伍提供防火保护；圣卢特加迪斯的狂喜反而增加了耐力而非消耗；圣乔治、圣米迦勒、圣隆吉尼乌斯、圣莫里斯与圣埃利吉乌斯从未改善过武器或盔甲；圣克里斯平改良了金属盔甲，圣邓斯坦则未做任何改进；声望流向错误的土地或毫无声望……如今每位圣人都按其文本行事（新的角色表与祈祷界面详细描述了每位圣人）。盔甲与防火圣人需要“药剂与圣人提供的盔甲与防火保护可持续一天”的修复。", 1 },
    { "nightbowyer", "弓匠夜间也出售弓箭", "夜间醒来时，弓匠开设了一家专门销售铁匠武器与盔甲的店铺，却不卖弓箭：夜卡将铁匠的商品交给了这家店。如今这里出售的正是弓匠白天所售之物。", 1 },
    { "confession", "告解至少需一小时", "队伍在城镇的声望越高，教堂内的告解时间就越短：声望达110时无需等待，120以上则会倒转钟表。如今告解至少需一小时。", 1 },
    { "appointment", "明日的听众预约会被遵守", "议会书记员或领主管家若“帮你安排明日的听众时间”，实际上并未真正落实预约，第二天队伍只得像其他人一样再次申请。如今听众已确定在次日到场。", 1 },
    { "pileup", "同一战士同时遭受多次打击，所有伤害均计入统计", "战士所受的伤势会累计直至处理完毕，总数最多不超过127。四次重击同时命中（成员被群殴）超过了这一上限并导致负数：战士反而恢复了耐力与力量，而非继续跌落。如今总和上限定为127。", 1 },
    { "castlefreeze", "将队伍安置于城堡时不会冻结", "为将队伍安置于城堡的某些楼层，游戏会随机抽取房间，直到出现非门厅的房间；若门厅是唯一被抽中的房间，游戏便会无限循环并冻结。如今在这种情况下，系统会优先选择第一个房间。", 1 },
    { "university", "大学搜索配方失败后，关闭的是搜索本身，而非圣人", "向大学咨询炼金术配方却失败，反而使关于圣人的提问被视为已提出：一周内禁止再问圣人相关问题，而配方问题则可立即重新询问。如今系统会标记配方问题。", 1 },
    { "partyitems", "分发给全队的物品只发放一次", "当一张卡片为每位成员分发某样东西（来自地牢或小偷），已有此物的成员却收到了两次：既加入了他的原有储备，又作为新储备再次领取。如今每人仅领取一次。", 1 },
    { "itemflags", "骑马逃亡会计算最差的骑手坐骑", "骑马逃离野猎或狼群时，会把最差骑手的坐骑品质加到几率上，但游戏在错误的位置寻找坐骑，因此什么也没加上。现在它能找到坐骑了。", 1 },
    { "membervalue", "教堂什一税会祝福最擅长宗教的成员", "当队伍在城镇教堂缴纳什一税时，本应首先归于最擅长宗教的成员的神恩却落到了没有任何成员身上（游戏把技能的数值当成了成员的编号）；而夜间潜入公会会长之家时，显示的名字混乱不清，而非最不敏捷的成员。现在两者都会使用正确的成员。", 1 },
    { "cardslips", "单张卡片上的小失误", "与胡斯派营地结交的成功率恒为百分之一，无论显示的几率是多少。投向脚夫的药水从未被用掉。旅店的马厩管理员只在队伍没有坐骑时才询问是否出售队伍的坐骑。村庄的舒尔茨说「请收下这枚弗罗林」却什么也没给。圣殿骑士团的写字台教给读者他已经知道的配方。女巫的药膏无法用来在浪潮之间逃出安息日（它的两个选项都被关闭了）。塔佐蠕虫的死亡文本即使没人被吃掉也会出现。穿着盔甲穿越火与冰雹的洞窟、以及清理蜘蛛仓库，都会治疗队伍，尽管它们的文本说那是伤害；而仓库的主人没有自己的名字（「我是 。你们是勇敢的冒险者吗？」），再次遇到戴着红帽的矮人王时他也没有名字（「又是 来了，矮人王」）。当队伍因一名强盗骑士、汉萨的向导或村里的舒尔茨而获得报酬时，会说同行的那人（甚至他根本没来过）以掷硬币的方式离开，然后他又留了下来。与城镇守卫的第二次战斗不消耗声望（卡片保留了它到达时找到的声望），码头对付不起船费的队伍说「你需要一件远程武器」，夜间在前往城镇树林的路上遇到麻烦时，即使天已亮也会进入树林的夜卡，而潜入公会驻地会让整个队伍按每位成员各练习一次。现在每张卡片都按它的文本行事。", 1 },
    { "nightwall", "夜间攀爬城墙使用攀爬者自己的几率", "在夜间沿城墙攀爬时，用绳索的最佳攀爬者的几率是上一位攀爬者失败后留下的数字（第一次为 0：他总会摔下），而所有人不用绳索攀爬的几率则是将每位成员与那个数字比较，而非取最弱者（第一次必然如此）。第一名成员从未被选为攀爬者。现在攀爬者是所有人中最好的，他的几率是他自己的，而所有人攀爬的几率取最弱的成员。", 1 },
    { "campguard", "擅长使用冲击武器的守卫让营地更安全", "营地值守的每位成员都能降低遭遇袭击的风险，若其武器熟练度达到30或以上，则每提高一点；但游戏忘记统计冲击类武器（棍棒、狼牙棒和锤子）。现在这些武器也计入了。", 1 },
    { "enemygroups", "战斗中不会出现敌人群体消失的情况", "设置战斗时，游戏会剔除那些最终空无一人的敌人群体，但有个错误导致第一个空群之后的所有群体也被误删：有些战斗中的敌人数量少于预设。现在只有空群才会被剔除。", 1 },
    { "cityalert", "处于警戒状态的城镇会记得之前的麻烦", "与城门守卫发生冲突后（以及起义后的数周内），城镇进入警戒状态，在此期间，-10或更糟的数值本应使城镇变得敌对（包括城门守卫、码头和市政厅），而非仅需达到-75或更糟。原测试方式正好相反，因此警戒从未生效。现在终于生效了。", 1 },
    { "dayjobs", "方济各会修士的日常工作被提供", "成员在旅店能找到的两种日常工作——传道修士和乞讨修士——本应在特定类型的城镇开放（但没有这样的城镇）；测试失误导致它们在各地都被关闭。现在符合条件的成员可以被提供这些工作。", 1 },
    { "cloudline", "云雾只阻挡穿过它们的射击", "判断射击是否穿过云雾（黑云、烟）的方式有误：射击会被不在路线上的云阻挡，或穿过挡路的云。现在战场上的每朵云都会对照射击自身的路线进行检测。", 1 },
    { "attrsum", "测试两项属性的卡牌会同时计入两项", "三张卡牌寻找在两项属性之和上表现最佳的成员（一处地下城、一处城镇、一处乡村）：游戏曾两次只计算第一项属性，依据的是最后比较的那位成员。现在每位成员的两项属性相加，以总和最高者为准。", 1 },
    { "lootcount", "不再丢失超过255件同类战利品", "战后收集的战利品堆叠时，相同物品最多只能计数255件；超出部分将被丢弃（例如箭矢或弩箭过多的战斗）。现在无法容纳的部分会被移至另一行。", 1 },
    { "corridors", "城堡走廊的装饰留在其通道上", "布置城堡时，游戏会沿着每条通道的墙壁选一个位置来装饰，但它是从通道的远端而非起点开始计数的：除一种可能的掷骰外，所有结果都把位置放到了通道尽头之外，落在另一个房间里。现在该位置落在通道上。", 1 },
    { "battledead", "战死的雇佣兵不会在事后派来替代者", "雇佣兵战死后，其合同终止及“已离队”通知并未取消：日后仍可能派遣接替者加入队伍。现在这些通知在战斗结束后即被取消。", 1 },
    { "createback", "创建角色时按Esc键不会重新打开已选择的童年", "在创建角色时，按Esc键返回童年列表，即便该童年已被占用（角色已满15岁）：此时再从中选择一行，就会以同一编号开启成年职业。现在按Esc键可直接进入职业，就像按Esc键进入职业属性一样。", 1 },
    { "ruinkeep", "烧毁的城堡要塞让队伍在搜查后继续前行", "搜查强盗骑士烧毁的废墟时，结束语写着“当你重新踏上旅程……”，但卡片却回到了第一页，仿佛队伍刚刚抵达。现在队伍正如文字所述，已重返旅途。", 1 },
    { "rkonce", "施予者只需向强盗骑士请求一次", "城镇领主或议会（以及任何施予者）总会指定离城镇最近的强盗骑士；一周后再问，又给了队伍同一位骑士第二次任务，且有独立截止日期，尽管队伍已有第一次任务。现在已向队伍请求过该骑士的施予者，只需重复请求，无需再增加新任务。", 1 },
    { "bestteacher", "同一技能的两位教师中，更优秀的那一位负责授课", "当队伍在城镇雇用了两位教授同一技能的老师（比如大学和炼金术士），旅店总是优先选用先被雇用的一位，无论其收费和授课效果如何，即使另一位明显更优秀。现在学习该技能的成员将跟随最优秀的那位（能力最高；两人价格相当时选较便宜者）。", 1 },
    { "minerelic", "矿井中发现的遗物与遗物本身同样珍贵", "在矮人、地精和火山族的矿井里，队伍能在深处找到的遗物品质始终为35，无论具体为何种遗物（遗物品质范围为50至99），而且它甚至并非自身种类：圣阿努尔夫的大剑或圣奥拉夫的战斧都无法作为真正的武器使用。现在遗物的价值等同于物品清单所列。", 1 },
    { "ownplace", "紧邻其他地点的村庄也可以进入", "踏上村庄时，游戏会寻找周围四格内的地点，并取列表中的第一个，而非最近的：如果矿井、城堡或其他地点恰好在那么近的地方，村庄就永远无法进入。走上施派尔以西的劳夫时，会进入东边四格处的拉梅尔斯堡矿井。现在队伍会进入它踏上的那一格所在的地点。", 1 },
    { "hitsound", "普通击打会产生声音", "在原版中，未使目标后退的击打完全无声：只有未命中、格挡、后退、射击和死亡才会被听见，因此许多击打都悄然掠过。现在普通击打也会发出声响（游戏自带的后退声，或自定义音效包中的击打声）。", 1 },
    { "rkcastle", "强盗骑士在城堡中讨命", "城镇议会、领主或商人，或某个村庄，会要求队伍杀死离他们最近的城堡中的强盗骑士。一旦任何地方都不再有强盗骑士的城堡（每座要塞都被烧毁：队伍杀死骑士的速度快过新人搬入废墟，每三个月一个），游戏就会点名城里的一个骑士，而他永远找不到，还可能再次被点名。现在，新的骑士会先搬入离该城镇最近的烧毁要塞，正如游戏自身的规则所安排的那样，任务也会在那里点名他；已发给某个城镇或村庄的任务也会指向那座要塞。", 1 },
    { "dragonhide", "躲避巨龙需要潜行与木系知识", "当巨龙飞过（或从地里爬出）时，躲藏并跟随它到其巢穴的几率来自队长的魅力和拉丁语，这一组合在游戏中没有其他检查使用，代码中也没有说明原因：很可能是笔误。既然游戏没有说明，此处猜测其本意：队伍的平均潜行（所有人都必须保持隐蔽）加上最佳的木系知识（用于跟随那头野兽），取其和的一半。", 1 },
    { "knightdead", "被杀的强盗骑士会一直死去", "每种杀死强盗骑士的方式都会烧毁他的要塞并让队伍继续赶路，但在队伍离开该地之前，他塔楼的卡片可能再次出现，骑士在其中活着：他可以一次又一次地被杀死，每次都获得一笔悬赏（GitHub 问题 7）。现在，一旦他的要塞被烧毁，队伍在那里找到的就是那座烧毁的要塞。", 1 },
    { "alertonce", "戒备中的城堡是一条记录，而非上百条", "在强盗骑士的塔楼处，挑战他、潜近和其他尝试每次都会留下城堡处于戒备状态的记录，持续 270 天，每次都在上一条旁边新增一条。足够多的尝试填满了游戏 300 条记录的表格，之后便无法再记录任何新内容：每次回合都出现「EVENT Failure」（GitHub 问题 8）。现在新记录会替换上一条，存档中多出的副本会在加载时被丢弃。", 1 },
    { "kingdoor", "矮人王在他自己的门后等待", "在戴着红帽的矮人王的矿井中，通往国王的铁门在谜语被解开后会带着他的帽子把他带「回来」；但 12 条谜语中有 2 条反而通向了他让队伍离开的那张卡片，而矿井的另一扇门总是会通向那里。在那里擦身而过时门打开了，但国王永远消失，因此他的宝藏再也无法献还给他（GitHub 问题 9）。现在那扇门的每条谜语都会带来戴着红帽的国王。", 1 },
    { "dwarfrelic", "矮人的圣物是从其守卫手中赢来的", "在矿井中过一扇铁门时，一个咧嘴笑的矮人说门后的敌人守着一件他们偷走的圣物，矮人无法触碰，并请队伍夺回它；然而游戏当时就把圣物放进了队长的手里。现在圣物会出现在队伍在该矿井中赢得的下一场战斗的战利品里（它会显示在那场战斗的战利品中），矮人只有在圣物到手后才说它已被取回。", 1 },
    { "minechests", "矿井中的箱子是队伍可以打开的", "矿井楼层上画出的一些箱子只是图画：那里并没有箱子，点击或站在旁边也不会有任何反应。它们不再被画出；留在矿井楼层上的每个箱子都可以被打开（或撬开，或溶解其锁）。", 1 },
    { "carditems", "卡片在战斗后给予的物品就是它所说的那些", "矿井被攻占后，矿工会给队伍十袋矿物（十种材料，每种 10 到 14 个），但没有一袋进入队伍的清单：战斗后或地城中的卡片会从游戏的物品列表副本中生成物品，而战斗自身的数据随后会覆盖该副本，于是物品变成了错误的种类。现在卡片给予的每件物品都从物品列表本身获取其种类和重量。", 1 },
    { "prayfor", "每次祈祷都会询问是为谁而做", "从角色面板发起的祈祷会询问是为谁而做，但卡片要求的祈祷（在遭遇战或城镇中召唤圣人）不会：游戏把它当作全队的祈祷，而圣人给予个人的东西（增益、治疗）会落到队长身上，尽管祈祷并消耗神恩的是另一个人。用 P 祈祷而不作选择时也是如此。现在每次祈祷都会询问是为谁而做，即第一个祈祷的人，而未作选择的祈祷会帮助祈祷者本人。", 1 },
    { "westedge", "从地图西缘离开地点会把队伍置于其旁边", "从旅行地图西缘附近的九个地点（南锡、埃尔贝、法尔肯贝根、通厄伦、赫默特、海尔蒙德、林贝、斯帕和布里耶）离开时，队伍会被放在其东边 2 到 5 格处；而且无论旅行地图的视野到达该边缘的何处，队伍都会被画在比游戏所记录位置往东一格的地方。现在队伍会像在其他地方一样出现在该地点旁边，并被画在它实际所在的位置。", 1 },
};
int FIX_COUNT = (int)(sizeof FIXES / sizeof FIXES[0]);

/* ---------------------------------------------------------------- prayfor
 * saint_screen(who, saint, party) asks who a prayer is for (pray_choose) by the saint's flags (its
 * handler, what 6): bit 0 for a prayer from the sheet, bit 1 for one a card makes for the party
 * (saint_pray_party), which no saint but one has. Unasked, the target (pray_target) stays -1, and the
 * game's routines give -1's help to the leader. Here a card's prayer is asked as the sheet's is (party
 * passed as 0 to the game's own screen; mods/prayview.c asks the same of ours), and -1 left at
 * saint_finish, which every prayer passes before its help, becomes the one who prays. */
static void prayfor_screen_hook(void)          /* saint_screen (who, saint, party), s034 1462:022c */
{
    u16 at = (u16)(SP + 8);                     /* (taken before the call: its return moves SP) */
    int party = (s16)rw(SS, at);
    if (party) ww(SS, at, 0);
    hook_call_original();
    if (unw_active) return;
    if (party) ww(SS, at, (u16)party);
}
static void prayfor_hook(void)                 /* saint_finish (who, saint), s034 1462:06e2 */
{
    int who = (s16)rw(SS, (u16)(SP + 4));
    if ((s16)g_rw(GV_pray_target) == -1 && who >= 0 && who < 5) {
        g_ww(GV_pray_target, (u16)who);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix prayfor: the help for member %d, who prays\n", who);
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- blessing */
#define ev_ptr g_event

static void blessing_hook(void)            /* raise_attr_temp(who, k, d, h) */
{
    if ((s16)rw(SS, (u16)(SP + 4)) == -2) {  /* (everyone: the game's loop ended at the first member already boosted past
                                                d, and carried a cut d on to the next; Arkana Mechanika's B-8b2837) */
        int k = (s16)rw(SS, (u16)(SP + 6)), d = (s16)rw(SS, (u16)(SP + 8)), h = (s16)rw(SS, (u16)(SP + 10));
        for (int m = 0; m < 5; m++) if (g_rb(G_MEMBER(m))) { ns_call(GF_raise_attr_temp, 4, m, k, d, h); if (ns_unwinding) return; }
        hook_return_far(0);
        return;
    }
    static u8 had[300];
    u8 before[5][7];
    for (int i = 0; i < 300; i++) had[i] = ev_ptr(i) != 0;
    for (int m = 0; m < 5; m++) for (int a = 0; a < 7; a++) before[m][a] = g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + a));
    hook_call_original();
    for (int i = 0; i < 300; i++) {
        u32 e = ev_ptr(i);
        if (!e || had[i]) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) != 0x48) continue;
        int m = (s16)rw(s, (u16)(o + 0x1a)), a = (s16)rw(s, o);
        if (m < 0 || m >= 5 || a < 0 || a >= 7) continue;
        int added = (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + a)) - (s8)before[m][a];
        int rec = (s16)rw(s, (u16)(o + 0x1e));
        if (rec >= 0) { if (added < 0) added = 0; if (added > rec) added = rec; }     /* a boost: what it really added */
        else { if (added > 0) added = 0; if (added < rec) added = rec; }              /* a saint's lowering (some lower Perception, Intelligence ...): what it really took */
        if (added != rec) {
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix blessing: member %d attr %d recorded %d, really added %d\n", m, a, rec, added);
            ww(s, (u16)(o + 0x1e), (u16)added);
        }
    }
}

/* a boost runs out (s029 0e76:448a, the expiry of kinds 0x48 / 0x49 (event i): +0x1a the member,
   +0 the attribute, +0x1e the points): it takes off no more than what is still above what the
   attribute should be without it (the maximum and the other boosts and lowerings still running);
   a lowering that runs out gives back no more than up to that. (A lowering pulls the attribute
   below the maximum: a boost that ends while it runs takes its whole amount, as the lowering
   gives it back later.) */
static void blessing_end_hook(void)
{
    int i = (s16)rw(SS, (u16)(SP + 4));
    u32 e = i >= 0 && i < 300 ? ev_ptr(i) : 0;
    if (!e) { hook_call_original(); return; }
    u16 s = (u16)(e >> 16), o = (u16)e;
    int m = (s16)rw(s, (u16)(o + 0x1a)), a = (s16)rw(s, o), rec = (s16)rw(s, (u16)(o + 0x1e));
    if ((s16)rw(s, (u16)(o + 0x22)) != 0x48 || m < 0 || m >= 5 || a < 0 || a >= 7 || rec == 0) { hook_call_original(); return; }
    int others = 0;                                          /* the member's other boosts and lowerings of this attribute */
    for (int j = 0; j < 300; j++) {
        u32 f = j != i ? ev_ptr(j) : 0;
        if (!f) continue;
        u16 fs = (u16)(f >> 16), fo = (u16)f;
        if ((s16)rw(fs, (u16)(fo + 0x22)) == 0x48 && (s16)rw(fs, (u16)(fo + 0x1a)) == m && (s16)rw(fs, fo) == a) others += (s16)rw(fs, (u16)(fo + 0x1e));
    }
    int cur = (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + a)), max = (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_MAX + a)), base = max + others;
    int now;                                                 /* what the expiry takes off (a lowering: negative, given back) */
    if (rec > 0) { now = cur - base; if (now < 0) now = 0; if (now > rec) now = rec; }
    else { now = cur - base; if (now > 0) now = 0; if (now < rec) now = rec; }
    if (now == rec) { hook_call_original(); return; }
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix blessing: member %d attr %d runs out: %d off, not %d (%d/%d, others %d)\n", m, a, now, rec, cur, max, others);
    ww(s, (u16)(o + 0x1e), (u16)now);
    hook_call_original();
    if (ev_ptr(i) == e && (s16)rw(s, (u16)(o + 0x1e)) == now) ww(s, (u16)(o + 0x1e), (u16)rec);
}

/* ---------------------------------------------------------------- fireward (+ potions) */
static int fix_on(const char *id);
static void hitdamage_hook(void)           /* hit_damage(a, t) */
{
    int a = (s16)rw(SS, (u16)(SP + 4)), t = (s16)rw(SS, (u16)(SP + 6));
    u16 ta = (u16)(0x9c55 + 0x80 * t + 0x55), aa = (u16)(0x9c55 + 0x80 * a + 0x55), ba = (u16)(0x9c55 + 0x80 * a + 0x48);
    int fire = fix_on("fireward");
    int fix_t = fire && t >= 0 && t < 5, fix_a = fire && a >= 0 && a < 5;
    u8 st = fix_t ? g_rb(ta) : 0, sa = fix_a ? g_rb(aa) : 0, sb = 0;
    int fix_b = 0;
    if (fix_t) g_wb(ta, (u8)(st & 0x8f));
    if (fix_a && a != t) g_wb(aa, (u8)(sa & 0x7f));
    if (fix_on("potions") && a >= 0 && a < 5) {   /* penetration = (bonus + weapon) & 15: stop at 15 */
        sb = g_rb(ba);
        int wp = g_rb((u16)(GV_combatants + 0x80 * a + 0x53)) & 15;
        if ((sb & 15) + wp > 15) {
            g_wb(ba, (u8)((sb & 0xf0) | (15 - wp))); fix_b = 1;
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix potions: %d hits %d, penetration %d + %d held at 15\n", a, t, sb & 15, wp);
        }
    }
    hook_call_original();
    if (fix_b) g_wb(ba, sb);
    if (fix_a && a != t) g_wb(aa, sa);
    if (fix_t) g_wb(ta, st);
}

/* ---------------------------------------------------------------- nullitem */
static int fix_on(const char *id) { for (int i = 0; i < FIX_COUNT; i++) if (!strcmp(FIXES[i].id, id)) return FIXES[i].on; return 0; }

static int stacks(int m, u16 code, u8 type, u8 qual, int cnt)
{
    int n = g_rb((u16)(G_MEMBER(m) + G_M_NITEMS));
    for (int j = 0; j < n && j < 64; j++) {
        u16 o = G_ITEM(m, j);
        if (rw(G_FAR2E38, o) == code && rb(G_FAR2E38, (u16)(o + 2)) == type && rb(G_FAR2E38, (u16)(o + 3)) == qual && rb(G_FAR2E38, (u16)(o + 4)) + cnt <= 0xff) return 1;
    }
    return 0;
}

static void additem_hook(void)             /* add_item_rec_impl(who, struct item {code, type, qual, cnt, weight}) */
{
    int who = (s16)rw(SS, (u16)(SP + 4));
    u16 code = rw(SS, (u16)(SP + 6)); u8 type = rb(SS, (u16)(SP + 8)), qual = rb(SS, (u16)(SP + 9));
    int cnt = rb(SS, (u16)(SP + 10));
    if (fix_on("nullitem") && ((code & 0xfff) >= 200 || cnt == 0)) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix nullitem: refused item %d x%d for %d\n", code & 0xfff, cnt, who);
        int m = who == -1 ? (s8)g_rb(G_LEADER) : who;
        AX = (m >= 0 && m < 5) ? g_rb((u16)(G_MEMBER(m) + G_M_NITEMS)) : 0;
        hook_return_far(0);
        return;
    }
    if (fix_on("fullpack") && who != -2) {
        int m = who == -1 ? (s8)g_rb(G_LEADER) : who, size = (s16)g_rw(G_PARTY_SIZE);
        if (m >= 0 && m < 5 && g_rb((u16)(G_MEMBER(m) + G_M_NITEMS)) >= 64 && !stacks(m, code, type, qual, cnt)) {
            int best = -1;
            for (int k = 0; k < size && k < 5; k++) {
                if (k == m || g_rb(G_MEMBER(k)) == 0) continue;
                if (stacks(k, code, type, qual, cnt)) { best = k; break; }
                if (g_rb((u16)(G_MEMBER(k) + G_M_NITEMS)) < 64 && (best < 0 || g_rb((u16)(G_MEMBER(k) + G_M_NITEMS)) < g_rb((u16)(G_MEMBER(best) + G_M_NITEMS)))) best = k;
            }
            if (best >= 0) {
                if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix fullpack: item %d for %d goes to %d\n", code & 0xfff, m, best);
                ww(SS, (u16)(SP + 4), (u16)best);
            } else fprintf(stderr, "fullpack: no room anywhere for item %d\n", code & 0xfff);
        }
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- castle */
static void castle_hook(void)              /* combat_over */
{
    if ((g_rb(GV_combat_flags) & 1) && g_rb(0x9084) != 1)
        for (int i = 5; i < 13; i++)
            if (g_rb((u16)(0xc1f3 + i)) == 1 && (g_rb((u16)(GV_cbt_flags + 2 * i)) & 0x80)) {
                if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix castle: boss %d is dead\n", i);
                g_wb(0x9084, 1);
                break;
            }
    hook_call_original();
}

/* ---------------------------------------------------------------- bounty */
static void bounty_hook(void)              /* $Raubi01 card, s080 1870:000c */
{
    int giver = (s16)g_rw(GV_saved_card2), loc = (s16)g_rw(G_CUR_LOC), match = 0, only = -1, n = 0;
    for (int i = 0; i < 300; i++) {
        u32 e = ev_ptr(i);
        if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) != 0x24 || (s16)rw(s, (u16)(o + 0x28)) != 3 || (s16)rw(s, (u16)(o + 0x1c)) != loc) continue;
        int owner = (s16)rw(s, (u16)(o + 0x1a));
        if (owner == giver) match = 1;
        only = owner; n++;
    }
    if (!match && n >= 1) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix bounty: giver %d -> %d\n", giver, only);
        g_ww(GV_saved_card2, (u16)only);
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- alchemy */
static int stock(int id)
{
    int n = 0, size = (s16)g_rw(G_PARTY_SIZE);
    for (int m = 0; m < 5 && m < size; m++) {
        int k = g_rb((u16)(G_MEMBER(m) + G_M_NITEMS));
        for (int j = 0; j < k && j < 64; j++)
            if ((rw(G_FAR2E38, G_ITEM(m, j)) & 0xfff) == id) n += rb(G_FAR2E38, (u16)(G_ITEM(m, j) + 4));
    }
    return n;
}

static void alchemy_hook(void)             /* s033 1462:1630 (first, id, n): take n of item id */
{
    int first = (s16)rw(SS, (u16)(SP + 4)), id = (s16)rw(SS, (u16)(SP + 6)), n = (s16)rw(SS, (u16)(SP + 8));
    int before = stock(id);
    hook_call_original();
    int left = n - (before - stock(id));
    void (*f)(void) = game_func(33, 0x1462, 0x1630);
    if (left > 0 && f) {
        hook_remove(33, 0x1462, 0x1630, alchemy_hook);
        for (int tries = 0; left > 0 && tries < 64; tries++) {
            int s0 = stock(id);
            if (!s0) break;
            u16 args[3] = { (u16)first, (u16)id, (u16)left };
            game_call_native(f, 3, args);
            int took = s0 - stock(id);
            if (took <= 0) break;
            left -= took;
        }
        hook_install(33, 0x1462, 0x1630, alchemy_hook);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix alchemy: item %d: %d needed, %d still missing\n", id, n, left);
    }
}

/* ---------------------------------------------------------------- colours
 * member_palette (far 20a5:33fc, 24 bytes per party slot) and the figure codes (DS:a668, 4
 * per slot) only change at character creation (s038), when a game is loaded (s037, or before
 * the game proper) or when slots move (a member leaves). Anything else is the "neon hair"
 * corruption: the slot gets its last good colours back. */
#include "../runtime/dispatch.h"
int ui_is_open(void);
#define G_FAR20A5 ((u16)(LOADSEG + 0x20a5))
static u8 col_good[5][28]; static int col_have;
static void col_read(u8 cur[5][28])
{
    for (int m = 0; m < 5; m++) {
        for (int i = 0; i < 24; i++) cur[m][i] = rb(G_FAR20A5, (u16)(0x33fc + m * 24 + i));
        for (int i = 0; i < 4; i++) cur[m][24 + i] = g_rb((u16)(GV_member_shield + m * 4 + i));
    }
}
static int col_grace;
static void colours_after_load(void) { col_grace = 300; }   /* a game loaded (saves_on_loaded) */
static void colours_frame(void)
{
    if (!fix_on("colors")) { col_have = 0; return; }
    u8 cur[5][28]; col_read(cur);
    u32 plat_ticks_ms(void);
    /* a game was loaded (saves_on_loaded: quick load, autosave, the save browser, Alt-L, Quickstart): its
       colours come in over the next moments; counted in frames as well as time (a first start of the MT-32
       or a slow disk held the frames for longer than the time: the stale colours went back) */
    static int prev_mode = -1;                                   /* (the game proper just began: Quickstart, a new game) */
    if (prev_mode == GM_OTHER && game_mode() != GM_OTHER && col_grace < 300) col_grace = 300;
    prev_mode = game_mode();
    if (col_grace > 0) col_grace--;
    int loading = col_grace > 0 || (saves_loaded_ms && plat_ticks_ms() - saves_loaded_ms < 4000);
    if (!col_have || loading || game_mode() == GM_OTHER || ovl_loaded[37] || ovl_loaded[38] || ui_is_open()) { memcpy(col_good, cur, sizeof cur); col_have = 1; return; }
    if (!memcmp(cur, col_good, sizeof cur)) return;
    int size = (s16)g_rw(G_PARTY_SIZE), moved = 1;
    for (int m = 0; m < size && m < 5; m++) {           /* slots that moved are fine */
        int found = 0;
        for (int k = 0; k < 5 && !found; k++) found = !memcmp(cur[m], col_good[k], 28);
        if (!found) moved = 0;
    }
    if (moved) { memcpy(col_good, cur, sizeof cur); return; }
    for (int m = 0; m < size && m < 5; m++) {
        if (!memcmp(cur[m], col_good[m], 28)) continue;
        if (col_good[m][24] < 'A' || col_good[m][24] > 'Z') continue;    /* the slot was empty: a new member */
        { int z = 1; for (int i = 0; i < 24; i++) if (col_good[m][i]) z = 0; if (z) continue; }   /* (no colours kept yet: nothing to put back) */
        fprintf(stderr, "fix colors: member %d's colors/figure changed outside creation and loading (mode %d): put back\n", m, game_mode());
        if (getenv("DL_FIX_LOG")) {
            fprintf(stderr, "  was:"); for (int i = 0; i < 28; i++) fprintf(stderr, " %02x", col_good[m][i]);
            fprintf(stderr, "\n  now:"); for (int i = 0; i < 28; i++) fprintf(stderr, " %02x", cur[m][i]);
            fprintf(stderr, "\n  loaded:"); for (int i = 0; i < 140; i++) if (ovl_loaded[i]) fprintf(stderr, " %d", i);
            fprintf(stderr, "\n");
        }
        for (int i = 0; i < 24; i++) wb(G_FAR20A5, (u16)(0x33fc + m * 24 + i), col_good[m][i]);
        for (int i = 0; i < 4; i++) g_wb((u16)(GV_member_shield + m * 4 + i), col_good[m][24 + i]);
    }
    col_read(cur); memcpy(col_good, cur, sizeof cur);
}
static void potions_frame(void);
/* the last row the party can walk to: a step moves the party's marker no lower than 0xb1 on the screen
   (s030 1462:0c44), and with the view at the bottom of the map that is row 929 - the map's own last
   row, 930, is never stepped on, so a place there is never entered (a place is entered by stepping
   onto its icon, 1462:1300). Towns are let be: their entrance reaches four rows round them */
#define OFFMAP_LAST_ROW 929
static void offmap_move(void)                  /* places below or right of the land onto its edge */
{
    for (int k = 0x5c; k < G_NLOC; k++) {       /* (0..0x5b: the towns) */
        u32 r = g_location(k);
        if (!r) continue;
        u16 s = (u16)(r >> 16), o = (u16)r;
        int x = (s16)rw(s, (u16)(o + 4)), y = (s16)rw(s, (u16)(o + 6));
        if (y > OFFMAP_LAST_ROW && y < 940) { ww(s, (u16)(o + 6), OFFMAP_LAST_ROW); if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix offmap: location %d y %d -> %d\n", k, y, OFFMAP_LAST_ROW); }
        if (x > 326 && x < 336) { ww(s, (u16)(o + 4), 326); if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix offmap: location %d x %d -> 326\n", k, x); }
    }
}
static void offmap_frame(void)                 /* (the world map and the quests read the places too: kept moved) */
{
    static u32 n;
    if ((n++ & 63) || game_mode() == GM_OTHER) return;
    offmap_move();
}
static void offmap_hook(void)                  /* s030 1462:1c76 (f): the overland map loaded, the list of its places' icons made from the
                                                  places (k 92..413) - where a step looks for them: moved first, so they are right from the start */
{
    offmap_move();
    hook_call_original();
}
/* ---------------------------------------------------------------- westedge */
static void westedge_hook(void)                /* s030 1462:1c76, right after overland_open placed the view and the marker */
{
    int col = (s16)g_rw(GV_map_view_col), mx = (s16)g_rw(GV_map_marker_x);
    if (col == 0 && mx > 0x82 && mx <= 0x8a) {   /* (only the west clamp gives these: 0x82 - (x - 8), x 0..7) */
        int x = 0x8a - mx;
        g_ww(GV_map_marker_x, (u16)(x * 16 + 2));
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix westedge: marker x %d -> %d (column %d)\n", mx, x * 16 + 2, x);
    }
    hook_call_original();
}
static void westedge_view_hook(void)           /* s030 1462:20d2 (col, row): the view drawn - as from column 1 and row 1 when they are 0
                                                  ("if (col == 0) col = 1"), while the marker's tile is still counted from 0: the
                                                  view's first column and row are made 1, the marker moved back by one, the same tile */
{
    if ((s16)g_rw(GV_map_view_col) == 0) {
        int mx = (s16)g_rw(GV_map_marker_x) - 16;
        g_ww(GV_map_view_col, 1);
        g_ww(GV_map_marker_x, (u16)(mx < 11 ? 11 : mx));      /* (11: as far left as a step takes it; column 0 is never drawn) */
    }
    if ((s16)g_rw(GV_map_view_row) == 0) {
        g_ww(GV_map_view_row, 1);
        g_ww(GV_map_marker_y, (u16)((s16)g_rw(GV_map_marker_y) - 4));
    }
    hook_call_original();
}
/* ---------------------------------------------------------------- itemcount */
static int clamp_items(int i)              /* combatant i's item count into 0..64 (enemies 0..16) */
{
    u16 a = CBF(i, CB_NITEMS);  /* = party member i + 0x6a */
    int n = (s8)g_rb(a), max = i < 5 ? 64 : 16;
    if (n >= 0 && n <= max) return 0;
    fprintf(stderr, "fix itemcount: %s %d had %d items: %d\n", i < 5 ? "member" : "combatant", i, n, n < 0 ? 0 : max);
    g_wb(a, (u8)(n < 0 ? 0 : max));
    return 1;
}
/* the encounters' robberies (mods/encounters.c loseitems, keepitems): their own counts kept in bounds,
   whether this fix is on or not - the game's loss routines leave -1 behind, and the crash it brings is
   no part of the original a script of ours should bring back */
void fixes_clamp_party_items(void)
{
    int size = (s16)g_rw(G_PARTY_SIZE);
    for (int m = 0; m < size && m < 5; m++) clamp_items(m);
}
static void itemcount_frame(void)
{
    int size = (s16)g_rw(G_PARTY_SIZE);
    if (size < 1 || size > 5) return;
    for (int m = 0; m < size; m++) clamp_items(m);
}
static void falls_hook(void)               /* s014 0e76:2324 (i, ...): a combatant falls; his items become a pile */
{
    int i = (s16)rw(SS, (u16)(SP + 2));      /* near call */
    if (i >= 0 && i < 13) clamp_items(i);
    hook_call_original();
}
static int idx_missing(int who, int idx)   /* who (0..12) has no item idx */
{
    if (who < 0 || who > 12) return 0;
    int n = (s8)g_rb(CBF(who, CB_NITEMS));
    if (idx >= 0 && idx < n) return 0;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix itemcount: %d has no item %d (%d items): nothing removed\n", who, idx, n);
    return 1;
}
static void removeitem_hook(void)          /* remove_item_rec(who, idx), s031 18e7:0406 */
{
    if (idx_missing((s16)rw(SS, (u16)(SP + 4)), (s16)rw(SS, (u16)(SP + 6)))) { hook_return_far(0); return; }
    hook_call_original();
}
static void useitem_hook(void)             /* use_item_idx(who, idx): one of a stack used up, s014 18bf:0fa4 */
{
    if (idx_missing((s16)rw(SS, (u16)(SP + 4)), (s16)rw(SS, (u16)(SP + 6)))) { AX = 0xffff; hook_return_far(0); return; }
    hook_call_original();
}

static void altar_frame(void);
static void armourday_frame(void);
static void greatpower_frame(void);
static void rkcastle_frame(void);
void fixes_frame(void)
{
    colours_frame();
    if (fix_on("itemcount")) itemcount_frame();
    if (fix_on("potions")) potions_frame();
    if (fix_on("offmap")) offmap_frame();
    if (fix_on("altar")) altar_frame();
    if (fix_on("armorday")) armourday_frame();
    if (fix_on("greatpower") || sainttexts_on) greatpower_frame();
    if (fix_on("rkcastle")) rkcastle_frame();
}

/* ---------------------------------------------------------------- potions
 * The kind 0x4a events say which potions are still running: +0 0x69 for Deadly Blade, else the
 * potion's inventory index (the original passes idx for Strongedge and Greatpower), +0x1a the
 * member, +0x1e the amount (3..5 for the first two, 14/22/30 for Greatpower), +0x12 the end
 * (hour, day, month, year). The bonus byte is rebuilt from them every frame: it is nothing else
 * (only use_potion writes it), so saves spoilt by the old bug mend themselves.
 * A battle closes the cards (run_battle: cards_shutdown, s043 1462:054e, writes the events to
 * events.tmp and frees them; cards_startup, 1462:078e, reads them back after the battle and its
 * loot): in between the table holds only what the battle makes, a potion drunk there, and the
 * table read back over it loses it (but what lay past the slots read back). So the party's running potions are kept when the cards close and counted until
 * they open, and those drunk meanwhile are put back into the table when it is read back (as are
 * a saint's boosts called in a battle, with "battlesaints": the game never took them off). */
typedef struct { int m, type0, t; s16 end[4]; } PotEv;   /* end: hour, day, month, year (+0x12 ..) */
static PotEv pot_kept[64];
static int pot_nkept, cards_down;
static void battledead_after(void);

static int pot_running(const s16 *end)
{
    static const u16 now[4] = { G_CUR_HOUR, G_CUR_DAY, G_CUR_MONTH, G_CUR_YEAR };
    for (int k = 3; k >= 0; k--) { int n = (s16)g_rw(now[k]); if (end[k] != n) return end[k] > n; }
    return 0;
}
/* the party's running potions in the table now (kind 0x4a, a member, a sane amount) */
static int pot_table(PotEv *out, int max)
{
    int n = 0;
    for (int i = 0; i < 300 && n < max; i++) {
        u32 e = ev_ptr(i);
        if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        PotEv v = { (s16)rw(s, (u16)(o + 0x1a)), (s16)rw(s, o), (s16)rw(s, (u16)(o + 0x1e)), { 0 } };
        if ((s16)rw(s, (u16)(o + 0x22)) != 0x4a || v.m < 0 || v.m > 4 || v.t < 0 || v.t > 0x7f) continue;
        for (int k = 0; k < 4; k++) v.end[k] = (s16)rw(s, (u16)(o + 0x12 + 2 * k));
        if (pot_running(v.end)) out[n++] = v;
    }
    return n;
}
/* all the party's running potions: the table's (while the cards are closed: those drunk in the
   battle), and while the cards are closed those kept from before */
static int pot_all(PotEv *out, int max)
{
    int n = pot_table(out, max);
    if (cards_down)
        for (int k = 0; k < pot_nkept && n < max; k++) if (pot_running(pot_kept[k].end)) out[n++] = pot_kept[k];
    return n;
}
static int any_potion_fix(void) { return fix_on("potions") || fix_on("armorday") || fix_on("greatpower") || sainttexts_on; }   /* (their kind 0x4a events kept over a battle) */
static void cards_down_hook(void)          /* cards_shutdown(), s043 1462:054e */
{
    pot_nkept = any_potion_fix() ? pot_table(pot_kept, 64) : 0;
    hook_call_original();
    cards_down = 1;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix potions: the cards close, %d potions running kept\n", pot_nkept);
}
/* the cards open again after a battle: the table is read back into its first slots, so what the
   battle put in it is lost (but what lay past them) unless it is carried over - the members'
   potions (kind 0x4a: the potion fixes) and a saint's help called in it (0x48 attribute, 0x49
   skill, its price too: battlesaints), whole records */
static void cards_up_hook(void)            /* cards_startup(), s043 1462:078e */
{
    static u8 rec[48][0x30];
    int n = 0, pots = cards_down && any_potion_fix(), saints = cards_down && fix_on("battlesaints");
    for (int i = 0; i < 300 && n < 48 && (pots || saints); i++) {
        u32 e = ev_ptr(i);
        if (!e) continue;
        int kind = g_ev(e, G_EV_KIND), m = g_ev(e, G_EV_OWNER);
        if (m < 0 || m > 4 || !((kind == 0x4a && pots) || ((kind == 0x48 || kind == 0x49) && saints))) continue;
        for (int k = 0; k < 0x30; k++) rec[n][k] = rb((u16)(e >> 16), (u16)((u16)e + k));
        n++;
    }
    hook_call_original();
    if (!cards_down) return;
    u16 ax = AX, dx = DX;
    cards_down = 0; pot_nkept = 0;
    battledead_after();
    if (ns_unwinding) return;
    for (int r = 0; r < n; r++) {
        const u8 *b = rec[r];
        int kind = b[0x22] | b[0x23] << 8, m = b[0x1a] | b[0x1b] << 8, there = 0;
        for (int i = 0; i < 300 && !there; i++) {          /* (the table is read back into its first slots: */
            u32 e = ev_ptr(i);                              /*  what the battle put past them is still there) */
            if (!e) continue;
            int k = 0; while (k < 0x30 && rb((u16)(e >> 16), (u16)((u16)e + k)) == b[k]) k++;
            there = k == 0x30;
        }
        if (there) continue;
        int i = (s16)ns_call(GF_event_new, 10, m, (s16)g_rw(G_CUR_LOC), 0x63, kind, b[0] | b[1] << 8, b[0x1e] | b[0x1f] << 8, 6, 24, 1, 0);
        u32 e = ev_ptr(i);
        if (!e) continue;
        for (int k = 0; k < 0x30; k++) wb((u16)(e >> 16), (u16)((u16)e + k), b[k]);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix %s: member %d's kind %#x (%d, %d) from the battle kept (event %d)\n",
                                          kind == 0x4a ? "potions" : "battlesaints", m, kind, (s16)(b[0] | b[1] << 8), (s16)(b[0x1e] | b[0x1f] << 8), i);
    }
    AX = ax; DX = dx;
}

/* a running kind 0x4a event that is Greatpower's: marked 0x6b by greatpower, else (the original's)
   an inventory index with 14, 22 or 30 */
static int gp_event(int type0, int t) { return type0 == 0x6b || (type0 >= 0 && type0 < 64 && t > 5); }

void potion_bonus(int m, int *dmg, int *pen)
{
    *dmg = *pen = 0;
    PotEv v[96]; int n = pot_all(v, 96);
    for (int i = 0; i < n; i++) {
        int type0 = v[i].type0, t = v[i].t;
        if (v[i].m != m) continue;
        if (type0 != 0x69 && type0 != 0x6b && (type0 < 0 || type0 >= 64)) continue;   /* (armour, fire: armourday's) */
        if (gp_event(type0, t) && fix_on("greatpower")) continue;   /* (the weapon's quality: greatpower's) */
        if (type0 == 0x69) *dmg += t;                      /* Deadly Blade */
        else if (t <= 5) *pen += t;                         /* Strongedge */
        else { *dmg += t >> 4; *pen += t & 15; }            /* Greatpower, without the carry */
    }
    if (*dmg > 15) *dmg = 15;
    if (*pen > 15) *pen = 15;
}

static void potions_frame(void)
{
    if (game_mode() == GM_OTHER) return;
    int size = (s16)g_rw(G_PARTY_SIZE);
    if (size < 1 || size > 5) return;
    for (int m = 0; m < size; m++) {
        int d, p; potion_bonus(m, &d, &p);
        u8 v = (u8)(d << 4 | p), was = g_rb((u16)(G_MEMBER(m) + 0x34));
        if (v == was) continue;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix potions: member %d bonus %02x -> %02x\n", m, was, v);
        g_wb((u16)(G_MEMBER(m) + 0x34), v);
    }
}

/* ---------------------------------------------------------------- armourday, greatpower
 * armourday: Hardarmor adds 1/2/3 to the member's armour byte (party +0x41, low nibble; hit_damage
 * adds it to the armour of every hit location), St. Dunstan (s039 165c:2052) 1 and St. Patrick
 * (165c:5b82) 2, and Firewall sets the fire protection (party +0x10 = combatant +0x24: Arabian
 * Fire does a tenth of it less, the blows of a foe with bit 7 of +0x55 a twentieth), with no event: all of
 * it stays for good and the armour adds up (past 15 into the fire bits). The manual: a day.
 * Here each records a kind 0x4a event of about a day (type0 0x6d Hardarmor, the value its grade;
 * 0x16d a saint's armour, the value the levels; 0x74 Firewall), and the low nibble and the fire
 * protection are rebuilt from the running ones every frame (nothing else writes them), so armour
 * piled up in a save goes back too. Hardarmor follows the manual (hardarmor_give).
 * greatpower: Greatpower adds 14/22/30 to the damage-and-penetration byte (party +0x34, see
 * potions); the manual: "increases the overall quality of a weapon ... up to a limit of 99".
 * Here the byte is left alone, the event is marked 0x6b, and in battle the quality a member's
 * blows count (combatant +0x0d, set by weapon_ready, s014 18bf:0a40, from +0x58, or +0x5a for the
 * missile weapon: hit_damage's (quality - armour) / 10 is a step of damage each 10 points) is
 * the weapon's + the running Greatpowers', up to 99. */
static int ev_new_count(u8 *had)
{
    int n = 0;
    for (int i = 0; i < 300; i++) { had[i] = g_event(i) != 0; n += had[i]; }
    return n;
}
static void record_potion(int who, int type0, int t)
{
    ns_call(GF_event_new, 10, who, (s16)g_rw(G_CUR_LOC), 0x63, 0x4a, type0, t, 6, 19 + rand() % 3, 1, 0);
}
/* the sum (a saint's armour: 0x16d) or the largest (Hardarmor's grade: 0x6d, Firewall: 0x74,
   Greatpower: 0x6b) of member m's running
   kind 0x4a events of type0 k */
static int running_potions(int m, int k, int largest)
{
    PotEv p[96]; int n = pot_all(p, 96), v = 0;
    for (int i = 0; i < n; i++) {
        if (p[i].m != m || (k == 0x6b ? !gp_event(p[i].type0, p[i].t) : p[i].type0 != k)) continue;
        if (largest) { if (p[i].t > v) v = p[i].t; } else v += p[i].t;
    }
    return v;
}
int fixes_running(int m, int type0) { return running_potions(m, type0, 1); }
int fixes_cards_down(void) { return cards_down; }   /* (fixes.h) */   /* (fixes.h: also over a battle, when the table holds only the battle's) */

static void potion_fixes_hook(void)        /* use_potion(who, idx), s031 1462:43b2 */
{
    int who = (s16)rw(SS, (u16)(SP + 4)), idx = (s16)rw(SS, (u16)(SP + 6));
    int code = who >= 0 && who < 5 && idx >= 0 && idx < 64 ? rw(G_FAR2E38, G_ITEM(who, idx)) & 0xfff : -1;
    int arm = fix_on("armorday") && (code == 0x6d || code == 0x74), gp = fix_on("greatpower") && code == 0x6b;
    if (!arm && !gp) { hook_call_original(); return; }
    u16 mb = (u16)G_MEMBER(who);
    u8 a41 = g_rb((u16)(mb + 0x41)), b34 = g_rb((u16)(mb + 0x34)), had[300];
    ev_new_count(had);
    hook_call_original();
    u16 ax = AX, dx = DX;
    if (code == 0x6d) {
        g_wb((u16)(mb + 0x41), a41);                    /* (the frame puts what the running ones give) */
        int q = rb(G_FAR2E38, (u16)(G_ITEM(who, idx) + 3)), grade = q <= 25 ? 1 : q <= 35 ? 2 : 3;
        record_potion(who, 0x6d, grade);                /* (the grade: hardarmor_give) */
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d Hardarmor grade %d for a day\n", who, grade);
    } else if (code == 0x74) {
        int t = (s8)g_rb((u16)(mb + 0x10));
        g_wb((u16)(mb + 0x41), a41);                    /* (its +16 armour: fireward's, gone with it) */
        if (t > 0) record_potion(who, 0x74, t);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d Firewall %d for a day\n", who, t);
    } else {
        g_wb((u16)(mb + 0x34), b34);                    /* (the byte as it was) */
        for (int i = 0; i < 300; i++) {                 /* the event it made: marked Greatpower's */
            u32 e = ev_ptr(i);
            if (!e || had[i]) continue;
            u16 s = (u16)(e >> 16), o = (u16)e;
            if ((s16)rw(s, (u16)(o + 0x22)) == 0x4a && (s16)rw(s, (u16)(o + 0x1a)) == who) {
                ww(s, o, 0x6b);
                if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix greatpower: member %d weapon quality +%d for a day\n", who, (s16)rw(s, (u16)(o + 0x1e)));
            }
        }
    }
    AX = ax; DX = dx;
}

static void saint_armour(int add)          /* St. Dunstan's (1) or St. Patrick's (2) blessing, (who, x, what) */
{
    int who = (s16)rw(SS, (u16)(SP + 4)), what = (s16)rw(SS, (u16)(SP + 8));
    if (what != 8 || who < 0 || who > 4 || sainttexts_handles(add == 1 ? 39 : 104)) { hook_call_original(); return; }   /* (sainttexts: theirs) */
    u16 mb = (u16)G_MEMBER(who);
    u8 a41 = g_rb((u16)(mb + 0x41));
    hook_call_original();
    u16 ax = AX, dx = DX;                               /* (what it returns) */
    g_wb((u16)(mb + 0x41), a41);
    record_potion(who, 0x16d, add);                     /* (0x16d: a saint's armour, added to Hardarmor's) */
    AX = ax; DX = dx;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d blessed with +%d armor for a day\n", who, add);
}
static void dunstan_hook(void) { saint_armour(1); }
static void patrick_hook(void) { saint_armour(2); }

/* Hardarmor by the manual: "Quality improves by 20 to 30 points, and thickness by one or two levels"
   (the game: thickness 1, 2 or 3, no quality). Grade 1, 2, 3: thickness 1, 2, 2 and quality 20,
   25, 30. A second while one runs keeps the stronger; a saint's armour adds to it. */
/* a Hardarmor dose's armour by its grade (1 to 3, from the potion's quality: the manual), for the hit and for
   the margins' line (ui/effects.cpp) */
void fixes_hardarmor_grade(int g, int *thick, int *qual)
{
    static const int T[4] = { 0, 1, 2, 2 }, Q[4] = { 0, 20, 25, 30 };
    if (g < 0) g = 0; if (g > 3) g = 3;
    *thick = T[g]; *qual = Q[g];
}
static void hardarmor_give(int m, int *thick, int *qual)
{
    fixes_hardarmor_grade(running_potions(m, 0x6d, 1), thick, qual);
    *thick += running_potions(m, 0x16d, 0);
    if (*thick > 15) *thick = 15;
}
/* the quality: added to the target's armour (+0x4f body, +0x50 legs) for the hit alone - Hardarmor's
   everywhere, St. Crispin's on the armour worn (party +0x37 body, +0x38 legs: -1 none) */
/* sainttexts adds: St. Dunstan's quality to metal armour worn (0x16f: hardness 3 scale and up), St.
   Crispin's to the other (0x170), and St. Patrick's thickness against a missile (0x175: the
   attacker's +0x57 bit 7, the missile weapon in use; added to the armour byte +0x55's low bits) */
static int worn_quality(int t, int part)            /* the saints' and potions' quality for the armour worn on the body (0) or legs (1) */
{
    int type = (s8)g_rb((u16)(G_MEMBER(t) + 0x37 + part));
    if (type < 0) return 0;
    int hard = rb(G_FAR20A5, (u16)(0x781e + type));
    return running_potions(t, 0x16e, 0) + running_potions(t, hard >= 3 ? 0x16f : 0x170, 0);
}
static void armour_hit_hook(void)          /* hit_damage (a, t, r, loc) */
{
    int a = (s16)rw(SS, (u16)(SP + 4)), t = (s16)rw(SS, (u16)(SP + 6)), th, q = 0, q0 = 0, q1 = 0, miss = 0;
    if (t >= 0 && t < 5 && g_rb(G_MEMBER(t))) {
        hardarmor_give(t, &th, &q);
        q0 = q + worn_quality(t, 0); q1 = q + worn_quality(t, 1);
        if (a >= 0 && a < 13 && (g_rb((u16)(GV_combatants + 0x80 * a + 0x57)) & 0x80)) miss = running_potions(t, 0x175, 0);
    }
    if (!q0 && !q1 && !miss) { hook_call_original(); return; }
    u16 c = (u16)(GV_combatants + 0x80 * t + 0x4f), ab = (u16)(GV_combatants + 0x80 * t + 0x55);
    u8 a0 = g_rb(c), a1 = g_rb((u16)(c + 1)), b = g_rb(ab);
    g_wb(c, (u8)((s8)a0 + q0 > 99 ? 99 : (s8)a0 + q0)); g_wb((u16)(c + 1), (u8)((s8)a1 + q1 > 99 ? 99 : (s8)a1 + q1));
    if (miss) { int lo = (b & 0x7f) + miss; g_wb(ab, (u8)((b & 0x80) | (lo > 0x7f ? 0x7f : lo))); }
    hook_call_original();
    g_wb(c, a0); g_wb((u16)(c + 1), a1); g_wb(ab, b);
}

/* St. Crispin (s039 165c:1868): +10 to the quality of every member's worn body and leg armour
   (+0x3b, +0x3c), for good and piling up; and as the worn armour is known by its type and quality,
   the armour in use then no longer matched its record in the pack. Here the bytes stay, and each
   member has a kind 0x4a event of about a day (type0 0x16e, the value the points), counted in each
   hit on the armour worn (armour_hit_hook). */
static void crispin_hook(void)
{
    int what = (s16)rw(SS, (u16)(SP + 8));
    if (what != 8 || sainttexts_handles(28)) { hook_call_original(); return; }
    u8 q[5][2];
    for (int m = 0; m < 5; m++) { q[m][0] = g_rb((u16)(G_MEMBER(m) + 0x3b)); q[m][1] = g_rb((u16)(G_MEMBER(m) + 0x3c)); }
    hook_call_original();
    u16 ax = AX, dx = DX;
    for (int m = 0; m < 5; m++) {
        g_wb((u16)(G_MEMBER(m) + 0x3b), q[m][0]); g_wb((u16)(G_MEMBER(m) + 0x3c), q[m][1]);
        if (!g_rb(G_MEMBER(m))) continue;
        record_potion(m, 0x16e, 10);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d armor quality +10 (St. Crispin) for a day\n", m);
    }
    AX = ax; DX = dx;
}

/* the saints of fire: St. Charity (165c:125a), St. Erasmus (2628) and St. Pantaleon (5a84) set the
   high nibble of the member's armour byte (+0x41) to 0xa, St. Polycarp (6122) every member's to 0xb
   and their fire protection (+0x10) to 90 - all for good. Those bits are not fire protection: the
   battle counts bits 4-6 as armour (+32, +48: the "fireward" fix hides them) and bit 7 makes the
   member's own blows fire. Here the byte stays as it was and the member (Polycarp: each member)
   has fire protection 90 for about a day (a kind 0x4a event, type0 0x174), as Polycarp's. */
static void fire_saint(int whole, int saint)
{
    int who = (s16)rw(SS, (u16)(SP + 4)), what = (s16)rw(SS, (u16)(SP + 8));
    if (what != 8 || who < 0 || who > 4 || sainttexts_handles(saint)) { hook_call_original(); return; }
    u8 a41[5], f10[5];
    for (int m = 0; m < 5; m++) { a41[m] = g_rb((u16)(G_MEMBER(m) + 0x41)); f10[m] = g_rb((u16)(G_MEMBER(m) + 0x10)); }
    hook_call_original();
    u16 ax = AX, dx = DX;
    for (int m = 0; m < 5; m++) {
        g_wb((u16)(G_MEMBER(m) + 0x41), a41[m]); g_wb((u16)(G_MEMBER(m) + 0x10), f10[m]);
        if (!g_rb(G_MEMBER(m)) || (!whole && m != who)) continue;
        record_potion(m, 0x174, 90);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d fire protection 90 (a saint's) for a day\n", m);
    }
    AX = ax; DX = dx;
}
static void charity_hook(void) { fire_saint(0, 20); }
static void erasmus_hook(void) { fire_saint(0, 45); }
static void pantaleon_hook(void) { fire_saint(0, 103); }
static void polycarp_hook(void) { fire_saint(1, 110); }

static void armourday_frame(void)
{
    if (game_mode() == GM_OTHER) return;
    int size = (s16)g_rw(G_PARTY_SIZE);
    if (size < 1 || size > 5) return;
    for (int m = 0; m < size; m++) {
        if (!g_rb(G_MEMBER(m))) continue;
        u16 mb = (u16)G_MEMBER(m);
        int arm, q, fire = running_potions(m, 0x74, 1), sf = running_potions(m, 0x174, 1);
        if (sf > fire) fire = sf;                       /* (Firewall's or a saint's, the stronger) */
        hardarmor_give(m, &arm, &q);
        u8 was = g_rb((u16)(mb + 0x41)), v = (u8)((was & 0xf0) | arm);
        if (v != was) {
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d armor bonus %d -> %d\n", m, was & 15, arm);
            g_wb((u16)(mb + 0x41), v);
        }
        if ((s8)g_rb((u16)(mb + 0x10)) != fire) {
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorday: member %d fire protection %d -> %d\n", m, (s8)g_rb((u16)(mb + 0x10)), fire);
            g_wb((u16)(mb + 0x10), (u8)fire);
        }
    }
}

/* the saints' weapon quality (sainttexts) for weapon type w: any (0x171), an edged one (0x172),
   St. Eligius's blades, hammers and maces, polearms, crossbows and guns (0x173) */
static int saint_weapon_quality(int m, int w)
{
    if (w < 0 || w > 33) return 0;
    int sk = rb(G_FAR20A5, (u16)(0x75ac + w));
    return running_potions(m, 0x171, 0) + (sk == 0 ? running_potions(m, 0x172, 0) : 0) +
           (sk == 0 || sk == 1 || sk == 3 || sk == 6 ? running_potions(m, 0x173, 0) : 0);
}
static int gp_boost[5];                    /* what greatpower_frame added to each member's +0x0d */
static void greatpower_frame(void)         /* (Great Power's, with greatpower; the saints', with sainttexts) */
{
    if (game_mode() != GM_BATTLE) { memset(gp_boost, 0, sizeof gp_boost); return; }
    for (int m = 0; m < 5; m++) {
        u16 c = (u16)(GV_combatants + 0x80 * m);
        if (!g_rb(G_MEMBER(m)) || g_rb((u16)(c + 0x14)) != 1) continue;
        int base = (s8)g_rb((u16)(c + ((g_rb((u16)(c + 0x57)) & 0x80) ? 0x5a : 0x58)));
        int q = (s8)g_rb((u16)(c + 0x0d)), add = fix_on("greatpower") ? running_potions(m, 0x6b, 0) : 0;
        if (add && !oils_allow(0x6b, oils_weapon(m))) add = 0;     /* (oilweapons: not on this weapon) */
        if (sainttexts_on) add += saint_weapon_quality(m, oils_weapon(m));
        int want = add ? (base + add > 99 ? 99 : base + add) : base;
        if (q == want) { gp_boost[m] = want - base; continue; }
        if (!add && !gp_boost[m]) continue;             /* (not ours to touch) */
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix greatpower: member %d blows count quality %d (the weapon's %d)\n", m, want, base);
        g_wb((u16)(c + 0x0d), (u8)want);
        gp_boost[m] = want - base;
    }
}

/* ---------------------------------------------------------------- innsave */
static void innsave_hook(void)             /* the lodging screen, s036 1462:0082 */
{
    hook_call_original();
    if (g_rb(GV_font_flag_a) == 2 || g_rb(GV_font_flag_b) == 2) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix innsave: Save/Load enabled again\n");
        g_wb(GV_font_flag_a, 1); g_wb(GV_font_flag_b, 1);
    }
}

/* ---------------------------------------------------------------- copyprot */
static void copyprot_hook(void)            /* copy_protection(), s041 1462:0000: answered */
{
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix copyprot: skipped\n");
    hook_return_far(0);
}

/* ---------------------------------------------------------------- bestweapon */
static void bestweapon_hook(void)          /* member_best_skill(who), s029 0e76:01c0 */
{
    int who = (s16)rw(SS, (u16)(SP + 4));
    hook_call_original();
    if (who == -1) who = (s8)g_rb(G_LEADER);
    if (who < 0 || who > 4) return;
    int best = 0, bv = 0;                      /* the first of the highest weapon skills, as intended */
    for (int i = 0; i < 7; i++) { int v = g_rb((u16)(G_MEMBER(who) + G_M_SKILLS + i)); if (v > bv) { bv = v; best = i; } }
    if (getenv("DL_FIX_LOG") && g_rb(GV_best_skill) != best) fprintf(stderr, "fix bestweapon: member %d best skill %d (the game said %d)\n", who, best, g_rb(GV_best_skill));
    g_wb(GV_best_skill, (u8)best);                    /* best_skill */
}

/* ---------------------------------------------------------------- switching */
/* ---------------------------------------------------------------- storeroom
 * The store rooms (s035): the open store is a table at far 2e38:37f4, 4 bytes an entry {item,
 * quality, number (signed char)}, room for 100 (the next table starts at 3984); DS:89f6 the
 * entries, DS:89f2 the member shown, DS:37c4[5] the list in use (0 the member's), DS:37ce[5]
 * + DS:37e2[5] the member's cursor. cache_add stacks without a limit and appends without
 * one: P (put) is refused when it would overflow either. */
void ui_notice(const char *msg);
void ui_notice_for(const char *msg, unsigned ms);
/* why a P (put) with key `key` would overflow the open store, or 0 (also mods/sharedstore.c) */
const char *storeroom_put_refusal(int key)
{
    int m = (s16)g_rw(GV_cache_who);
    if ((key == 'p' || key == 'P') && m >= 0 && m < 5 && !g_rw((u16)(0x37c4 + 2 * m)) && g_rb((u16)(G_MEMBER(m) + G_M_NITEMS))) {
        int idx = (s16)g_rw((u16)(0x37e2 + 2 * m)) + (s16)g_rw((u16)(0x37ce + 2 * m));
        if (idx >= 0 && idx < 64) {
            u16 o = G_ITEM(m, idx);
            int id = rw(G_FAR2E38, o) & 0xfff, q = rb(G_FAR2E38, (u16)(o + 3));
            unsigned n = g_rw(GV_cache_n); int found = -1;
            for (unsigned i = 0; i < n && i < 100; i++) {
                u16 e = (u16)(0x37f4 + 4 * i);
                if ((rw(G_FAR2E38, e) & 0xfff) == id && rb(G_FAR2E38, (u16)(e + 2)) == q) { found = (int)i; break; }
            }
            return found >= 0 ? ((s8)rb(G_FAR2E38, (u16)(0x37f4 + 4 * found + 3)) >= 127 ? "储藏室最多存放127件同类物品" : 0)
                              : (n >= 100 ? "储藏室已满：存放了100种不同物品" : 0);
        }
    }
    return 0;
}
int storeroom_fix_on(void) { return fix_on("储物室"); }
static void storeroom_hook(void)           /* cache_key(key), s035 1462:0a46 */
{
    const char *why = storeroom_put_refusal(rw(SS, (u16)(SP + 4)) & 0xff);
    if (why) {
        ui_notice(why);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix store room: %s\n", why);
        hook_return_far(0);
        return;
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- testcard
 * Card 0 (the card table's first entry, s044 1838:0000, $Initi00) is the developers' test card:
 * it sets the purse to 200 florins and loads pics\xhouse3.pic, which the game does not have, so
 * the game stops (exit 99). A card that ends by going back where it came from (next_card =
 * prev_screen: the town gate's challenge, s044 18d1, after its fight) goes there when prev_screen
 * is 0. Instead: the town's own card if the party is in one (its location record +0x0c, as the
 * map enters it), else the map (card 12). */
static void testcard_hook(void)            /* card 0 (), s044 1838:0000 */
{
    int loc = (s16)g_rw(GV_cur_loc), to = 12;
    if (loc >= 0 && loc < 92) { u32 r = g_location(loc); if (r) { int c = (s16)rw((u16)(r >> 16), (u16)((u16)r + 0x0c)); if (c > 0) to = c; } }
    fprintf(stderr, "fix testcard: card 0 asked for (prev_screen %d, cur_loc %d, mode %d): card %d instead\n",
            (s16)g_rw(GV_prev_screen), loc, game_mode(), to);
    g_ww(GV_next_card, (u16)to);
    AX = (u16)to;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- teachers */
static int lodging_open;
void fixes_lodging(int open) { lodging_open = open; }      /* (the native lodging screen, which runs instead of the original) */
static void teachers_lodging_hook(void)    /* the lodging screen, s036 1462:0082 */
{
    lodging_open = 1;
    hook_call_original();
    lodging_open = 0;
}
static int ev_started(u32 e)
{
    u16 s = (u16)(e >> 16), o = (u16)e;
    int d[4][2] = { { (s16)rw(s, (u16)(o + 0x10)), (s16)g_rw(GV_cur_year) }, { (s16)rw(s, (u16)(o + 0x0e)), (s16)g_rw(GV_cur_month) },
                    { (s16)rw(s, (u16)(o + 0x0c)), (s16)g_rw(GV_cur_day) }, { (s16)rw(s, (u16)(o + 0x0a)), (s16)g_rw(GV_cur_hour) } };
    for (int k = 0; k < 3; k++) { if (d[k][0] > d[k][1]) return 0; if (d[k][0] < d[k][1]) return 1; }
    return d[3][0] <= d[3][1];
}
static void teachers_next_hook(void)       /* event_next(k, i), s029 0e76:324c */
{
    int k = (s16)rw(SS, (u16)(SP + 4)), i = (s16)rw(SS, (u16)(SP + 6));
    if (!lodging_open || k != 0x28 || i < 0) { hook_call_original(); return; }
    int loc = (s16)g_rw(GV_cur_loc), r = -1;
    for (; i < 300 && r < 0; i++) {
        u32 e = ev_ptr(i);
        if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + 0x22)) == 0x28 && (s16)rw(s, (u16)(o + 0x1c)) == loc && ev_started(e)) r = i;
    }
    if (r >= 0) g_ww(GV_last_event, (u16)r);
    AX = (u16)r;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- armourwear */
static int inv_find(int i, int type, int q)
{
    int n = (s8)g_rb(CBF(i, CB_NITEMS));
    for (int k = 0; k < n && k < 64; k++)
        if (rb(G_FAR2E38, (u16)(G_ITEM(i, k) + 2)) == type && rb(G_FAR2E38, (u16)(G_ITEM(i, k) + 3)) == q) return k;
    return -1;
}
static void armourwear_hook(void)          /* s019 1ef8:0968 (i, type, quality, amount): wear an armour down */
{
    int i = (s16)rw(SS, (u16)(SP + 4)), type = rw(SS, (u16)(SP + 6)) & 0xff, q = rw(SS, (u16)(SP + 8)) & 0xff, amt = (s16)rw(SS, (u16)(SP + 10));
    if (i < 0 || i >= 5) { hook_call_original(); return; }
    int k = inv_find(i, type, q), n = (s8)g_rb(CBF(i, CB_NITEMS));
    if (k < 0) { hook_call_original(); return; }
    u16 o = G_ITEM(i, k);
    int cnt = rb(G_FAR2E38, (u16)(o + 4));
    if (cnt <= 1) { hook_call_original(); return; }
    u8 nq = (u8)(q - amt);
    u16 id = rw(G_FAR2E38, o);
    int to = -1;
    for (int j = 0; j < n && j < 64; j++) {
        u16 p = G_ITEM(i, j);
        if (j != k && rw(G_FAR2E38, p) == id && rb(G_FAR2E38, (u16)(p + 2)) == type && rb(G_FAR2E38, (u16)(p + 3)) == nq && rb(G_FAR2E38, (u16)(p + 4)) < 255) { to = j; break; }
    }
    if (to >= 0) wb(G_FAR2E38, (u16)(G_ITEM(i, to) + 4), (u8)(rb(G_FAR2E38, (u16)(G_ITEM(i, to) + 4)) + 1));
    else if (n < 64) {
        u16 p = G_ITEM(i, n);
        for (int b = 0; b < 6; b++) wb(G_FAR2E38, (u16)(p + b), rb(G_FAR2E38, (u16)(o + b)));
        wb(G_FAR2E38, (u16)(p + 3), nq); wb(G_FAR2E38, (u16)(p + 4), 1);
        g_wb(CBF(i, CB_NITEMS), (u8)(n + 1));
    } else { hook_call_original(); return; }                 /* (a full pack: as the original) */
    wb(G_FAR2E38, (u16)(o + 4), (u8)(cnt - 1));
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorwear: member %d, one of %d (type %d, q%d) worn to q%d alone\n", i, cnt, type, q, (s8)nq);
    hook_return_far(0);
}
static void wornout_hook(void)             /* s019 1ef8:0914 (i): throw away worn-out armour */
{
    int i = (s16)rw(SS, (u16)(SP + 4));
    if (i < 0 || i >= 5) { hook_call_original(); return; }
    u16 cb = (u16)(GV_combatants + i * 0x80);
    for (int s = 0; s < 2; s++) {
        if ((s8)g_rb((u16)(cb + 0x4f + s)) > 0) continue;
        int k = inv_find(i, g_rb((u16)(cb + 0x4b + s)), g_rb((u16)(cb + 0x4f + s)));
        if (k >= 0) {                                       /* one of it (consume_item, s014 18bf:0db4) */
            u16 o = G_ITEM(i, k);
            int cnt = rb(G_FAR2E38, (u16)(o + 4)), n = (s8)g_rb(CBF(i, CB_NITEMS));
            if (cnt > 1) wb(G_FAR2E38, (u16)(o + 4), (u8)(cnt - 1));
            else {
                for (int j = k; j < n - 1; j++) for (int b = 0; b < 6; b++) wb(G_FAR2E38, (u16)(G_ITEM(i, j) + b), rb(G_FAR2E38, (u16)(G_ITEM(i, j + 1) + b)));
                g_wb(CBF(i, CB_NITEMS), (u8)(n - 1));
            }
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix armorwear: member %d's worn-out armor thrown away (%d left of it)\n", i, cnt - 1);
        }
        g_wb((u16)(cb + 0x4b + s), 0xff); g_wb((u16)(cb + 0x4d + s), 0); g_wb((u16)(cb + 0x4f + s), 0x19);
    }
    hook_return_far(0);
}

/* ---------------------------------------------------------------- cardopts */
int fix_is_on(const char *id) { for (int i = 0; i < FIX_COUNT; i++) if (!strcmp(FIXES[i].id, id)) return FIXES[i].on && !config_get_int("original", 0); return 0; }
static int gcall(int sect, u16 seg, u16 off, int n, u16 a0, u16 a1) { u16 a[2] = { a0, a1 }; return (s16)game_call_far(sect, seg, off, n, a); }
static void w04_options(void);
static void witches_offer(void), nightwall_options(void);
void fixes_card_opts(void)                  /* before a card's option list is shown: its opt_on[] put right */
{
    char nm[16]; g_str(G_DGROUP, GV_card_name, nm, sizeof nm);
    if (!strcasecmp(nm, "$Witcd40") && (s16)rw(SS, (u16)(SP + 4)) == 4 && fix_is_on("cardslips")) { witches_offer(); return; }
    if (!strcasecmp(nm, "$CityW01") && (s16)rw(SS, (u16)(SP + 4)) == 0 && fix_is_on("nightwall")) { nightwall_options(); return; }
    if (!fix_is_on("cardopts")) return;
    if (!strcasecmp(nm, "$CityW04") && (s16)rw(SS, (u16)(SP + 4)) == 0) { w04_options(); return; }     /* (its option list, block 0) */
    if (!strcasecmp(nm, "$Cloth00") && (s16)rw(SS, (u16)(SP + 4)) == 0 && !g_rb(GV_opt_on)) {           /* Clothmakers' Street by day (s053 1924:0004): */
        g_wb(GV_opt_on, 1);                 /* its script hides the shopfronts (opt_flag_0 = 0), which every other guild's street */
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardopts: $Cloth00 shopfronts open by day\n");   /* opens, and whose handler is there (02be) */
        return;
    }
    if (!strcasecmp(nm, "$CityC00") && (s16)rw(SS, (u16)(SP + 4)) == 0 && g_rb((u16)(GV_opt_on + 3)) == 2) {   /* a town church by day (s057 1838:0000): */
        long pf = (long)g_rw(G_MONEY) * 240 + (long)g_rw((u16)(G_MONEY + 2)) * 12 + g_rw((u16)(G_MONEY + 4));       /* the tenth is greyed when the purse's */
        if (pf >= 10) {                                                                                              /* pfennigs' low word is under 10 (cmp ax, 10: */
            g_wb((u16)(GV_opt_on + 3), 1);                                                                            /* Arkana Mechanika's B-42f89c) */
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardopts: $CityC00's tenth open (%ld pf)\n", pf);
        }
        return;
    }
    int c = !strcasecmp(nm, "$Selec01") ? 1 : !strcasecmp(nm, "$Revol14") ? 2 : !strcasecmp(nm, "$CityW02") ? 3 : 0;
    if (!c) return;
    int rope = gcall(GF_count_items, 2, (u16)-2, 0x3b) > 0;
    u8 o[8]; for (int i = 0; i < 8; i++) o[i] = g_rb((u16)(GV_opt_on + i));
    if (c == 1) {                           /* 1 abandon the horses and crawl out (greyed after a failed sewer, event 0x10), 2 the bribe */
        int loc = (s16)g_rw(G_CUR_LOC);
        int poor = gcall(GF_money_cmp, 2, G_MONEY, GV_var_money1) == -1;
        if (o[1] == 2 && !gcall(GF_event_at, 2, 0x10, (u16)loc)) g_wb((u16)(GV_opt_on + 1), 1);
        if (o[2] == 1 && poor) g_wb((u16)(GV_opt_on + 2), 2);
    } else if (c == 2) {                    /* 4 the climb with a rope (also greyed after waiting a day: prev_screen 0xbc) */
        if (o[4]) g_wb((u16)(GV_opt_on + 4), (rope && g_rw(GV_prev_screen) != 0xbc) ? 1 : 2);
    } else if (c == 3) {                    /* 0 the climber carries a rope for everyone */
        if (o[0]) g_wb((u16)(GV_opt_on + 0), rope ? 1 : 2);
    }
    if (getenv("DL_FIX_LOG")) { int ch = 0; for (int i = 0; i < 8; i++) ch |= o[i] != g_rb((u16)(GV_opt_on + i)); if (ch) fprintf(stderr, "fix cardopts: %s options put right\n", nm); }
}
/* $CityW04 (the wall by an old sally port, s073 1a7e): its script knows seven options (0 rope, 1
   climb, 2 sewer grate, 3 alchemy, 4 saint, 5 bribe, 6 sneak back) but its text has ten lines, with
   three "abandon the horses" variants the script never handles; the game numbers the lines in
   order, so from the second line on each line runs the next one's option (and lines 7-9 run
   nothing). Put right: the horse variants hidden, each line given its own option (shown, chance,
   choice), and the choice handed back to the script as its own number. */
static const int W04_LINE[10] = { 0, -1, 1, -1, 2, -1, 3, 4, 5, 6 };   /* line -> the script's option */
static u16 w04_fn[7][2]; static int w04_active;
static void w04_menu_hook(void)             /* card_menu (win, win2): the line chosen -> the script's option */
{
    hook_call_original();
    if (!w04_active) return;
    w04_active = 0;
    for (int i = 0; i < 7; i++) { g_ww((u16)(GV_opt_fn + 4 * i), w04_fn[i][0]); g_ww((u16)(GV_opt_fn + 4 * i + 2), w04_fn[i][1]); }
    int line = (s16)AX;
    if (line >= 0 && line < 10 && W04_LINE[line] >= 0) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardopts: $CityW04 line %d is option %d\n", line, W04_LINE[line]);
        AX = (u16)W04_LINE[line];
    }
}
static void w04_options(void)
{
    static int hooked;
    if (!hooked) { hook_install(GF_card_menu, w04_menu_hook); hooked = 1; }      /* (installed last: it runs first, round the card view's) */
    u8 on[7]; for (int i = 0; i < 7; i++) on[i] = g_rb((u16)(GV_opt_on + i));
    for (int i = 0; i < 7; i++) { w04_fn[i][0] = g_rw((u16)(GV_opt_fn + 4 * i)); w04_fn[i][1] = g_rw((u16)(GV_opt_fn + 4 * i + 2)); }
    for (int l = 0; l < 10; l++) {
        int o = W04_LINE[l];
        g_wb((u16)(GV_opt_on + l), o < 0 ? 0 : on[o]);
        if (o >= 0) { g_ww((u16)(GV_opt_fn + 4 * l), w04_fn[o][0]); g_ww((u16)(GV_opt_fn + 4 * l + 2), w04_fn[o][1]); }
    }
    w04_active = 1;
}

static void cardopts_hook(void)             /* card_options (block) */
{
    fixes_card_opts();
    hook_call_original();
}

/* $Cathe00's "ask about the patron saint" (s056 19c4:0952) calls set_member_bit (s029 0e76:1302,
   (who, bit): saints_known[who] |= ...) with who -1, which other routines take for the leader but
   this one does not */
static void patron_hook(void)
{
    if ((s16)rw(SS, (u16)(SP + 4)) == -1) {
        ww(SS, (u16)(SP + 4), (u16)g_rb(G_LEADER));
        fprintf(stderr, "fix patronsaint: saint %u taught to the leader\n", rw(SS, (u16)(SP + 6)));
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- debtnote
 * A merchant's errand for papers in a town (kind 8, tag 10; s039 1462:1712, type0 6, 9, 10, 11) is
 * done at night: the town's marketplace ($Marke01), the offices upstairs ($Insid00, s108 1925): the
 * leading merchant's, the Fugger's, the Medici's, the Hanse's (GV_insid_office 0, 2, 1, 3 as chosen). The
 * papers are found when the event's +0x2a % 4 is the office (s108 1925:0438). The giver ($McGuf12,
 * s128 18f7) names the holder by the same +0x2a % 4 (goods merchant, Medici, Fugger, Hanse), but for
 * debt notes (type0 10) by +0x2a % 2 (Medici, Fugger): the office named never held them. Here the
 * office named holds them for the moment of the search (and no other does); +0x2a is put back. */
static int papers_event(int loc, u16 *seg, u16 *off)
{
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i);
        if (e && g_ev(e, G_EV_KIND) == 8 && g_ev(e, G_EV_LOC) == loc && g_ev(e, G_EV_TAG) == 10) { *seg = (u16)(e >> 16); *off = (u16)e; return i; }
    }
    return -1;
}
int fixes_debt_note_office(int aux) { return (aux % 2 == 0) ? 1 : 2; }     /* the office the giver names (Medici 1, Fugger 2) */
static void debtnote_hook(void)            /* the lock is open: search the office chosen */
{
    u16 sg, o;
    int loc = (s16)g_rw(G_CUR_LOC), i = papers_event(loc, &sg, &o);
    if (i < 0 || (s16)rw(sg, o) != 10) { hook_call_original(); return; }
    int aux = (s16)rw(sg, (u16)(o + 0x2a)), named = fixes_debt_note_office(aux), office = g_rb(GV_insid_office);
    int want = aux - aux % 4 + named;                        /* (the same +0x2a but for the office) */
    ww(sg, (u16)(o + 0x2a), (u16)want);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix debtnote: office %d searched, the notes in office %d (game: %d)\n", office, named, aux % 4);
    hook_call_original();
    if (g_event(i) == (((u32)sg << 16) | o) && (s16)rw(sg, (u16)(o + 0x2a)) == want) ww(sg, (u16)(o + 0x2a), (u16)aux);   /* (still the same errand: as it was) */
}

/* ---------------------------------------------------------------- alchrisk
 * A brew's end (s029 0e76:3fb2, event i: +0x1e the chance, +0x2a where it was made, +0x2c the
 * formula's risk +0x52, 0 low .. 2 high as the alchemy page says): on a failure, half the time
 * and only at a place (cur_loc != -1), it starts an accident event 0x51 whose value is
 * rand_mod(4 - risk) != 0: 1 the explosion ($Alche02: hurt, damages, the inn barred four months),
 * 0 the stink (a month). That is 3/4 for low risk and 1/2 for high, the wrong way round; here the
 * roll counts when it is 0 (1/4 low, 1/3 medium, 1/2 high). rand_mod is hooked only for the
 * brew's end: its second roll, made only after a failure, is that one. */
static int alch_rolls, alch_risk, alch_chance, alch_failed;
static void alchrisk_rand_hook(void)
{
    int n = (s16)rw(SS, (u16)(SP + 4));
    hook_call_original();
    alch_rolls++;
    if (alch_rolls == 1) { alch_failed = alch_chance <= (s16)AX; return; }      /* (the chance against rand_mod(100)) */
    if (alch_rolls == 2 && alch_failed && n == 4 - alch_risk) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix alchrisk: risk %d, roll %d of %d: %s\n", alch_risk, (s16)AX, n, AX == 0 ? "explosion" : "stink");
        AX = AX == 0 ? 1 : 0;
    }
}
void fixes_alchemy_blast_odds(int risk, int *num, int *den)    /* of a failure's accident, the share that is an explosion */
{
    if (risk < 0) risk = 0;
    if (risk > 2) risk = 2;
    *den = 4 - risk;
    *num = fix_is_on("alchrisk") ? 1 : 3 - risk;
}
static void alchrisk_hook(void)            /* the brew's end (i) */
{
    int i = (s16)rw(SS, (u16)(SP + 4));
    u32 e = i >= 0 && i < 300 ? ev_ptr(i) : 0;
    if (!e) { hook_call_original(); return; }
    u16 s = (u16)(e >> 16), o = (u16)e;
    alch_risk = (s16)rw(s, (u16)(o + 0x2c));
    if (alch_risk < 0 || alch_risk > 2) { hook_call_original(); return; }
    alch_chance = (s16)rw(s, (u16)(o + 0x1e));
    alch_rolls = 0; alch_failed = 0;
    if (!hook_install(GF_rand_mod, alchrisk_rand_hook)) { hook_call_original(); return; }
    hook_call_original();
    hook_remove(GF_rand_mod, alchrisk_rand_hook);
}

/* ---------------------------------------------------------------- bishop
 * $Cathe00 (s056 19c4), option 4 "ask the canon" (19c4:0ac2) makes a fetching errand with
 * vec_21b1 (s039 1462:1712 (t): t the giver, 3 the bishop) - but with t 0, the merchants': the
 * card's own tests (ev8_owner_value(10, 3, town): an errand open; ev24_at_owner(10, 3, town): one
 * done, option 8) look for 3. Here t is 3 while that option runs. Option 8, "accept a relic"
 * (19c4:0e0a), gives the town's relic (rep above -10) but neither takes the thing fetched nor ends
 * the reward (kind 0x24, owner 3, tag 10 - or 3, the bishop's robber knight - the town): it is
 * ended here, the thing taken. */
static int canon_asking;
static void canon_hook(void) { canon_asking++; hook_call_original(); canon_asking--; }
static void errand_hook(void)               /* s039 1462:1712 (t) */
{
    if (canon_asking && (s16)rw(SS, (u16)(SP + 4)) == 0) {
        ww(SS, (u16)(SP + 4), 3);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix bishop: the canon's errand is the bishop's\n");
    }
    hook_call_original();
}
static void relic_hook(void)                /* $Cathe00 option 8, s056 19c4:0e0a */
{
    int loc = (s16)g_rw(G_CUR_LOC), found = -1;
    u32 e = 0;
    for (int i = 0; i < G_NEVENTS && found < 0; i++) {
        u32 p = g_event(i);
        if (p && g_ev(p, G_EV_KIND) == 0x24 && g_ev(p, G_EV_OWNER) == 3 && (g_ev(p, G_EV_TAG) == 10 || g_ev(p, G_EV_TAG) == 3) && g_ev(p, G_EV_LOC) == loc) { found = i; e = p; }
    }
    int paid = (s16)g_rw(GV_cur_rep) > -10;            /* (the card's test: the town's regard) */
    hook_call_original();
    if (found < 0 || !paid || g_event(found) != e) return;
    int item = g_ev(e, G_EV_ITEM);
    if (item >= 0x78 && item <= 0x8a) ns_call(31, 0x18e7, 0x04d4, 2, -2, item);    /* take_item(-2, the thing fetched) */
    ns_call(GF_event_free, 1, found);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix bishop: the relic paid, reward %d ended (item %#x taken)\n", found, item);
}

/* ---------------------------------------------------------------- altar
 * Purging a pagan altar (s107 19ce) sets the place's menu (+0x0c) to 0x62: it is drawn no more
 * and cannot be entered. The errand's site picker (s030 1462:28f2 (x, y, icon), and its fallback
 * nearest_loc_type_impl) look only at the icon: such places are hidden from it (their icon
 * changed for the call). Errands already sent to one (kind 8, tag 10) go to the nearest standing
 * place of the same kind. */
static int loc_purged(u32 r) { return r && (s16)rw((u16)(r >> 16), (u16)((u16)r + 0x0c)) == 0x62; }
static void sites_hook(void)
{
    int icon = (s16)rw(SS, (u16)(SP + 8));
    static int hidden[414]; int n = 0;
    for (int i = 0; i < 414; i++) {
        u32 r = g_location(i);
        if (loc_purged(r) && (s16)rw((u16)(r >> 16), (u16)r) == icon) { ww((u16)(r >> 16), (u16)r, 0x7ff0); hidden[n++] = i; }
    }
    hook_call_original();
    for (int k = 0; k < n; k++) { u32 r = g_location(hidden[k]); ww((u16)(r >> 16), (u16)r, (u16)icon); }
}
static void altar_frame(void)
{
    static int tick;
    if (++tick % 120) return;
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i);
        if (!e || g_ev(e, G_EV_KIND) != 8 || g_ev(e, G_EV_TAG) != 10) continue;
        int site = g_ev(e, G_EV_LOC);
        u32 r = site >= 0 && site < 414 ? g_location(site) : 0;
        if (!loc_purged(r)) continue;
        u16 rs = (u16)(r >> 16), ro = (u16)r;
        int icon = (s16)rw(rs, ro), x = (s16)rw(rs, (u16)(ro + 4)), y = (s16)rw(rs, (u16)(ro + 6)), best = -1, bd = 1 << 30;
        for (int j = 0; j < 414; j++) {
            u32 q = g_location(j);
            if (!q || j == site || loc_purged(q) || (s16)rw((u16)(q >> 16), (u16)q) != icon || (rb((u16)(q >> 16), (u16)((u16)q + 0x14)) & 4)) continue;
            int dx = abs((s16)rw((u16)(q >> 16), (u16)((u16)q + 4)) - x), dy = abs((s16)rw((u16)(q >> 16), (u16)((u16)q + 6)) - y) / 3;
            int d = dx >= dy ? dy / 2 + dx : dx / 2 + dy;
            if (d < bd) { bd = d; best = j; }
        }
        if (best < 0) continue;
        g_ev_set(e, G_EV_LOC, best);
        fprintf(stderr, "fix altar: errand %d sent from the purged place %d to %d\n", i, site, best);
    }
}

/* ---------------------------------------------------------------- tomb
 * $McGuf08 (s108 1838), "take the $NamedOneName and leave" (1838:07ca): the dead wake (1838:02b0,
 * run_battle) and then vec_214d(cur_loc) (s039 1462:1ab0) gives the thing if an errand (ev8_at(10,
 * cur_loc)) is there. But the victory has set cur_loc to -1 already, so nothing is given; a
 * defeat (3, 4) leaves it set, and the thing is given. Here the battle's result is caught: the
 * giving is let pass only after a victory, with the place for the moment of the call. */
static int tomb_loc = -1, tomb_result = -1;
static void tomb_battle_hook(void) { hook_call_original(); tomb_result = (s16)AX; }
static void tomb_give_hook(void)            /* s039 1462:1ab0 (loc), during the option */
{
    if (tomb_loc < 0) { hook_call_original(); return; }
    if (tomb_result != 0) {                                  /* lost, fled: no thing */
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix tomb: battle %d, nothing given\n", tomb_result);
        AX = 0; hook_return_far(0); return;
    }
    u16 cur = g_rw(G_CUR_LOC);
    g_ww(G_CUR_LOC, (u16)tomb_loc); ww(SS, (u16)(SP + 4), (u16)tomb_loc);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix tomb: won, the thing given (place %d)\n", tomb_loc);
    hook_call_original();
    g_ww(G_CUR_LOC, cur);
}
static void tomb_hook(void)                 /* $McGuf08 option, s108 1838:07ca */
{
    tomb_loc = (s16)g_rw(G_CUR_LOC); tomb_result = -1;
    hook_install(GF_run_battle, tomb_battle_hook);
    hook_install(39, 0x1462, 0x1ab0, tomb_give_hook);
    hook_call_original();
    hook_remove(39, 0x1462, 0x1ab0, tomb_give_hook);
    hook_remove(GF_run_battle, tomb_battle_hook);
    tomb_loc = -1;
}

static int started_ev(u32 e)                /* (event_has_started: its start not after now) */
{
    static const u16 F[4] = { 0x10, 0x0e, 0x0c, 0x0a }, NOW[4] = { G_CUR_YEAR, G_CUR_MONTH, G_CUR_DAY, G_CUR_HOUR };
    for (int k = 0; k < 4; k++) { int a = g_ev(e, F[k]), n = (s16)g_rw(NOW[k]); if (a != n) return a < n; }
    return 1;
}
/* ---------------------------------------------------------------- lordreward
 * A town without a council (city word +0x5e bit 2 clear) has its lord in the fortress ($CityF00,
 * s050 1a06): an audience (option 0, 1a06:026c (chance)) gives a robber knight for giver 10, but
 * the reward (kind 0x24, owner 10, the town) is paid only by $Counc00's audience (s054 19df:0298),
 * a card such a town never shows. Here, with a reward waiting, the fortress's audience is that
 * one: the council's routine pays it (money by the town's size, then $Raubi01 or $McGuf13). */
static void lord_hook(void)                 /* $CityF00 option 0, s050 1a06:026c (chance) */
{
    int loc = (s16)g_rw(G_CUR_LOC), waits = 0;
    for (int i = 0; i < G_NEVENTS && !waits; i++) {
        u32 p = g_event(i);
        if (p && g_ev(p, G_EV_KIND) == 0x24 && g_ev(p, G_EV_OWNER) == 10 && (g_ev(p, G_EV_TAG) == 3 || g_ev(p, G_EV_TAG) == 10) && g_ev(p, G_EV_LOC) == loc && started_ev(p)) waits = 1;
    }
    if (!waits || !(s16)rw(SS, (u16)(SP + 4))) { hook_call_original(); return; }
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix lordreward: the lord pays the reward waiting at %d\n", loc);
    ns_call(54, 0x19df, 0x0298, 1, 0);
    hook_return_far(0);
}

/* ---------------------------------------------------------------- standings
 * The party's standing with the Fugger, the Medici and the Hanse (standing_fugger .. _hanse,
 * DS:4bb6..4bba: their cards add and take; offices.h says what it decides) is not among what a
 * save holds (save: s037 1462:2ace; load: 1c7d:0d24), so a load kept the last game's, or 0. The
 * header has 35 words written as 0 and read into nothing (from 0xa7 on): the three go into the
 * first three, with "D+" (0x2b44) in the fourth to tell them from an older save's zeros; a save
 * without it loads with standing 0. The original game reads such saves as before. */
#define STANDING_AT 0xa7
static const u16 STANDING_GV[3] = { GV_standing_fugger, GV_standing_medici, GV_standing_hanse };
static FILE *save_file(const char *mode)       /* the save last written or read (DS:a680 "dksaveN.sav") */
{
    char name[16], dn[40], p[1200];
    g_str(G_DGROUP, 0xa680, name, sizeof name);
    if (strncasecmp(name, "dksave", 6)) return 0;
    snprintf(dn, sizeof dn, "SAVES\\%s", name);
    dos_host_path(dn, p, sizeof p);
    return fopen(p, mode);
}
static void standings_save_hook(void)          /* s037 1462:2ace (comment): write a save */
{
    hook_call_original();
    u16 ax = AX, dx = DX;
    FILE *f = save_file("r+b");
    if (f) {
        u8 b[8];
        for (int k = 0; k < 3; k++) { u16 v = g_rw(STANDING_GV[k]); b[2 * k] = (u8)v; b[2 * k + 1] = (u8)(v >> 8); }
        b[6] = 0x44; b[7] = 0x2b;
        if (fseek(f, STANDING_AT, SEEK_SET) == 0) fwrite(b, 1, 8, f);
        fclose(f);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix standings: saved %d %d %d\n", (s16)g_rw(STANDING_GV[0]), (s16)g_rw(STANDING_GV[1]), (s16)g_rw(STANDING_GV[2]));
    }
    AX = ax; DX = dx;
}
static void standings_load_hook(void)          /* s037 1c7d:0d24 (file): read a save */
{
    hook_call_original();
    u16 ax = AX, dx = DX;
    FILE *f = save_file("rb");
    u8 b[8] = { 0 };
    int ok = f && fseek(f, STANDING_AT, SEEK_SET) == 0 && fread(b, 1, 8, f) == 8 && b[6] == 0x44 && b[7] == 0x2b;
    if (f) fclose(f);
    for (int k = 0; k < 3; k++) g_ww(STANDING_GV[k], ok ? (u16)(b[2 * k] | b[2 * k + 1] << 8) : 0);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix standings: loaded %d %d %d%s\n", (s16)g_rw(STANDING_GV[0]), (s16)g_rw(STANDING_GV[1]), (s16)g_rw(STANDING_GV[2]), ok ? "" : " (an older save: 0)");
    AX = ax; DX = dx;
}

/* ---------------------------------------------------------------- healboost
 * heal_attr_pct (s029 0e76:08f2, (who, k, pct)) sets attribute k to cur + (max - cur) * pct / 100:
 * from above the maximum (a boost: Ironarm, New-wind, a saint) that brings it down. Essence
 * o'Grace heals Strength so (use_potion: pct = twice its quality): drunk while Ironarm ran it took
 * most of the boost away ("Ironarm interacts poorly with Essence of Grace"). A member above the
 * maximum keeps what they have. */
static void healboost_hook(void)
{
    int who = (s16)rw(SS, (u16)(SP + 4)), k = (s16)rw(SS, (u16)(SP + 6));
    if (who == -1) who = (s8)g_rb(G_LEADER);
    s8 was[5]; int keep[5] = { 0 };
    for (int m = 0; m < 5 && k >= 0 && k < 7; m++) {
        if (who != -2 && m != who) continue;
        was[m] = (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + k));
        keep[m] = was[m] > (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_MAX + k));
    }
    hook_call_original();
    for (int m = 0; m < 5; m++) {
        if (!keep[m] || (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + k)) == was[m]) continue;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix healboost: member %d's attribute %d stays %d (the healing made it %d)\n", m, k, was[m], (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + k)));
        g_wb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + k), (u8)was[m]);
    }
}

/* ---------------------------------------------------------------- oilweapons
 * The weapon oils are the member's in the game: one byte (party +0x34 = combatant +0x48, damage
 * high, penetration low) counts in every hit the member lands (hit_damage, s014 18bf:0778), with
 * any weapon, and Great Power (the greatpower fix) raises whatever weapon is in hand. The manual:
 * Deadly Blade works on "all weapons except impact and flail types", Strongedge "only ... edged
 * weapons and polearms", Great Power "only impact weapons, flails and handguns" (Deadly Blade's
 * label: "applied to non-impact weapons", Strongedge's: "to blades and poles"). Here a hit leaves
 * out what the weapon used may not have: its type is +0x22 (the missile weapon) when
 * readied with the alternate skill (+0x57 bit 7), else +0x51; 0-8 edged, 10-15 impact, 16-17
 * flails, 18-22 polearms, 23-26 thrown, 27-29 bows, 30-31 crossbows, 32-33 handguns (9 fists). */
int oils_weapon(int i)                          /* the weapon type member i strikes with now */
{
    u16 c = (u16)(GV_combatants + 0x80 * i);
    return (s8)g_rb((u16)(c + ((g_rb((u16)(c + 0x57)) & 0x80) ? 0x22 : 0x51)));
}
int oils_allow(int oil, int w)                  /* 0x69 Deadly Blade, 0x6a Strongedge, 0x6b Great Power on weapon type w */
{
    if (!fix_on("oilweapons")) return 1;
    int edged = w >= 0 && w <= 8, impact = w >= 10 && w <= 15, flail = w == 16 || w == 17, pole = w >= 18 && w <= 22;
    if (w < 0 || w == 9 || w > 33) return 0;
    if (oil == 0x69) return !impact && !flail;
    if (oil == 0x6a) return edged || pole;
    return impact || flail || w == 32 || w == 33;
}
static void oils_hit_hook(void)                 /* hit_damage (a, t, r, loc) */
{
    int a = (s16)rw(SS, (u16)(SP + 4));
    if (a < 0 || a > 4) { hook_call_original(); return; }
    u16 b = (u16)(GV_combatants + 0x80 * a + 0x48);
    u8 was = g_rb(b), v = was;
    int w = oils_weapon(a);
    if (!oils_allow(0x69, w)) v &= 0x0f;
    if (!oils_allow(0x6a, w)) v &= 0xf0;
    if (v != was) { g_wb(b, v); if (getenv("DL_FIX_LOG")) fprintf(stderr, "oils: member %d hits with weapon type %d: bonus %02x counts as %02x\n", a, w, was, v); }
    hook_call_original();
    if (v != was) g_wb(b, was);
}

/* ---------------------------------------------------------------- teachcap
 * A day at the inn (camp_spend_day, s036 1462:147a): a member set to "Train or study" (activity 6,
 * the skill in camp_activity_arg) pays the fee of the town's teacher of it (kind 0x28: +0x1e the
 * fee, +0x20 the ability S) and practice_skill rolls for +1, whatever the skill already is. The
 * guilds' text "our teacher's ability is $Text2, so you cannot progress beyond that under our
 * instruction" (never shown) and the physician's refusal of a party that heals better than he
 * does say what was meant. Here a member whose skill has reached S rests that day instead (no
 * fee, no lesson), with a word why; the inn's lodging view greys such a teacher. */
static void teachcap_hook(void)            /* camp_spend_day(mode), s036 1462:147a */
{
    for (int i = 0; i < 5; i++) {
        if (g_rb((u16)(G_MEMBER(i) + G_M_STATUS)) != 1 || (s16)g_rw((u16)(GV_camp_activity + 2 * i)) != 6) continue;
        int k = (s16)g_rw((u16)(GV_camp_activity_arg + 2 * i)), a = teach_ability_here(k);
        if (!teach_capped(i, k, a)) continue;
        g_ww((u16)(GV_camp_activity + 2 * i), 0);       /* rests */
        char name[32], msg[160];
        gm_member_name(i, name, sizeof name);
        snprintf(msg, sizeof msg, "%s已学会%s老师所能教授的一切（能力%d），并得以休息", name, gm_skill_name(k), a);
        ui_notice_for(msg, 6000);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix teachcap: member %d's %s (%d) has reached the teacher's %d\n", i, gm_skill_name(k), (s8)g_rb((u16)(G_MEMBER(i) + G_M_SKILLS + k)), a);
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- pauls
 * The saints' table (saint_fns, far 290e:2937) sends St. Paul (105) to 165c:5cca, which heals all
 * lost Strength and Endurance and takes half the Intelligence for a day, with virtue 70 - what the
 * texts and the manual give St. Paul the Simple - and St. Paul the Simple (106) to 5d6c, a copy of
 * St. Thomas Aquinas's help (6bb8) with the table values of 75d8. 75d8, which nothing calls, is the
 * Apostle's: Speak Common, Latin, Read & Write, Charisma, Endurance, a tenth of lost Strength,
 * virtue 19, as his text and the manual. Here Paul is answered by 75d8, Paul the Simple by 5cca. */
static int pauls_inner;
static void paul_hook(void)                /* 165c:5cca (who, x, what), reached for St. Paul */
{
    if (pauls_inner) { hook_call_original(); return; }  /* (Paul the Simple's, below) */
    int who = (s16)rw(SS, (u16)(SP + 4)), x = (s16)rw(SS, (u16)(SP + 6)), what = (s16)rw(SS, (u16)(SP + 8));
    int v = ns_call(39, 0x165c, 0x75d8, 3, who, x, what);
    AX = (u16)v; DX = (u16)ns_call_dx();
    if (getenv("DL_FIX_LOG") && what == 8) fprintf(stderr, "fix pauls: St. Paul the Apostle's help for member %d\n", who);
    hook_return_far(0);
}
static void paul_simple_hook(void)         /* 165c:5d6c, reached for St. Paul the Simple */
{
    int who = (s16)rw(SS, (u16)(SP + 4)), x = (s16)rw(SS, (u16)(SP + 6)), what = (s16)rw(SS, (u16)(SP + 8));
    pauls_inner = 1;
    int v = ns_call(39, 0x165c, 0x5cca, 3, who, x, what);
    pauls_inner = 0;
    AX = (u16)v; DX = (u16)ns_call_dx();
    if (getenv("DL_FIX_LOG") && what == 8) fprintf(stderr, "fix pauls: St. Paul the Simple's help for member %d\n", who);
    hook_return_far(0);
}

/* ---------------------------------------------------------------- saintrep
 * St. John Nepomuk (165c:41fa) in Prag (location 0x46) passes loc_reputation_adj (s029 0e76:19d0)
 * a local never set as the place, so the +15..24 went wherever that pointed; St. Cecilia (1024)
 * passes cur_loc, -1 on the map (the other saints take the nearest town then): the place before
 * the first, memory that is not a place's, is written. */
static int nep_prag;
static void nepomuk_hook(void)
{
    nep_prag = (s16)rw(SS, (u16)(SP + 8)) == 8 && (s16)g_rw(G_CUR_LOC) == 0x46;
    hook_call_original();
    nep_prag = 0;
}
static void repadj_hook(void)              /* loc_reputation_adj(loc, a, b) */
{
    int loc = (s16)rw(SS, (u16)(SP + 4)), to = loc;
    if (nep_prag) to = 0x46;
    else if (loc < 0 || loc >= 0x19e) to = ns_call(GF_nearest_city_impl, 2, g_rw(GV_party_x), g_rw((u16)(GV_party_x + 2)));
    if (to < 0 || to >= 0x19e) { AX = 0; DX = 0; hook_return_far(0); return; }   /* (still no place: nothing written) */
    if (to != loc) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix saintrep: reputation for location %d goes to %d\n", loc, to);
        ww(SS, (u16)(SP + 4), (u16)to);
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- nightbowyer
 * The card call that opens a shop, s029 0e76:21aa (shop, name far, cats lo, cats hi): the night
 * bowyer's (barter shop 5) comes with the smiths' categories, the day bowyer's with its own. */
static void nightbowyer_hook(void)          /* s029 0e76:21aa */
{
    int shop = (s16)rw(SS, (u16)(SP + 4));
    u32 cats = rw(SS, (u16)(SP + 10)) | (u32)rw(SS, (u16)(SP + 12)) << 16;
    if (shop == 5 && cats == 0x040000FFu) {
        ww(SS, (u16)(SP + 10), 0x0030); ww(SS, (u16)(SP + 12), 0x083c);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix nightbowyer: the bowyer's own goods (0x083C0030, not the smiths' 0x040000FF)\n");
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- confession
 * The confession ($CityC00's option, s057 1838:03cc) reads the town's reputation (cur_rep, DS:ee74)
 * only for its hours: past 109 it is read as 109 (an hour), and put back. */
static void confession_hook(void)           /* s057 1838:03cc */
{
    int rep = (s16)g_rw(GV_cur_rep);
    if (rep <= 109) { hook_call_original(); return; }
    g_ww(GV_cur_rep, 109);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix confession: reputation %d read as 109 (an hour, not %d)\n", rep, 11 - rep / 10);
    hook_call_original();
    g_ww(GV_cur_rep, (u16)rep);
}

/* ---------------------------------------------------------------- pileup */
static void pileup_hook(void)               /* damage_queue (d, t, pen, arm) */
{
    int t = (s16)rw(SS, (u16)(SP + 6));
    if (t < 0 || t >= 13) { hook_call_original(); return; }
    u16 q = (u16)(GV_cbt_queued_damage + 2 * t);
    int e0 = (s8)g_rb(q), s0 = (s8)g_rb((u16)(q + 1));
    hook_call_original();
    int e1 = (s8)g_rb(q), s1 = (s8)g_rb((u16)(q + 1));
    if (e1 < e0) g_wb(q, 127);
    if (s1 < s0) g_wb((u16)(q + 1), 127);
    if ((e1 < e0 || s1 < s0) && getenv("DL_FIX_LOG")) fprintf(stderr, "fix pileup: fighter %d's queued damage held at 127\n", t);
}

/* ---------------------------------------------------------------- castlefreeze */
static int cf_tries;
static void cf_dice_hook(void)              /* roll_dice (n, d), while 1d78 places the party on a kind 0x7c level */
{
    hook_call_original();
    if ((s16)AX != (s16)g_rw(0xdb63)) return;
    if (++cf_tries < 50) return;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix castlefreeze: only the gate room to draw: room 0\n");
    AX = 0;
}
static void castlefreeze_hook(void)         /* s007 15af:1d78 (l, t, pn, room*, dir*) */
{
    if ((s16)rw(SS, (u16)(SP + 6)) != 0x7c) { hook_call_original(); return; }
    cf_tries = 0;
    hook_install(GF_roll_dice, cf_dice_hook);
    hook_call_original();
    hook_remove(GF_roll_dice, cf_dice_hook);
}

/* ---------------------------------------------------------------- university */
static void uni_mark_hook(void)             /* event_here_type (kind, type, hours), within $Unive00's formulae question */
{
    if ((s16)rw(SS, (u16)(SP + 4)) == 0x4e && (s16)rw(SS, (u16)(SP + 6)) == 0) {
        ww(SS, (u16)(SP + 6), 1);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix university: the formulae noted as asked (not the saints)\n");
    }
    hook_call_original();
}
static void university_hook(void)           /* s056 18c0:0494 (chance) */
{
    hook_install(GF_event_here_type, uni_mark_hook);
    hook_call_original();
    hook_remove(GF_event_here_type, uni_mark_hook);
}

/* ---------------------------------------------------------------- partyitems, itemflags */
static void partyitems_hook(void)           /* add_item_rec_impl (who, struct item) */
{
    if ((s16)rw(SS, (u16)(SP + 4)) != -2) { hook_call_original(); return; }
    u16 w0 = rw(SS, (u16)(SP + 6)), w1 = rw(SS, (u16)(SP + 8)), w2 = rw(SS, (u16)(SP + 10));
    int last = 0;
    for (int m = 0; m < 5; m++) {
        if (!g_rb(G_MEMBER(m))) continue;                    /* (present, as the original's party[who].present) */
        last = ns_call(GF_add_item_rec_impl, 4, m, w0, w1, w2);
        if (ns_unwinding) return;
    }
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix partyitems: item %d given once to each member\n", w0 & 0xfff);
    AX = (u16)last;
    hook_return_far(0);
}
static int best_quality_of(int m, u32 flags)
{
    int best = 0, n = g_rb((u16)(G_MEMBER(m) + G_M_NITEMS));
    for (int i = 0; i < n && i < 64; i++) {
        u16 code = rw(G_FAR2E38, G_ITEM(m, i)) & 0xfff;
        unsigned long cats; if (!gm_itemdef_cats(code, &cats)) continue;
        u32 f = (u32)cats;
        int q = rb(G_FAR2E38, (u16)(G_ITEM(m, i) + 3));
        if (f == flags && q > best) best = q;
    }
    return best;
}
static void itemflags_hook(void)            /* item_quality_flags_impl (who, long flags) */
{
    int who = (s16)rw(SS, (u16)(SP + 4));
    u32 flags = (u32)rw(SS, (u16)(SP + 6)) | (u32)rw(SS, (u16)(SP + 8)) << 16;
    int best = 0;
    if (flags == 0x02000000UL) flags = 0x2000;     /* ($meetw00's: 0x200 for 0x2000, a flag no thing has) */
    if (who == -1) who = (s8)g_rb(G_LEADER);
    if (who == -2) { for (int m = 0; m < 5; m++) if (g_rb(G_MEMBER(m)) && g_rb((u16)(G_MEMBER(m) + G_M_NITEMS))) { int q = best_quality_of(m, flags); if (q > best) best = q; } }
    else if (who >= 0 && who < 5) best = best_quality_of(who, flags);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix itemflags: best quality of flags %08x for %d: %d\n", (unsigned)flags, who, best);
    AX = (u16)best;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- appointment */
static void appointment_hook(void)          /* the sweep of the events that ended, s029 0e76:3f30 */
{
    for (int i = 0; i < 300; i++) {
        u32 e = g_event(i);
        if (!e) continue;
        u16 sg = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(sg, (u16)(o + 0x22)) != 0x3a || (s16)rw(sg, (u16)(o + 0x26)) != 4 || (rw(sg, (u16)(o + 0x24)) & 1)) continue;
        if (!ns_call(GF_event_has_ended, 1, i)) continue;
        int loc = (s16)rw(sg, (u16)(o + 0x1c));
        ns_call(GF_event_new, 10, -2, loc, 0, 0x3b, 0, 100, 0, 24, 0, 0);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix appointment: the audience at place %d tomorrow\n", loc);
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- membervalue
 * Within a card option that mistakes a party query's value for its member (above), the first
 * party_max_skill / party_min_attr returns the member it found (best_member). */
static int mv_active, mv_done;
static void mv_query_hook(void)             /* party_max_skill (k), party_min_attr (k) */
{
    hook_call_original();
    if (!mv_active || mv_done) return;
    mv_done = 1;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix membervalue: %d taken for member %d\n", (s16)AX, g_rb(GV_best_member));
    AX = (u16)g_rb(GV_best_member);
}
static void mv_wrap_hook(void)              /* $CityC00's tenth (s057 1838:067c), $Snuck00's raised house (s071 1838:02a8) */
{
    int was = mv_active, done = mv_done;
    mv_active = 1; mv_done = 0;
    hook_call_original();
    mv_active = was; mv_done = done;
}

/* ---------------------------------------------------------------- cardslips
 * Each card routine below is wrapped; while it runs (slip), a call it makes is put right. */
enum { SLIP_NONE, SLIP_HUSSITE, SLIP_STABLES, SLIP_SCHULZ, SLIP_TATZEL, SLIP_HURT, SLIP_RAUBI };
static int slip, slip_p, slip_saw, raubi_card, raubi_guide;
static void slip_rand_hook(void)            /* rand_mod (n): the Hussites' == made <=; $Raubi01's coin */
{
    int n = (s16)rw(SS, (u16)(SP + 4));
    hook_call_original();
    if (slip == SLIP_RAUBI && n == 2) { AX = (u16)(raubi_guide >= 0); return; }   /* (text 6: the guide leaves; 7: there was none) */
    if (slip != SLIP_HUSSITE) return;
    int r = (s16)AX;
    if (r <= slip_p) AX = (u16)slip_p;      /* (the script's test is r == p; a higher r stays, never p) */
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: Hussites' friendship, %d against %d%%: %s\n", r, slip_p, r <= slip_p ? "won" : "lost");
    slip = SLIP_NONE;
}
static void slip_text_hook(void)            /* card_text (block, how) */
{
    int b = (s16)rw(SS, (u16)(SP + 4));
    if (slip == SLIP_STABLES && (b == 1 || b == 7)) ww(SS, (u16)(SP + 4), (u16)(b == 1 ? 7 : 1));
    if (slip == SLIP_SCHULZ && b == 6) slip_saw = 1;
    if (slip == SLIP_RAUBI && raubi_card == 0x86 && raubi_guide < 0 && b == 1) ww(SS, (u16)(SP + 4), 2);   /* (no Schulz came along) */
    hook_call_original();
}
static void slip_away_hook(void)            /* s029 0e76:2474: one member who fled is lost; returns how many fled */
{
    hook_call_original();
    if (slip == SLIP_TATZEL) AX = (u16)((s16)AX > 0 ? (s16)(s8)g_rb(GV_best_member) : -1);
}
static void slip_attr_hook(void)            /* add_attr (who, k, d): Endurance, Strength taken, not given */
{
    int k = (s16)rw(SS, (u16)(SP + 6)), d = (s16)rw(SS, (u16)(SP + 8));
    if (slip == SLIP_HURT && (k == 0 || k == 1) && d > 0) {
        ww(SS, (u16)(SP + 8), (u16)-d);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: attribute %d taken by %d, not given\n", k, d);
    }
    hook_call_original();
}
static void slip_run(int which, int sect, unsigned seg, unsigned off, void (*fn)(void))
{
    int was = slip; slip = which; slip_saw = 0;
    hook_install(sect, (u16)seg, (u16)off, fn);
    hook_call_original();
    hook_remove(sect, (u16)seg, (u16)off, fn);
    slip = was;
}
static void hussite_hook(void)              /* $Witcd.. Hussite band, "claim friendship" (s114 18d3:041c, chance) */
{
    slip_p = (s16)rw(SS, (u16)(SP + 4));
    slip_run(SLIP_HUSSITE, GF_rand_mod, slip_rand_hook);
}
static void footpads_potion_hook(void)      /* footpads, a potion thrown (s053 1838:0aa8) */
{
    int slot = (s16)g_rw(GV_menu_slot), m = (s16)g_rw(GV_menu_member);
    int code = slot >= 0 && slot < 4 ? (s16)g_rw((u16)(GV_card_items + 2 * slot)) : -1;
    if (code > 0 && m >= 0 && m < 5) {
        ns_call(GF_take_item_impl, 2, m, code);
        if (ns_unwinding) return;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: footpads, potion %d used up by member %d\n", code, m);
    }
    hook_call_original();
}
static void stables_hook(void) { slip_run(SLIP_STABLES, GF_card_text, slip_text_hook); }   /* $Urban00 stables (s051 18c9:05d0) */
static void schulz_hook(void)               /* a village's Schulz, asked for help (s077 1996:0378) */
{
    u16 fl = g_rw(G_MONEY), gr = g_rw(G_MONEY + 2), pf = g_rw(G_MONEY + 4);
    slip_run(SLIP_SCHULZ, GF_card_text, slip_text_hook);
    if (ns_unwinding) return;
    if (slip_saw && g_rw(G_MONEY) == fl && g_rw(G_MONEY + 2) == gr && g_rw(G_MONEY + 4) == pf) {
        ns_call(GF_money_add_fgp, 4, G_MONEY, 1, 0, 0);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: the Schulz's florin given\n");
    }
}
static int knows_formula(int m, int f) { return (rb(G_FAR2E38, (u16)(0x07be + m * 22 + f / 3)) >> (f % 3)) & 1; }
static void templar_hook(void)              /* $Witcd02, the writing desk read (s118 1a2b:02ac, chance) */
{
    int p = (s16)rw(SS, (u16)(SP + 4));
    g_ww(GV_opt_flag_1, 2);
    ns_call(GF_party_max_skill_sum, 2, 0xc, 0xb);
    if (ns_unwinding) return;
    if (ns_call(GF_rand_mod, 1, 100) <= p) {
        ns_call(GF_card_text, 2, 3, 0);
        if (ns_unwinding) return;
        int m = (s8)g_rb(GV_best_member);
        if (m < 0 || m > 4) m = 0;
        for (int k = 0; k < 3; k++) {          /* three formulae, each from a random start to the next unknown one */
            int start = ns_call(GF_rand_mod, 1, 66), f = start;
            while (knows_formula(m, f)) { if (++f >= 66) f = 0; if (f == start) break; }
            u16 a = (u16)(0x07be + m * 22 + f / 3);
            if (!knows_formula(m, f) && getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: Templars' desk teaches formula %d to member %d\n", f, m);
            wb(G_FAR2E38, a, (u8)(rb(G_FAR2E38, a) | 1 << f % 3));
        }
    } else ns_call(GF_card_text, 2, 4, 0);
    if (ns_unwinding) return;
    hook_return_far(0);
}
static void tatzel_hook(void) { slip_run(SLIP_TATZEL, 29, 0x0e76, 0x2474, slip_away_hook); }   /* $McGuf02's tatzelwurm (s106 19bc:03e8) */
static void hurt_hook(void) { slip_run(SLIP_HURT, GF_add_attr, slip_attr_hook); }              /* the cavern (s130 190c:033e), the spiders (s134 1838:0550, 0692) */
static void kingname_hook(void)             /* $MineB03 (s101 18bd:0000): "It is $NamedOneName again", never set there */
{
    for (int i = 0; i < G_NEVENTS; i++) {       /* the king's name as $MineB00 and B02 set it: name_to_slot (7, vec_219d (), 0), the f2e of the quest's kind-8 tag-12 event */
        u32 e = g_event(i);
        if (!e || g_ev(e, G_EV_KIND) != 8 || g_ev(e, G_EV_TAG) != 12) continue;
        ns_call(GF_name_to_slot, 3, 7, g_ev(e, G_EV_ITEM), 0);
        if (ns_unwinding) return;
        break;
    }
    hook_call_original();
}
static void spider_man_hook(void)           /* $DPost00 (s134 1838:0000): the warehouse owner's $NamedOneName, never set */
{
    ns_call(GF_random_name, 3, (s16)g_rw(GV_cur_loc) + 0x9c4, 0, 0);
    if (ns_unwinding) return;
    hook_call_original();
}
static int guide_member(const char *shortname)
{
    for (int m = 0; m < 5; m++) {
        if (!g_rb(G_MEMBER(m))) continue;
        char nm[12]; int i = 0;
        for (; i < 11; i++) { nm[i] = (char)g_rb((u16)(G_MEMBER(m) + 0x2a + i)); if (!nm[i]) break; }
        nm[i] = 0;
        if (!strcmp(nm, shortname)) return m;
    }
    return -1;
}
static void raubi_hook(void)                /* $Raubi01, a robber knight's bounty paid (s080 1870:000c) */
{
    int sc = (s16)g_rw(GV_saved_card);
    if (sc != 0x47 && sc != 0x86) { hook_call_original(); return; }
    int guide = guide_member(sc == 0x47 ? "汉萨" : "舒尔茨"), was = slip;
    slip = SLIP_RAUBI; raubi_card = sc; raubi_guide = guide;
    hook_install(GF_rand_mod, slip_rand_hook); hook_install(GF_card_text, slip_text_hook);
    hook_call_original();
    hook_remove(GF_card_text, slip_text_hook); hook_remove(GF_rand_mod, slip_rand_hook);
    slip = was;
    if (ns_unwinding || guide < 0 || g_rb(G_MEMBER(guide)) == 0) return;
    u16 ax = AX, dx = DX;                   /* (the card's next) */
    g_wb(G_MEMBER(guide), 0);               /* the guide who came along goes home, as the text says */
    ns_call(GF_party_delete, 1, guide);
    if (ns_unwinding) return;
    ns_call(GF_party_panel_rows, 1, 0x100);
    if (ns_unwinding) return;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: the %s guide (member %d) leaves the party\n", sc == 0x47 ? "Hanse" : "Schulz", guide);
    AX = ax; DX = dx;
}
static void witches_offer(void)             /* (from fixes_card_opts) $Witcd40 block 4: the ointment, or stay and fight */
{
    if (!g_rb(GV_opt_on) && !g_rb((u16)(GV_opt_on + 1))) {
        g_wb(GV_opt_on, 1); g_wb((u16)(GV_opt_on + 1), 1);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: $Witcd40's ointment offer switched on\n");
    }
}

/* ---------------------------------------------------------------- nightwall */
static int climb_value(int m) { return (ns_call(GF_member_eff_attr2, 1, m) * 2 + ns_call(GF_member_skill, 2, m, 0xf)) / 2; }
static void nightwall_options(void)         /* (from fixes_card_opts) $CityW01 block 0: the climber */
{
    int best = -1, bv = -1, n = (s16)g_rw(GV_party_size);
    for (int m = 0; m < n && m < 5; m++) {
        if (g_rb(G_MEMBER(m)) != 1) continue;
        int v = climb_value(m);
        if (ns_unwinding) return;
        if (v > bv) { bv = v; best = m; }
    }
    if (best < 0) return;
    g_ww(0x5828, (u16)best); g_ww(0x582a, (u16)bv); g_ww(GV_var_chosen1, (u16)best);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix nightwall: the climber %d, %d%%\n", best, bv);
}
static void nightwall_rope_hook(void)       /* s047 1a44:03e2 () -> (the rope's chance, the climber) */
{
    int m = (s16)g_rw(0x5828);
    if (m < 0 || m > 4) { hook_call_original(); return; }
    int v = climb_value(m);
    if (ns_unwinding) return;
    g_ww(0x582a, (u16)v); g_ww(GV_var_chosen1, (u16)m);
    AX = (u16)v; DX = (u16)m;
    hook_return_far(0);
}
static void nightwall_all_hook(void)        /* s047 1a44:05f0 () -> (everyone's chance, -2) */
{
    int low = 999, who = -1, n = (s16)g_rw(GV_party_size);
    for (int m = 0; m < n && m < 5; m++) {
        if (g_rb(G_MEMBER(m)) != 1) continue;
        int v = climb_value(m);
        if (ns_unwinding) return;
        if (v < low) { low = v; who = m; }
    }
    if (who >= 0) g_ww(GV_var_chosen1, (u16)who);
    AX = (u16)low; DX = 0xfffe;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- campguard */
static void campguard_hook(void)            /* s036 1462:1b80 (): the camp's risk, grown */
{
    int started = ns_call(GF_event_started, 1, 0x60);
    if (ns_unwinding) return;
    if (started) { g_ww(0x8a34, 0); AX = 0; hook_return_far(0); return; }
    int total = (s16)g_rw(0x8a36);
    for (int i = 0; i < 5; i++) {
        u16 r = G_MEMBER(i);
        if (g_rb(r) != 1 || (s16)g_rw((u16)(GV_camp_activity + 2 * i)) != 5) continue;
        int wood = (s8)g_rb((u16)(r + G_M_SKILLS + 18)), st = (s8)g_rb((u16)(r + G_M_SKILLS + 15)), armed = 0;
        total -= wood >= 70 ? 3 : wood >= 30 ? 2 : 1;
        if (st >= 70) total -= 2; else if (st >= 30) total--;
        for (int k = 0; k < 7; k++) if ((s8)g_rb((u16)(r + G_M_SKILLS + k)) >= 30) armed = 1;   /* (Impact, +0x58, too) */
        if (armed) total--;
    }
    if (total >= 15) total = 15; else if (total <= 1) total = 1;
    int risk = (s16)g_rw(0x8a34) + total;
    g_ww(0x8a34, (u16)risk);
    AX = (u16)(risk / 5);
    hook_return_far(0);
}

/* ---------------------------------------------------------------- enemygroups */
static void enemygroups_hook(void)          /* groups_fixup (nlev), s004 0e76:1d90 */
{
    int nlev = (s16)rw(SS, (u16)(SP + 4));
    u16 seg = g_rw(GV_enemy_seg), base = g_rw(GV_bf_groups);
    for (int lv = 0; lv < nlev && lv < 16; lv++) {
        int n = (s16)g_rw((u16)(GV_group_count + 2 * lv)), mons = (s16)g_rw((u16)(GV_mon_count + 2 * lv)), kept = 0;
        if (n < 0 || n > 10) continue;
        for (int k = 0; k < n; k++) {
            u16 g = (u16)(base + (lv * 10 + k) * 0x1e);
            int used = 0;
            for (int j = 0; j < 3; j++) { int t = (s8)rb(seg, (u16)(g + 8 + j)), c = (s8)rb(seg, (u16)(g + 0x0b + j)); if (t >= 0 && t < mons && c > 0) used = 1; }
            if (!used) continue;
            if (kept != k) { u16 to = (u16)(base + (lv * 10 + kept) * 0x1e); for (int b = 0; b < 0x1e; b++) wb(seg, (u16)(to + b), rb(seg, (u16)(g + b))); }
            kept++;
        }
        if (kept != n) {
            g_ww((u16)(GV_group_count + 2 * lv), (u16)kept);
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix enemygroups: level %d keeps %d of %d bands (the game kept the ones before the first empty one)\n", lv, kept, n);
        }
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- cityalert
 * loc_hostile (s039 1462:00ba): hostile is an event 0x11 here, or a standing of -75 or less, or of
 * -10 or less while the town is on alert (event 0x13: $Selec00, s058 191a, for 24 hours after a
 * fight or a failed potion at the gate; conflict_outcome for 28 days). The alert's test passes its
 * arguments the wrong way round (event_at(loc, 0x13)), so it never held. Found by Arkana Mechanika
 * (Darklands Restoration), B-55afd6. (mods/war.c does the same for its own cards.) */
static void cityalert_hook(void)
{
    hook_call_original();
    if (AX || ns_unwinding) return;
    u16 ax = AX, dx = DX;
    int loc = (s16)g_rw(G_CUR_LOC);
    int alert = loc >= 0 ? ns_call(GF_event_at, 2, 0x13, loc) : 0, rep = alert ? ns_call(GF_loc_reputation, 1, loc) : 0;
    if (ns_unwinding) return;
    AX = ax; DX = dx;
    if (alert && rep <= -10) {
        AX = 1;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cityalert: %d on alert, standing %d: hostile\n", loc, rep);
    }
}

/* ---------------------------------------------------------------- dayjobs
 * The best day job of a member (s036 1462:197e): the table at DS:3ace, 31 jobs of 18 bytes (+2 the
 * town kinds it needs, +6 the kinds it is closed in, +0x0a the place's flags it needs). The second
 * test reads `!cats2 & city.cats`, so a job with cats2 (3, preaching friar, 0x4000; 5, mendicant
 * friar, 0x8000) is never open; meant is !(cats2 & city.cats) (no town has those bits). Here those
 * jobs' cats2 is cleared for the call where the town has none of them. Found by Arkana Mechanika
 * (Darklands Restoration), B-074e99. */
static void dayjobs_hook(void)
{
    u16 o = g_rw(GV_city_rec), sg = g_rw((u16)(GV_city_rec + 2));
    u32 cats = sg ? ((u32)rw(sg, (u16)(o + 0x5e)) | (u32)rw(sg, (u16)(o + 0x60)) << 16) : 0;
    u32 kept[31]; int changed = 0;
    for (int i = 0; i < 31; i++) {
        u16 a = (u16)(0x3ace + 18 * i + 6);
        kept[i] = (u32)g_rw(a) | (u32)g_rw((u16)(a + 2)) << 16;
        if (kept[i] && !(kept[i] & cats)) { g_ww(a, 0); g_ww((u16)(a + 2), 0); changed = 1; }
    }
    hook_call_original();
    if (changed) for (int i = 0; i < 31; i++) { u16 a = (u16)(0x3ace + 18 * i + 6); g_ww(a, (u16)kept[i]); g_ww((u16)(a + 2), (u16)(kept[i] >> 16)); }
}

/* ---------------------------------------------------------------- cloudline
 * cloud_blocks_line (a, b), s014 1ec9:0142: does the line from combatant a to b pass through an effect
 * cloud? The clouds: DS:a33d their number, DS:a2eb 10 bytes each (+0 x, +2 y, +4 the size in the high
 * nibble); a cloud's box is its centre +- the bytes DS:18b6[n], 18bc[n]; its n*n puffs lie at the
 * centre + the signed bytes the near pointers DS:21b2[n], 21bc[n] lead to. A puff that the centre
 * sees (line_blocked, 1d69:000c, on the line DS:e0a5/e0a3 - e0a9/e0a7) blocks the shot when the shot
 * crosses its box (+-8, +-4; 1d69:0354, the box DS:e1d5, e1d9, e1d7, e1db). The loop ran to the count
 * inclusive (a slot past the last cloud), and after a cloud whose box the shot missed it reloaded the
 * line from words saved only when a box was hit (stack leftovers until then). Found by Arkana
 * Mechanika (Darklands Restoration), B-3cc78a, B-bfb04e. */
static void cl_line(int x0, int y0, int x1, int y1) { g_ww(0xe0a5, (u16)x0); g_ww(0xe0a3, (u16)y0); g_ww(0xe0a9, (u16)x1); g_ww(0xe0a7, (u16)y1); }
static void cl_box(int x0, int y0, int x1, int y1) { g_ww(0xe1d5, (u16)x0); g_ww(0xe1d9, (u16)y0); g_ww(0xe1d7, (u16)x1); g_ww(0xe1db, (u16)y1); }
static void cloudline_hook(void)
{
    int a = (s16)rw(SS, (u16)(SP + 4)), b = (s16)rw(SS, (u16)(SP + 6)), n = (s8)g_rb(0xa33d);
    if (n <= 0 || n > 8 || a < 0 || a > 40 || b < 0 || b > 40) { hook_call_original(); return; }
    u16 ca = (u16)(GV_combatants + 0x80 * a), cb = (u16)(GV_combatants + 0x80 * b);
    int ax = (s16)g_rw((u16)(ca + 4)), ay = (s16)g_rw((u16)(ca + 6)), bx = (s16)g_rw((u16)(cb + 4)), by = (s16)g_rw((u16)(cb + 6));
    int hit = 0;
    for (int i = 0; i < n && !hit; i++) {
        u16 c = (u16)(0xa2eb + 10 * i);
        int x = (s16)g_rw(c), y = (s16)g_rw((u16)(c + 2)), sz = g_rb((u16)(c + 4)) >> 4;
        int hx = (s8)g_rb((u16)(0x18b6 + sz)), hy = (s8)g_rb((u16)(0x18bc + sz));
        cl_line(ax, ay, bx, by); cl_box(x - hx, y - hy, x + hx, y + hy);
        int crosses = ns_call(14, 0x1d69, 0x0354, 0);
        if (ns_unwinding) return;
        if (crosses != 1) continue;
        u16 px = g_rw((u16)(0x21b2 + 2 * sz)), py = g_rw((u16)(0x21bc + 2 * sz));
        for (int k = 0; k < sz * sz && !hit; k++) {
            int fx = x + (s8)g_rb((u16)(px + k)), fy = y + (s8)g_rb((u16)(py + k));
            cl_line(x, y, fx, fy);
            int blocked = ns_call(GF_line_blocked, 3, -1, -1, 0);
            if (ns_unwinding) return;
            if (blocked) continue;
            cl_box(fx - 8, fy - 4, fx + 8, fy + 4); cl_line(ax, ay, bx, by);
            hit = ns_call(14, 0x1d69, 0x0354, 0) == 1;
            if (ns_unwinding) return;
        }
    }
    cl_line(ax, ay, bx, by);
    AX = (u16)hit; DX = 0;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- attrsum
 * party_max_attr_sum (a, b), s029 0e76:179c: the highest sum of attributes a + b over the present
 * members, kept in best_value (a byte) and best_member. It stores a * 2 instead of the sum (and
 * compares the next member's sum with that). Callers: $CityM00 (s057 192f: maxima 11 and 12),
 * $Dunge00 (s047 1852), $Count02 (s105 1838), and the encounter scripts' maxattrsum. Found by Arkana Mechanika (Darklands Restoration), B-82a302. */
static void attrsum_hook(void)
{
    int a = (s16)rw(SS, (u16)(SP + 4)), b = (s16)rw(SS, (u16)(SP + 6));
    if (a < 0 || a > 13 || b < 0 || b > 13) { hook_call_original(); return; }   /* (7..13: the maxima; $CityM00 asks 11 and 12) */
    int best = 0;
    g_wb(GV_best_value, 0);
    for (int m = 0; m < 5; m++) {
        if (!g_rb(G_MEMBER(m))) continue;
        int v = (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + a)) + (s8)g_rb((u16)(G_MEMBER(m) + G_M_ATTR_CUR + b));
        if (v > best) { best = v; g_wb(GV_best_value, (u8)v); g_wb(GV_best_member, (u8)m); }
    }
    AX = (u16)(best & 0xff); DX = 0;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- more for cardslips, patronsaint, fullpack, blessing */
/* $Selec00 (the town gate, s058 191a): its fights (01f4, 09ba) set the standing to cur_rep - 40, cur_rep
   read once when the card was entered: after a fight that keeps the party at the gate, a second one
   took nothing more. Read again before each (B-0818ac). */
static void gate_rep_hook(void)
{
    int loc = (s16)g_rw(G_CUR_LOC);
    if (loc >= 0) {
        int rep = ns_call(GF_loc_reputation, 1, loc);
        if (ns_unwinding) return;
        g_ww(GV_cur_rep, (u16)rep);
    }
    hook_call_original();
}
/* The docks' passage hints (s051 19f2:0792, 09a6, 0bba, 0dd0, 0fe4): short of money they copy the
   string table's entry 290e:1e5f ("You need a missile weapon") for the one before it, 1e5b ("Not
   enough money") (B-a73a8e). */
static void docks_hint_hook(void)
{
    hook_call_original();
    if (ns_unwinding) return;
    u16 ax = AX, dx = DX, t = (u16)(LOADSEG + 0x290e);
    char now[48], want[48], wrong[48];
    g_str(G_DGROUP, GV_msg_buf, now, sizeof now);
    g_str(rw(t, 0x1e61), rw(t, 0x1e5f), wrong, sizeof wrong);
    g_str(rw(t, 0x1e5d), rw(t, 0x1e5b), want, sizeof want);
    if (want[0] && !strcmp(now, wrong)) {
        int i = 0; for (; want[i] && i < 47; i++) g_wb((u16)(GV_msg_buf + i), (u8)want[i]);
        g_wb((u16)(GV_msg_buf + i), 0);
    }
    AX = ax; DX = dx;
}
/* $SideS01 (a side street at night, s046 194b:0736), "to the grove": trouble on the way sends the
   party to an encounter (0x3c) that returns to the night grove (0x22) though the hour spent may have
   brought the day; the clear way picks the day's grove (0x21) or the night's by daylight (B-ed73b9). */
static void grove_hook(void)
{
    hook_call_original();
    if (ns_unwinding) return;
    u16 ax = AX, dx = DX;
    if (g_rw(GV_next_card) == 0x3c && g_rw(GV_saved_card) == 0x22 && ns_call(GF_is_daylight, 0)) g_ww(GV_saved_card, 0x21);
    AX = ax; DX = dx;
}
/* St. Cosmas (s039 165c:17a4) and St. Damian (1988) teach each other (saints_known[who][3] |= 2,
   0x10), with who -1 from the prayer screen's P (s034 saints.c): row -1 is the fifth member's
   formulae (one of them set), and nobody learnt the saint. As the cathedral's patron (below): the
   leader (B-94efea). */
static void teach_saint_hook(void)
{
    if ((s16)rw(SS, (u16)(SP + 8)) == 8 && (s16)rw(SS, (u16)(SP + 4)) == -1) {
        ww(SS, (u16)(SP + 4), (u16)(s8)g_rb(G_LEADER));
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix patronsaint: the saint taught to the leader\n");
    }
    hook_call_original();
}
/* room for one of (id, quality) with member m, or (fullpack passes it on) with anyone */
static int room_anywhere(int m, int id, int q)
{
    int t0 = gm_itemdef_type(id); u8 type = t0 < 0 ? 0 : (u8)t0;
    int size = (s16)g_rw(G_PARTY_SIZE);
    for (int k = 0; k < 5 && k < size; k++) {
        if (k != m && (!fix_on("fullpack") || !g_rb(G_MEMBER(k)))) continue;
        if (g_rb((u16)(G_MEMBER(k) + G_M_NITEMS)) < 64 || stacks(k, (u16)id, type, (u8)q, 1)) return 1;
    }
    return 0;
}
/* buying (barter_key P, s032 1462:0f60) pays and then adds the thing, with no test of the pack: with
   no room for it the money was gone (B-fd421f). Refused. */
static void buy_room_hook(void)
{
    int k = (s16)rw(SS, (u16)(SP + 4));
    if (k == 0x1950 || k == 0x1970) {
        int i = (s16)g_rw(GV_barter_ware_cur) + (s16)g_rw(GV_barter_ware_top), m = (s16)g_rw(GV_barter_who);
        int id = i >= 0 && i < 100 ? (s16)g_rw((u16)(GV_barter_wares + 2 * i)) : -1;
        if (id >= 0 && m >= 0 && m < 5) {                                  /* (item 0: the Hand Axe) */
            int q = ns_call(32, 0x1462, 0x0efa, 1, id);
            if (ns_unwinding) return;
            if (!room_anywhere(m, id, q)) {
                if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix fullpack: no room to buy item %d\n", id);
                int barterview_on(void);
                if (!barterview_on()) ui_notice("未购买：队伍中无人再有空间容纳其他种类的物品");   /* (the shop drawn anew says it itself) */
                AX = (u16)k; hook_return_far(0); return;
            }
        }
    }
    hook_call_original();
}
/* taking from a store room (cache_key G, s035 1462:0a46) gives up the store's entry whatever the
   adding did: with no room the thing was lost (B-2cf199). Refused. */
static void store_get_room_hook(void)
{
    int k = rw(SS, (u16)(SP + 4)) & 0xff, m = (s16)g_rw(GV_cache_who);
    if ((k == 'g' || k == 'G') && m >= 0 && m < 5 && g_rw((u16)(0x37c4 + 2 * m))) {
        int e = (s16)g_rw((u16)(0x37d8 + 2 * m)) + (s16)g_rw((u16)(0x37ec + 2 * m));
        if (e >= 0 && e < 100 && e < (s16)g_rw(GV_cache_n)) {
            u16 o = (u16)(0x37f4 + 4 * e);
            if (!room_anywhere(m, rw(G_FAR2E38, o) & 0xfff, rb(G_FAR2E38, (u16)(o + 2)))) {
                int cacheview_is_on(void);
                if (!cacheview_is_on()) ui_notice("队伍中无人再有空间容纳其他种类的物品");
                hook_return_far(0); return;
            }
        }
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- lootcount
 * The loot list after a battle (s014 19cd:33c0, (struct loot far *p, int c)): c records of 6 bytes
 * (+0 the item, +2, +3 its type and quality, +4 the count) merged into the list at far DS:c654
 * (DS:c67f entries), the count added in a byte with no test: past 255 it wrapped. Here a record
 * fills the ones that match up to 255 and the rest goes on as a new one. Found by Arkana Mechanika
 * (Darklands Restoration), B-e080d4. The list is in EMS (logical page 10 in physical page 3, which
 * the original maps in and out around its loop: CURSOR_BUSY / CURSOR_IDLE in s014_19cd.c). */
static void lootcount_hook(void)
{
    u16 po = rw(SS, (u16)(SP + 4)), ps = rw(SS, (u16)(SP + 6)), lo = g_rw(0xc654), ls = g_rw(0xc656);
    int c = (s16)rw(SS, (u16)(SP + 8));
    ns_call(GF_ems_map_page, 3, 3, 10, g_rw(GV_ems_frame_handle));   /* the list lives in EMS page 10, mapped in as the original does
                                                                        (its "busy cursor" macro); without it the list was another page's memory */
    if (ns_unwinding) return;
    for (; c > 0; c--, po = (u16)(po + 6)) {
        u16 id = rw(ps, po); u8 a = rb(ps, (u16)(po + 2)), b = rb(ps, (u16)(po + 3));
        int left = rb(ps, (u16)(po + 4)), n = (s16)g_rw(0xc67f), had = 0;
        for (int k = 0; k < n && left > 0; k++) {
            u16 q = (u16)(lo + 6 * k);
            if (rw(ls, q) != id || rb(ls, (u16)(q + 2)) != a || rb(ls, (u16)(q + 3)) != b) continue;
            had = 1;
            int room = 255 - rb(ls, (u16)(q + 4)), t = left < room ? left : room;
            wb(ls, (u16)(q + 4), (u8)(rb(ls, (u16)(q + 4)) + t)); left -= t;
        }
        if (left > 0 || !had) {
            u16 q = (u16)(lo + 6 * n);
            for (int k = 0; k < 6; k++) wb(ls, (u16)(q + k), rb(ps, (u16)(po + k)));
            wb(ls, (u16)(q + 4), (u8)left);
            g_ww(0xc67f, (u16)(n + 1));
            if (had && getenv("DL_FIX_LOG")) fprintf(stderr, "fix lootcount: item %d past 255, %d more in a new line\n", id & 0xfff, left);
        }
    }
    ns_call(GF_ems_map_page, 3, 3, 3, g_rw(GV_ems_frame_handle));    /* (and page 3 back) */
    if (ns_unwinding) return;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- corridors
 * A castle level's rooms (s007 15af:0698): a passage (room kind 7, its cells o0..o1 by o2..o3) gets
 * a mark on one wall cell, x = o1 + roll_dice(1, o1 - o0 + 1) - 1 (or y from o3): from the far end,
 * so every roll but 1 falls past it. Here those two rolls (the calls returning to 15af:0a71 and
 * 0adf) come back less the passage's length minus one: x = o0 + roll - 1. Found by Arkana Mechanika
 * (Darklands Restoration), B-c90740. */
static void corridor_roll_hook(void)        /* roll_dice (n, d), only within 0698 */
{
    u16 ret = rw(SS, SP);
    int d = (s16)rw(SS, (u16)(SP + 6));
    hook_call_original();
    if (ns_unwinding) return;
    if (ret == 0x0a71 || ret == 0x0adf) {
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix corridors: roll %d of %d becomes %d\n", (s16)AX, d, (s16)AX - (d - 1));
        AX = (u16)((s16)AX - (d - 1));
    }
}
static void corridors_hook(void)
{
    hook_install(GF_roll_dice, corridor_roll_hook);
    hook_call_original();
    hook_remove(GF_roll_dice, corridor_roll_hook);
}

/* ---------------------------------------------------------------- battledead
 * party_delete (s037 1462:3f30) cancels the member's contract (kind 0x43) and "has left" notice
 * (0x47). A member who dies in a battle is deleted while the table is in events.tmp (between
 * cards_shutdown and cards_startup: root 03a1:000a, run_battle), so both came back with it; the
 * notice later sent away whoever had that place in the party then. Here they are cancelled once
 * the table is back (cards_up_hook). Found by Arkana Mechanika (Darklands Restoration), B-f7799c. */
static int dead_pending;
static void battledead_hook(void)          /* party_delete (p) */
{
    int p = (s16)rw(SS, (u16)(SP + 4));
    if (cards_down && p >= 0 && p < 5) dead_pending |= 1 << p;
    hook_call_original();
}
static void battledead_after(void)          /* (cards_up_hook, the table back) */
{
    int was = dead_pending; dead_pending = 0;
    if (!was || !fix_on("battledead")) return;
    for (int p = 0; p < 5; p++) {
        if (!(was & 1 << p) || g_rb(G_MEMBER(p))) continue;          /* (still gone) */
        for (int kind = 0x43; kind <= 0x47; kind += 4) {
            int i = ns_call(GF_event_find, 6, kind, p, -1, -1, -1, -1);
            if (ns_unwinding) return;
            if (i >= 0) {
                ns_call(GF_event_free, 1, i);
                if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix battledead: member %d's kind %#x event cancelled\n", p, kind);
            }
        }
    }
}

/* ---------------------------------------------------------------- createback
 * Making a character (s037 18c8): Esc on the attribute points of the childhood (state 2) sets
 * state 1, the list of childhoods, though the childhood is applied (age 15); a row there takes the
 * adult path (356c) with the profession of that number. State 1 at 15 or more is reached only so:
 * the screen is drawn (053e) as state 3, the professions, as Esc on a profession's points (4 to 3)
 * does. Found by Arkana Mechanika (Darklands Restoration), B-126118. */
static void createback_hook(void)
{
    if ((s16)g_rw(GV_create_state) == 1 && g_rw(GV_create_age) >= 15) {
        g_ww(GV_create_state, 3);
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix createback: on to the professions\n");
    }
    hook_call_original();
}

/* $Cloth00 and the other guild quarters by day, "sneak in" (s053 1924:0a5e, s062 19fe:0a06,
   s063 1a3b:09cc, s064 190c:0a1a, s065 18fb:0a44, s065 1afe:09cc, s067 1869:06e6): for each able
   member, practice_skill(-2, that member's Stealth or Streetwise): the whole party practised once
   per able member. Each call is given its member (B-ba6123). Reached in a political season. */
static int sneak_next;
static void sneak_practice_hook(void)       /* practice_skill_impl (who, k, mode, pct, lvl) */
{
    if ((s16)rw(SS, (u16)(SP + 4)) == -2) {
        int size = (s16)g_rw(G_PARTY_SIZE);
        while (sneak_next < 5 && sneak_next < size && g_rb(G_MEMBER(sneak_next)) != 1) sneak_next++;
        if (sneak_next < 5 && sneak_next < size) {
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix cardslips: sneaking in, member %d practises skill %d\n", sneak_next, (s16)rw(SS, (u16)(SP + 6)));
            ww(SS, (u16)(SP + 4), (u16)sneak_next++);
        }
    }
    hook_call_original();
}
static void sneak_hook(void)
{
    sneak_next = 0;
    hook_install(GF_practice_skill_impl, sneak_practice_hook);
    hook_call_original();
    hook_remove(GF_practice_skill_impl, sneak_practice_hook);
}

/* ---------------------------------------------------------------- rkonce
 * A robber knight asked for (s039 1462:10b6 (who, castle, person, v, lvl, x)): the givers all pick
 * the castle nearest their town (nearest_loc_type), so asking again names the same knight; the
 * routine finds the task already given (event_find(8, -1, castle, -1, 3, -1)) and makes another
 * anyway. Here a task from the same giver of the same town for that castle stands: no second one. */
static void rkonce_hook(void)
{
    int who = (s16)rw(SS, (u16)(SP + 4)), castle = (s16)rw(SS, (u16)(SP + 6)), here = (s16)g_rw(G_CUR_LOC);
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i); if (!e) continue;
        u16 s = (u16)(e >> 16), o = (u16)e;
        if ((s16)rw(s, (u16)(o + G_EV_KIND)) != 8 || (s16)rw(s, (u16)(o + G_EV_TAG)) != 3 || (s16)rw(s, (u16)(o + G_EV_LOC)) != castle
            || (s16)rw(s, (u16)(o + G_EV_OWNER)) != who || (s16)rw(s, (u16)(o + G_EV_VALUE)) != here) continue;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix rkonce: giver %d already asked for the knight of %d (task %d)\n", who, castle, i);
        AX = 1;
        hook_return_far(0);
        return;
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- bestteacher
 * The inn finds a member's teacher by its skill (event_find(0x28, skill, town), the first hired):
 * with two teachers of one skill the first always taught. Here the best of them answers. */
static void bestteacher_hook(void)          /* event_find (kind, type, loc, owner, tag, f2a) */
{
    int kind = (s16)rw(SS, (u16)(SP + 4)), type = (s16)rw(SS, (u16)(SP + 6)), loc = (s16)rw(SS, (u16)(SP + 8));
    hook_call_original();
    if (ns_unwinding || kind != 0x28 || type < 0 || loc != (s16)g_rw(GV_cur_loc) || (s16)AX < 0) return;
    int b = teach_best_here(type);
    if (b >= 0 && b != (s16)AX) { if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix bestteacher: %s taught by teacher %d, not %d\n", gm_skill_name(type), b, (s16)AX); AX = (u16)b; }
}

/* ---------------------------------------------------------------- ruinkeep
 * $Raubr00, the burnt-out keep (s129 1942:0004): its first option, "investigate the ruin" (1942:021c),
 * shows the search ("... As you resume your journey ...") and an hour passes, but sets no next card:
 * the card's loop runs again from its first page. Here the journey resumes (next card 0x0c, as its
 * second option "continue on your journey", 1942:026c, sets) */
static void ruinkeep_hook(void)
{
    hook_call_original();
    if (ns_unwinding) return;
    g_ww(GV_next_card, 0x0c);
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix ruinkeep: searched, the journey resumes\n");
}

/* ---------------------------------------------------------------- minerelic
 * A mine's levels (s009): the deepest section of the dwarfs', kobolds' and vulcans' mines (0x0e..0x10,
 * 0x84..0x86, section 12; s009 182c:0db6) gets one item group of a random relic (items 0x96..0xab) through
 * 182c:1146 (group, item, item type, quality, number) with the type 0x9f and the quality 0x23 for any
 * relic, where the item list (DARKLAND.LST +0x1e, +0x26; bf_items, D_db35 based on D_dae3, while the
 * battle is built) gives each its own (qualities 50..99; the weapons among them typed as weapons).
 * Here they come from the list. */
static void minerelic_hook(void)            /* s009 182c:1146 (struct grp far *g, int item, char type, char quality, int n) */
{
    int item = (s16)rw(SS, (u16)(SP + 8));
    if (item >= 0x96 && item <= 0xab && (s16)rw(SS, (u16)(SP + 12)) == 0x23) {
        u16 sg = g_rw(0xdae3), o = (u16)(g_rw(0xdb35) + item * 0x2e);      /* (bf_items: DARKLAND.LST as read for the battle's build, based on D_dae3) */
        if (sg) {
            int type = (s16)rw(sg, (u16)(o + 0x1e)), q = rb(sg, (u16)(o + 0x26));
            ww(SS, (u16)(SP + 10), (u16)type); ww(SS, (u16)(SP + 12), (u16)q);
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix minerelic: relic %#x in the mine: type %#x, quality %d (not 0x9f, 35)\n", item, type, q);
        }
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- dwarfrelic
 * $MineH00 (s101 1957:0004), past an iron door's riddle of its fourth group (prev_screen 0xd8..0xde): "Further beyond
 * this door, Enemies linger, to Guard with their lives holy Treasure they've stole ... Try to regain it", the dwarfs
 * "always vulnerable to Christian symbols"; the script then picks a relic the party lacks (vec_207b(0x1000)), gives it
 * to the leader (add_item, s031 18e7:0128) and notes the mine as despoiled for good (event_new(-2, loc, 0x5a, 0x1c,
 * 7, 0x63, 0, 9999 h, 0, 0xc), its last argument the note's tag (+0x28); a later visit's text 8: "you've taken their
 * relic"). Here the relic is held back: the note is made with tag DR_TAG and the relic in its aux (+0x2a) (so loc_fact(0xc,
 * 7) does not find it, and the save keeps it), and the next loot of a fight at that place (a loot screen in the battle, not a chest's,
 * s023 1ef8:13d0) has the relic
 * in its pile; the note then becomes the game's own (tag 0xc). */
#define DR_TAG 0x7c
static int dr_pending, dr_chest;
static int card_named(const char *n) { char nm[16]; g_str(G_DGROUP, GV_card_name, nm, sizeof nm); return !strcasecmp(nm, n); }
/* the note of a relic waiting at loc in EVENTS.TMP (u16 count, then 0x30-byte records, as cards_shutdown wrote it): its relic,
   the note made the game's own there (tag 0xc), as cards_startup will read it back after the battle; 0 none */
static int dr_take_from_file(int loc)
{
    char p[1200]; dos_host_path("EVENTS.TMP", p, sizeof p);
    FILE *f = fopen(p, "r+b");
    if (!f) return 0;
    unsigned char c[2], r[0x30]; int code = 0;
    if (fread(c, 1, 2, f) == 2) {
        int n = c[0] | c[1] << 8;
        for (int i = 0; i < n && i < G_NEVENTS && !code; i++) {
            long at = ftell(f);
            if (fread(r, 1, 0x30, f) != 0x30) break;
            int kind = (s16)(r[0x22] | r[0x23] << 8), t0 = (s16)(r[0] | r[1] << 8), tag = (s16)(r[0x28] | r[0x29] << 8), l = (s16)(r[0x1c] | r[0x1d] << 8);
            if (kind != 0x1c || t0 != 7 || tag != DR_TAG || l != loc) continue;
            code = r[0x2a] | r[0x2b] << 8;
            r[0x28] = 0xc; r[0x29] = 0; r[0x2a] = 0; r[0x2b] = 0;
            fseek(f, at, SEEK_SET); fwrite(r, 1, 0x30, f);
        }
    }
    fclose(f);
    return code;
}
static void dwarfrelic_item_hook(void)      /* add_item_impl (who, code), s031 18e7:0128 */
{
    int code = (s16)rw(SS, (u16)(SP + 6));
    if (code >= 0x96 && code <= 0xab && card_named("$MineH00")) {
        dr_pending = code;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix dwarfrelic: relic %#x held back for the guards beyond the door\n", code);
        hook_return_far(0);
        return;
    }
    hook_call_original();
}
static void dwarfrelic_event_hook(void)     /* event_new (owner, loc, c, kind, type, f, g, hours, x, tag), s029 0e76:2c4e */
{
    if (dr_pending && (s16)rw(SS, (u16)(SP + 10)) == 0x1c && (s16)rw(SS, (u16)(SP + 12)) == 7 && (s16)rw(SS, (u16)(SP + 22)) == 0xc) {
        int loc = (s16)rw(SS, (u16)(SP + 6));
        for (int i = 0; i < G_NEVENTS; i++) {               /* (the door passed again before the fight: the relic already waits) */
            u32 e = g_event(i);
            if (e && g_ev(e, G_EV_KIND) == 0x1c && g_ev(e, G_EV_TYPE0) == 7 && g_ev(e, G_EV_TAG) == DR_TAG && g_ev(e, G_EV_LOC) == loc) { dr_pending = 0; AX = (u16)i; hook_return_far(0); return; }
        }
        int code = dr_pending; dr_pending = 0;
        ww(SS, (u16)(SP + 22), DR_TAG);                     /* (the last argument: the note's tag, +0x28) */
        hook_call_original();
        u32 e = (s16)AX >= 0 && !unw_active ? g_event((s16)AX) : 0;
        if (e) ww((u16)(e >> 16), (u16)((u16)e + G_EV_AUX), (u16)code);   /* (the relic waiting, in the note itself: a save keeps it) */
        return;
    }
    hook_call_original();
}
static void dwarfrelic_chest_hook(void) { dr_chest++; hook_call_original(); if (dr_chest > 0) dr_chest--; }   /* (chest_open, s023 1ef8:13d0: a chest's things, not a fight's) */
static void dwarfrelic_screen_hook(void)    /* loot_distribute (items far, u8 *count, cash far), s040 1462:0000: the loot screen's entry; after a
                                               fight at a place where a relic waits, it is in the pile (the native loot screen reads it from there) */
{
    int loc = (s16)g_rw(G_CUR_LOC);
    u16 off = rw(SS, (u16)(SP + 4)), sg = rw(SS, (u16)(SP + 6)), cp = rw(SS, (u16)(SP + 8));
    int n = g_rb(cp), code = 0;
    if (!dr_chest && g_rb(GV_in_battle) == 1 && loc >= 0 && sg && n < 200) {
        if (fixes_cards_down()) code = dr_take_from_file(loc);       /* (a battle has closed the cards: the party's notes are in EVENTS.TMP) */
        else for (int i = 0; i < G_NEVENTS && !code; i++) {
            u32 e = g_event(i);
            if (!e || g_ev(e, G_EV_KIND) != 0x1c || g_ev(e, G_EV_TYPE0) != 7 || g_ev(e, G_EV_TAG) != DR_TAG || g_ev(e, G_EV_LOC) != loc) continue;
            code = g_ev(e, G_EV_AUX);
            ww((u16)(e >> 16), (u16)((u16)e + G_EV_TAG), 0xc); ww((u16)(e >> 16), (u16)((u16)e + G_EV_AUX), 0);
        }
    }
    if (code >= 0x96 && code <= 0xab) {
        u16 o = (u16)(off + 6 * n);
        ww(sg, o, (u16)code); wb(sg, (u16)(o + 2), (u8)gm_itemdef_u8(code, 0x1e)); wb(sg, (u16)(o + 3), (u8)gm_itemdef_u8(code, 0x26));
        wb(sg, (u16)(o + 4), 1); wb(sg, (u16)(o + 5), (u8)gm_itemdef_u8(code, 0x25));
        g_wb(cp, (u8)(n + 1));
        fprintf(stderr, "fix dwarfrelic: the guards' relic %#x is in the loot\n", code);
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- minechests
 * A floor's cells (20a5:0000, 40 x 40 x 4 bytes) hold a furniture piece in their first byte's high nibble; in a mine's
 * set 5 is a closed chest and 6 an open one: the mine builder draws 5 at each chest it keeps (s009 1432: lv->cell[x6][x7]
 * |= 0x50, the chest record's +6, +7), and opening one turns it to 6 (chest_open, s023 1ef8:13d0). A layout's chests the
 * builder did not keep stay drawn, with no record (20a5:300c records of 0x2c bytes from 300e): nothing to open (his
 * Döfering mine, floor 5). Here, as a mine's floor comes in (level_switch, s024 1ef8:059a), a 5 or 6 that is no
 * record's picture is cleared. Only in a mine (the place's kind 6): elsewhere 5 and 6 are other furniture (a castle's
 * set has dozens). */
static void minechests_hook(void)
{
    hook_call_original();
    if (unw_active) return;
    int loc = (s16)g_rw(G_CUR_LOC);
    u32 lr = loc >= 0 && loc < G_NLOC ? g_location(loc) : 0;
    if (!lr || (s16)rw((u16)(lr >> 16), (u16)lr) != 6) return;
    static unsigned char pic[40][40];
    memset(pic, 0, sizeof pic);
    int nc = (s8)rb(G_FAR20A5, 0x300c);
    for (int k = 0; k < nc && k < 12; k++) {
        int x = rb(G_FAR20A5, (u16)(0x300e + 0x2c * k + 6)), y = rb(G_FAR20A5, (u16)(0x300e + 0x2c * k + 7));
        if (x < 40 && y < 40) pic[x][y] = 1;
    }
    int cleared = 0;
    for (int x = 0; x < 40; x++) for (int y = 0; y < 40; y++) {
        u16 o = (u16)((x * 40 + y) * 4);
        int b0 = rb(G_FAR20A5, o), piece = b0 >> 4;
        if ((piece == 5 || piece == 6) && !pic[x][y]) { wb(G_FAR20A5, o, (u8)(b0 & 0x0f)); cleared++; }
    }
    if (cleared && getenv("DL_FIX_LOG")) fprintf(stderr, "fix minechests: %d chest%s with nothing there not drawn\n", cleared, cleared == 1 ? "" : "s");
}

/* ---------------------------------------------------------------- carditems
 * A card gives things by kind through add_items (who, code, n), add_item (who, code) and add_item_q (who, code,
 * quality), s031 18e7:00e0, 0128, 016e, which fill the record's type, quality and weight from item_defs (2e38:0890),
 * the game's copy of DARKLAND.LST: a battle or a dungeon lays its own data over it, and a card right after one ($MineE00,
 * the miners' ten sacks once the mine is won: ten ingredients, 10 to 14 each) made records of the wrong type (72 as
 * type 5, not 109), which the party's lists then did not show. Here the record (add_item_rec_impl, its struct on the
 * stack: code, type, quality, count, weight) takes them from the list itself; add_item_q's quality stays. */
static int ci_kind;                         /* 1 a thing of the list's quality, 2 of the caller's */
static void carditems_n_hook(void)   { ci_kind = 1; hook_call_original(); ci_kind = 0; }
static void carditems_one_hook(void) { ci_kind = 1; hook_call_original(); ci_kind = 0; }
static void carditems_q_hook(void)   { ci_kind = 2; hook_call_original(); ci_kind = 0; }
static void carditems_rec_hook(void)        /* add_item_rec_impl (who, struct item) */
{
    if (ci_kind) {
        int code = rw(SS, (u16)(SP + 6)) & 0xfff, t = gm_itemdef_u8(code, 0x1e), w = gm_itemdef_u8(code, 0x25), q = gm_itemdef_u8(code, 0x26);
        if (t >= 0 && rb(SS, (u16)(SP + 8)) != (u8)t) {
            if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix carditems: item %d of type %d, not %d\n", code, t, rb(SS, (u16)(SP + 8)));
            wb(SS, (u16)(SP + 8), (u8)t);
            if (w >= 0) wb(SS, (u16)(SP + 11), (u8)w);
            if (ci_kind == 1 && q >= 0) wb(SS, (u16)(SP + 9), (u8)q);
        }
    }
    hook_call_original();
}

/* ---------------------------------------------------------------- rkcastle
 * The givers of robber knights (the councils, lords, merchants, the Hanse, the Fuggers, a village:
 * s039 1462:10b6 and its ten callers) name the knight of the castle nearest them: nearest_loc_type
 * (x, y, 2), s030 1462:2842, the nearest place of icon 2 not marked gone (+0x14 bit 2). With none on
 * the map it answers the place the party is in (cur_loc): a knight in the town itself, never to be
 * found, asked for again and again ("Robber knight @ Goslar", two at once: GitHub issue 5). The game
 * brings a new knight into a ruin only every third month (s029 0e76:281c: a place of icon 0x1a, the
 * burnt-out keep, becomes icon 2 again: +0x14 cleared, +0x0c 0x93, +0x0e 0x92, +8 4); a party killing
 * them faster runs out. Here, asked for a castle and finding none, a new knight moves into the ruin
 * nearest the place first, as that rule does. Tasks a game holds already for a town or a village
 * (kind 8, tag 3, and their facts, kind 0x1c tag 3, at the same place) go to such a keep. */
static int rk_dist(u32 q, int x, int y)
{
    int dx = abs((s16)rw((u16)(q >> 16), (u16)((u16)q + 4)) - x), dy = abs((s16)rw((u16)(q >> 16), (u16)((u16)q + 6)) - y) / 3;
    return dx >= dy ? dy / 2 + dx : dx / 2 + dy;
}
static int rk_icon(int i) { u32 r = i >= 0 && i < 414 ? g_location(i) : 0; return r ? (s16)rw((u16)(r >> 16), (u16)r) : -1; }
static int rk_nearest(int x, int y, int icon)          /* (as nearest_loc_type_impl: not a place marked gone) */
{
    int best = -1, bd = 1 << 30;
    for (int j = 0; j < 414; j++) {
        u32 q = g_location(j);
        if (!q || (s16)rw((u16)(q >> 16), (u16)q) != icon || (rb((u16)(q >> 16), (u16)((u16)q + 0x14)) & 4)) continue;
        int d = rk_dist(q, x, y);
        if (d < bd) { bd = d; best = j; }
    }
    return best;
}
static int rk_fell_lately(int loc)                    /* the game's word that the knight of loc fell (kind 0x1c, type 1, tag 3: 100 days) */
{
    for (int k = 0; k < G_NEVENTS; k++) {
        u32 f = g_event(k);
        if (f && g_ev(f, G_EV_KIND) == 0x1c && g_ev(f, G_EV_TYPE0) == 1 && g_ev(f, G_EV_TAG) == 3 && g_ev(f, G_EV_LOC) == loc) return 1;
    }
    return 0;
}
static int rk_new_knight(int x, int y)                 /* the ruin nearest x, y a robber knight's castle again; -1 none */
{
    int best = -1, bd = 1 << 30;                       /* (a keep whose knight fell within the last 100 days only when every ruin's did) */
    for (int j = 0; j < 414; j++) {
        u32 q = g_location(j);
        if (!q || (s16)rw((u16)(q >> 16), (u16)q) != 0x1a) continue;
        int d = rk_dist(q, x, y) + (rk_fell_lately(j) ? 1 << 20 : 0);
        if (d < bd) { bd = d; best = j; }
    }
    if (best < 0) return -1;
    u32 q = g_location(best); u16 qs = (u16)(q >> 16), qo = (u16)q;
    ww(qs, qo, 2); wb(qs, (u16)(qo + 0x14), 0); ww(qs, (u16)(qo + 0x0c), 0x93); ww(qs, (u16)(qo + 0x0e), 0x92); ww(qs, (u16)(qo + 8), 4);
    fprintf(stderr, "fix rkcastle: no robber knight's castle left: one moves into the burnt-out keep %d\n", best);
    return best;
}
static void rkcastle_hook(void)             /* nearest_loc_type_impl (x, y, icon) */
{
    int x = (s16)rw(SS, (u16)(SP + 4)), y = (s16)rw(SS, (u16)(SP + 6)), icon = (s16)rw(SS, (u16)(SP + 8));
    hook_call_original();
    if (ns_unwinding || icon != 2) return;
    int r = (s16)AX;
    if (r >= 0 && rk_icon(r) == 2) return;
    int c = rk_new_knight(x, y);
    if (c >= 0) AX = (u16)c;
}
static void rkcastle_frame(void)
{
    static int tick;
    if (++tick % 120) return;
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i);
        if (!e || g_ev(e, G_EV_KIND) != 8 || g_ev(e, G_EV_TAG) != 3) continue;
        int town = g_ev(e, G_EV_LOC), ic = rk_icon(town);
        if (ic != 0 && ic != 8) continue;                            /* (a city or a village: never a knight's) */
        u32 t = g_location(town);
        int x = (s16)rw((u16)(t >> 16), (u16)((u16)t + 4)), y = (s16)rw((u16)(t >> 16), (u16)((u16)t + 6));
        int c = rk_nearest(x, y, 2);
        if (c < 0) c = rk_new_knight(x, y);
        if (c < 0) continue;
        for (int k = 0; k < G_NEVENTS; k++) {
            u32 f = g_event(k);
            if (f && (g_ev(f, G_EV_KIND) == 8 || g_ev(f, G_EV_KIND) == 0x1c) && g_ev(f, G_EV_TAG) == 3 && g_ev(f, G_EV_LOC) == town) g_ev_set(f, G_EV_LOC, c);
        }
        fprintf(stderr, "fix rkcastle: the robber knight asked for in place %d now lives at %d\n", town, c);
    }
}

/* ---------------------------------------------------------------- knightdead
 * The robber knight's tower ($Raubi03, s080 1929:0006, card 0x93) and the cards of sneaking up ($Raubi04,
 * s081 1838:0000, 0x94) and of a saint's help ($Raubi05, s081 18bb:0004, 0x95) never ask whether the knight
 * still lives. Each way of killing him burns the keep (the place's icon 0x1a, gone, its own card 0x157, the
 * burnt-out keep) and sends the party on its way (0x0c), but the tower's card came round again before the
 * party left the place, knight and all: "Sneak in, set on fire, kill knight in combat, repeat", a bounty
 * each time (GitHub issue 7). Here those cards, at a keep already burnt, give way to the place's own card. */
static void knightcard_hook(void)
{
    int loc = (s16)g_rw(G_CUR_LOC);
    u32 r = loc >= 0 && loc < 414 ? g_location(loc) : 0;
    if (!r || (s16)rw((u16)(r >> 16), (u16)r) != 0x1a) { hook_call_original(); return; }
    int c = (s16)rw((u16)(r >> 16), (u16)((u16)r + 0x0c));
    if (c <= 0) c = 0x0c;
    g_ww(GV_next_card, (u16)c);
    fprintf(stderr, "fix knightdead: the knight of %d fell and his keep burnt: its own card %#x\n", loc, c);
    AX = (u16)c;
    hook_return_far(0);
}

/* ---------------------------------------------------------------- alertonce
 * At the robber knight's tower, challenging him, sneaking up and other tries each leave word that the castle
 * is on its guard (event_here(0x26, 6480 hours): 270 days, "ask politely" greyed), a new event each time
 * beside the last. Enough tries (the tower's card coming round again, knightdead) filled the game's 300
 * events, and every new one then failed: "EVENT Failure" (event_slot_free, s029; GitHub issue 8). Here a new
 * one replaces the place's last, and a saved game's copies, and those of the word that the knight fell (kind
 * 0x1c type 1 tag 3, one a kill), are dropped as it loads, the one ending last kept. */
static int alert_same(u32 e, int kind, int type0, int loc, int tag)
{
    return e && g_ev(e, G_EV_KIND) == kind && g_ev(e, G_EV_TYPE0) == type0 && g_ev(e, G_EV_OWNER) == -2 && g_ev(e, G_EV_LOC) == loc && g_ev(e, G_EV_TAG) == tag;
}
static void alertonce_hook(void)            /* event_here (kind, hours), s029 0e76:2930 */
{
    int kind = (s16)rw(SS, (u16)(SP + 4)), loc = (s16)g_rw(G_CUR_LOC);
    if (kind == 0x26)
        for (int i = 0; i < G_NEVENTS && !ns_unwinding; i++)
            if (alert_same(g_event(i), 0x26, 0, loc, 0)) { ns_call(GF_event_free, 1, i); if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix alertonce: the guard at %d renewed (event %d)\n", loc, i); }
    if (!ns_unwinding) hook_call_original();
}
static long ev_end_hours(u32 e)
{
    static const int DAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int h = g_ev(e, G_EV_END), d = g_ev(e, G_EV_END + 2), mo = g_ev(e, G_EV_END + 4), y = g_ev(e, G_EV_END + 6);
    long days = (long)y * 365;
    for (int i = 0; i < mo && i < 12; i++) days += DAYS[i];
    return (days + d) * 24 + h;
}
/* ---------------------------------------------------------------- kingdoor
 * The mine of the dwarf king with the red hat (the quest's way 0, vec_2193 = s039 1462:16b4; its mine
 * card $MineS00, s055 18c7:02b8, builds battlefield 0x0b) has three iron doors with a riddle each, drawn
 * when the mine is built (s009 182c:1cb4 (0x0b, section)): section 3's from $MineT00..T11 (cards 0x5c,
 * 0x71, 0xc5..0xce), section 10's from T12..T23 (0xcf..0xda), section 8's, the king's, from T24..T35
 * (0xdb..0xe0 with 0xe0 twice, 0xe2, 0x153..0x156; checked by calling it). A riddle solved leads by its
 * own number: T00..T11 to $MineB00 (0xe3, the dwarf's demands), T12..T25 to $MineB02 (0xe5, "You meet
 * ... king of the troublesome dwarfs", go away), T26..T35 to $MineB03 (0xe6, "It is ... again", the red
 * hat). So the king's door brought B02 for T24 and T25, 2 of its 12; brushing past him there (-3) opened
 * it onto a passage the builder leaves without a section beyond (a ladder back to the same floor) and
 * turned its trigger off, the king gone; B03 then came only from $MineB01's box when slamming it fails,
 * never with the treasure taken (kind 0x1c tag 12 type 0xb), which its "offer the king his treasure
 * back" needs (GitHub issue 9). Here B02 coming straight from T24 or T25 (prev_screen) gives way to B03.
 * (The other ways' mines lead from those riddles to $MineH00 or $MineR07, never to B02.) */
static void kingdoor_hook(void)             /* $MineB02, s101 189a:0002 */
{
    int prev = (s16)g_rw(GV_prev_screen);
    if (prev != 0xdb && prev != 0xdc) { hook_call_original(); return; }
    g_ww(GV_next_card, 0xe6);
    fprintf(stderr, "fix kingdoor: the riddle %#x of the king's own door: the king again ($MineB03), not the first meeting\n", prev);
    AX = 0xe6;
    hook_return_far(0);
}

static void alertonce_after_load(void)      /* (saves_on_loaded) */
{
    if (!fix_on("alertonce")) return;
    int freed = 0;
    for (int i = 0; i < G_NEVENTS; i++) {
        u32 e = g_event(i);
        int kind = e ? g_ev(e, G_EV_KIND) : 0, type0 = e ? g_ev(e, G_EV_TYPE0) : 0, tag = e ? g_ev(e, G_EV_TAG) : 0;
        if (!e || !((kind == 0x26 && type0 == 0 && tag == 0) || (kind == 0x1c && type0 == 1 && tag == 3)) || g_ev(e, G_EV_OWNER) != -2) continue;
        int loc = g_ev(e, G_EV_LOC);
        for (int k = i + 1; k < G_NEVENTS; k++) {
            u32 f = g_event(k);
            if (!alert_same(f, kind, type0, loc, tag)) continue;
            int drop = ev_end_hours(f) > ev_end_hours(e) ? i : k;       /* (the one ending last stays) */
            ns_call(GF_event_free, 1, drop); freed++;
            if (ns_unwinding) return;
            if (drop == i) break;
        }
    }
    if (freed) fprintf(stderr, "fix alertonce: %d copies of a castle's guard or of a fallen knight dropped from the saved game\n", freed);
}

/* ---------------------------------------------------------------- dragonhide
 * $Drago01, the dragon seen in its land (s081 19bc): its first option, "hide, then follow it back to
 * its lair" (and "hide, wait until the crack cools, and enter its lair"), asks its chance of 19bc:0a40:
 * (member_attr(-1, 5) + member_skill(-1, 11)) / 2, the leader's Charisma and Speak Latin, clamped 0..99
 * (AX the chance, DX the leader). No other check pairs them (s076's priest adds Religion, Charisma,
 * Latin and Virtue); the card's other options use the numbers they mean (Riding 17 for riding away), so
 * an old numbering is unlikely. Here: the party's average Stealth (party_avg_skill(15)) and its best
 * Woodwise (party_max_skill(18)), half the sum, clamped as before; the leader still the one named. */
static void dragonhide_hook(void)           /* s081 19bc:0a40 () -> AX chance, DX who */
{
    int st = ns_call(GF_party_avg_skill, 1, 15); if (ns_unwinding) return;
    int ww_ = ns_call(GF_party_max_skill, 1, 18); if (ns_unwinding) return;
    int c = (st + ww_) / 2; if (c < 0) c = 0; if (c > 99) c = 99;
    if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix dragonhide: Stealth %d, Woodwise %d: %d%%\n", st, ww_, c);
    AX = (u16)c; DX = (u16)(s16)(s8)g_rb(GV_leader);
    hook_return_far(0);
}

/* ---------------------------------------------------------------- ownplace
 * The place a step on the overland map enters (s030 1462:1300 (mx, my), the marker's place on the
 * screen): the terrain under it from the view buffer (an icon, 50 and up, counts as 28; the
 * terrain kept in D_d843), then the first location by index within rr tiles of it, rr 4 on terrain
 * 29 (a town's or village's) and 1 elsewhere: in the game's columns x' (the view's column + the
 * marker's) the window is x'-rr-1 .. x'+rr, and so on for the rows; a location's own tile is
 * x'-1, y'-1. Four tiles took in places beside a village that come earlier in the list: Lauf
 * (197, 48,693), west of Speyer, led into the Rammelsburg mine (178, 52,695). Here, on terrain 29,
 * a location whose own tile the step is on is entered rather than another one near it. */
static void ownplace_hook(void)             /* s030 1462:1300 (mx, my): the place entered, or none */
{
    int mx = (s16)rw(SS, (u16)(SP + 4)), my = (s16)rw(SS, (u16)(SP + 6));
    hook_call_original();
    if (unw_active) return;
    int r = (s16)AX;
    if (r < 0 || r >= G_NLOC || (s16)g_rw(0xd843) != 29) return;
    int row = (my + 11) / 4, y1 = row + (s16)g_rw(GV_map_buf_row) + (s16)g_rw(GV_map_view_row);
    int col = (((y1 - 1) % 2) ? mx + 16 : mx + 24) / 16, x1 = (s16)g_rw(GV_map_view_col) + col;
    int tx = x1 - 1, ty = y1 - 1;
    u32 p = g_location(r);
    if (!p || ((s16)rw((u16)(p >> 16), (u16)(p + 4)) == tx && (s16)rw((u16)(p >> 16), (u16)(p + 6)) == ty)) return;   /* (the game's is on this tile) */
    for (int i = 0; i < G_NLOC; i++) {
        u32 q = g_location(i);
        if (!q || rw((u16)(q >> 16), (u16)(q + 0x0c)) == 0x62) continue;
        if ((s16)rw((u16)(q >> 16), (u16)(q + 4)) != tx || (s16)rw((u16)(q >> 16), (u16)(q + 6)) != ty) continue;
        if (getenv("DL_FIX_LOG")) fprintf(stderr, "fix ownplace: tile %d,%d is location %d's own, not %d's\n", tx, ty, i, r);
        g_ww(GV_cur_loc, (u16)i); AX = (u16)i;
        return;
    }
}

static const struct { int sect; unsigned seg, off; void (*fn)(void); } HOOKS[] = {
    { 29, 0x0e76, 0x4c04, blessing_hook }, { 14, 0x18bf, 0x0778, hitdamage_hook }, { 31, 0x18e7, 0x01b4, additem_hook },
    { 14, 0x0e76, 0x08fe, castle_hook }, { 80, 0x1870, 0x000c, bounty_hook }, { 33, 0x1462, 0x1630, alchemy_hook },
    { 31, 0x18e7, 0x01b4, additem_hook }, { 36, 0x1462, 0x0082, innsave_hook }, { -2, 0, 0, 0 }, { 41, 0x1462, 0x0000, copyprot_hook },
    { 14, 0x18bf, 0x0778, hitdamage_hook }, { -2, 0, 0, 0 }, { 30, 0x1462, 0x1c76, offmap_hook }, { 29, 0x0e76, 0x01c0, bestweapon_hook }, { 31, 0x18e7, 0x0406, removeitem_hook },
    { 35, 0x1462, 0x0a46, storeroom_hook }, { 44, 0x1838, 0x0000, testcard_hook },
    { 19, 0x1ef8, 0x0968, armourwear_hook }, { 36, 0x1462, 0x0082, teachers_lodging_hook }, { GF_card_options, cardopts_hook },
    { GF_set_member_bit, patron_hook }, { 108, 0x1925, 0x0438, debtnote_hook }, { 29, 0x0e76, 0x3fb2, alchrisk_hook },
    { 56, 0x19c4, 0x0ac2, canon_hook }, { 30, 0x1462, 0x28f2, sites_hook }, { 108, 0x1838, 0x07ca, tomb_hook },
    { 50, 0x1a06, 0x026c, lord_hook }, { GF_use_potion, potion_fixes_hook }, { GF_use_potion, potion_fixes_hook },
    { 37, 0x1462, 0x2ace, standings_save_hook }, { -2, 0, 0, 0 }, { GF_heal_attr_pct, healboost_hook },
    { GF_camp_spend_day, teachcap_hook }, { GF_hit_damage, oils_hit_hook }, { 39, 0x165c, 0x5cca, paul_hook },
    { 39, 0x165c, 0x41fa, nepomuk_hook }, { -2, 0, 0, 0 }, { 29, 0x0e76, 0x21aa, nightbowyer_hook },
    { 57, 0x1838, 0x03cc, confession_hook }, { 29, 0x0e76, 0x3f30, appointment_hook }, { GF_damage_queue, pileup_hook }, { 7, 0x15af, 0x1d78, castlefreeze_hook }, { 56, 0x18c0, 0x0494, university_hook }, { GF_add_item_rec_impl, partyitems_hook }, { GF_item_quality_flags_impl, itemflags_hook },
    { 57, 0x1838, 0x067c, mv_wrap_hook }, { 114, 0x18d3, 0x041c, hussite_hook }, { 47, 0x1a44, 0x05f0, nightwall_all_hook },
    { 36, 0x1462, 0x1b80, campguard_hook }, { GF_groups_fixup, enemygroups_hook }, { GF_loc_hostile_impl, cityalert_hook },
    { 36, 0x1462, 0x197e, dayjobs_hook }, { GF_cloud_blocks_line, cloudline_hook }, { GF_party_max_attr_sum, attrsum_hook },
    { 14, 0x19cd, 0x33c0, lootcount_hook }, { 7, 0x15af, 0x0698, corridors_hook }, { GF_party_delete, battledead_hook },
    { GF_create_draw, createback_hook }, { 129, 0x1942, 0x021c, ruinkeep_hook }, { 39, 0x1462, 0x10b6, rkonce_hook }, { GF_event_find, bestteacher_hook }, { 9, 0x182c, 0x1146, minerelic_hook }, { 30, 0x1462, 0x1300, ownplace_hook }, { -2, 0, 0, 0 }, { 30, 0x1462, 0x2842, rkcastle_hook }, { 81, 0x19bc, 0x0a40, dragonhide_hook },
    { 80, 0x1929, 0x0006, knightcard_hook }, { GF_event_here, alertonce_hook }, { 101, 0x189a, 0x0002, kingdoor_hook }, { 31, 0x18e7, 0x0128, dwarfrelic_item_hook }, { GF_level_switch, minechests_hook }, { GF_add_item_rec_impl, carditems_rec_hook }, { GF_saint_finish, prayfor_hook }, { 30, 0x1462, 0x1c76, westedge_hook } };   /* (hitsound: mods/sfx.c) */
_Static_assert(sizeof HOOKS / sizeof HOOKS[0] == sizeof FIXES / sizeof FIXES[0], "a hook (or { -2 }) for each fix, in the same order");
/* further hooks that belong to a fix */
static const struct { const char *fix; int sect; unsigned seg, off; void (*fn)(void); } MORE[] = {
    { "dwarfrelic", GF_event_new, dwarfrelic_event_hook }, { "dwarfrelic", GF_chest_open, dwarfrelic_chest_hook }, { "dwarfrelic", 40, 0x1462, 0x0000, dwarfrelic_screen_hook },
    { "carditems", GF_add_items_impl, carditems_n_hook }, { "carditems", 31, 0x18e7, 0x0128, carditems_one_hook }, { "carditems", GF_add_item_q, carditems_q_hook },
    { "knightdead", 81, 0x1838, 0x0000, knightcard_hook }, { "knightdead", 81, 0x18bb, 0x0004, knightcard_hook },
    { "itemcount", 14, 0x0e76, 0x2324, falls_hook }, { "itemcount", 14, 0x18bf, 0x0fa4, useitem_hook },
    { "teachers", 29, 0x0e76, 0x324c, teachers_next_hook }, { "armorwear", 19, 0x1ef8, 0x0914, wornout_hook },
    { "blessing", 29, 0x0e76, 0x448a, blessing_end_hook },
    { "bishop", 39, 0x1462, 0x1712, errand_hook }, { "bishop", 56, 0x19c4, 0x0e0a, relic_hook },
    { "armorday", 39, 0x165c, 0x2052, dunstan_hook }, { "armorday", 39, 0x165c, 0x5b82, patrick_hook },
    { "armorday", GF_hit_damage, armour_hit_hook }, { "armorday", 39, 0x165c, 0x1868, crispin_hook },
    { "armorday", 39, 0x165c, 0x125a, charity_hook }, { "armorday", 39, 0x165c, 0x2628, erasmus_hook },
    { "armorday", 39, 0x165c, 0x5a84, pantaleon_hook }, { "armorday", 39, 0x165c, 0x6122, polycarp_hook },
    { "prayfor", GF_saint_screen, prayfor_screen_hook },
    { "pauls", 39, 0x165c, 0x5d6c, paul_simple_hook }, { "saintrep", GF_loc_reputation_adj, repadj_hook },
    { "standings", 37, 0x1c7d, 0x0d24, standings_load_hook },
    { "membervalue", 71, 0x1838, 0x02a8, mv_wrap_hook }, { "membervalue", GF_party_max_skill, mv_query_hook },
    { "membervalue", GF_party_min_attr, mv_query_hook },
    { "cardslips", 53, 0x1838, 0x0aa8, footpads_potion_hook }, { "cardslips", 51, 0x18c9, 0x05d0, stables_hook },
    { "cardslips", 77, 0x1996, 0x0378, schulz_hook }, { "cardslips", 118, 0x1a2b, 0x02ac, templar_hook },
    { "cardslips", 106, 0x19bc, 0x03e8, tatzel_hook }, { "cardslips", 130, 0x190c, 0x033e, hurt_hook },
    { "nightwall", 47, 0x1a44, 0x03e2, nightwall_rope_hook },
    { "cardslips", 80, 0x1870, 0x000c, raubi_hook }, { "cardslips", 134, 0x1838, 0x0000, spider_man_hook }, { "cardslips", 101, 0x18bd, 0x0000, kingname_hook }, { "cardslips", 134, 0x1838, 0x0550, hurt_hook }, { "cardslips", 134, 0x1838, 0x0692, hurt_hook },
    { "cardslips", 58, 0x191a, 0x01f4, gate_rep_hook }, { "cardslips", 58, 0x191a, 0x09ba, gate_rep_hook },
    { "cardslips", 51, 0x19f2, 0x0792, docks_hint_hook }, { "cardslips", 51, 0x19f2, 0x09a6, docks_hint_hook },
    { "cardslips", 51, 0x19f2, 0x0bba, docks_hint_hook }, { "cardslips", 51, 0x19f2, 0x0dd0, docks_hint_hook },
    { "cardslips", 51, 0x19f2, 0x0fe4, docks_hint_hook }, { "cardslips", 46, 0x194b, 0x0736, grove_hook },
    { "cardslips", 53, 0x1924, 0x0a5e, sneak_hook }, { "cardslips", 62, 0x19fe, 0x0a06, sneak_hook },
    { "cardslips", 63, 0x1a3b, 0x09cc, sneak_hook }, { "cardslips", 64, 0x190c, 0x0a1a, sneak_hook },
    { "cardslips", 65, 0x18fb, 0x0a44, sneak_hook }, { "cardslips", 65, 0x1afe, 0x09cc, sneak_hook },
    { "cardslips", 67, 0x1869, 0x06e6, sneak_hook },
    { "patronsaint", 39, 0x165c, 0x17a4, teach_saint_hook }, { "patronsaint", 39, 0x165c, 0x1988, teach_saint_hook },
    { "fullpack", GF_barter_key, buy_room_hook }, { "fullpack", 35, 0x1462, 0x0a46, store_get_room_hook },
    { "westedge", 30, 0x1462, 0x20d2, westedge_view_hook } };

void fix_enable(int i, int on)
{
    FIXES[i].on = on;
    char key[64]; snprintf(key, sizeof key, "fix.%s", FIXES[i].id);
    config_set_int(key, on);
    int shared = 0;                         /* another fix with the same hook still on (hooks of different fixes on one function chain) */
    for (int k = 0; k < FIX_COUNT; k++)
        if (k != i && FIXES[k].on && HOOKS[k].fn == HOOKS[i].fn && HOOKS[k].sect == HOOKS[i].sect && HOOKS[k].seg == HOOKS[i].seg && HOOKS[k].off == HOOKS[i].off) shared = 1;
    for (unsigned k = 0; k < sizeof MORE / sizeof MORE[0]; k++)
        if (!strcmp(MORE[k].fix, FIXES[i].id)) { if (on) hook_install(MORE[k].sect, MORE[k].seg, MORE[k].off, MORE[k].fn); else hook_remove(MORE[k].sect, MORE[k].seg, MORE[k].off, MORE[k].fn); }
    if (!strcmp(FIXES[i].id, "sainttexts")) sainttexts_enable(on);                /* (its hooks are its own) */
    if (!HOOKS[i].fn) return;                /* checked every frame (fixes_frame), no hook */
    if (on) { if (!hook_install(HOOKS[i].sect, HOOKS[i].seg, HOOKS[i].off, HOOKS[i].fn)) fprintf(stderr, "fix %s: no hook point\n", FIXES[i].id); }
    else if (!shared) hook_remove(HOOKS[i].sect, HOOKS[i].seg, HOOKS[i].off, HOOKS[i].fn);
}

/* debugging (DL_CHECK_COPY): far copies (_fmemcpy, root 098c4) that would reach the stack */
static void copy_check_hook(void)
{
    u16 doff = rw(SS, (u16)(SP + 4)), dseg = rw(SS, (u16)(SP + 6)), soff = rw(SS, (u16)(SP + 8)), sseg = rw(SS, (u16)(SP + 10)), n = rw(SS, (u16)(SP + 12));
    if (dseg == SS && doff == 0xa895) fprintf(stderr, "pile copy: %u bytes (%u items) from %04x:%04x\n", n, n / 6, sseg, soff);
    if (dseg == SS && n && (u32)doff + n > (u32)SP - 0x200) {
        fprintf(stderr, "COPY INTO THE STACK: %04x:%04x <- %04x:%04x, %u bytes (SP %04x, return %04x:%04x)\n", dseg, doff, sseg, soff, n, SP, rw(SS, (u16)(SP + 2)), rw(SS, SP));
        ftrace_dump();
    }
    hook_call_original();
}

void fixes_init(void)
{
    saves_on_loaded(colours_after_load); saves_on_loaded(alertonce_after_load);
    if (getenv("DL_CHECK_COPY")) hook_install(-1, 0x06a1, 0x2eb4, copy_check_hook);
    hook_install(GF_cards_shutdown, cards_down_hook);   /* (the potions kept over a battle: they ask any_potion_fix) */
    hook_install(GF_cards_startup, cards_up_hook);
    for (int i = 0; i < FIX_COUNT; i++) {
        char key[64]; snprintf(key, sizeof key, "fix.%s", FIXES[i].id);
        fix_enable(i, config_get_int(key, FIXES[i].on));
    }
}
