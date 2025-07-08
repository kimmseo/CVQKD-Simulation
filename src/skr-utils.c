/*
 * This file contains the necessary utility functions for
 * calculating the SKR for different links.
 * This file is based on the paper "Dynamic Continuous Variable Quantum Key
 * Distribution for Securing a Future Global Quantum Network" by Sayat et al. (2025).
 * All equations and parameters are sourced from this paper unless otherwise noted.
*/

#include <glib.h>
#include <math.h>
#include <stdio.h> // Added for printf
#include "qth-data.h"
#include "sgpsdp/sgp4sdp4.h"
#include "calc-dist-two-sat.h" // For distance calculation functions
#include "skr-utils.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0)
#endif

/*
 * =====================================================================================
 * BEGIN: Constants and Parameters from Sayat et al. (2025)
 * =====================================================================================
 */

// General constants
#define EARTH_RADIUS_KM 6371.0     // Mean radius of the Earth in km [source: Fig 4]
#define ATMOSPHERE_H_KM 20.0       // Atmosphere thickness from ground to zenith in km [source: Sec 4.4]

// --- Protocol and Security Parameters ---
// Using heterodyne detection and MD reconciliation as it performs best [source: Appendix A]

// Laser repetition rate in Hz [source: Sec 3]
const gdouble f_rep_rate = 50e6;
// Modulation Variance in Shot Noise Units (SNU) [source: Fig 6 Caption]
const gdouble V_A = 5.0;
// Excess Noise in SNU [source: Fig 6 Caption]
const gdouble xi = 0.03;

// --- Security parameters for finite size limit SKR from Table 3 [source: Sec 3] ---
const gdouble d_discretisation = 5.0;
const gdouble epsilon_s = 2e-10;
const gdouble epsilon_security = 1e-9;
const gdouble N_symbols = 1e11;

// --- Coefficients for Multidimensional (MD) Reconciliation from Table 4 [source: Sec 3] ---
const gdouble c1_md = 8.250e-2;
const gdouble c2_md = 0.1834;
const gdouble c3_md = 0.9821;
const gdouble c4_md = -2.815e-5;

// --- Coefficients for Frame Error Rate (FER) from Eq. (13) [source: Sec 3] ---
const gdouble m1_fer = 0.8218;
const gdouble m2_fer = -19.46;
const gdouble m3_fer = -298.1;


// --- Link-Specific Parameters ---
const gdouble lambda_wavelength_m = 1550e-9; // Wavelength in meters [source: Sec 4.4.1]

// Satellite-to-Ground parameters for "Good" conditions from Table 6 [source: Sec 4.4]
const gdouble visibility_km = 200.0;
const gdouble Cn2_good = 1e-16;
const gdouble p_threshold = 1e-6; // Outage probability (Assumed, common value)

// Satellite aperture parameters from figure captions [source: Fig 9, 11]
const gdouble D_T_downlink_m = 0.3; // Satellite transmitter
const gdouble D_R_downlink_m = 1.0; // Ground receiver
const gdouble D_T_uplink_m = 1.0;   // Ground transmitter
const gdouble D_R_uplink_m = 0.3;   // Satellite receiver
const gdouble r_a_intersat_m = 0.2; // Inter-sat receiver aperture radius
const gdouble w_0_intersat_m = 0.2; // Inter-sat beam-waist radius


/*
 * =====================================================================================
 * END: Constants and Parameters
 * =====================================================================================
 */


/*
 * =====================================================================================
 * BEGIN: Helper Functions for SKR Calculation
 * =====================================================================================
 */

/**
 * @brief The g(x) function for Holevo bound calculation, from Eq. (6) [source: Sec 3]
 * g(x) = (x+1)log2(x+1) - xlog2(x)
 */
static gdouble g_holevo(gdouble x) {
    if (x <= 0) return 0.0;
    return (x + 1) * log2(x + 1) - x * log2(x);
}

/**
 * @brief Rational approximation for the inverse error function (erfinv).
 * Not available in standard math.h. This is a common numerical approximation.
 */
