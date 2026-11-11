/**
 * @file equipment.h
 * @brief Equipment and service-log data model and operations.
 *
 * The collections are fixed-size arrays inside structures; they are owned by
 * the caller (main) and passed by pointer. Every function that modifies a
 * collection is all-or-nothing: on any error the collection is unchanged.
 */

#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "common.h"
#include "date_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Values of Equipment_t.underRepair. */
#define UNDER_REPAIR_NO         0
#define UNDER_REPAIR_YES        1

/** Equipment category. EQTYPE_COUNT is the number of types, not a real type. */
typedef enum
{
    EQTYPE_LATHE = 0,
    EQTYPE_DRILL,
    EQTYPE_MILL,
    EQTYPE_COMPRESSOR,
    EQTYPE_CONVEYOR,
    EQTYPE_GENERATOR,
    EQTYPE_OTHER,
    EQTYPE_COUNT
} EquipmentType_t;

/** One machine under maintenance. */
typedef struct
{
    char id[ID_LENGTH + 1];
    char name[MAX_NAME_LEN + 1];
    EquipmentType_t type;
    char location[MAX_LOCATION_LEN + 1];
    Date_t installDate;
    int serviceIntervalDays;
    Date_t lastServiceDate;
    double runningHours;
    int underRepair;
} Equipment_t;

/** One completed service, linked to a machine by equipmentId. */
typedef struct
{
    char equipmentId[ID_LENGTH + 1];
    Date_t serviceDate;
    char technician[MAX_TECHNICIAN_LEN + 1];
    double cost;
    char remarks[MAX_REMARKS_LEN + 1];
} ServiceRecord_t;

/** Collection of equipment records. */
typedef struct
{
    Equipment_t items[MAX_EQUIPMENT];
    int count;
} EquipmentList_t;

/** Collection of service-log entries. */
typedef struct
{
    ServiceRecord_t records[MAX_SERVICE_RECORDS];
    int count;
} ServiceLog_t;

/**
 * @brief Validate every field of an equipment record, including cross-field
 *        rules (last service not before install date, no future dates).
 * @param item  [in] Record to check.
 * @param today [in] Current date, used for the "not in the future" rules.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, or the first error found:
 *         STATUS_ERR_INVALID_ID, STATUS_ERR_INVALID_DATE or
 *         STATUS_ERR_INVALID_VALUE.
 */
Status_t validateEquipmentRecord(const Equipment_t *item, const Date_t *today);

/**
 * @brief Validate every field of a service record (no future service date).
 * @param record [in] Record to check.
 * @param today  [in] Current date.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, or the first error found.
 */
Status_t validateServiceRecord(const ServiceRecord_t *record,
                               const Date_t *today);

/**
 * @brief Empty an equipment list.
 * @param list [out] List to reset.
 * @return STATUS_SUCCESS or STATUS_ERR_NULL_POINTER.
 */
Status_t initEquipmentList(EquipmentList_t *list);

/**
 * @brief Empty a service log.
 * @param log [out] Log to reset.
 * @return STATUS_SUCCESS or STATUS_ERR_NULL_POINTER.
 */
Status_t initServiceLog(ServiceLog_t *log);

/**
 * @brief Append a validated, unique record to the list.
 * @param list    [in,out] List to extend.
 * @param newItem [in]     Record to add (copied).
 * @param today   [in]     Current date, for validation.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER,
 *         STATUS_ERR_INVALID_ARGUMENT (corrupt count), a validation code,
 *         STATUS_ERR_DUPLICATE or STATUS_ERR_FULL.
 */
Status_t addEquipment(EquipmentList_t *list, const Equipment_t *newItem,
                      const Date_t *today);

/**
 * @brief Find a record by its ID (linear search).
 * @param list  [in]  List to search.
 * @param id    [in]  ID to look for.
 * @param index [out] Position of the record; written only on success.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ARGUMENT
 *         (corrupt count), STATUS_ERR_INVALID_ID or STATUS_ERR_NOT_FOUND.
 */
Status_t findEquipmentById(const EquipmentList_t *list, const char *id,
                           int *index);

/**
 * @brief Replace the fields of an existing record. The ID never changes: the
 *        id member of newData is ignored.
 * @param list    [in,out] List holding the record.
 * @param id      [in]     ID of the record to update.
 * @param newData [in]     New field values.
 * @param today   [in]     Current date, for validation.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ID,
 *         STATUS_ERR_NOT_FOUND or a validation code.
 */
Status_t updateEquipment(EquipmentList_t *list, const char *id,
                         const Equipment_t *newData, const Date_t *today);

/**
 * @brief Remove a record and close the gap, keeping the order of the rest.
 *        The service history of the machine is not touched.
 * @param list [in,out] List holding the record.
 * @param id   [in]     ID of the record to delete.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER, STATUS_ERR_INVALID_ID or
 *         STATUS_ERR_NOT_FOUND.
 */
Status_t deleteEquipment(EquipmentList_t *list, const char *id);

/**
 * @brief Append an entry to the service log without further checks.
 * @param log    [in,out] Log to extend.
 * @param record [in]     Entry to add (copied).
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER,
 *         STATUS_ERR_INVALID_ARGUMENT (corrupt count) or STATUS_ERR_FULL.
 */
Status_t addServiceRecord(ServiceLog_t *log, const ServiceRecord_t *record);

/**
 * @brief Record a service and update the machine.
 *
 * On success the entry is appended to the log, the machine's lastServiceDate
 * moves forward if the service is later than the current one, and the
 * under-repair flag is cleared.
 *
 * @param equipment [in,out] Equipment list.
 * @param log       [in,out] Service log.
 * @param record    [in]     Service to record.
 * @param today     [in]     Current date, for validation.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER,
 *         STATUS_ERR_INVALID_ARGUMENT (corrupt count), a validation code,
 *         STATUS_ERR_NOT_FOUND or STATUS_ERR_FULL.
 */
Status_t logServiceEvent(EquipmentList_t *equipment, ServiceLog_t *log,
                         const ServiceRecord_t *record, const Date_t *today);

#ifdef __cplusplus
}
#endif

#endif /* EQUIPMENT_H */
