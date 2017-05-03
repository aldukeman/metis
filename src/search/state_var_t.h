#ifndef STATE_VAR_T_H
#define STATE_VAR_T_H

#if STATE_VAR_BYTES == 1
typedef uint8_t state_var_t;

#elif STATE_VAR_BYTES == 2
typedef uint16_t state_var_t;

#elif STATE_VAR_BYTES == 4
typedef uint32_t state_var_t;

#endif


#endif