static gdouble erfinv(gdouble y) {
    if (fabs(y) >= 1.0) return y > 0 ? INFINITY : -INFINITY;
    gdouble a = 0.147;
    gdouble term1 = 2 / (M_PI * a) + log(1 - y*y) / 2;
    gdouble term2 = log(1 - y*y) / a;
    gdouble sign = (y > 0) ? 1.0 : -1.0;
    return sign * sqrt(sqrt(term1*term1 - term2) - term1);
}

/**
 * @brief Core function to calculate the finite size SKR for a given transmittance.
 * Implements Eq. (9): SKR_fin = f * [(1-FER)*beta*I_AB - S_BE - delta_n_privacy] [source: Sec 3]
 * @param T The total channel transmittance.
 * @param enable_debug_prints Flag to turn on/off console printing for debugging.
 * @return The final secret key rate in bits/second.
 */
static gdouble calculate_skr_finite(gdouble T, gboolean enable_debug_prints) {
    if (enable_debug_prints) printf("\n--- SKR CALCULATION START ---\n");
    if (enable_debug_prints) printf("Input Transmittance (T): %.4e\n", T);

    if (T <= 0 || T > 1) {
        if (enable_debug_prints) printf("DEBUG: Transmittance is zero or invalid. SKR = 0.\n");
        return 0.0;
    }

    // 1. Calculate Signal-to-Noise Ratio (SNR)
    gdouble snr_linear = (T * V_A) / (1.0 + T * xi);
    if (enable_debug_prints) printf("SNR (linear): %.4e\n", snr_linear);
    if (snr_linear <= 0) {
        if (enable_debug_prints) printf("DEBUG: SNR is zero or negative. SKR = 0.\n");
        return 0.0;
    }
    
    gdouble snr_db = 10.0 * log10(snr_linear);
    if (enable_debug_prints) printf("SNR (dB): %.4f\n", snr_db);

    // 2. Calculate Reconciliation Efficiency (beta) and Frame Error Rate (FER)
    gdouble beta = pow(c1_md, c2_md * snr_db) - pow(c3_md, c4_md * snr_db);
    if (beta < 0) beta = 0;
    if (beta > 1) beta = 1;
    if (enable_debug_prints) printf("Reconciliation Eff (beta): %.4f\n", beta);

    gdouble fer = 0.5 * (1.0 + m1_fer * atan(m2_fer * snr_db + m3_fer));
    if (fer < 0) fer = 0;
    if (fer > 1) fer = 1;
    if (enable_debug_prints) printf("Frame Error Rate (FER): %.4f\n", fer);

    // 3. Calculate Mutual Information (I_AB)
    gdouble I_AB = log2(1.0 + snr_linear);
    if (enable_debug_prints) printf("Mutual Information (I_AB): %.4f\n", I_AB);

    // 4. Calculate Holevo Bound (S_BE)
    gdouble V = V_A + 1.0;
    gdouble W = 1.0 + T * V_A + T * xi;
    gdouble Z = T * sqrt(V * V - 1.0);

    gdouble A_term = V*V + W*W - 2*Z*Z;
    gdouble B_term = pow(V*W - Z*Z, 2);
    if (A_term*A_term < 4.0*B_term) {
         if (enable_debug_prints) printf("DEBUG: Negative sqrt for eigenvalues. SKR = 0.\n");
         return 0.0;
    }
    gdouble det_sqrt = sqrt(A_term*A_term - 4.0*B_term);

    gdouble lambda_1 = sqrt((A_term + det_sqrt) / 2.0);
    gdouble lambda_2 = sqrt((A_term - det_sqrt) / 2.0);
    gdouble lambda_3 = V - (Z*Z)/(W + 1.0);
    
    gdouble S_BE = g_holevo((lambda_1 - 1.0)/2.0) + g_holevo((lambda_2 - 1.0)/2.0) - g_holevo((lambda_3 - 1.0)/2.0);
    if (S_BE < 0) S_BE = 0;
    if (enable_debug_prints) printf("Holevo Bound (S_BE): %.4f\n", S_BE);

    // 5. Calculate Privacy Amplification Term (delta_n_privacy)
    gdouble term1 = pow(d_discretisation + 1.0, 2) / sqrt(N_symbols);
    gdouble term2 = (4.0 * (d_discretisation + 1.0) * sqrt(log2(2.0/epsilon_s))) / sqrt(N_symbols);
    gdouble term3 = (2.0 * log2(2.0 / (epsilon_security * epsilon_security))) / N_symbols;
    gdouble delta_n_privacy = term1 + term2 + term3;
    if (enable_debug_prints) printf("Privacy Amplification (delta_n): %.4e\n", delta_n_privacy);

    // 6. Calculate Final SKR
    gdouble info_gain = (1.0 - fer) * beta * I_AB;
    gdouble info_loss = S_BE + delta_n_privacy;
    gdouble key_fraction = info_gain - info_loss;
    if (enable_debug_prints) printf("Final Key Fraction: %.4f (Gain: %.4f, Loss: %.4f)\n", key_fraction, info_gain, info_loss);

    if (key_fraction < 0) {
        if (enable_debug_prints) printf("DEBUG: Key fraction is negative. SKR = 0.\n");
        return 0.0;
    }

    gdouble final_skr = f_rep_rate * key_fraction;
    if (enable_debug_prints) printf("Final SKR: %.4e bps\n--- SKR CALCULATION END ---\n", final_skr);
    
    return final_skr;
}


