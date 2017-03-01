% Test the and (,/2) operator works correctly.
tt :- true, true.
tf :- true, false.
ft :- false, true.
ff :- false, false.

test :-	tt,
		\+tf,
		\+ft,
		\+ff.