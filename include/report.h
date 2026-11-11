/**
 * @file report.h
 * @brief Reports and formatted output.
 *
 * Every report writes to a caller-supplied stream, so the application can
 * print to the screen and the unit tests can write to a temporary file.
 * This module performs no calculation of its own; it asks the maintenance
 * module and only formats the answers.
 */

#ifndef REPORT_H
#define REPORT_H

#include <stdio.h>

#include "common.h"
#include "date_utils.h"
#include "equipment.h"
#include "maintenance.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Text name of an equipment type.
 * @param type Equipment type.
 * @return "Lathe", "Drill", "Mill", "Compressor", "Conveyor", "Generator",
 *         "Other", or "Unknown" for an out-of-range value.
 */
const char *getEquipmentTypeName(EquipmentType_t type);

/**
 * @brief User-friendly text for a status code.
 * @param status Status code.
 * @return Message text; "Unknown error" for an unrecognised code.
 */
const char *getStatusMessage(Status_t status);

/**
 * @brief Print every field of one machine, plus its derived due date and
 *        maintenance status.
 * @param out       [in] Output stream.
 * @param equipment [in] Machine to describe.
 * @param today     [in] Current date.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, a date error from the
 *         record, or STATUS_ERR_FILE_IO (write failure).
 */
Status_t printEquipmentDetails(FILE *out, const Equipment_t *equipment,
                               const Date_t *today);

/**
 * @brief Print a table of machines with next due date, days left and status.
 *        An empty list prints a short notice.
 * @param out   [in] Output stream.
 * @param list  [in] Machines to show (printed in list order).
 * @param today [in] Current date.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER,
 *         STATUS_ERR_INVALID_ARGUMENT (corrupt count), a date error, or
 *         STATUS_ERR_FILE_IO.
 */
Status_t printEquipmentTable(FILE *out, const EquipmentList_t *list,
                             const Date_t *today);

/**
 * @brief Maintenance due report: machines (not under repair) whose next
 *        service is within a number of days, overdue machines included,
 *        earliest due date first.
 * @param out        [in] Output stream.
 * @param list       [in] Equipment list.
 * @param today      [in] Current date.
 * @param withinDays [in] Look-ahead window, 0 to MAX_INTERVAL_DAYS.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER,
 *         STATUS_ERR_INVALID_ARGUMENT (bad window or corrupt count), a date
 *         error, or STATUS_ERR_FILE_IO.
 */
Status_t generateDueReport(FILE *out, const EquipmentList_t *list,
                           const Date_t *today, int withinDays);

/**
 * @brief Overdue report: machines whose service date has passed, most
 *        overdue first, with the number of days late.
 * @param out   [in] Output stream.
 * @param list  [in] Equipment list.
 * @param today [in] Current date.
 * @return Same codes as generateDueReport (except the window error).
 */
Status_t generateOverdueReport(FILE *out, const EquipmentList_t *list,
                               const Date_t *today);

/**
 * @brief Fleet health summary: counts and percentages per status.
 * @param out     [in] Output stream.
 * @param summary [in] Figures from calculateFleetSummary.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_EMPTY (total
 *         is not positive) or STATUS_ERR_FILE_IO.
 */
Status_t generateFleetSummaryReport(FILE *out, const FleetSummary_t *summary);

/**
 * @brief Service history of one machine with total cost and average
 *        interval between services.
 * @param out [in] Output stream.
 * @param log [in] Service log.
 * @param id  [in] Machine ID.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ID,
 *         STATUS_ERR_NOT_FOUND (no entries; nothing is printed),
 *         STATUS_ERR_INVALID_ARGUMENT (corrupt count), a date error, or
 *         STATUS_ERR_FILE_IO.
 */
Status_t generateServiceHistoryReport(FILE *out, const ServiceLog_t *log,
                                      const char *id);

#ifdef __cplusplus
}
#endif

#endif /* REPORT_H */
