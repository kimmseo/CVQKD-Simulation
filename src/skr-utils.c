/*
 * skr-utils.c - Implementation of Secret Key Rate (SKR) calculation functions.
 *
 * This file provides the implementation for calculating the Secret Key Rate (SKR)
 * for different quantum communication links (fiber, satellite-to-ground, inter-satellite)
 * based on the models provided in the Python scripts and the research paper
 * "Dynamic Continuous Variable Quantum Key Distribution for Securing a Future Global Quantum Network".
 * It uses the finite-size key rate formulas for Gaussian Modulated (GM) CV-QKD.
 */

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <math.h>
#include <float.h>

#include "skr-utils.h"
#include "calc-dist-two-sat.h"
#include "max-capacity-path/path-util.h"

/* Constant parameters for SKR calculation based on the provided Python scripts and paper */
#define ALPHA_MOD_AMP 2.236       // sqrt(5) -> Corresponds to VA = 5 SNU
#define EXCESS_NOISE 0.03         // Excess noise (ksi) in SNU
#define RECONCILIATION_EFF 0.98   // Initial reconciliation efficiency (beta)
#define LASER_REPETITION_RATE 100e6 // Repetition rate (f) in Hz
#define WAVELENGTH 1550e-9        // Wavelength of the laser in meters
#define TRANSMITTER_APT_DIAMETER 1.0 // Transmitter aperture diameter (Dt) in meters for ground
#define RECEIVER_APT_DIAMETER 0.3  // Receiver aperture diameter (Dr) in meters for satellite
#define TRANSMITTER_OPTICS_EFF 0.95 // Transmitter optics efficiency (Tt)
#define RECEIVER_OPTICS_EFF 0.95    // Receiver optics efficiency (Tr)
#define POINTING_LOSS 0.1          // Pointing loss (Lp)
#define ATMOSPHERE_THICKNESS 20.0  // Effective atmosphere thickness in km
#define VISIBILITY 200.0           // Atmospheric visibility in km
#define CN2 1e-16                  // Refractive index structure parameter (good conditions)
#define P_TH 1e-6                  // Outage time fraction for scintillation
#define INTER_SAT_APT_RADIUS 0.2   // Inter-satellite aperture radius (ra) in meters
#define INTER_SAT_BEAM_WAIST 0.2   // Inter-satellite beam waist (w0) in meters
#define SKR_SCALE ((xmnpda * 60.0) / 8000.0) /* Units of skr from (bits / second).
                                    currently set to (kilobytes / minute) */


/* --- Helper Function Prototypes --- */
static gdouble entropy(gdouble x);
static gdouble key_rate_finite(gdouble transmittance, gdouble alpha, gdouble ksi);
static gdouble haversine_dist_calc(gdouble lat1, gdouble lon1, gdouble lat2, gdouble lon2);
static gdouble atmosphere_length(gdouble elevation_angle, gdouble ogs_altitude_km);
static gdouble ugaussian_Pinv_approx(gdouble p);
static gdouble transmittance_downlink(gdouble link_dist_km, gdouble elevation_angle, gdouble ogs_altitude_km);
static gdouble transmittance_uplink(gdouble link_dist_km, gdouble elevation_angle, gdouble ogs_altitude_km);
static gdouble transmittance_inter_satellite(gdouble distance_km);
static gdouble transmittance_fibre(gdouble distance_km);

/*
 * entropy() - Entropy function for Holevo bound calculation.
 * @x: The input value.
 *
 * Return: The calculated entropy value.
 */
static gdouble entropy(gdouble x)
{
    if (x <= 0) {
        return 0.0;
    }
    return (x + 1.0) * log2(x + 1.0) - x * log2(x);
}


/*
 * key_rate_finite() - Calculates the finite size secret key rate for a given link.
 * @transmittance: The total transmittance of the channel (0 to 1).
 * @alpha: The modulation amplitude.
 * @ksi: The excess noise in SNU.
 *
 * Return: The secret key rate in bits per second.
 */
