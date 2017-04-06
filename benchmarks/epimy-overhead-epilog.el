iterate(0).
iterate(N) :- is(M, +(N, -1)), iterate(M).

?- iterate(100000).