/*
 * =====================================================================================
 * END: Helper Functions
 * =====================================================================================
 */


/*
 * =====================================================================================
 * BEGIN: Main SKR Link Functions
 * =====================================================================================
 */

/**
 * @brief Calculates SKR for a terrestrial fiber link.
 * @param ground1 First ground station.
 * @param ground2 Second ground station.
 * @return Secret Key Rate in bits/second.
 */
gdouble fibre_link(qth_t *ground1, qth_t *ground2)
{
    gdouble lat1_rad = ground1->lat * M_PI / 180.0;
    gdouble lon1_rad = ground1->lon * M_PI / 180.0;
    gdouble lat2_rad = ground2->lat * M_PI / 180.0;
    gdouble lon2_rad = ground2->lon * M_PI / 180.0;

    gdouble dLat = lat2_rad - lat1_rad;
    gdouble dLon = lon2_rad - lon1_rad;

    gdouble a = sin(dLat / 2.0) * sin(dLat / 2.0) +
                cos(lat1_rad) * cos(lat2_rad) *
                sin(dLon / 2.0) * sin(dLon / 2.0);
    gdouble c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    gdouble dist_km = EARTH_RADIUS_KM * c;

    gdouble loss_db = 0.2 * dist_km;
    gdouble transmittance = pow(10.0, -loss_db / 10.0);

    return calculate_skr_finite(transmittance, FALSE);
}

/**
 * @brief Calculates SKR for an inter-satellite link.
 * @param sat1 Origin satellite.
 * @param sat2 Target satellite.
 * @return Secret Key Rate in bits/second.
 */
gdouble inter_sat_link(sat_t *sat1, sat_t *sat2)
{
    if (!is_los_clear(sat1, sat2)) {
        return 0.0;
    }

    gdouble z_km = dist_calc(sat1, sat2);
    gdouble z_m = z_km * 1000.0;

    gdouble term = (lambda_wavelength_m * z_m) / (M_PI * w_0_intersat_m * w_0_intersat_m);
    gdouble w_z = w_0_intersat_m * sqrt(1.0 + term*term);

    gdouble transmittance = 1.0 - exp(-(2.0 * r_a_intersat_m * r_a_intersat_m) / (w_z * w_z));

    return calculate_skr_finite(transmittance, FALSE);
}

/**
 * @brief Calculates SKR for a satellite-to-ground downlink.
 * @param sat Origin satellite.
 * @param ground Target ground station.
 * @return Secret Key Rate in bits/second.
 */