static gdouble key_rate_finite(gdouble transmittance, gdouble alpha, gdouble ksi)
{
    if (transmittance <= 0) {
        return 0.0;
    }

    gdouble t = sqrt(transmittance);
    gdouble VA = 2.0 * alpha * alpha;

    // Covariance matrix elements
    gdouble A = 1.0 + VA;
    gdouble B = 1.0 + transmittance * (VA + ksi);
    // Z is calculated using the Gaussian upper bound Z = 2 * sqrt(alpha^4 + alpha^2)
    gdouble C = t * (2.0 * sqrt(pow(alpha, 4) + pow(alpha, 2)));

    // Symplectic eigenvalues of the covariance matrix
    gdouble delta = A * A + B * B - 2.0 * C * C;
    gdouble det = pow((A * B - C * C), 2);

    if (delta * delta - 4.0 * det < 0) {
        return 0.0; // Avoid complex numbers
    }

    gdouble nu1 = sqrt(0.5 * (delta + sqrt(delta * delta - 4.0 * det)));
    gdouble nu2 = sqrt(0.5 * (delta - sqrt(delta * delta - 4.0 * det)));
    gdouble nu3 = A - (C * C) / (B + 1.0);

    // Holevo Bound (Eve's information)
    gdouble chiBE = entropy((nu1 - 1.0) / 2.0) + entropy((nu2 - 1.0) / 2.0) - entropy((nu3 - 1.0) / 2.0);

    // Mutual Information (Alice-Bob) for Heterodyne detection
    gdouble IAB = log2(1.0 + (transmittance * VA) / (1.0 + transmittance * ksi));

    // Finite-size effects
    gdouble d = 5.0;
    gdouble es = 2e-10;
    gdouble e = 1e-9;
    gdouble N = 1e11;
    gdouble dn_privacy = (pow(d + 1, 2) + 4 * (d + 1) * sqrt(log2(2 / es)) + 2 * log2(2 / (es * e * e))) / sqrt(N);

    // SNR in dB
    gdouble SNR = (transmittance * VA) / (1.0 + transmittance * ksi);
    gdouble SNR_dB = (SNR > 0) ? 10.0 * log10(SNR) : -DBL_MAX;

    // Frame Error Rate (FER)
    gdouble fer = 0.5 * (1.0 + 0.8218 * atan(-19.46 * SNR_dB - 298.1));
    fer = fmax(0.0, fmin(1.0, fer));

    // Reconciliation efficiency (beta) for MD reconciliation
    gdouble beta = 0.0825 * exp(0.1834 * SNR_dB) + 0.9821 * exp(-0.00002815 * SNR_dB);
    beta = fmax(0.0, fmin(1.0, beta));

    gdouble skr = LASER_REPETITION_RATE * ((1.0 - fer) * beta * IAB - chiBE - dn_privacy);

    return (skr > 0.0) ? skr : 0.0;
}


/*
 * haversine_dist_calc() - Calculates the great-circle distance between two points.
 * @lat1: Latitude of point 1 in degrees.
 * @lon1: Longitude of point 1 in degrees.
 * @lat2: Latitude of point 2 in degrees.
 * @lon2: Longitude of point 2 in degrees.
 *
 * Return: The distance in kilometers.
 */
static gdouble haversine_dist_calc(gdouble lat1, gdouble lon1, gdouble lat2, gdouble lon2)
{
    gdouble dLat = DEG_TO_RAD(lat2 - lat1);
    gdouble dLon = DEG_TO_RAD(lon2 - lon1);

    lat1 = DEG_TO_RAD(lat1);
    lat2 = DEG_TO_RAD(lat2);

    gdouble a = pow(sin(dLat / 2.0), 2) + cos(lat1) * cos(lat2) * pow(sin(dLon / 2.0), 2);
    gdouble c = 2.0 * asin(sqrt(a));

    return EARTH_RADIUS * c;
}

/*
 * atmosphere_length() - Calculates the effective atmospheric path length.
 * @elevation_angle: The satellite's elevation angle in degrees.
 * @ogs_altitude_km: The altitude of the ground station in km.
 *
 * Return: The effective atmospheric path length in km.
 */
