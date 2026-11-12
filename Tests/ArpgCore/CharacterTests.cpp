#include "ArpgCore/Character/CharacterStats.h"
#include "TestFramework.h"

using namespace arpg;

ARPG_TEST(Character_AllClassesStartValid)
{
	for (int i = 0; i < static_cast<int>(ECharacterClass::Count); ++i)
	{
		const CharacterProgress P = CreateCharacter(static_cast<ECharacterClass>(i));
		const DerivedStats S = ComputeBaseStats(P.Class, P.Level, P.Attr);
		EXPECT_EQ(P.Level, 1);
		EXPECT_TRUE(S.MaxLife >= 40 && S.MaxMana >= 10 && S.AttackRating > 0);
		// Todas las clases reparten los mismos 85 puntos iniciales.
		EXPECT_EQ(P.Attr.Strength + P.Attr.Dexterity + P.Attr.Vitality + P.Attr.Energy, 85);
	}
}

ARPG_TEST(Character_ClassIdentity)
{
	const CharacterProgress W = CreateCharacter(ECharacterClass::Warrior);
	const CharacterProgress S = CreateCharacter(ECharacterClass::Sorcerer);
	const DerivedStats Ws = ComputeBaseStats(W.Class, 1, W.Attr);
	const DerivedStats Ss = ComputeBaseStats(S.Class, 1, S.Attr);
	EXPECT_TRUE(Ws.MaxLife > Ss.MaxLife);
	EXPECT_TRUE(Ss.MaxMana > Ws.MaxMana);
}

ARPG_TEST(Character_VitalityAndLevelRaiseLife)
{
	CharacterProgress P = CreateCharacter(ECharacterClass::Warrior);
	const int32 Base = ComputeBaseStats(P.Class, 1, P.Attr).MaxLife;
	P.Attr.Vitality += 10;
	EXPECT_EQ(ComputeBaseStats(P.Class, 1, P.Attr).MaxLife, Base + 40);
	EXPECT_EQ(ComputeBaseStats(P.Class, 11, P.Attr).MaxLife, Base + 40 + 25);
}

ARPG_TEST(Character_ExperienceLevelsUpAndCarriesRemainder)
{
	CharacterProgress P = CreateCharacter(ECharacterClass::Hunter);
	const int64 L1 = ExperienceToNextLevel(1), L2 = ExperienceToNextLevel(2);
	EXPECT_EQ(L1, 200);
	EXPECT_EQ(AddExperience(P, L1 - 1), 0);
	EXPECT_EQ(AddExperience(P, 1 + L2 + 7), 2);
	EXPECT_EQ(P.Level, 3);
	EXPECT_EQ(P.Experience, 7);
	EXPECT_EQ(P.UnspentAttributePoints, 10);
	EXPECT_EQ(P.UnspentSkillPoints, 2);
	EXPECT_EQ(AddExperience(P, 0), 0);
	EXPECT_EQ(AddExperience(P, -50), 0);
	EXPECT_EQ(P.Experience, 7);
}

ARPG_TEST(Character_LevelCapHolds)
{
	CharacterProgress P = CreateCharacter(ECharacterClass::Paladin);
	AddExperience(P, INT64_MAX / 2);
	EXPECT_EQ(P.Level, MaxCharacterLevel);
	EXPECT_EQ(P.Experience, 0);
	EXPECT_EQ(ExperienceToNextLevel(MaxCharacterLevel), 0);
	EXPECT_EQ(AddExperience(P, 1000), 0);
	EXPECT_EQ(P.UnspentAttributePoints, (MaxCharacterLevel - 1) * AttributePointsPerLevel);
}

ARPG_TEST(Character_SpendAttributePointsValidates)
{
	CharacterProgress P = CreateCharacter(ECharacterClass::Assassin);
	EXPECT_TRUE(!SpendAttributePoints(P, EAttribute::Strength, 1)); // sin puntos
	AddExperience(P, ExperienceToNextLevel(1));
	const int32 Dex = P.Attr.Dexterity;
	EXPECT_TRUE(!SpendAttributePoints(P, EAttribute::Dexterity, 6));
	EXPECT_TRUE(!SpendAttributePoints(P, EAttribute::Dexterity, 0));
	EXPECT_TRUE(!SpendAttributePoints(P, EAttribute::Dexterity, -3));
	EXPECT_TRUE(SpendAttributePoints(P, EAttribute::Dexterity, 5));
	EXPECT_EQ(P.Attr.Dexterity, Dex + 5);
	EXPECT_EQ(P.UnspentAttributePoints, 0);
}
