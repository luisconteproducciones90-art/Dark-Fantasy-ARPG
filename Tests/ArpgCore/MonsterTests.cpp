#include "ArpgCore/Enemy/Monster.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Monster_TableCoversEveryBehavior)
{
	bool Seen[static_cast<int>(EMonsterBehavior::Count)] = {};
	for (int32 i = 0; i < GetMonsterDefCount(); ++i)
	{
		const MonsterDef* Def = GetMonsterDef(i);
		EXPECT_EQ(FindMonsterDef(Def->Id), i);
		EXPECT_TRUE(Def->BaseLife > 0 && Def->BaseDamage.Min <= Def->BaseDamage.Max && Def->BaseExperience > 0);
		Seen[static_cast<int>(Def->Behavior)] = true;
	}
	for (bool bSeen : Seen) { EXPECT_TRUE(bSeen); }
	EXPECT_TRUE(GetMonsterDef(-1) == nullptr && FindMonsterDef("nadie") == -1 && FindMonsterDef(nullptr) == -1);
}

ARPG_TEST(Monster_ScalesWithLevelRankAndDifficulty)
{
	const int32 Walker = FindMonsterDef("ash_walker");
	const MonsterInstance L1 = SpawnMonster(Walker, 1, EMonsterRank::Normal, EDifficulty::Pilgrim);
	const MonsterInstance L20 = SpawnMonster(Walker, 20, EMonsterRank::Normal, EDifficulty::Pilgrim);
	const MonsterInstance Boss = SpawnMonster(Walker, 20, EMonsterRank::Boss, EDifficulty::Pilgrim);
	const MonsterInstance Hard = SpawnMonster(Walker, 20, EMonsterRank::Normal, EDifficulty::Condemned);

	EXPECT_EQ(L1.Body.MaxLife, 30);
	EXPECT_EQ(L1.Attack.Damage[0].Min, 3);
	EXPECT_EQ(L1.Attack.Damage[0].Max, 6);
	EXPECT_EQ(L20.Body.MaxLife, 30 + 12 * 19);
	EXPECT_TRUE(L20.Attack.Damage[0].Max > L1.Attack.Damage[0].Max * 3);
	EXPECT_EQ(Boss.Body.MaxLife, L20.Body.MaxLife * 15);
	EXPECT_EQ(Boss.StatusDurationPercent, 40);
	EXPECT_TRUE(Boss.Experience == L20.Experience * 20);
	EXPECT_EQ(Hard.Body.MaxLife, L20.Body.MaxLife * 3);
	EXPECT_EQ(Hard.Defense.ResistPercent[1], L20.Defense.ResistPercent[1] + 30);
	EXPECT_TRUE(Hard.Attack.Damage[0].Max > L20.Attack.Damage[0].Max * 2);

	EXPECT_EQ(SpawnMonster(-1, 5, EMonsterRank::Normal, EDifficulty::Pilgrim).DefIndex, -1);
	const MonsterInstance Extreme = SpawnMonster(Walker, INT32_MAX, EMonsterRank::Boss, EDifficulty::Condemned);
	EXPECT_TRUE(Extreme.Level == 99 && Extreme.Body.MaxLife > 0 && Extreme.Attack.Damage[0].Max > 0);

	// Los que hacen daño elemental atacan como hechizo y con su tipo.
	const MonsterInstance Bloat = SpawnMonster(FindMonsterDef("bloat"), 5, EMonsterRank::Normal, EDifficulty::Pilgrim);
	EXPECT_TRUE(Bloat.Attack.bIsSpell && Bloat.Attack.Damage[static_cast<int>(EDamageType::Fire)].Max > 0);
	EXPECT_EQ(Bloat.Attack.Damage[0].Max, 0);
}

ARPG_TEST(Monster_KillExperienceDecaysWhenOverleveled)
{
	EXPECT_EQ(ComputeKillExperience(100, 10, 10), 100);
	EXPECT_EQ(ComputeKillExperience(100, 15, 10), 100);
	EXPECT_EQ(ComputeKillExperience(100, 16, 10), 85);
	EXPECT_EQ(ComputeKillExperience(100, 5, 30), 100);
	EXPECT_EQ(ComputeKillExperience(100, 80, 1), 5);
	EXPECT_EQ(ComputeKillExperience(3, 80, 1), 1);
	EXPECT_EQ(ComputeKillExperience(0, 10, 10), 0);
}