static gdouble atmosphere_length(gdouble elevation_angle, gdouble ogs_altitude_km)
{
    if (elevation_angle < 0) return DBL_MAX;

    gdouble angle_rad = DEG_TO_RAD(90.0 + elevation_angle);
    gdouble R_E = EARTH_RADIUS;

    gdouble A_rad = asin(((R_E + ogs_altitude_km) * sin(angle_rad)) / (R_E + ATMOSPHERE_THICKNESS));
    gdouble atm_angle_rad = G_PI - A_rad - angle_rad;

    gdouble L_atm_sq = pow(R_E + ATMOSPHERE_THICKNESS, 2) + pow(R_E + ogs_altitude_km, 2) -
                       2 * (R_E + ATMOSPHERE_THICKNESS) * (R_E + ogs_altitude_km) * cos(atm_angle_rad);

    return sqrt(L_atm_sq);
}

/*
 * ugaussian_Pinv_approx() - Approximation of the inverse standard normal CDF.
 * @p: The probability (0 < p < 1).
 *
 * This function uses the Abramowitz and Stegun formula 26.2.23 to approximate
 * the inverse of the standard normal cumulative distribution function (quantile).
 * It is a replacement for gsl_cdf_ugaussian_Pinv.
 *
 * Return: The value x such that P(N(0,1) <= x) = p.
 */
static gdouble ugaussian_Pinv_approx(gdouble p)
{
    const gdouble c0 = 2.515517;
    const gdouble c1 = 0.802853;
    const gdouble c2 = 0.010328;
    const gdouble d1 = 1.432788;
    const gdouble d2 = 0.189269;
    const gdouble d3 = 0.001308;
    gdouble t, x;

    if (p <= 0.0 || p >= 1.0) {
        return NAN;
    }

    if (p < 0.5) {
        t = sqrt(-2.0 * log(p));
        x = t - ((c0 + c1 * t + c2 * t * t) / (1.0 + d1 * t + d2 * t * t + d3 * t * t * t));
        return -x;
    }

    t = sqrt(-2.0 * log(1.0 - p));
    x = t - ((c0 + c1 * t + c2 * t * t) / (1.0 + d1 * t + d2 * t * t + d3 * t * t * t));
    return x;
}

/*
 * transmittance_downlink() - Calculates transmittance for a sat-to-ground downlink.
 * @link_dist_km: The total link distance in km.
 * @elevation_angle: The satellite's elevation angle in degrees.
 * @ogs_altitude_km: The altitude of the ground station in km.
 *
 * Return: The downlink transmittance (0 to 1).
 */
