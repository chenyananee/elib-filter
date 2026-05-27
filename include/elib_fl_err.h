/* elib_fl_err.h - Filter Library Error Codes */

#ifndef ELIB_FL_ERR_H
#define ELIB_FL_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ELIB_FL_OK = 0,
    ELIB_FL_ERR_INVALID_PARAM,
    ELIB_FL_ERR_NOT_INITIALIZED,
} elib_fl_err_t;

#ifdef __cplusplus
}
#endif

#endif /* ELIB_FL_ERR_H */