ARPG_TEST(Monster_LootFollowsRankRules)
{
	Rng R(2026);
	const int32 Walker = FindMonsterDef("ash_walker");
	const MonsterInstance Normal = SpawnMonster(Walker, 5, EMonsterRank::Normal, EDifficulty::Pilgrim);
	const MonsterInstance Elite = SpawnMonster(Walker, 5, EMonsterRank::Elite, EDifficulty::Pilgrim);
	const MonsterInstance Boss = SpawnMonster(Walker, 5, EMonsterRank::Boss, EDifficulty::Pilgrim);

	int NormalItems = 0, NormalGold = 0;
	for (int i = 0; i < 4000; ++i)
	{
		const LootDrop Drop = RollLoot(Normal, 0, R);
		EXPECT_TRUE(Drop.ItemCount <= 1);
		NormalItems += Drop.ItemCount;
		NormalGold += Drop.Gold > 0 ? 1 : 0;
		for (int32 k = 0; k < Drop.ItemCount; ++k)
		{
			// Nivel 5: nada que exija más nivel del que tiene el monstruo.
			EXPECT_TRUE(GetItemBase(Drop.Items[k].BaseIndex)->RequiredLevel <= 5);
			EXPECT_EQ(Drop.Items[k].ItemLevel, 5);
		}
	}
	EXPECT_TRUE(NormalItems > 850 && NormalItems < 1150);  // ~25 %
	EXPECT_TRUE(NormalGold > 2250 && NormalGold < 2550);   // ~60 %

	for (int i = 0; i < 500; ++i)
	{
		const LootDrop E = RollLoot(Elite, 0, R);
		EXPECT_TRUE(E.ItemCount >= 2 && E.ItemCount <= 3 && E.Gold > 0);
		const LootDrop B = RollLoot(Boss, 0, R);
		EXPECT_TRUE(B.ItemCount >= 4 && B.ItemCount <= MaxDropItems && B.Gold > E.Gold / 4);
		EXPECT_TRUE(B.Items[0].Rarity >= ERarity::Rare); // garantía de jefe
	}
	EXPECT_EQ(RollLoot(MonsterInstance{}, 0, R).ItemCount, 0);
}

ARPG_TEST(Monster_AiBehavioursDiffer)
{
	AiContext Far; Far.bHasTarget = true; Far.DistanceCm = 900; Far.AttackRangeCm = 150;
	AiContext Near = Far; Near.DistanceCm = 100;
	AiContext NoTarget;

	EXPECT_TRUE(DecideAction(EMonsterBehavior::MeleeChaser, NoTarget) == EAiAction::Idle);
	EXPECT_TRUE(DecideAction(EMonsterBehavior::MeleeChaser, Far) == EAiAction::Approach);
	EXPECT_TRUE(DecideAction(EMonsterBehavior::MeleeChaser, Near) == EAiAction::Attack);

	AiContext Bow; Bow.bHasTarget = true; Bow.AttackRangeCm = 1200; Bow.DistanceCm = 800;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::RangedKiter, Bow) == EAiAction::Attack);
	Bow.DistanceCm = 200;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::RangedKiter, Bow) == EAiAction::Retreat);
	Bow.DistanceCm = 2000;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::RangedKiter, Bow) == EAiAction::Approach);

	AiContext Charge = Far; Charge.bSpecialReady = true;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Charger, Charge) == EAiAction::UseSpecial);
	Charge.DistanceCm = 5000; // demasiado lejos para embestir
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Charger, Charge) == EAiAction::Approach);
	Charge.DistanceCm = 100;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Charger, Charge) == EAiAction::Attack);

	AiContext Hidden = Far; Hidden.bRevealed = false;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Ambusher, Hidden) == EAiAction::StayHidden);
	Hidden.DistanceCm = 250;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Ambusher, Hidden) == EAiAction::UseSpecial);
	Hidden.bRevealed = true;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Ambusher, Hidden) == EAiAction::Approach);
	AiContext HiddenAlone; HiddenAlone.bRevealed = false;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Ambusher, HiddenAlone) == EAiAction::StayHidden);

	EXPECT_TRUE(DecideAction(EMonsterBehavior::Exploder, Far) == EAiAction::Approach);
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Exploder, Near) == EAiAction::UseSpecial);

	AiContext Caster; Caster.bHasTarget = true; Caster.AttackRangeCm = 900; Caster.DistanceCm = 600;
	Caster.bSpecialReady = true;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Summoner, Caster) == EAiAction::UseSpecial);
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Support, Caster) == EAiAction::Attack);
	Caster.bAllyNeedsHelp = true;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Support, Caster) == EAiAction::UseSpecial);
	Caster.DistanceCm = 100; Caster.bAllyNeedsHelp = false;
	EXPECT_TRUE(DecideAction(EMonsterBehavior::Summoner, Caster) == EAiAction::Retreat);
}

ARPG_TEST(Monster_FullFightIsDeterministic)
{
	// Guerrero de nivel 1 sin equipo contra un caminante de ceniza: misma semilla, mismo desenlace.
	int32 Turns[2] = {};
	for (int Run = 0; Run < 2; ++Run)
	{
		Rng R(4242);
		MonsterInstance M = SpawnMonster(FindMonsterDef("ash_walker"), 1, EMonsterRank::Normal, EDifficulty::Pilgrim);
		AttackProfile Player;
		Player.Damage[0] = {3, 7}; Player.AttackRating = 104; Player.Level = 1;
		while (M.Body.IsAlive() && Turns[Run] < 1000)
		{
			PerformAttack(Player, M.Defense, M.Body, R);
			++Turns[Run];
		}
		EXPECT_TRUE(!M.Body.IsAlive());
	}
	EXPECT_EQ(Turns[0], Turns[1]);
	EXPECT_TRUE(Turns[0] >= 5 && Turns[0] < 40);
}
