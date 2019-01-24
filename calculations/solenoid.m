#geometry of the coil
D = 47E-3
l = 120E-3

#wire diameted
Dw = 1.5E-3

#turns
n = 30

#material
mu_r = 1

#constants
mu_0 = 4 * pi * 1E-7

A = D^2 * pi / 4

L = mu_0 * mu_r * n^2 * A / l

#pitch
p = l / n

#Wire length
wl = n * sqrt(((D + Dw) * pi)^2 + p^2)


