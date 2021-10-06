/*
 * File:   linear_trajectory.c
 * Author: Pavlo Vlastos
 * Brief:  Creates linear trajectories for a vehicle to follow 
 * Note:   This module is meant to be used in place of the some-what deprecated 
 *         gnc.X library. Distances and positions are in meters, and angles are
 *         in radians
 * Created on March 11, 2021, 1:48 PM
 */

/*******************************************************************************
 * #INCLUDES
 ******************************************************************************/
#include "linear_trajectory.h"
#include "board.h"
#include <math.h>

/*******************************************************************************
 * PRIVATE #DEFINES
 ******************************************************************************/
#define EARTH_RADIUS 6378137.0 /* units are meters */
#define DEG_TO_RAD (M_PI / 180.0)

/*******************************************************************************
 * PRIVATE DATATYPES 
 ******************************************************************************/
typedef struct trajectory {
    float vehi_pt[DIM];
    float prev_wp[DIM];
    float next_wp[DIM];
    float clos_pt[DIM];
    float path_angle;
    float cte;
} trajectory_t;

/*******************************************************************************
 * PRIVATE VARIABLES 
 ******************************************************************************/
static trajectory_t lin_tra;
static float north_v[DIM] = {0.0, 0.1};

/*******************************************************************************
 * Public Function Implementations
 ******************************************************************************/
int lin_tra_init(float prev_wp[DIM], float next_wp[DIM], float vehi_pt[DIM]) {
    int i = 0;
    for (i = 0; i < DIM; i++) {
        lin_tra.prev_wp[i] = prev_wp[i];
        lin_tra.next_wp[i] = next_wp[i];
        lin_tra.vehi_pt[i] = vehi_pt[i];
        lin_tra.clos_pt[i] = 0.0;
    }

    return SUCCESS;
}

int lin_tra_update(float new_position[DIM]) {
    int i = 0;

    for (i = 0; i < DIM; i++) {
        lin_tra.vehi_pt[i] = new_position[i];
    }

    if (lin_tra_project(lin_tra.clos_pt) == SUCCESS) {
        lin_tra.path_angle = lin_tra_calc_path_angle();
        lin_tra.cte = lin_tra_calc_cte();
    } else {
        return ERROR;
    }

    return SUCCESS;
}

float lin_tra_project(float point[DIM]) {
    int i;
    float a[DIM]; /* Vector connecting the previous waypoint to the 
                        *  current vehicle position */
    float a1[DIM];
    float a1_sum = 0.0;
    float b[DIM]; /* Vector connecting the next waypoint to the previous 
                        * waypoint */
    float bhat[DIM]; // Normalized 'b' vector
    float b_mag = 0.0;

    b_mag = lin_tra_calc_dist(lin_tra.next_wp, lin_tra.prev_wp);

    if (b_mag == 0) {
        return ERROR;
    }

    for (i = 0; i < DIM; i++) {
        a[i] = lin_tra.vehi_pt[i] - lin_tra.prev_wp[i];
        b[i] = lin_tra.next_wp[i] - lin_tra.prev_wp[i];
        bhat[i] = b[i] / b_mag;
        a1_sum += a[i] * bhat[i];
    }

    // Projection
    for (i = 0; i < DIM; i++) {
        a1[i] = a1_sum * bhat[i];
        /* This places the origin-anchored vector back to being relative to the
         * previous waypoint */
        point[i] = a1[i] + lin_tra.prev_wp[i];
    }

    return SUCCESS;
}

float lin_tra_calc_cte(void) {
    /* Move segment to origin, basically creating the Serret-Frenet (SF) 
     * reference frame. */
    int i = 0;
    float vehi_pt_sf[DIM] = {0.0};
    float clos_pt_sf[DIM] = {0.0};
    float rot_vehi_pt_sf[DIM] = {0.0};
    float rot_clos_pt_sf[DIM] = {0.0};

    for (i = 0; i < DIM; i++) {
        vehi_pt_sf[i] = lin_tra.vehi_pt[i] - lin_tra.prev_wp[i];
        clos_pt_sf[i] = lin_tra.clos_pt[i] - lin_tra.prev_wp[i];
    }

    /* Finish making the SF frame using rotation */
    lin_tra_rotate2d(vehi_pt_sf, lin_tra.path_angle, rot_vehi_pt_sf);
    lin_tra_rotate2d(clos_pt_sf, lin_tra.path_angle, rot_clos_pt_sf);

    return (rot_clos_pt_sf[0] - rot_vehi_pt_sf[0]);
}

