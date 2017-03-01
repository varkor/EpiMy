% Test the unification operator (=/2) works correctly.
test :-	=(1, 1),
		=(X, 1),
		=(A, B),
		=(C, D), =(C, abc), =(D, abc),
		=(f(E, def), f(def, F)), =(E, def), =(F, def),
		\+ =(1, 2),
		\+ =(g(G), f(f(G))),
		\+ =(f(H, 1), f(a(H))),
		\+ =(f(I, J, I), f(a(I), a(J), J, 2)),
		=(f(K, L, M), f(g(L, L), g(M, M), g(N, N))), =(K, g(g(g(N, N), g(N, N)), g(g(N, N), g(N, N)))), =(L, g(g(N, N), g(N, N))), = (M, g(N, N)).