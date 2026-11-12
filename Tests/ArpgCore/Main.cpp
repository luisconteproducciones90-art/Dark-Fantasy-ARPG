#include "TestFramework.h"

int main()
{
	int Failed = 0;
	for (const arpgtest::TestCase& Test : arpgtest::Registry())
	{
		const int Before = arpgtest::Failures();
		Test.Fn();
		const bool bOk = arpgtest::Failures() == Before;
		std::printf("[%s] %s\n", bOk ? " OK " : "FALLO", Test.Name);
		Failed += bOk ? 0 : 1;
	}
	std::printf("%zu pruebas, %d fallidas\n", arpgtest::Registry().size(), Failed);
	return Failed == 0 ? 0 : 1;
}
