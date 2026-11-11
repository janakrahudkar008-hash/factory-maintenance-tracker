/**
 * @file common.h
 * @brief Project-wide constants and the status-code enumeration.
 *
 * This header has no matching source file. Every module includes it so that
 * limits and error codes are defined in exactly one place.
 */

#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ---------- Storage capacity ---------- */
#define MAX_EQUIPMENT           100
#define MAX_SERVICE_RECORDS     500

/* ---------- Equipment ID format: "EQ-" followed by 4 digits ---------- */
#define ID_PREFIX               "EQ-"
#define ID_PREFIX_LENGTH        3
#define ID_DIGITS               4
#define ID_LENGTH               7

/* ---------- Maximum text lengths (without the terminating null) ---------- */
#define MAX_NAME_LEN            30
#define MAX_LOCATION_LEN        30
#define MAX_TECHNICIAN_LEN      30
#define MAX_REMARKS_LEN         60

/* ---------- Valid numeric ranges ---------- */
#define MIN_INTERVAL_DAYS       1
#define MAX_INTERVAL_DAYS       365
#define MAX_RUNNING_HOURS       500000.0
#define MAX_COST                10000000.0

/* ---------- Accepted calendar years ---------- */
#define MIN_YEAR                2000
#define MAX_YEAR                2100

/* ---------- Maintenance rule ---------- */
#define DUE_SOON_DAYS           7

/* ---------- Input and file handling ---------- */
#define MAX_LINE_LEN            256
#define FIELD_DELIMITER         '|'
#define EQUIPMENT_FILE_PATH     "data/equipment.txt"
#define SERVICE_LOG_FILE_PATH   "data/service_log.txt"

/**
 * @brief Result code returned by every function that can fail.
 */
typedef enum
{
    STATUS_SUCCESS = 0,
    STATUS_ERR_NULL_POINTER,
    STATUS_ERR_INVALID_ARGUMENT,
    STATUS_ERR_INVALID_ID,
    STATUS_ERR_INVALID_DATE,
    STATUS_ERR_INVALID_VALUE,
    STATUS_ERR_DUPLICATE,
    STATUS_ERR_NOT_FOUND,
    STATUS_ERR_EMPTY,
    STATUS_ERR_FULL,
    STATUS_ERR_INSUFFICIENT_DATA,
    STATUS_ERR_FILE_OPEN,
    STATUS_ERR_FILE_IO,
    STATUS_ERR_FILE_FORMAT
} Status_t;

#ifdef __cplusplus
}
#endif

#endif /* COMMON_H */
