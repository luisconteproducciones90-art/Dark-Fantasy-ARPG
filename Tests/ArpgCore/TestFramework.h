// Mini framework de pruebas sin dependencias (ni excepciones ni RTTI).
#pragma once

#include <cstdio>
#include <vector>

namespace arpgtest
{
	struct TestCase { const char* Name; void (*Fn)(); };
	inline std::vector<TestCase>& Registry() { static std::vector<TestCase> R; return R; }
	inline int& Failures() { static int F = 0; return F; }
	struct Registrar { Registrar(const char* Name, void (*Fn)()) { Registry().push_back({Name, Fn}); } };
}

#define ARPG_TEST(Name) \
	static void Name(); \
	static arpgtest::Registrar Name##_registrar(#Name, &Name); \
	static void Name()

#define EXPECT_TRUE(Cond) \
	do { if (!(Cond)) { ++arpgtest::Failures(); \
		std::printf("    FALLO %s:%d: %s\n", __FILE__, __LINE__, #Cond); } } while (0)

#define EXPECT_EQ(A, B) EXPECT_TRUE((A) == (B))
