#include "skr-utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Assume this function exists to extract data from the structs.
// In a real implementation, this would be defined elsewhere to calculate
// the 3D distance between satellite position vectors.
double get_distance_between_sats(sat_t *s1, sat_t *s2);


// #############################################################################
// #                                CONSTANTS                                  #
// #############################################################################

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Physical Constants ---
#define EARTH_RADIUS_KM 6371.0
#define ATMOSPHERE_THICKNESS_KM 20.0

// --- Finite Size Limit Security Parameters (from paper Table 3) ---
#define FINITE_SIZE_D 5.0
#define FINITE_SIZE_ES 2.0e-10
#define FINITE_SIZE_E 1.0e-9
#define FINITE_SIZE_N 1.0e11

// --- Laser Repetition Rate (from paper) ---
#define LASER_REPETITION_RATE 50.0e6 // 50 MHz

// --- FER Empirical Coefficients (from paper) ---
#define FER_M1 0.8218
#define FER_M2 -19.46
#define FER_M3 -298.1

// --- Beta Empirical Coefficients (from paper Table 4) ---
// For MLC-MSD Reconciliation
#define BETA_MLC_C1 0.9655
#define BETA_MLC_C2 0.0001507
#define BETA_MLC_C3 -0.04696
#define BETA_MLC_C4 -0.2238
// For MD Reconciliation
#define BETA_MD_C1 -0.0825
#define BETA_MD_C2 0.1834
#define BETA_MD_C3 0.9821
#define BETA_MD_C4 -0.00002815

// #############################################################################
// #                             HELPER FUNCTIONS                              #
// #############################################################################

/**
 * @brief Numerical approximation for the inverse error function (erfinv).
 * This is not in the standard C math library. This implementation is a
 * rational approximation from "Approximating the erfinv function" by W. J. Cody.
 * @param y A value between -1.0 and 1.0.
 * @return The inverse error function of y.
 */
static double erfinv(double y) {
    if (y < -1.0 || y > 1.0) {
        return NAN; // Not a number for out-of-range input
    }
    if (fabs(y) == 1.0) {
        return y * INFINITY;
    }

    double a[4] = {0.886226899, -1.645349621, 0.914624893, -0.140543331};
    double b[4] = {-2.118377725, 1.442710462, -0.329097515, 0.012229801};
    double c[4] = {-1.970840454, -1.624906493, 3.429567803, 1.641345311};
    double d[2] = {3.543889200, 1.637067800};

    double y_abs = fabs(y);
    double x, z;

    if (y_abs <= 0.7) {
        z = y * y;
        double num = ((a[3] * z + a[2]) * z + a[1]) * z + a[0];
        double den = (((b[3] * z + b[2]) * z + b[1]) * z + 1.0);
        x = y * num / den;
    } else {
        z = sqrt(-log((1.0 - y_abs) / 2.0));
        double num = ((c[3] * z + c[2]) * z + c[1]) * z + c[0];
        double den = (d[1] * z + d[0]) * z + 1.0;
        x = num / den;
    }

    // Two steps of Newton-Raphson refinement
    for (int i = 0; i < 2; i++) {
        x = x - (erf(x) - y) / ((2.0 / sqrt(M_PI)) * exp(-x * x));
    }
    
    return x;
}


// Helper to convert degrees to radians
static double to_radians(double degrees) {
    return degrees * M_PI / 180.0;
}

/**
 * @brief Calculates the great-circle distance between two points on Earth.
 * @param lat1 Latitude of point 1 in decimal degrees.
 * @param lon1 Longitude of point 1 in decimal degrees.
 * @param lat2 Latitude of point 2 in decimal degrees.
 * @param lon2 Longitude of point 2 in decimal degrees.
 * @return The distance in kilometers.
 */
