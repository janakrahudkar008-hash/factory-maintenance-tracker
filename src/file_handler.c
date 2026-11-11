/**
 * @file file_handler.c
 * @brief Implementation of text-file persistence.
 */

#include <stdio.h>
#include <string.h>

#include "file_handler.h"
#include "validation.h"

#define COMMENT_CHAR            '#'
#define TEMP_SUFFIX             ".tmp"
#define MAX_PATH_LEN            260

#define EQUIPMENT_FIELD_COUNT   9
#define EQ_FIELD_ID             0
#define EQ_FIELD_NAME           1
#define EQ_FIELD_TYPE           2
#define EQ_FIELD_LOCATION       3
#define EQ_FIELD_INSTALL_DATE   4
#define EQ_FIELD_INTERVAL       5
#define EQ_FIELD_LAST_SERVICE   6
#define EQ_FIELD_HOURS          7
#define EQ_FIELD_UNDER_REPAIR   8

#define SERVICE_FIELD_COUNT     5
#define SR_FIELD_ID             0
#define SR_FIELD_DATE           1
#define SR_FIELD_TECHNICIAN     2
#define SR_FIELD_COST           3
#define SR_FIELD_REMARKS        4

/* The '|' in these formats must match FIELD_DELIMITER. */
#define EQUIPMENT_HEADER \
    "# ID|Name|Type|Location|InstallDate|IntervalDays|LastServiceDate|" \
    "RunningHours|UnderRepair"
#define EQUIPMENT_LINE_FORMAT   "%s|%s|%d|%s|%s|%d|%s|%.2f|%d\n"
#define SERVICE_HEADER          "# ID|ServiceDate|Technician|Cost|Remarks"
#define SERVICE_LINE_FORMAT     "%s|%s|%s|%.2f|%s\n"

/* Decides what to do with one data line; returns SUCCESS, FILE_FORMAT to skip
   the line, or FULL to stop reading. */
typedef Status_t (*LineHandler)(const char *line, void *collection,
                                const Date_t *today);

/* Writes record number index of a collection to the file. */
typedef Status_t (*RecordWriter)(FILE *file, const void *collection,
                                 int index);

/* Returns 1 if count is a sane number of used slots for the capacity. */
static int isCountValid(int count, int capacity)
{
    return (count >= 0 && count <= capacity);
}

/* Copies source into destination only if it fits, including the terminator. */
static Status_t copyField(char *destination, size_t size, const char *source)
{
    size_t length = strlen(source);

    if (length >= size)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    memcpy(destination, source, length + 1);

    return STATUS_SUCCESS;
}

/* Turns any failure of a field conversion into the file-format error. */
static Status_t toFormatError(Status_t status)
{
    return (status == STATUS_SUCCESS) ? STATUS_SUCCESS
                                      : STATUS_ERR_FILE_FORMAT;
}

/* Splits a mutable line at FIELD_DELIMITER. Returns the number of fields, or
   maxFields + 1 if there are more than maxFields. */
static int splitFields(char *line, char **fields, int maxFields)
{
    int count = 0;
    char *cursor = line;

    fields[count] = cursor;
    count += 1;

    while (*cursor != '\0')
    {
        if (*cursor == FIELD_DELIMITER)
        {
            *cursor = '\0';

            if (count == maxFields)
            {
                return maxFields + 1;
            }

            fields[count] = cursor + 1;
            count += 1;
        }

        ++cursor;
    }

    return count;
}

/* Copies a line into a work buffer and splits it. */
static Status_t splitLine(const char *line, char *work, size_t workSize,
                          char **fields, int expectedFields)
{
    Status_t status = copyField(work, workSize, line);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (splitFields(work, fields, expectedFields) != expectedFields)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    return STATUS_SUCCESS;
}

static Status_t fillEquipmentTexts(Equipment_t *item, char **fields)
{
    Status_t status = copyField(item->id, sizeof item->id,
                                fields[EQ_FIELD_ID]);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = copyField(item->name, sizeof item->name, fields[EQ_FIELD_NAME]);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return copyField(item->location, sizeof item->location,
                     fields[EQ_FIELD_LOCATION]);
}

