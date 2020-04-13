#geometry of the coil
D = 48.0E-3
#l = 36E-3

#wire diameted
Dw = 1.5E-3

#Inductance
L = 78E-6

#material
mu_r = 1

mu = 4 * pi * 1E-7 * mu_r;

A = D^2 * pi / 4;

n = L * Dw / mu / A


function calcL(n, mu, D, Dw)
	l = n * Dw;
	A = D^2 * pi / 4;

	L = mu * n^2 * A / l;
	printf("\nL = %G n=%d l=%d\n", L, n, l);

	#pitch
	p = l / n

	#Wire length
	wl = n * sqrt(((D + Dw) * pi)^2 + p^2)
endfunction


calcL(floor(n), mu, D, Dw)
calcL(ceil(n), mu, D, Dw)