static gdouble transmittance_downlink(gdouble link_dist_km, gdouble elevation_angle, gdouble ogs_altitude_km)
{
    gdouble L_total_m = link_dist_km * 1000.0;
    gdouble L_atm_eff_km = atmosphere_length(elevation_angle, ogs_altitude_km);
    gdouble L_atm_eff_m = L_atm_eff_km * 1000.0;

    // Geometric Loss
    gdouble geo_loss_db = 10.0 * log10((pow(L_total_m, 2) * pow(WAVELENGTH, 2)) /
                                     (pow(RECEIVER_APT_DIAMETER, 2) * pow(TRANSMITTER_APT_DIAMETER, 2) *
                                      TRANSMITTER_OPTICS_EFF * (1.0 - POINTING_LOSS) * RECEIVER_OPTICS_EFF));

    // Mie Scattering Loss (Kim Model)
    gdouble p = (VISIBILITY >= 50.0) ? 1.6 :
                (VISIBILITY >= 6.0)  ? 1.3 :
                (VISIBILITY >= 1.0)  ? 0.16 * VISIBILITY + 0.34 :
                (VISIBILITY >= 0.5)  ? VISIBILITY - 0.5 : 0.0;
    gdouble mie_scat_db_per_km = (4.343 * 3.912 / VISIBILITY) * pow(WAVELENGTH / 550e-9, -p);
    gdouble total_mie_loss_db = mie_scat_db_per_km * L_atm_eff_km;

    // Scintillation Loss
    gdouble d_scint = RECEIVER_APT_DIAMETER * sqrt(G_PI / (2.0 * WAVELENGTH * L_atm_eff_m));
    gdouble k = 2.0 * G_PI / WAVELENGTH;

    // Rytov variance (using analytical solution for the integral)
    gdouble integral_result = CN2 * (6.0 / 11.0) * pow(L_atm_eff_m, 11.0 / 6.0);
    gdouble rytov_var = 2.25 * pow(k, 7.0 / 6.0) * integral_result;

    // Scintillation index (spherical wave)
    gdouble sig_I2 = exp( (0.2 * rytov_var) / pow(1.0 + 0.18 * pow(d_scint, 2) + 0.20 * pow(rytov_var, 6.0/5.0), 7.0/6.0) +
                           (0.21 * rytov_var) / pow(1.0 + 0.90 * pow(d_scint, 2) + 0.21 * pow(d_scint, 2) * pow(rytov_var, 6.0/5.0), 5.0/6.0) ) - 1.0;

    // Scintillation loss formula corrected to match paper/python source
    gdouble erfinv_term = ugaussian_Pinv_approx(P_TH) / sqrt(2.0);
    gdouble scint_loss_db = -4.343 * (erfinv_term * sqrt(2.0 * log(sig_I2 + 1.0)) - 0.5 * log(sig_I2 + 1.0));

    // Total Attenuation
    gdouble total_attenuation_db = geo_loss_db + total_mie_loss_db + scint_loss_db;
    gdouble transmittance = pow(10.0, -total_attenuation_db / 10.0);

    return fmax(0.0, fmin(1.0, transmittance));
}

/*
 * transmittance_uplink() - Calculates transmittance for a ground-to-sat uplink.
 * @link_dist_km: The total link distance in km.
 * @elevation_angle: The satellite's elevation angle in degrees.
 * @ogs_altitude_km: The altitude of the ground station in km.
 *
 * Return: The uplink transmittance (0 to 1).
 */
static gdouble transmittance_uplink(gdouble link_dist_km, gdouble elevation_angle, gdouble ogs_altitude_km)
{
    gdouble L_total_m = link_dist_km * 1000.0;
    gdouble L_atm_eff_km = atmosphere_length(elevation_angle, ogs_altitude_km);
    gdouble L_atm_eff_m = L_atm_eff_km * 1000.0;

    // Geometric Loss (Note: Dt and Dr are swapped for uplink)
    gdouble geo_loss_db = 10.0 * log10((pow(L_total_m, 2) * pow(WAVELENGTH, 2)) /
                                     (pow(TRANSMITTER_APT_DIAMETER, 2) * pow(RECEIVER_APT_DIAMETER, 2) *
                                      TRANSMITTER_OPTICS_EFF * (1.0 - POINTING_LOSS) * RECEIVER_OPTICS_EFF));

    // Mie Scattering Loss is the same
    gdouble p = (VISIBILITY >= 50.0) ? 1.6 :
                (VISIBILITY >= 6.0)  ? 1.3 :
                (VISIBILITY >= 1.0)  ? 0.16 * VISIBILITY + 0.34 :
                (VISIBILITY >= 0.5)  ? VISIBILITY - 0.5 : 0.0;
    gdouble mie_scat_db_per_km = (4.343 * 3.912 / VISIBILITY) * pow(WAVELENGTH / 550e-9, -p);
    gdouble total_mie_loss_db = mie_scat_db_per_km * L_atm_eff_km;

    // Scintillation Loss (Uplink has higher scintillation)
    gdouble d_scint = RECEIVER_APT_DIAMETER * sqrt(G_PI / (2.0 * WAVELENGTH * L_atm_eff_m));
    gdouble k = 2.0 * G_PI / WAVELENGTH;

    // Rytov variance (using analytical solution for the integral)
    gdouble integral_result = CN2 * (6.0 / 11.0) * pow(L_atm_eff_m, 11.0 / 6.0);
    gdouble rytov_var = 2.25 * pow(k, 7.0 / 6.0) * integral_result;

    // Uplink scintillation index is higher
    gdouble sig_I2_downlink = exp( (0.2 * rytov_var) / pow(1.0 + 0.18 * pow(d_scint, 2) + 0.20 * pow(rytov_var, 6.0/5.0), 7.0/6.0) +
                           (0.21 * rytov_var) / pow(1.0 + 0.90 * pow(d_scint, 2) + 0.21 * pow(d_scint, 2) * pow(rytov_var, 6.0/5.0), 5.0/6.0) ) - 1.0;
    gdouble sig_I2_uplink = sig_I2_downlink + 0.2; // Approximation from paper

    // Scintillation loss formula corrected to match paper/python source
    gdouble erfinv_term = ugaussian_Pinv_approx(P_TH) / sqrt(2.0);
    gdouble scint_loss_db = -4.343 * (erfinv_term * sqrt(2.0 * log(sig_I2_uplink + 1.0)) - 0.5 * log(sig_I2_uplink + 1.0));

    // Total Attenuation
    gdouble total_attenuation_db = geo_loss_db + total_mie_loss_db + scint_loss_db;
    gdouble transmittance = pow(10.0, -total_attenuation_db / 10.0);

    return fmax(0.0, fmin(1.0, transmittance));
}


