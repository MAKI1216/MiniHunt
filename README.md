# MiniHunt: 追猎 (The Hunter)

<p align="center">
  <img src="https://img.shields.io/badge/Engine-Unreal%20Engine%205.2.1-white?style=for-the-badge&logo=unrealengine&logoColor=white&color=0E1128" />
  <img src="https://img.shields.io/badge/Genre-Multiplayer%20FPS%20PvPvE-red?style=for-the-badge" />
</p>

> **“那是人类文明的黄昏，却是‘它们’的黎明。”**
>
> 在大崩塌后的废墟中，你是旧世界的拾荒者，也是新世界的亡命徒。深入禁区，在利爪与背叛的缝隙中寻找财富。欢迎加入狩猎，或者，成为猎物。

---

## 📽️ 演示与截图 (Gallery)

### 📺 视频演示
[![Watch the video](https://img.shields.io/badge/Video-Gameplay_Demo-red?style=for-the-badge&logo=youtube)](https://www.acfun.cn/v/ac48237028)

### 📸 游戏截图
<p align="center">
  <img src="picture/main.jpg" width="45%" alt="游戏主视觉" />
  <img src="picture/chose.png" width="45%" alt="角色选择" />
</p>
<p align="center">
  <img src="picture/battle.png" width="45%" alt="战斗画面" />
  <img src="picture/rank.png" width="45%" alt="排名结算" />
</p>

---

## 🎮 核心玩法 (Gameplay)

`MiniHunt` 是一款基于 **Unreal Engine 5** 开发的局域网多人博弈 FPS 游戏。玩家将扮演猎人在地图中击杀魔兽，寻找物资，与其他猎手竞争物资。

### ⚔️ 战术竞技机制
* **多人对战**：支持最多 4 名玩家通过局域网同场竞技。
* **角色选择**：提供不同的猎手。
* **随机空降**：每局随机降落点，确保每场狩猎的未知性。

### 🏆 胜利目标：资源争夺战
在 **6分钟** 的极限行动中，积累并成功提交最多的资源。

* **积累资本**：搜集废墟物资或猎杀 AI 魔兽。
* **落袋为安**：资源点必须运输至 **【马车提交点】** 才计入最终成绩。
* **黑暗森林 (PvPvE)**：
    * 击杀玩家可掠夺其身上 **50%** 未提交资源。
    * 死亡惩罚：扣除 **50%** 携带资源并在出生点重生。

---

## ✨ 核心功能 (Features)

- [x] **网络同步**：基于 UE5 C++ 的多人联机同步。
- [x] **角色系统**：不同猎手供玩家选择。
- [x] **武器系统**：多样的枪械手感与弹药资源管理。
- [x] **AI 系统**：具有攻击欲望的异体魔兽行为树。
- [x] **UI 交互**：包含实时排名、倒计时及资源反馈系统。

---

## 🛠️ 技术栈 (Tech Stack)

* **引擎**: Unreal Engine 5.2.1
* **脚本**: C++ / Blueprints
* **同步**: Listen Server 架构

---