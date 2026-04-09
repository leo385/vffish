#include "../external/include/Unity/unity.h"
#include "basic_math.h"

void setUp(void) {}
void tearDown(void) {}

void test_addFloats(void) {
	float fNum1 = 1.0f;
	float fNum2 = 2.0f;
	TEST_ASSERT_EQUAL_FLOAT(3.0f, addFloats(fNum1, fNum2));
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_addFloats);
	return UNITY_END();
}