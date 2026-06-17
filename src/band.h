#pragma once

const char *band_find(double frequency_mhz);
int band_get_edges(double frequency_mhz, double *lower, double *upper);
