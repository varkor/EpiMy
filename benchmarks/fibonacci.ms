func fib(n) {
	if (n < 2) {
		return n;
	}
	var fa = fib(n - 1);
	var fb = fib(n - 2);
	return fa + fb;
};

fib(24);
