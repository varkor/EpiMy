// Check that classes can have custom operator behaviour.

class A {
	var value;
	func init(v) {
		value = v;
	}
	func getValue() {
		return value;
	}
	func add(other) {
		var result = new A;
		var v = value + other.getValue();
		result.init(v);
		return result;
	}
}

func test() {
	var a1 = new A;
	a1.init(4);
	if (a1.getValue() != 4) {
		return 0;
	}
	var a2 = a1 + a1;
	// Addition should not modify the original instance.
	if (a1.getValue() != 4) {
		return 0;
	}
	// Check that the addition was correct.
	if (a2.getValue() != 8) {
		return 0;
	}
	return 1;
};