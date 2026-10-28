# 卡片文本生成

`make_cards.mjs` 从**两层**译表生成 `mods/chinese/MSGFILES`：

1. **共享表** `D:\dkch\zh\work\translations.tsv` —— 上游项目（C# 重写版）的译表，约 7700 行。本 fork 不拥有它，**不编辑**。
2. **覆盖表** `tools/zh/overrides.tsv` —— 本 fork 自己的表。优先级高于共享表（`overrides.get(en) ?? map.get(en)`），一行覆盖表就赢过一行共享表。

## 覆盖表为什么存在

共享表有几类问题，`--dry` 都报不出来——它只报「没有对应行」的 run：

- 4 行的中文列是**德语**（`pay the toll and enter.` → `Zahlung des Mauts und Eintreten.`）；
- 十余行长正文的中文列是**英文原文**（翻译步骤失败，原样复制）；
- 249 个 run **没有对应行**。

## $变量

卡片里的 `$his`、`$he`、`$PlaceName` 这类是**游戏变量**，运行时由游戏自己展开（`$his` → `his`，`$PlaceName` → 地名）。所以：

- 译表必须**原样保留**它们，一个都不能翻译；
- 共享表把 `$his` 写成「你的」、`$he` 写成「你」（444 行），这会在屏幕上留下英文代词。

`make_cards.mjs` 的 `restoreVars()` 在写卡前把代词变量的替代词（你的 / 她的 / 你 / 她）还原成 `$变量`。它只替换该 span，所以同句里普通英文 `your` 的「你的」不受影响。

生成后核对：卡片里 92 个 `$变量`，0 丢失。

## 用法

```
node tools/zh/make_cards.mjs            # 生成
node tools/zh/make_cards.mjs --dry      # 只报告未译 run
```

生成之后还要两步，否则游戏读到的仍是旧卡：

```
Copy-Item D:\dkplusch\mods\chinese\* D:\dkplusch\build\port_win64\mods\chinese -Recurse -Force
Remove-Item D:\goggame\Darklands\DARKLAND\mods\.cache\* -Recurse -Force
```