static double haversine_distance(double lat1, double lon1, double lat2, double lon2) {
    double r = EARTH_RADIUS_KM;
    double phi1 = to_radians(lat1);
    double phi2 = to_radians(lat2);
    double delta_phi = to_radians(lat2 - lat1);
    double delta_lambda = to_radians(lon2 - lon1);

    double a = sin(delta_phi / 2.0) * sin(delta_phi / 2.0) +
               cos(phi1) * cos(phi2) *
               sin(delta_lambda / 2.0) * sin(delta_lambda / 2.0);
    double c = 2 * atan2(sqrt(a), sqrt(1.0 - a));

    return r * c;
}


/**
 * @brief Shannon entropy function g(x).
 * @param x The input value.
 * @return The calculated entropy (x+1)log2(x+1) - x*log2(x).
 * Formula from paper and secret_key_rate.py.
 */
static double entropy(double x) {
    if (x <= 0) {
        return 0.0;
    }
    return (x + 1.0) * log2(x + 1.0) - x * log2(x);
}

/**
 * @brief Calculates the total link distance between a ground station and a satellite.
 * @param elev_angle_deg Elevation angle of the satellite in degrees.
 * @param ogs_alt_km Altitude of the Optical Ground Station (OGS) in km.
 * @param sat_alt_km Altitude of the satellite in km.
 * @return The total link distance in km.
 * Based on link_distance function in STG.py and paper.
 */
static double calculate_link_distance(double elev_angle_deg, double ogs_alt_km, double sat_alt_km) {
    double r_e = EARTH_RADIUS_KM;
    double l_ogs = ogs_alt_km;
    double l_zen = sat_alt_km; // Use actual satellite altitude
    double theta_rad = to_radians(elev_angle_deg);

    // Prevent math errors for angles at or below the horizon
    if (cos(theta_rad) * (r_e + l_ogs) / (r_e + l_zen) >= 1.0) {
        return sqrt(pow(l_zen - l_ogs, 2)); // Simplified vertical distance
    }

    double angle_rad = asin(cos(theta_rad) * (r_e + l_ogs) / (r_e + l_zen));
    double central_angle_rad = M_PI / 2.0 - theta_rad - angle_rad;

    double l_tot_sq = pow(r_e + l_zen, 2) + pow(r_e + l_ogs, 2) - 2 * (r_e + l_zen) * (r_e + l_ogs) * cos(central_angle_rad);
    
    return sqrt(l_tot_sq);
}

/**
 * @brief Calculates the effective atmospheric path length.
 * @param elev_angle_deg Elevation angle of the satellite in degrees.
 * @param ogs_alt_km Altitude of the OGS in km.
 * @return The effective atmospheric distance in km.
 * Based on atmosphere_length function in STG.py and paper.
 */
static double calculate_atmosphere_length(double elev_angle_deg, double ogs_alt_km) {
    double r_e = EARTH_RADIUS_KM;
    double l_ogs = ogs_alt_km;
    double l_atm = ATMOSPHERE_THICKNESS_KM;
    double theta_rad = to_radians(elev_angle_deg);

    // Prevent math errors for angles at or below the horizon
    if (cos(theta_rad) * (r_e + l_ogs) / (r_e + l_atm) >= 1.0) {
        return l_atm - l_ogs; // Simplified vertical distance
    }

    double angle_rad = asin(cos(theta_rad) * (r_e + l_ogs) / (r_e + l_atm));
    double central_angle_rad = M_PI / 2.0 - theta_rad - angle_rad;

    double l_atm_eff_sq = pow(r_e + l_atm, 2) + pow(r_e + l_ogs, 2) - 2 * (r_e + l_atm) * (r_e + l_ogs) * cos(central_angle_rad);

    return sqrt(l_atm_eff_sq);
}


/**
 * @brief Core function to calculate the finite size secret key rate.
 *
 * @param T Transmittance of the channel (0 to 1).
 * @param alpha Modulation amplitude (sqrt(VA/2)).
 * @param ksi Excess noise (in SNU).
 * @param R_method Reconciliation method ("MD" or "MLC-MSD").
 * @return The secret key rate in bits per second (bit/s).
 * Implements the logic from key_rate_finite in STG.py and IS_SKR.py.
 */
