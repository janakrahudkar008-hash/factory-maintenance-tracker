/**
 * @file file_handler.h
 * @brief Saving and loading of equipment and service-log text files.
 *
 * Files are plain text, one record per line, fields separated by
 * FIELD_DELIMITER. Blank lines and lines starting with '#' are ignored.
 * Every line read from a file passes through the same validators as
 * keyboard input.
 *
 * equipment.txt (9 fields):
 *   ID|Name|Type|Location|InstallDate|IntervalDays|LastServiceDate|
 *   RunningHours|UnderRepair
 * service_log.txt (5 fields):
 *   ID|ServiceDate|Technician|Cost|Remarks
 */

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "common.h"
#include "date_utils.h"
#include "equipment.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Convert one text line to a validated equipment record.
 * @param line  [in]  Line without a trailing newline.
 * @param item  [out] Result; written only on success.
 * @param today [in]  Current date, for the "not in the future" rules.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_FILE_FORMAT
 *         (wrong field count, unparsable or invalid field, line too long).
 */
Status_t parseEquipmentLine(const char *line, Equipment_t *item,
                            const Date_t *today);

/**
 * @brief Convert one text line to a validated service record.
 * @param line   [in]  Line without a trailing newline.
 * @param record [out] Result; written only on success.
 * @param today  [in]  Current date.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER or STATUS_ERR_FILE_FORMAT.
 */
Status_t parseServiceRecordLine(const char *line, ServiceRecord_t *record,
                                const Date_t *today);

/**
 * @brief Write all equipment records to a file.
 *
 * The data goes to "<path>.tmp" first and replaces the real file only after
 * a complete, successful write, so a failure cannot destroy existing data.
 *
 * @param path [in] Destination file.
 * @param list [in] Records to write.
 * @return STATUS_SUCCESS, STATUS_ERR_NULL_POINTER,
 *         STATUS_ERR_INVALID_ARGUMENT (path too long or corrupt count),
 *         STATUS_ERR_INVALID_DATE (record holds an invalid date),
 *         STATUS_ERR_FILE_OPEN or STATUS_ERR_FILE_IO.
 */
Status_t saveEquipmentToFile(const char *path, const EquipmentList_t *list);

/**
 * @brief Read equipment records from a file.
 *
 * The list is emptied first. Invalid and duplicate lines are skipped and
 * counted; the valid lines are still loaded.
 *
 * @param path         [in]  File to read.
 * @param list         [out] Loaded records.
 * @param skippedLines [out] Number of lines that were skipped.
 * @return STATUS_SUCCESS (all lines valid, or empty file),
 *         STATUS_ERR_NULL_POINTER, STATUS_ERR_FILE_OPEN,
 *         STATUS_ERR_FILE_FORMAT (at least one line skipped),
 *         STATUS_ERR_FULL (more records than MAX_EQUIPMENT; the rest are
 *         ignored) or STATUS_ERR_FILE_IO.
 */
Status_t loadEquipmentFromFile(const char *path, EquipmentList_t *list,
                               int *skippedLines);

/**
 * @brief Write all service-log entries to a file (same rules as
 *        saveEquipmentToFile).
 * @param path [in] Destination file.
 * @param log  [in] Entries to write.
 * @return Same codes as saveEquipmentToFile.
 */
Status_t saveServiceLogToFile(const char *path, const ServiceLog_t *log);

/**
 * @brief Read service-log entries from a file (same rules as
 *        loadEquipmentFromFile; capacity is MAX_SERVICE_RECORDS).
 * @param path         [in]  File to read.
 * @param log          [out] Loaded entries.
 * @param skippedLines [out] Number of lines that were skipped.
 * @return Same codes as loadEquipmentFromFile.
 */
Status_t loadServiceLogFromFile(const char *path, ServiceLog_t *log,
                                int *skippedLines);

#ifdef __cplusplus
}
#endif

#endif /* FILE_HANDLER_H */
