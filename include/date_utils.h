/**
 * @file date_utils.h
 * @brief Calendar arithmetic used by validation and maintenance modules.
 *
 * Dates are plain day/month/year values. A "valid calendar date" here means
 * a real date (month 1-12, day within the month, year 1-9999). The narrower
 * business range MIN_YEAR..MAX_YEAR is enforced by the validation module.
 */

#ifndef DATE_UTILS_H
#define DATE_UTILS_H

#include <stddef.h>

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Buffer size needed for a formatted date "DD-MM-YYYY" plus terminator. */
#define DATE_TEXT_SIZE 11

/** A calendar date. */
typedef struct
{
    int day;
    int month;
    int year;
} Date_t;

/**
 * @brief Test whether a year is a leap year (Gregorian rules).
 * @param year Year to test.
 * @return 1 if leap year, 0 otherwise.
 */
int isLeapYear(int year);

/**
 * @brief Get the number of days in a month.
 * @param month Month number, 1 to 12.
 * @param year  Year (needed for February).
 * @return 28 to 31, or 0 if the month is invalid.
 */
int getDaysInMonth(int month, int year);

/**
 * @brief Convert a date to a day count (days since 01-01-1970).
 * @param date      [in]  Date to convert.
 * @param dayNumber [out] Day count; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_DATE.
 */
Status_t dateToDayNumber(const Date_t *date, long *dayNumber);

/**
 * @brief Convert a day count (days since 01-01-1970) back to a date.
 * @param dayNumber [in]  Day count.
 * @param date      [out] Resulting date; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_DATE.
 */
Status_t dayNumberToDate(long dayNumber, Date_t *date);

/**
 * @brief Compute the date that is a given number of days after another date.
 * @param start  [in]  Starting date (must be a valid calendar date).
 * @param days   [in]  Days to add, 0 to MAX_INTERVAL_DAYS.
 * @param result [out] Resulting date; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_DATE
 *         (bad start date) or STATUS_ERR_INVALID_ARGUMENT (days out of range).
 */
Status_t addDaysToDate(const Date_t *start, int days, Date_t *result);

/**
 * @brief Compare two dates.
 * @param a First date.
 * @param b Second date.
 * @return -1 if a is earlier, 0 if equal (or either pointer is NULL),
 *         1 if a is later.
 */
int compareDates(const Date_t *a, const Date_t *b);

/**
 * @brief Read the current local date from the system clock.
 * @param today [out] Today's date; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_DATE.
 */
Status_t getTodayDate(Date_t *today);

/**
 * @brief Format a date as text "DD-MM-YYYY".
 * @param date   [in]  Date to format.
 * @param buffer [out] Destination buffer.
 * @param size   [in]  Buffer size; must be at least DATE_TEXT_SIZE.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_DATE
 *         or STATUS_ERR_INVALID_ARGUMENT (buffer too small).
 */
Status_t formatDate(const Date_t *date, char *buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* DATE_UTILS_H */