static double calculate_skr_finite(double T, double alpha, double ksi, const char* R_method) {
    if (T <= 0) return 0.0;

    double t = sqrt(T);
    double VA = 2 * alpha * alpha;

    // Use Gaussian approximation for correlation coefficient Z, as seen in
    // the gaussian_key_rate functions in the python files.
    // Z = 2*t*sqrt(alpha^4 + alpha^2) -> Z_norm = Z/t = 2*sqrt(alpha^4 + alpha^2)
    double Z_norm = 2 * sqrt(pow(alpha, 4) + pow(alpha, 2));

    // Covariance matrix elements A, B, C (from secret_key_rate.py)
    double A = 1.0 + VA;
    double B = 1.0 + T * (VA + ksi);
    double C = t * Z_norm;

    // Symplectic invariants
    double Delta = A * A + B * B - 2 * C * C;
    double Det = pow(A * B - C * C, 2);

    if (Delta * Delta - 4 * Det < 0) return 0.0;

    double nu1 = sqrt(0.5 * (Delta + sqrt(Delta * Delta - 4 * Det)));
    double nu2 = sqrt(0.5 * (Delta - sqrt(Delta * Delta - 4 * Det)));
    // nu3 for heterodyne detection (from paper and python code)
    double nu3 = A - (C * C) / (B + 1.0);

    // Holevo Bound chi(B;E)
    double chiBE = entropy((nu1 - 1.0) / 2.0) + entropy((nu2 - 1.0) / 2.0) - entropy((nu3 - 1.0) / 2.0);

    // Mutual Information I(A;B) for heterodyne detection
    double IAB = log2(1.0 + T * VA / (2.0 + T * ksi));

    // --- Finite Size Corrections ---

    // dn (privacy amplification term)
    double dn = (pow(FINITE_SIZE_D + 1.0, 2) +
                 4.0 * (FINITE_SIZE_D + 1.0) * sqrt(log2(2.0 / FINITE_SIZE_ES)) +
                 2.0 * log2(2.0 / (FINITE_SIZE_ES * FINITE_SIZE_E * FINITE_SIZE_E)) +
                 (4.0 * FINITE_SIZE_ES * FINITE_SIZE_D) / (FINITE_SIZE_E * sqrt(FINITE_SIZE_N))) / sqrt(FINITE_SIZE_N);

    // SNR in dB
    double SNR_linear = T * VA / (2.0 + T * ksi);
    if (SNR_linear <= 0) return 0.0;
    double SNR_dB = 10.0 * log10(SNR_linear);

    // Frame Error Rate (FER)
    double fer_arg = FER_M2 * SNR_dB + FER_M3;
    double FER = 0.5 * (1.0 + FER_M1 * atan(fer_arg));
    FER = fmax(0.0, fmin(1.0, FER));

    // Reconciliation efficiency (beta)
    double beta_val;
    if (strcmp(R_method, "MD") == 0) {
        beta_val = BETA_MD_C1 * exp(BETA_MD_C2 * SNR_dB) + BETA_MD_C3 * exp(BETA_MD_C4 * SNR_dB);
    } else { // Default to MLC-MSD
        beta_val = BETA_MLC_C1 * exp(BETA_MLC_C2 * SNR_dB) + BETA_MLC_C3 * exp(BETA_MLC_C4 * SNR_dB);
    }
    // Python code normalizes by 100
    beta_val /= 100.0;
    beta_val = fmax(0.0, fmin(1.0, beta_val));
    
    // Final SKR
    double key_rate = LASER_REPETITION_RATE * ((1.0 - FER) * beta_val * IAB - chiBE - dn);

    return (key_rate > 0.0) ? key_rate : 0.0;
}