/*
 * transmittance_inter_satellite() - Calculates transmittance for an inter-satellite link.
 * @distance_km: The distance between the two satellites in km.
 *
 * Return: The inter-satellite link transmittance (0 to 1).
 */
static gdouble transmittance_inter_satellite(gdouble distance_km)
{
    gdouble z = distance_km * 1000.0;
    gdouble wz = INTER_SAT_BEAM_WAIST * sqrt(1.0 + pow((WAVELENGTH * z) / (G_PI * pow(INTER_SAT_BEAM_WAIST, 2)), 2));
    gdouble transmittance = 1.0 - exp(-(2.0 * pow(INTER_SAT_APT_RADIUS, 2)) / pow(wz, 2));
    return fmax(0.0, fmin(1.0, transmittance));
}

/*
 * transmittance_fibre() - Calculates the transmittance for a fiber optic link.
 * @distance_km: The length of the fiber in km.
 *
 * Return: The fiber link transmittance (0 to 1).
 */
static gdouble transmittance_fibre(gdouble distance_km)
{
    // Standard fiber attenuation of 0.2 dB/km
    gdouble attenuation_db = 0.2 * distance_km;
    gdouble transmittance = pow(10.0, -attenuation_db / 10.0);
    return fmax(0.0, fmin(1.0, transmittance));
}


/* --- Main SKR Functions --- */

