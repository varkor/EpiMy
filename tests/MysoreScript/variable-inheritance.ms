// Test methods can be inherited correctly.
class Base {
	var a;
	var b;
}

class Sub: Base {
	var c;
	var d;
	func getValues() {
		return a + b + c + d;
	}
	func setValues() {
		a = "a";
		b = "b";
		c = "c";
		d = "d";
	}
}

func test() {
	var s = new Sub;
	s.setValues();
	var values = s.getValues();
	if (values.compare("abcd") != 0) {
		return 0;
	}
	return 1;
};