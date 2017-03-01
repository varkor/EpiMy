// Test methods can be inherited and overridden correctly.

class Base {
	func overridden() {
		return 2;
	}
	func inherited() {
		return 1;
	}
}

class Sub: Base {
	func overridden() {
		return 4;
	}
}

func test() {
	var sub = new Sub;
	var base = new Base;

	if (sub.overridden() != 4) {
		return 0;
	}
	if (sub.inherited() != 1) {
		return 0;
	}
	if (base.overridden() != 2) {
		return 0;
	}
	if (base.inherited() != 1) {
		return 0;
	}
	return 1;
};