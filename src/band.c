


typedef struct {
  const char *band;
  double lower_freq;   // Lower bound of the band in MHz
  double upper_freq;   // Upper bound of the band in MHz
} band_t;

// Lookup table for bands
const band_t band_table[] = {
  { "2190m", 0.1357, 0.1378 },
  { "630m", 0.472, 0.479 },
  { "560m", 0.501, 0.504 },
  { "160m", 1.8, 2.0 },
  { "80m", 3.5, 4.0 },
  { "60m", 5.06, 5.45 },
  { "40m", 7.0, 7.3 },
  { "30m", 10.1, 10.15 },
  { "20m", 14.0, 14.35 },
  { "17m", 18.068, 18.168 },
  { "15m", 21.0, 21.45 },
  { "12m", 24.89, 24.99 },
  { "10m", 28.0, 29.7 },
  { "6m", 50.0, 54.0 },
  { "4m", 70.0, 71.0 },
  { "2m", 144.0, 148.0 },
  { "1.25m", 222.0, 225.0 },
  { "70cm", 420.0, 450.0 },
  { "33cm", 902.0, 928.0 },
  { "23cm", 1240.0, 1300.0 },
  { "13cm", 2300.0, 2450.0 },
  { "9cm", 3300.0, 3500.0 },
  { "6cm", 5650.0, 5925.0 },
  { "3cm", 10000.0, 10500.0 },
  { "1.25cm", 24000.0, 24250.0 },
  { "6mm", 47000.0, 47200.0 },
  { "4mm", 75500.0, 81000.0 },
  { "2.5mm", 119980.0, 120020.0 },
  { "2mm", 142000.0, 149000.0 },
  { "1mm", 241000.0, 250000.0 }
};



// Function to find the band based on frequency
const char *band_find(double frequency_mhz) {
  int band_count = sizeof(band_table) / sizeof(band_table[0]);

  for (int i = 0; i < band_count; i++) {
    if (frequency_mhz >= band_table[i].lower_freq && frequency_mhz <= band_table[i].upper_freq) {
      return band_table[i].band;
    }
  }

  return "Unknown";   // Return "Unknown" if the frequency does not match any band
}


int band_get_edges(double frequency_mhz, double *lower, double *upper) {
  int band_count = sizeof(band_table) / sizeof(band_table[0]);

  for (int i = 0; i < band_count; i++) {
    if (frequency_mhz >= band_table[i].lower_freq && frequency_mhz <= band_table[i].upper_freq) {
      *lower = band_table[i].lower_freq;
      *upper = band_table[i].upper_freq;
      return 0;  // found
    }
  }

  return -1;  // not found
}
