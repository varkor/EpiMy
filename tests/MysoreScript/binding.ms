// Test closures bind variables in outer scopes.

func A() {
	var str = "B";
	func B() {
		return str;
	};
	str = "A";
	return B;
};

func test() {
	var b = A();
	var str = b();
	return str.compare("B") == 0;
};