/**
 * @brief Calculates the transmittance for a satellite-to-ground link.
 * This is a helper function for both uplink and downlink.
 * @return The channel transmittance (0 to 1).
 */
static double calculate_stg_transmittance(double D_t, double D_r, double elev_angle_deg, double sat_alt_km, double ogs_alt_km, gint is_uplink) {
    // --- Configurable Parameters ---
    const double lambda = 1550e-9;      // Wavelength (m)
    const double T_t_eff = 0.9;         // Transmitter Optics Efficiency
    const double T_r_eff = 0.9;         // Receiver Optics Efficiency
    const double L_p = 0.1;             // Pointing Loss
    const double C_n2 = 1e-16;          // Refractive Index Structure Param (Good weather)
    const double V_vis = 200.0;         // Visibility (km) (Good weather)
    const double p_thr = 1e-6;          // Outage time fraction

    if (elev_angle_deg <= 0) return 0.0;

    // Calculate geometric and atmospheric path lengths
    double L_tot_km = calculate_link_distance(elev_angle_deg, ogs_alt_km, sat_alt_km);
    double L_atm_eff_km = calculate_atmosphere_length(elev_angle_deg, ogs_alt_km);
    double L_tot_m = L_tot_km * 1000.0;
    double L_atm_eff_m = L_atm_eff_km * 1000.0;
    
    // --- Loss Calculations (from STG.py and paper) ---
    
    // 1. Geometric Loss
    double geo_loss_dB = 10 * log10((pow(L_tot_m, 2) * pow(lambda, 2)) / (pow(D_t, 2) * pow(D_r, 2) * T_t_eff * (1 - L_p) * T_r_eff));

    // 2. Mie Scattering (Kim Model)
    double p = 1.3; // For V > 6km
    double mie_scat_per_km_dB = 10.0 * log10(exp(1.0)) * (3.912 / V_vis) * pow(lambda / 550e-9, -p);
    double mie_scat_total_dB = mie_scat_per_km_dB * L_atm_eff_km;

    // 3. Scintillation Loss
    // Rytov variance (integral approximation)
    double k_wavenum = 2.0 * M_PI / lambda;
    double rytov_var = 2.25 * pow(k_wavenum, 7.0/6.0) * C_n2 * (6.0/11.0) * pow(L_atm_eff_m, 11.0/6.0);
    
    // Scintillation index (spherical wave for downlink)
    double d_scint = D_r * sqrt(M_PI / (2.0 * lambda * L_atm_eff_m));
    double sig_R2 = rytov_var;
    double term1_num = 0.20 * sig_R2;
    double term1_den = pow(1.0 + 0.18 * d_scint*d_scint + 0.20 * pow(sig_R2, 6.0/5.0), 7.0/6.0);
    double term2_num = 0.21 * sig_R2 * pow(1.0 + 0.24 * pow(sig_R2, 6.0/5.0), -5.0/6.0);
    double term2_den = 1.0 + 0.90 * d_scint*d_scint + 0.21 * d_scint*d_scint * pow(sig_R2, 6.0/5.0);
    double sig_I2 = exp(term1_num/term1_den + term2_num/term2_den) - 1.0;

    // Adjust scintillation for uplink as per paper Equation (30)
    if (is_uplink) {
        sig_I2 += 0.2;
    }

    double scint_loss_dB = -4.343 * (erfinv(2.0 * p_thr - 1.0) * sqrt(2.0 * log(sig_I2 + 1.0)) - 0.5 * log(sig_I2 + 1.0));

    // Total Attenuation and Transmittance
    double A_total_dB = geo_loss_dB + mie_scat_total_dB + scint_loss_dB;
    double T = pow(10.0, -A_total_dB / 10.0);

    return T;
}


// #############################################################################
// #                     PUBLIC INTERFACE IMPLEMENTATIONS                      #
// #############################################################################

/**
 * @brief Calculates SKR for a fibre link.
 */
