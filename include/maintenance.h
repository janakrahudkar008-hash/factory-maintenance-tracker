/**
 * @file maintenance.h
 * @brief Maintenance decisions and calculations.
 *
 * The maintenance status is always derived from dates; it is never stored.
 * "Today" is passed in so every calculation is deterministic.
 */

#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include "common.h"
#include "date_utils.h"
#include "equipment.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Derived condition of a machine. MAINT_COUNT is not a real status. */
typedef enum
{
    MAINT_OK = 0,
    MAINT_DUE_SOON,
    MAINT_OVERDUE,
    MAINT_UNDER_REPAIR,
    MAINT_COUNT
} MaintenanceStatus_t;

/** Counts of machines per status. */
typedef struct
{
    int total;
    int okCount;
    int dueSoonCount;
    int overdueCount;
    int underRepairCount;
    double overduePercent;
} FleetSummary_t;

/**
 * @brief Next service date = last service date + service interval.
 * @param equipment [in]  Machine.
 * @param nextDate  [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_DATE
 *         (bad last service date) or STATUS_ERR_INVALID_ARGUMENT (interval
 *         outside 0..MAX_INTERVAL_DAYS).
 */
Status_t calculateNextServiceDate(const Equipment_t *equipment,
                                  Date_t *nextDate);

/**
 * @brief Days from today until the next service (negative if overdue).
 * @param equipment [in]  Machine.
 * @param today     [in]  Current date.
 * @param days      [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_DATE
 *         or STATUS_ERR_INVALID_ARGUMENT.
 */
Status_t calculateDaysRemaining(const Equipment_t *equipment,
                                const Date_t *today, long *days);

/**
 * @brief Decide the maintenance status of one machine.
 *
 * Under repair overrides everything. Otherwise: fewer than 0 days left is
 * overdue, 0..DUE_SOON_DAYS is due soon, anything more is OK.
 *
 * @param equipment [in]  Machine.
 * @param today     [in]  Current date.
 * @param status    [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_DATE
 *         or STATUS_ERR_INVALID_ARGUMENT.
 */
Status_t determineStatus(const Equipment_t *equipment, const Date_t *today,
                         MaintenanceStatus_t *status);

/**
 * @brief Text name of a status.
 * @param status Status value.
 * @return "OK", "DUE SOON", "OVERDUE", "UNDER REPAIR", or "UNKNOWN".
 */
const char *getStatusName(MaintenanceStatus_t status);

/**
 * @brief Copy a list sorted by next service date, earliest first.
 *
 * The sort is stable (equal dates keep their original order). The source is
 * not modified and must be a different object from sorted.
 *
 * @param source [in]  List to sort.
 * @param sorted [out] Sorted copy; unchanged on error.
 * @return STATUS_SUCCESS (an empty source gives an empty result),
 *         STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ARGUMENT (same object
 *         or corrupt count) or a date error from a record.
 */
Status_t getSortedByNextDue(const EquipmentList_t *source,
                            EquipmentList_t *sorted);

/**
 * @brief Copy the machines that have a given status, in source order.
 * @param source [in]  List to filter; must differ from result.
 * @param wanted [in]  Status to select.
 * @param today  [in]  Current date.
 * @param result [out] Matches; unchanged on error.
 * @return STATUS_SUCCESS (no matches gives an empty result),
 *         STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ARGUMENT (bad status,
 *         same object, corrupt count) or a date error from a record.
 */
Status_t filterByStatus(const EquipmentList_t *source,
                        MaintenanceStatus_t wanted, const Date_t *today,
                        EquipmentList_t *result);

/**
 * @brief Average days between services of one machine:
 *        (latest - earliest) / (records - 1).
 * @param log     [in]  Service log.
 * @param id      [in]  Machine ID.
 * @param average [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ID,
 *         STATUS_ERR_INVALID_ARGUMENT (corrupt count), STATUS_ERR_INVALID_DATE
 *         (bad record date) or STATUS_ERR_INSUFFICIENT_DATA (fewer than 2
 *         records for this machine).
 */
Status_t calculateAverageServiceInterval(const ServiceLog_t *log,
                                         const char *id, double *average);

/**
 * @brief Count machines per status and the overdue percentage.
 * @param list    [in]  Equipment list.
 * @param today   [in]  Current date.
 * @param summary [out] Result; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_EMPTY (no
 *         equipment), STATUS_ERR_INVALID_ARGUMENT or a date error.
 */
Status_t calculateFleetSummary(const EquipmentList_t *list,
                               const Date_t *today, FleetSummary_t *summary);

#ifdef __cplusplus
}
#endif

#endif /* MAINTENANCE_H */
