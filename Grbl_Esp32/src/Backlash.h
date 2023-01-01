#pragma once

/**
 * Created by Nikos Siatras
 * Twitter: nsiatras
 * Website: https://www.sourcerabbit.com
 */

#include "Grbl.h"

extern float backlash_compensation_to_remove_from_mpos[MAX_N_AXIS];

void backlash_ini(void);
void backlash_compensate_backlash(float *target, plan_line_data_t *pl_data);

void backlash_reset_targets(void);
void backlash_synch_position_while_using_probe(void);