gdouble fibre_link(qth_t *ground1, qth_t *ground2) {
    // These parameters would typically be configurable
    const double alpha = sqrt(5.0/2.0); // Corresponds to VA = 5
    const double ksi = 0.03;
    const char* R_method = "MD";

    // Calculate distance using Haversine formula from lat/lon in the qth_t struct
    double distance_km = haversine_distance(ground1->lat, ground1->lon, ground2->lat, ground2->lon);

    // Transmittance for standard fiber
    double T = pow(10.0, -0.02 * distance_km);
    
    return calculate_skr_finite(T, alpha, ksi, R_method);
}


/**
 * @brief Calculates SKR for a satellite-to-ground downlink.
 */
gdouble sat_to_ground_downlink(sat_t *sat, qth_t *ground) {
    // Protocol parameters
    const double alpha = sqrt(5.0/2.0); // VA = 5
    const double ksi = 0.03;
    const char* R_method = "MD";
    
    // Downlink-specific hardware parameters
    const double D_t_down = 0.3; // Transmitter Aperture Diameter (m) [Satellite]
    const double D_r_down = 1.0; // Receiver Aperture Diameter (m) [OGS]

    // Get dynamic data from structs
    double elevation_angle_deg = sat->el;
    double sat_altitude_km = sat->alt;
    double ogs_altitude_km = (double)ground->alt / 1000.0; // Convert meters to km

    double T = calculate_stg_transmittance(D_t_down, D_r_down, elevation_angle_deg, sat_altitude_km, ogs_altitude_km, FALSE);

    return calculate_skr_finite(T, alpha, ksi, R_method);
}


/**
 * @brief Calculates SKR for a ground-to-satellite uplink.
 */
gdouble ground_to_sat_uplink(qth_t *ground, sat_t *sat) {
    // Protocol parameters
    const double alpha = sqrt(5.0/2.0); // VA = 5
    const double ksi = 0.03;
    const char* R_method = "MD";

    // Uplink-specific hardware parameters (swapped from downlink)
    const double D_t_up = 1.0; // Transmitter Aperture Diameter (m) [OGS]
    const double D_r_up = 0.3; // Receiver Aperture Diameter (m) [Satellite]
    
    // Get dynamic data from structs
    double elevation_angle_deg = sat->el;
    double sat_altitude_km = sat->alt;
    double ogs_altitude_km = (double)ground->alt / 1000.0; // Convert meters to km

    double T = calculate_stg_transmittance(D_t_up, D_r_up, elevation_angle_deg, sat_altitude_km, ogs_altitude_km, TRUE);

    return calculate_skr_finite(T, alpha, ksi, R_method);
}

/**
 * @brief Calculates SKR for an inter-satellite link.
 */
gdouble inter_sat_link(sat_t *sat1, sat_t *sat2) {
    // --- Configurable Parameters (from paper Figure A1a) ---
    const double lambda = 1550e-9;      // Wavelength (m)
    const double ra = 0.2;              // Receiver aperture radius (m)
    const double w0 = 0.2;              // Beam waist radius (m)

    const double alpha = sqrt(5.0/2.0); // VA = 5
    const double ksi = 0.03;
    const char* R_method = "MD";
    
    // Use hypothetical function to get distance between satellites.
    // This would likely calculate the Euclidean distance between the 'pos' vectors.
    // double distance_km = get_distance_between_sats(sat1, sat2);
    double distance_km = 1000.0; // Using placeholder for now
    (void)sat1; // Mark as used until get_distance_between_sats is implemented
    (void)sat2; // Mark as used until get_distance_between_sats is implemented

    // Transmittance calculation from T_IS.py and paper
    double z_m = distance_km * 1000.0;
    double wz = w0 * sqrt(1.0 + pow((lambda * z_m) / (M_PI * w0 * w0), 2));
    double T = 1.0 - exp(-(2.0 * ra * ra) / (wz * wz));

    return calculate_skr_finite(T, alpha, ksi, R_method);
}
