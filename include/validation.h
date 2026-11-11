/**
 * @file validation.h
 * @brief Input validation and strict text parsing.
 *
 * Every function here is side-effect free: validators only inspect their
 * arguments, and parsers write their output parameter only on success.
 */

#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h>

#include "common.h"
#include "date_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Check that an equipment ID has the format "EQ-" plus 4 digits.
 * @param id [in] Text to check.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_ID.
 */
Status_t validateEquipmentId(const char *id);

/**
 * @brief Check that a date is real and within MIN_YEAR..MAX_YEAR.
 * @param date [in] Date to check.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_DATE.
 */
Status_t validateDate(const Date_t *date);

/**
 * @brief Check a service interval, MIN_INTERVAL_DAYS..MAX_INTERVAL_DAYS.
 * @param days [in] Interval in days.
 * @return STATUS_SUCCESS or STATUS_ERR_INVALID_VALUE.
 */
Status_t validateServiceInterval(int days);

/**
 * @brief Check running hours, 0.0..MAX_RUNNING_HOURS (NaN is rejected).
 * @param hours [in] Running hours.
 * @return STATUS_SUCCESS or STATUS_ERR_INVALID_VALUE.
 */
Status_t validateRunningHours(double hours);

/**
 * @brief Check a service cost, 0.0..MAX_COST (NaN is rejected).
 * @param cost [in] Cost value.
 * @return STATUS_SUCCESS or STATUS_ERR_INVALID_VALUE.
 */
Status_t validateCost(double cost);

/**
 * @brief Check a text field.
 *
 * Rejects text longer than maxLen, text containing FIELD_DELIMITER or control
 * characters, and (unless allowEmpty is non-zero) empty or blank text.
 *
 * @param text       [in] Text to check.
 * @param maxLen     [in] Maximum allowed length, without the terminator.
 * @param allowEmpty [in] Non-zero if empty text is acceptable.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_VALUE.
 */
Status_t validateTextField(const char *text, size_t maxLen, int allowEmpty);

/**
 * @brief Check that a menu choice lies in an inclusive range.
 * @param choice [in] Chosen number.
 * @param min    [in] Smallest allowed choice.
 * @param max    [in] Largest allowed choice.
 * @return STATUS_SUCCESS, STATUS_ERR_INVALID_VALUE (out of range) or
 *         STATUS_ERR_INVALID_ARGUMENT (min greater than max).
 */
Status_t validateMenuChoice(int choice, int min, int max);

/**
 * @brief Convert text to an int. No spaces, no trailing characters allowed.
 * @param text  [in]  Text such as "42" or "-7".
 * @param value [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_VALUE.
 */
Status_t parseInteger(const char *text, int *value);

/**
 * @brief Convert text to a finite double. No spaces, no trailing characters.
 * @param text  [in]  Text such as "12.5" or "1e3".
 * @param value [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_VALUE.
 */
Status_t parseDouble(const char *text, double *value);

/**
 * @brief Convert text "DD-MM-YYYY" to a validated date.
 * @param text [in]  Text to convert.
 * @param date [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_INVALID_DATE.
 */
Status_t parseDate(const char *text, Date_t *date);

#ifdef __cplusplus
}
#endif

#endif /* VALIDATION_H */