gdouble sat_to_ground_downlink(sat_t *sat, qth_t *ground)
{
    gdouble el_rad = sat->el * M_PI / 180.0;
    if (el_rad <= 0) return 0.0;

    printf("\n--- DOWNLINK DEBUG START FOR SATELLITE: %s ---\n", sat->nickname);
    printf("Elevation: %.2f deg\n", sat->el);

    // --- Link Geometry Calculation ---
    gdouble L_tot_km = sat->range;
    gdouble L_ogs_km = ground->alt / 1000.0;
    printf("Total Distance: %.2f km, OGS Altitude: %.2f km\n", L_tot_km, L_ogs_km);

    gdouble angle_at_ground = M_PI_2 + el_rad;
    gdouble sin_angle_at_atm = (EARTH_RADIUS_KM + L_ogs_km) / (EARTH_RADIUS_KM + ATMOSPHERE_H_KM) * sin(angle_at_ground);
    if (fabs(sin_angle_at_atm) > 1.0) {
        printf("DEBUG: Satellite below horizon based on atmospheric shell calculation.\n");
        return 0.0;
    }
    gdouble angle_at_atm = asin(sin_angle_at_atm);
    gdouble angle_at_center = M_PI - angle_at_ground - angle_at_atm;
    gdouble L_atm_eff_km = sqrt(pow(EARTH_RADIUS_KM + L_ogs_km, 2) + pow(EARTH_RADIUS_KM + ATMOSPHERE_H_KM, 2)
                               - 2 * (EARTH_RADIUS_KM + L_ogs_km) * (EARTH_RADIUS_KM + ATMOSPHERE_H_KM) * cos(angle_at_center));
    printf("Effective Atm Distance: %.2f km\n", L_atm_eff_km);

    // --- Loss Calculation (in dB) ---
    gdouble total_loss_db = 0.0;
    gdouble geo_loss_db = 0.0, scat_loss_db = 0.0, scin_loss_db = 0.0;

    // 1. Geometric Loss
    gdouble L_tot_m = L_tot_km * 1000.0;
    gdouble geo_ratio = (L_tot_m * L_tot_m * lambda_wavelength_m * lambda_wavelength_m) / (D_T_downlink_m * D_T_downlink_m * D_R_downlink_m * D_R_downlink_m);
    geo_loss_db = 10.0 * log10(geo_ratio);
    total_loss_db += geo_loss_db;
    printf("Geometric Loss: %.2f dB\n", geo_loss_db);

    // 2. Scattering Loss
    gdouble V = visibility_km;
    gdouble p = (V >= 50.0) ? 1.6 : (V >= 6.0) ? 1.3 : (V >= 1.0) ? (0.16 * V + 0.34) : (V >= 0.5) ? (V - 0.5) : 0.0;
    gdouble scat_db_per_km = 4.343 * (3.912 / V) * pow(lambda_wavelength_m / 550e-9, -p);
    scat_loss_db = scat_db_per_km * L_atm_eff_km;
    total_loss_db += scat_loss_db;
    printf("Scattering Loss: %.2f dB (%.2f dB/km)\n", scat_loss_db, scat_db_per_km);

    // 3. Scintillation Loss
    gdouble k_wavenumber = (2.0 * M_PI) / lambda_wavelength_m;
    gdouble L_atm_eff_m = L_atm_eff_km * 1000.0;
    gdouble rytov_var = 2.25 * pow(k_wavenumber, 7.0/6.0) * Cn2_good * (6.0/11.0) * pow(L_atm_eff_m, 11.0/6.0);
    gdouble scin_index_num = 0.49 * rytov_var / pow(1.0 + 1.11 * pow(rytov_var, 12.0/5.0), 7.0/6.0);
    gdouble scin_index_den = 0.51 * rytov_var / pow(1.0 + 0.69 * pow(rytov_var, 12.0/5.0), 5.0/6.0);
    gdouble scin_index_sq = exp(scin_index_num + scin_index_den) - 1.0;
    printf("Rytov Variance: %.4e, Scintillation Index^2: %.4e\n", rytov_var, scin_index_sq);

    if (scin_index_sq > 0) {
        gdouble scin_term1 = erfinv(2.0*p_threshold - 1.0) * sqrt(2.0*log(scin_index_sq + 1.0));
        gdouble scin_term2 = 0.5 * log(scin_index_sq + 1.0);
        scin_loss_db = 4.343 * (scin_term1 - scin_term2);
        if (scin_loss_db > 0) total_loss_db += scin_loss_db;
    }
    printf("Scintillation Loss: %.2f dB\n", scin_loss_db);
    printf("Total Loss: %.2f dB\n", total_loss_db);

    // --- Total Transmittance ---
    gdouble transmittance = pow(10.0, -total_loss_db / 10.0);
    
    // Final SKR calculation with debug prints enabled
    return calculate_skr_finite(transmittance, TRUE);
}

