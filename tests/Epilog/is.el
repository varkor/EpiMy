% Test the is (,/2) operator works correctly.
test :-	is(Result, +(6, 8)), =(Result, 14),
		=(X, +(1, 2)), is(Y, *(X, 3)), =(X, +(1, 2)), =(Y, 9),
		\+is(seventy_seven, 77),
		\:is(77, N),
		\:is(77, seventy_seven).