float lin_tra_calc_path_angle(void) {
    int i = 0;
    float a[DIM] = {0.0};
    float path_angle = 0.0;

    /* Move vector to origin */
    for (i = 0; i < DIM; i++) {
        a[i] = lin_tra.next_wp[i] - lin_tra.prev_wp[i];
    }

    path_angle = atan2(north_v[1], north_v[0]) - atan2(a[1], a[0]);

    /* Ensure that the path angle is wrapped between -pi and pi */
    return fmod(path_angle + M_PI, 2.0 * M_PI) - M_PI;
}

int lin_tra_rotate2d(float v[DIM], float path_angle, float v_new[DIM]) {
    float c = cos(path_angle);
    float s = sin(path_angle);

    if (lin_tra.path_angle == 0.0) {
        return SUCCESS;
    }

    v_new[0] = (v[0] * c) - (v[1] * s);
    v_new[1] = (v[0] * s) + (v[1] * c);

    return SUCCESS;
}

float lin_tra_calc_dist(float a[DIM], float b[DIM]) {
    int i;
    float delta[DIM];
    float sum = 0.0;

    for (i = 0; i < DIM; i++) {
        delta[i] = fabs(a[i] - b[i]);
        sum += delta[i] * delta[i];
    }

    return (sqrt(sum));
}

int lin_tra_set_path_angle(float path_angle) {
    lin_tra.path_angle = path_angle;
    return SUCCESS;
}

float lin_tra_get_cte(void) {
    return lin_tra.cte;
}

float lin_tra_get_path_angle(void) {
    return lin_tra.path_angle;
}

int lin_tra_set_next_wp(float wp[DIM]) {
    int i = 0;
    
    for (i = 0; i < DIM; i++) {
        lin_tra.next_wp[i] = wp[i];
    }
    
    return SUCCESS;
}

int lin_tra_set_prev_wp(float wp[DIM]) {
    int i = 0;
    
    for (i = 0; i < DIM; i++) {
        lin_tra.prev_wp[i] = wp[i];
    }
    
    return SUCCESS;
}

void lin_tra_lla_to_enu(float lla_point[DIM + 1],
        float ref_point[DIM + 1],
        float enu_point[DIM + 1]) {
    int row;
    int col;
    float lat0;
    float long0;
    float alt0;
    float te2n[DIM + 1][DIM + 1];
    float ecef_point[DIM + 1];

    // Assume altitude is 0, sea-level, since vehicle is a boat
    lla_point[0] *= DEG_TO_RAD;
    lla_point[1] *= DEG_TO_RAD;

    lat0 = DEG_TO_RAD * ref_point[0];
    long0 = DEG_TO_RAD * ref_point[1];
    alt0 = ref_point[2];

    // Form rotation matrix
    te2n[0][0] = (-sin(lat0) * cos(long0));
    te2n[0][1] = (-sin(lat0) * sin(long0));
    te2n[0][2] = cos(lat0);

    te2n[1][0] = -sin(long0);
    te2n[1][1] = cos(long0);
    te2n[1][2] = 0.0;

    te2n[2][0] = (-cos(lat0) * cos(long0));
    te2n[2][1] = (-cos(lat0) * sin(long0));
    te2n[2][2] = -sin(lat0);

    lin_tra_lla_to_ecef(lla_point, ecef_point);

    // Multiply rotation matrix with ECEF vector
    for (row = 0; row < (DIM + 1); row++) {
        enu_point[row] = 0;
        for (col = 0; col < (DIM + 1); col++) {
            enu_point[row] += te2n[row][col] * ecef_point[col];
        }
    }

    enu_point[2] += (((float) EARTH_RADIUS) + alt0);
}

void lin_tra_lla_to_ecef(float lla_point[DIM + 1],
        float ecef_point[DIM + 1]) {
    float sinlat;
    float coslat;

    sinlat = sin(lla_point[0]);
    coslat = cos(lla_point[0]);

    ecef_point[0] = (EARTH_RADIUS - lla_point[2]) * coslat * cos(lla_point[1]);
    ecef_point[1] = (EARTH_RADIUS - lla_point[2]) * coslat * sin(lla_point[1]);
    ecef_point[2] = (EARTH_RADIUS - lla_point[2]) * sinlat;
}

#ifdef UNIT_TESTS

#include "serial.h"

#define PRECISION 0.0001

int main(void) {
    board_init();
    serial_init();

    float a[DIM] = {-30.0, 0.0};
    float b[DIM] = {30.0, 0.0};
    float c[DIM] = {0.0, 10.0};

    lin_tra_init(a, b, c);

    lin_tra_update(c);
    printf("cte = %f\r\n", lin_tra_get_cte());
    printf("path angle = %f\r\n", lin_tra_get_path_angle());

    while (1);

    return 1;
}
#endif