/*
 * fibre_link() - Calculates the SKR for a fiber link between two ground stations.
 * @ground1: Pointer to the first ground station data structure.
 * @ground2: Pointer to the second ground station data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble fibre_link(qth_t *ground1, qth_t *ground2)
{
    if (!ground1 || !ground2) {
        return 0.0;
    }

    gdouble distance = haversine_dist_calc(ground1->lat, ground1->lon, ground2->lat, ground2->lon);
    gdouble T = transmittance_fibre(distance);

    return key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE);
}

/*
 * ground_to_sat_uplink() - Calculates the SKR for a ground-to-satellite uplink.
 * @ground: Pointer to the ground station data structure.
 * @sat: Pointer to the satellite data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble ground_to_sat_uplink(qth_t *ground, sat_t *sat)
{
    if (!ground || !sat || sat->el < 0) {
        return 0.0;
    }

    gdouble distance = sat->range;
    gdouble elevation = sat->el;
    gdouble qth_altitude = ground->alt / 1000.0;

    gdouble T = transmittance_uplink(distance, elevation, qth_altitude);

    return key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE);
}

//light weight version
gdouble lw_ground_to_sat_uplink(qth_t *ground, gdouble elevation, gdouble range)
{
    if (!ground || elevation < 0) {
        return 0.0;
    }
    
    gdouble qth_altitude = ground->alt / 1000.0;

    gdouble T = transmittance_uplink(range, elevation, qth_altitude);

    return key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE) * SKR_SCALE;
}

/*
 * sat_to_ground_downlink() - Calculates SKR for a satellite-to-ground downlink.
 * @sat: Pointer to the satellite data structure.
 * @ground: Pointer to the ground station data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble sat_to_ground_downlink(sat_t *sat, qth_t *ground)
{
    if (!sat || !ground || sat->el < 0) {
        return 0.0;
    }

    gdouble distance = sat->range;
    gdouble elevation = sat->el;
    gdouble qth_altitude = ground->alt / 1000.0;

    gdouble T = transmittance_downlink(distance, elevation, qth_altitude);

    return key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE);
}

//light weight version
gdouble lw_sat_to_ground_downlink(qth_t *ground, gdouble elevation, gdouble range)
{
    if (!ground || elevation < 0) {
        return 0.0;
    }

    gdouble qth_altitude = ground->alt / 1000.0;

    gdouble T = transmittance_downlink(range, elevation, qth_altitude);

    return key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE) * SKR_SCALE;
}

/*
 * inter_sat_link() - Calculates the SKR for an inter-satellite link.
 * @sat1: Pointer to the first satellite data structure.
 * @sat2: Pointer to the second satellite data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble inter_sat_link(sat_t *sat1, sat_t *sat2)
{
    if (!sat1 || !sat2) {
        return 0.0;
    }

    gdouble distance = dist_calc(sat1, sat2);
    gdouble T = transmittance_inter_satellite(distance);

    return key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE);
}

// lightweight version
gdouble lw_inter_sat_link(vector_t *pos1, vector_t *pos2)
{
    if (!pos1 || !pos2) {
        return 0.0;
    }

    if (!is_pos_los_clear(pos1, pos2)) return 0; 

    gdouble distance = dist_calc_driver(
            pos1->x, pos1->y, pos1->z,
            pos2->x, pos2->y, pos2->z);
    gdouble T = transmittance_inter_satellite(distance);

    // (bits per second) * scale = (kilobytes per day)
    return  key_rate_finite(T, ALPHA_MOD_AMP, EXCESS_NOISE) * SKR_SCALE;
}


/**
 * Calc topocentric range and elevation
 * Copies second half of predict_calc() from predict-tools.c
 */
void calc_topocentric_el_range(lw_sat_t *sat, qth_t *qth, gdouble *el, gdouble *range) {
    obs_set_t       obs_set;
    geodetic_t      obs_geodetic;

    obs_geodetic.lon = qth->lon * de2ra;
    obs_geodetic.lat = qth->lat * de2ra;
    obs_geodetic.alt = qth->alt / 1000.0;
    obs_geodetic.theta = 0;

    Calculate_Obs(sat->jul_utc, &sat->pos, &sat->vel, &obs_geodetic, &obs_set);

    *el = Degrees(obs_set.el);
    *range = obs_set.range;
}

gdouble get_inter_node_skr(tdsp_node *src, tdsp_node *dst, lw_sat_t *src_hist, lw_sat_t *dst_hist, guint i) {
    gdouble el, range;

    if (src->node.type == path_SATELLITE && dst->node.type == path_SATELLITE) {
        return lw_inter_sat_link(&src_hist[i].pos, &dst_hist[i].pos);
    }
    
    if (src->node.type == path_STATION && dst->node.type == path_SATELLITE) {
        calc_topocentric_el_range(&dst_hist[i], src->node.obj, &el, &range);
        return lw_ground_to_sat_uplink(src->node.obj, el, range);
    }

    if (src->node.type == path_SATELLITE && dst->node.type == path_STATION) {
        calc_topocentric_el_range(&src_hist[i], dst->node.obj, &el, &range);
        return lw_sat_to_ground_downlink(dst->node.obj, el, range);
    }

    //if both ground stations, fiber optic link 
    return 0.0;
}