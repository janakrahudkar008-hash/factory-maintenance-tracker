/**
 * @file equipment.c
 * @brief Implementation of equipment and service-log management.
 */

#include <string.h>

#include "equipment.h"
#include "validation.h"

/* Returns 1 if the buffer contains a null terminator within size bytes. */
static int isTerminated(const char *buffer, size_t size)
{
    return (memchr(buffer, '\0', size) != NULL);
}

/* Returns 1 if count is a sane number of used slots for the capacity. */
static int isCountValid(int count, int capacity)
{
    return (count >= 0 && count <= capacity);
}

/* Returns the index of the record with this ID, or -1 if absent. */
static int locateId(const EquipmentList_t *list, const char *id)
{
    int index;

    for (index = 0; index < list->count; ++index)
    {
        if (strcmp(list->items[index].id, id) == 0)
        {
            return index;
        }
    }

    return -1;
}

/* Checks an ID stored in a fixed buffer, including its terminator. */
static Status_t validateIdBuffer(const char *id)
{
    if (!isTerminated(id, ID_LENGTH + 1))
    {
        return STATUS_ERR_INVALID_ID;
    }

    return validateEquipmentId(id);
}

/* Checks a text field stored in a fixed buffer, including its terminator. */
static Status_t validateTextBuffer(const char *buffer, size_t size,
                                   size_t maxLen, int allowEmpty)
{
    if (!isTerminated(buffer, size))
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return validateTextField(buffer, maxLen, allowEmpty);
}

/* A real, in-range date that is not later than today. */
static Status_t validateNotFutureDate(const Date_t *date, const Date_t *today)
{
    Status_t status = validateDate(date);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (compareDates(date, today) > 0)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    return STATUS_SUCCESS;
}

static Status_t validateEquipmentTexts(const Equipment_t *item)
{
    Status_t status = validateIdBuffer(item->id);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateTextBuffer(item->name, sizeof item->name,
                                MAX_NAME_LEN, 0);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return validateTextBuffer(item->location, sizeof item->location,
                              MAX_LOCATION_LEN, 0);
}

static Status_t validateEquipmentDates(const Equipment_t *item,
                                       const Date_t *today)
{
    Status_t status = validateNotFutureDate(&item->installDate, today);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateNotFutureDate(&item->lastServiceDate, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (compareDates(&item->lastServiceDate, &item->installDate) < 0)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    return STATUS_SUCCESS;
}

static Status_t validateEquipmentNumbers(const Equipment_t *item)
{
    /* Cast: an enum may be unsigned, which would make "< 0" a warning. */
    int typeValue = (int)item->type;
    Status_t status;

    if (typeValue < 0 || typeValue >= (int)EQTYPE_COUNT)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    status = validateServiceInterval(item->serviceIntervalDays);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateRunningHours(item->runningHours);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (item->underRepair != UNDER_REPAIR_NO &&
        item->underRepair != UNDER_REPAIR_YES)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return STATUS_SUCCESS;
}

Status_t validateEquipmentRecord(const Equipment_t *item, const Date_t *today)
{
    Status_t status;

    if (item == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = validateDate(today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateEquipmentTexts(item);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateEquipmentDates(item, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return validateEquipmentNumbers(item);
}

Status_t validateServiceRecord(const ServiceRecord_t *record,
                               const Date_t *today)
{
    Status_t status;

    if (record == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = validateDate(today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateIdBuffer(record->equipmentId);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateNotFutureDate(&record->serviceDate, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateTextBuffer(record->technician, sizeof record->technician,
                                MAX_TECHNICIAN_LEN, 0);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = validateCost(record->cost);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return validateTextBuffer(record->remarks, sizeof record->remarks,
                              MAX_REMARKS_LEN, 1);
}

Status_t initEquipmentList(EquipmentList_t *list)
{
    if (list == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    list->count = 0;

    return STATUS_SUCCESS;
}

Status_t initServiceLog(ServiceLog_t *log)
{
    if (log == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    log->count = 0;

    return STATUS_SUCCESS;
}

Status_t addEquipment(EquipmentList_t *list, const Equipment_t *newItem,
                      const Date_t *today)
{
    Status_t status;

    if (list == NULL || newItem == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(list->count, MAX_EQUIPMENT))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    status = validateEquipmentRecord(newItem, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (locateId(list, newItem->id) >= 0)
    {
        return STATUS_ERR_DUPLICATE;
    }

    if (list->count >= MAX_EQUIPMENT)
    {
        return STATUS_ERR_FULL;
    }

    list->items[list->count] = *newItem;
    list->count += 1;

    return STATUS_SUCCESS;
}

Status_t findEquipmentById(const EquipmentList_t *list, const char *id,
                           int *index)
{
    Status_t status;
    int found;

    if (list == NULL || id == NULL || index == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(list->count, MAX_EQUIPMENT))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    status = validateEquipmentId(id);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    found = locateId(list, id);
    if (found < 0)
    {
        return STATUS_ERR_NOT_FOUND;
    }

    *index = found;

    return STATUS_SUCCESS;
}

Status_t updateEquipment(EquipmentList_t *list, const char *id,
                         const Equipment_t *newData, const Date_t *today)
{
    Equipment_t updated;
    Status_t status;
    int index;

    if (list == NULL || id == NULL || newData == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = findEquipmentById(list, id, &index);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    /* Work on a copy so a validation failure leaves the record untouched. */
    updated = *newData;
    memcpy(updated.id, list->items[index].id, sizeof updated.id);

    status = validateEquipmentRecord(&updated, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    list->items[index] = updated;

    return STATUS_SUCCESS;
}

Status_t deleteEquipment(EquipmentList_t *list, const char *id)
{
    Status_t status;
    int index;
    int remaining;

    if (list == NULL || id == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = findEquipmentById(list, id, &index);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    remaining = list->count - index - 1;
    memmove(&list->items[index], &list->items[index + 1],
            (size_t)remaining * sizeof list->items[0]);
    list->count -= 1;

    return STATUS_SUCCESS;
}

Status_t addServiceRecord(ServiceLog_t *log, const ServiceRecord_t *record)
{
    if (log == NULL || record == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(log->count, MAX_SERVICE_RECORDS))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    if (log->count >= MAX_SERVICE_RECORDS)
    {
        return STATUS_ERR_FULL;
    }

    log->records[log->count] = *record;
    log->count += 1;

    return STATUS_SUCCESS;
}

Status_t logServiceEvent(EquipmentList_t *equipment, ServiceLog_t *log,
                         const ServiceRecord_t *record, const Date_t *today)
{
    Equipment_t *machine;
    Status_t status;
    int index;

    if (equipment == NULL || log == NULL || record == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(equipment->count, MAX_EQUIPMENT))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    status = validateServiceRecord(record, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    index = locateId(equipment, record->equipmentId);
    if (index < 0)
    {
        return STATUS_ERR_NOT_FOUND;
    }

    /* Append first: if the log is full, the machine must stay unchanged. */
    status = addServiceRecord(log, record);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    machine = &equipment->items[index];

    if (compareDates(&record->serviceDate, &machine->lastServiceDate) > 0)
    {
        machine->lastServiceDate = record->serviceDate;
    }

    machine->underRepair = UNDER_REPAIR_NO;

    return STATUS_SUCCESS;
}