/**
 * @brief Calculates SKR for a ground-to-satellite uplink.
 * @param ground Origin ground station.
 * @param sat Target satellite.
 * @return Secret Key Rate in bits/second.
 */
gdouble ground_to_sat_uplink(qth_t *ground, sat_t *sat)
{
    gdouble el_rad = sat->el * M_PI / 180.0;
    if (el_rad <= 0) return 0.0;

    // --- Link Geometry Calculation (same as downlink) ---
    gdouble L_tot_km = sat->range;
    gdouble L_ogs_km = ground->alt / 1000.0;
    gdouble angle_at_ground = M_PI_2 + el_rad;
    gdouble sin_angle_at_atm = (EARTH_RADIUS_KM + L_ogs_km) / (EARTH_RADIUS_KM + ATMOSPHERE_H_KM) * sin(angle_at_ground);
    if (fabs(sin_angle_at_atm) > 1.0) return 0.0;
    gdouble angle_at_atm = asin(sin_angle_at_atm);
    gdouble angle_at_center = M_PI - angle_at_ground - angle_at_atm;
    gdouble L_atm_eff_km = sqrt(pow(EARTH_RADIUS_KM + L_ogs_km, 2) + pow(EARTH_RADIUS_KM + ATMOSPHERE_H_KM, 2)
                               - 2 * (EARTH_RADIUS_KM + L_ogs_km) * (EARTH_RADIUS_KM + ATMOSPHERE_H_KM) * cos(angle_at_center));

    // --- Loss Calculation (in dB) ---
    gdouble total_loss_db = 0.0;

    // 1. Geometric Loss
    gdouble L_tot_m = L_tot_km * 1000.0;
    gdouble geo_ratio = (L_tot_m * L_tot_m * lambda_wavelength_m * lambda_wavelength_m) / (D_T_uplink_m * D_T_uplink_m * D_R_uplink_m * D_R_uplink_m);
    total_loss_db += 10.0 * log10(geo_ratio);

    // 2. Scattering Loss
    gdouble V = visibility_km;
    gdouble p = (V >= 50.0) ? 1.6 : (V >= 6.0) ? 1.3 : (V >= 1.0) ? (0.16 * V + 0.34) : (V >= 0.5) ? (V - 0.5) : 0.0;
    gdouble scat_db_per_km = 4.343 * (3.912 / V) * pow(lambda_wavelength_m / 550e-9, -p);
    total_loss_db += scat_db_per_km * L_atm_eff_km;

    // 3. Scintillation Loss (modified for uplink)
    gdouble k_wavenumber = (2.0 * M_PI) / lambda_wavelength_m;
    gdouble L_atm_eff_m = L_atm_eff_km * 1000.0;
    gdouble rytov_var = 2.25 * pow(k_wavenumber, 7.0/6.0) * Cn2_good * (6.0/11.0) * pow(L_atm_eff_m, 11.0/6.0);
    gdouble scin_index_down_num = 0.49 * rytov_var / pow(1.0 + 1.11 * pow(rytov_var, 12.0/5.0), 7.0/6.0);
    gdouble scin_index_down_den = 0.51 * rytov_var / pow(1.0 + 0.69 * pow(rytov_var, 12.0/5.0), 5.0/6.0);
    gdouble scin_index_down_sq = exp(scin_index_down_num + scin_index_down_den) - 1.0;

    gdouble scin_index_uplink_sq = pow(sqrt(fmax(0, scin_index_down_sq)) + 0.2, 2);

    if (scin_index_uplink_sq > 0) {
        gdouble scin_term1 = erfinv(2.0*p_threshold - 1.0) * sqrt(2.0*log(scin_index_uplink_sq + 1.0));
        gdouble scin_term2 = 0.5 * log(scin_index_uplink_sq + 1.0);
        gdouble A_scin_db = 4.343 * (scin_term1 - scin_term2);
        if (A_scin_db > 0) total_loss_db += A_scin_db;
    }

    // --- Total Transmittance ---
    gdouble transmittance = pow(10.0, -total_loss_db / 10.0);
    return calculate_skr_finite(transmittance, FALSE);
}

/*
// This function is commented out
gdouble underwater_link(qth_t *station1, qth_t *station2)
{
    gdouble skr;
    // Do SKR calculation here
    // temporary placeholder
    skr = 0.0;
    return skr;
}
*/