static Status_t fillEquipmentDates(Equipment_t *item, char **fields)
{
    Status_t status = parseDate(fields[EQ_FIELD_INSTALL_DATE],
                                &item->installDate);

    if (status != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    return toFormatError(parseDate(fields[EQ_FIELD_LAST_SERVICE],
                                   &item->lastServiceDate));
}

static Status_t fillEquipmentNumbers(Equipment_t *item, char **fields)
{
    int typeValue;

    if (parseInteger(fields[EQ_FIELD_TYPE], &typeValue) != STATUS_SUCCESS ||
        typeValue < 0 || typeValue >= (int)EQTYPE_COUNT)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    item->type = (EquipmentType_t)typeValue;

    if (parseInteger(fields[EQ_FIELD_INTERVAL],
                     &item->serviceIntervalDays) != STATUS_SUCCESS ||
        parseDouble(fields[EQ_FIELD_HOURS],
                    &item->runningHours) != STATUS_SUCCESS ||
        parseInteger(fields[EQ_FIELD_UNDER_REPAIR],
                     &item->underRepair) != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    return STATUS_SUCCESS;
}

Status_t parseEquipmentLine(const char *line, Equipment_t *item,
                            const Date_t *today)
{
    char work[MAX_LINE_LEN];
    char *fields[EQUIPMENT_FIELD_COUNT];
    Equipment_t parsed;
    Status_t status;

    if (line == NULL || item == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = splitLine(line, work, sizeof work, fields, EQUIPMENT_FIELD_COUNT);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    memset(&parsed, 0, sizeof parsed);

    status = fillEquipmentTexts(&parsed, fields);
    if (status == STATUS_SUCCESS)
    {
        status = fillEquipmentDates(&parsed, fields);
    }

    if (status == STATUS_SUCCESS)
    {
        status = fillEquipmentNumbers(&parsed, fields);
    }

    if (status != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    if (validateEquipmentRecord(&parsed, today) != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    *item = parsed;

    return STATUS_SUCCESS;
}

Status_t parseServiceRecordLine(const char *line, ServiceRecord_t *record,
                                const Date_t *today)
{
    char work[MAX_LINE_LEN];
    char *fields[SERVICE_FIELD_COUNT];
    ServiceRecord_t parsed;
    Status_t status;

    if (line == NULL || record == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = splitLine(line, work, sizeof work, fields, SERVICE_FIELD_COUNT);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    memset(&parsed, 0, sizeof parsed);

    if (copyField(parsed.equipmentId, sizeof parsed.equipmentId,
                  fields[SR_FIELD_ID]) != STATUS_SUCCESS ||
        copyField(parsed.technician, sizeof parsed.technician,
                  fields[SR_FIELD_TECHNICIAN]) != STATUS_SUCCESS ||
        copyField(parsed.remarks, sizeof parsed.remarks,
                  fields[SR_FIELD_REMARKS]) != STATUS_SUCCESS ||
        parseDate(fields[SR_FIELD_DATE], &parsed.serviceDate) != STATUS_SUCCESS ||
        parseDouble(fields[SR_FIELD_COST], &parsed.cost) != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    if (validateServiceRecord(&parsed, today) != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    *record = parsed;

    return STATUS_SUCCESS;
}

/* Replaces the real file with the finished temporary file. */
static Status_t replaceFile(const char *tempPath, const char *path)
{
    if (rename(tempPath, path) == 0)
    {
        return STATUS_SUCCESS;
    }

    /* Windows rename() refuses to overwrite: remove the old file and retry. */
    if (remove(path) != 0)
    {
        remove(tempPath);
        return STATUS_ERR_FILE_IO;
    }

    /* If this fails the temporary file is kept: it holds the new data. */
    if (rename(tempPath, path) != 0)
    {
        return STATUS_ERR_FILE_IO;
    }

    return STATUS_SUCCESS;
}

/* Writes header and records to "<path>.tmp", then replaces the real file. */
static Status_t saveFile(const char *path, const char *header,
                         const void *collection, int count,
                         RecordWriter writer)
{
    char tempPath[MAX_PATH_LEN];
    FILE *file;
    Status_t status = STATUS_SUCCESS;
    int written;
    int index;

    written = snprintf(tempPath, sizeof tempPath, "%s%s", path, TEMP_SUFFIX);
    if (written < 0 || (size_t)written >= sizeof tempPath)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    file = fopen(tempPath, "w");
    if (file == NULL)
    {
        return STATUS_ERR_FILE_OPEN;
    }

    if (fprintf(file, "%s\n", header) < 0)
    {
        status = STATUS_ERR_FILE_IO;
    }

    for (index = 0; status == STATUS_SUCCESS && index < count; ++index)
    {
        status = writer(file, collection, index);
    }

    if (fclose(file) != 0 && status == STATUS_SUCCESS)
    {
        status = STATUS_ERR_FILE_IO;
    }

    if (status != STATUS_SUCCESS)
    {
        remove(tempPath);
        return status;
    }

    return replaceFile(tempPath, path);
}

static Status_t writeEquipmentRecord(FILE *file, const void *collection,
                                     int index)
{
    const EquipmentList_t *list = (const EquipmentList_t *)collection;
    const Equipment_t *item = &list->items[index];
    char installText[DATE_TEXT_SIZE];
    char lastServiceText[DATE_TEXT_SIZE];
    Status_t status = formatDate(&item->installDate, installText,
                                 sizeof installText);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = formatDate(&item->lastServiceDate, lastServiceText,
                        sizeof lastServiceText);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (fprintf(file, EQUIPMENT_LINE_FORMAT, item->id, item->name,
                (int)item->type, item->location, installText,
                item->serviceIntervalDays, lastServiceText,
                item->runningHours, item->underRepair) < 0)
    {
        return STATUS_ERR_FILE_IO;
    }

    return STATUS_SUCCESS;
}

static Status_t writeServiceRecord(FILE *file, const void *collection,
                                   int index)
{
    const ServiceLog_t *log = (const ServiceLog_t *)collection;
    const ServiceRecord_t *record = &log->records[index];
    char dateText[DATE_TEXT_SIZE];
    Status_t status = formatDate(&record->serviceDate, dateText,
                                 sizeof dateText);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (fprintf(file, SERVICE_LINE_FORMAT, record->equipmentId, dateText,
                record->technician, record->cost, record->remarks) < 0)
    {
        return STATUS_ERR_FILE_IO;
    }

    return STATUS_SUCCESS;
}

Status_t saveEquipmentToFile(const char *path, const EquipmentList_t *list)
{
    if (path == NULL || list == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(list->count, MAX_EQUIPMENT))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    return saveFile(path, EQUIPMENT_HEADER, list, list->count,
                    writeEquipmentRecord);
}

Status_t saveServiceLogToFile(const char *path, const ServiceLog_t *log)
{
    if (path == NULL || log == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(log->count, MAX_SERVICE_RECORDS))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    return saveFile(path, SERVICE_HEADER, log, log->count,
                    writeServiceRecord);
}

/* Discards the rest of the current line. */
static void skipRestOfLine(FILE *file)
{
    int character = fgetc(file);

    while (character != '\n' && character != EOF)
    {
        character = fgetc(file);
    }
}

/* Removes a trailing "\n" or "\r\n" from a line. */
static void trimLineEnding(char *line)
{
    size_t length = strlen(line);

    while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r'))
    {
        line[length - 1] = '\0';
        --length;
    }
}

/* Returns 1 for blank lines and comment lines. */
static int isIgnorableLine(const char *line)
{
    size_t index = 0;

    while (line[index] == ' ' || line[index] == '\t')
    {
        ++index;
    }

    return (line[index] == '\0' || line[index] == COMMENT_CHAR);
}

/* Reads every data line of a file and gives it to the handler. */
static Status_t loadFile(const char *path, LineHandler handler,
                         void *collection, int *skippedLines)
{
    char buffer[MAX_LINE_LEN];
    Date_t today;
    FILE *file;
    Status_t result = STATUS_SUCCESS;
    Status_t status;
    size_t length;
    int skipped = 0;

    status = getTodayDate(&today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    file = fopen(path, "r");
    if (file == NULL)
    {
        return STATUS_ERR_FILE_OPEN;
    }

    while (fgets(buffer, (int)sizeof buffer, file) != NULL)
    {
        length = strlen(buffer);

        if (length > 0 && buffer[length - 1] != '\n' && !feof(file))
        {
            skipRestOfLine(file);
            skipped += 1;
            continue;
        }

        trimLineEnding(buffer);

        if (isIgnorableLine(buffer))
        {
            continue;
        }

        status = handler(buffer, collection, &today);
        if (status == STATUS_ERR_FULL)
        {
            result = STATUS_ERR_FULL;
            break;
        }

        if (status != STATUS_SUCCESS)
        {
            skipped += 1;
        }
    }

    if (result == STATUS_SUCCESS && ferror(file))
    {
        result = STATUS_ERR_FILE_IO;
    }

    if (fclose(file) != 0 && result == STATUS_SUCCESS)
    {
        result = STATUS_ERR_FILE_IO;
    }

    if (result == STATUS_SUCCESS && skipped > 0)
    {
        result = STATUS_ERR_FILE_FORMAT;
    }

    *skippedLines = skipped;

    return result;
}

static Status_t handleEquipmentLine(const char *line, void *collection,
                                    const Date_t *today)
{
    EquipmentList_t *list = (EquipmentList_t *)collection;
    Equipment_t item;
    Status_t status = parseEquipmentLine(line, &item, today);

    if (status != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    status = addEquipment(list, &item, today);
    if (status == STATUS_ERR_FULL)
    {
        return STATUS_ERR_FULL;
    }

    return toFormatError(status);
}

static Status_t handleServiceLine(const char *line, void *collection,
                                  const Date_t *today)
{
    ServiceLog_t *log = (ServiceLog_t *)collection;
    ServiceRecord_t record;
    Status_t status = parseServiceRecordLine(line, &record, today);

    if (status != STATUS_SUCCESS)
    {
        return STATUS_ERR_FILE_FORMAT;
    }

    status = addServiceRecord(log, &record);
    if (status == STATUS_ERR_FULL)
    {
        return STATUS_ERR_FULL;
    }

    return toFormatError(status);
}

Status_t loadEquipmentFromFile(const char *path, EquipmentList_t *list,
                               int *skippedLines)
{
    if (path == NULL || list == NULL || skippedLines == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    *skippedLines = 0;
    list->count = 0;

    return loadFile(path, handleEquipmentLine, list, skippedLines);
}

Status_t loadServiceLogFromFile(const char *path, ServiceLog_t *log,
                                int *skippedLines)
{
    if (path == NULL || log == NULL || skippedLines == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    *skippedLines = 0;
    log->count = 0;

    return loadFile(path, handleServiceLine, log, skippedLines);
}
