#ifndef FIG_CONFIG_H
#define FIG_CONFIG_H

/* To disable asserts: */
/* #define FIG_ASSERT(x) ((void) x) */

/* To provide custom typedefs for integer/bool types: */
/* #define FIG_TYPEDEFS
typedef TYPE_GOES_HERE fig_uint8_t;
typedef TYPE_GOES_HERE fig_uint16_t;
typedef TYPE_GOES_HERE fig_uint32_t; 
typedef TYPE_GOES_HERE fig_bool_t; */

/* To manually specify which formats to support: */
/* #define FIG_EXPLICIT_SUPPORT */

#ifndef FIG_EXPLICIT_SUPPORT
#define FIG_LOAD_GIF
#define FIG_SAVE_GIF
#endif 

#endif
