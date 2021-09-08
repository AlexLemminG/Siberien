#include "Reflect.h"
#include "SerializedObject.h"


class Test {
	int i;
	Test() {
		SerializedObject s;


		i += s.GetInt();
	}
};