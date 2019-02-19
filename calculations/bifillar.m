#geometry of the coil
D = 28E-3

#wire diameted
Dw = 2.4E-3

#turns
n = 20

#material
mu_r = 1

l = 2 * n * Dw

#constants
mu_0 = 4 * pi * 1E-7

A = D^2 * pi / 4

L = mu_0 * mu_r * n^2 * A / l

#pitch
p = l / n

#Wire length
wl = n * sqrt(((D + Dw) * pi)^2 + p^2)


