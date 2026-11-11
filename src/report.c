/**
 * @file report.c
 * @brief Implementation of reports and formatted output.
 *
 * Write errors on a stream are sticky, so each public function checks
 * ferror() once at the end instead of testing every fprintf() result.
 */

#include <string.h>

#include "report.h"
#include "validation.h"

#define SEPARATOR_LINE \
    "--------------------------------------------------------------------------------\n"

#define PERCENT_FACTOR      100.0

/* Everything needed to print one row about one machine. */
typedef struct
{
    char nextDueText[DATE_TEXT_SIZE];
    char lastServiceText[DATE_TEXT_SIZE];
    long daysRemaining;
    MaintenanceStatus_t status;
} MachineView_t;

/* Returns 1 if count is a sane number of used slots for the capacity. */
static int isCountValid(int count, int capacity)
{
    return (count >= 0 && count <= capacity);
}

/* Reports a write failure if any earlier fprintf on the stream failed. */
static Status_t finishOutput(FILE *out)
{
    return (ferror(out) != 0) ? STATUS_ERR_FILE_IO : STATUS_SUCCESS;
}

/* Asks the maintenance module about one machine and formats the dates. */
static Status_t describeMachine(const Equipment_t *equipment,
                                const Date_t *today, MachineView_t *view)
{
    Date_t nextDate;
    Status_t status;

    status = calculateNextServiceDate(equipment, &nextDate);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = formatDate(&nextDate, view->nextDueText,
                        sizeof view->nextDueText);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = formatDate(&equipment->lastServiceDate, view->lastServiceText,
                        sizeof view->lastServiceText);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = calculateDaysRemaining(equipment, today, &view->daysRemaining);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return determineStatus(equipment, today, &view->status);
}

static void printTableHeader(FILE *out)
{
    fprintf(out, "%-8s %-24s %-12s %-11s %9s  %s\n", "ID", "Name",
            "Last Service", "Next Due", "Days Left", "Status");
    fputs(SEPARATOR_LINE, out);
}

static void printTableRow(FILE *out, const Equipment_t *equipment,
                          const MachineView_t *view)
{
    fprintf(out, "%-8s %-24.24s %-12s %-11s %9ld  %s\n", equipment->id,
            equipment->name, view->lastServiceText, view->nextDueText,
            view->daysRemaining, getStatusName(view->status));
}

/* Prints the report title and the date it was produced for. */
static Status_t printReportTitle(FILE *out, const char *title,
                                 const Date_t *today)
{
    char todayText[DATE_TEXT_SIZE];
    Status_t status = formatDate(today, todayText, sizeof todayText);

    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    fprintf(out, "\n%s\n", title);
    fprintf(out, "Report date: %s\n", todayText);
    fputs(SEPARATOR_LINE, out);

    return STATUS_SUCCESS;
}

const char *getEquipmentTypeName(EquipmentType_t type)
{
    switch (type)
    {
        case EQTYPE_LATHE:
            return "Lathe";
        case EQTYPE_DRILL:
            return "Drill";
        case EQTYPE_MILL:
            return "Mill";
        case EQTYPE_COMPRESSOR:
            return "Compressor";
        case EQTYPE_CONVEYOR:
            return "Conveyor";
        case EQTYPE_GENERATOR:
            return "Generator";
        case EQTYPE_OTHER:
            return "Other";
        default:
            return "Unknown";
    }
}

const char *getStatusMessage(Status_t status)
{
    switch (status)
    {
        case STATUS_SUCCESS:
            return "Operation completed.";
        case STATUS_ERR_NULL_POINTER:
            return "Internal error (missing data).";
        case STATUS_ERR_INVALID_ARGUMENT:
            return "Internal error (invalid argument).";
        case STATUS_ERR_INVALID_ID:
            return "Invalid ID, use the format EQ-0001.";
        case STATUS_ERR_INVALID_DATE:
            return "Invalid date, use DD-MM-YYYY.";
        case STATUS_ERR_INVALID_VALUE:
            return "Value is outside the allowed range.";
        case STATUS_ERR_DUPLICATE:
            return "Equipment ID already exists.";
        case STATUS_ERR_NOT_FOUND:
            return "No matching record found.";
        case STATUS_ERR_EMPTY:
            return "No equipment records yet.";
        case STATUS_ERR_FULL:
            return "Storage is full.";
        case STATUS_ERR_INSUFFICIENT_DATA:
            return "Not enough records for this calculation.";
        case STATUS_ERR_FILE_OPEN:
            return "File could not be opened.";
        case STATUS_ERR_FILE_IO:
            return "File operation failed.";
        case STATUS_ERR_FILE_FORMAT:
            return "File contains invalid data.";
        default:
            return "Unknown error.";
    }
}

Status_t printEquipmentDetails(FILE *out, const Equipment_t *equipment,
                               const Date_t *today)
{
    MachineView_t view;
    char installText[DATE_TEXT_SIZE];
    Status_t status;

    if (out == NULL || equipment == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = describeMachine(equipment, today, &view);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = formatDate(&equipment->installDate, installText,
                        sizeof installText);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    fprintf(out, "\nEquipment details\n");
    fputs(SEPARATOR_LINE, out);
    fprintf(out, "ID               : %s\n", equipment->id);
    fprintf(out, "Name             : %s\n", equipment->name);
    fprintf(out, "Type             : %s\n", getEquipmentTypeName(equipment->type));
    fprintf(out, "Location         : %s\n", equipment->location);
    fprintf(out, "Installed        : %s\n", installText);
    fprintf(out, "Service interval : %d days\n", equipment->serviceIntervalDays);
    fprintf(out, "Last service     : %s\n", view.lastServiceText);
    fprintf(out, "Next service due : %s\n", view.nextDueText);
    fprintf(out, "Days remaining   : %ld\n", view.daysRemaining);
    fprintf(out, "Running hours    : %.1f\n", equipment->runningHours);
    fprintf(out, "Under repair     : %s\n",
            (equipment->underRepair == UNDER_REPAIR_YES) ? "Yes" : "No");
    fprintf(out, "Status           : %s\n", getStatusName(view.status));

    return finishOutput(out);
}

Status_t printEquipmentTable(FILE *out, const EquipmentList_t *list,
                             const Date_t *today)
{
    MachineView_t view;
    Status_t status;
    int index;

    if (out == NULL || list == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isCountValid(list->count, MAX_EQUIPMENT))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    if (list->count == 0)
    {
        fputs("No equipment to display.\n", out);
        return finishOutput(out);
    }

    printTableHeader(out);

    for (index = 0; index < list->count; ++index)
    {
        status = describeMachine(&list->items[index], today, &view);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }

        printTableRow(out, &list->items[index], &view);
    }

    fputs(SEPARATOR_LINE, out);
    fprintf(out, "Machines listed: %d\n", list->count);

    return finishOutput(out);
}

Status_t generateDueReport(FILE *out, const EquipmentList_t *list,
                           const Date_t *today, int withinDays)
{
    EquipmentList_t sorted;
    MachineView_t view;
    char title[MAX_LINE_LEN];
    Status_t status;
    int listed = 0;
    int index;

    if (out == NULL || list == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (withinDays < 0 || withinDays > MAX_INTERVAL_DAYS)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    status = getSortedByNextDue(list, &sorted);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    snprintf(title, sizeof title,
             "MAINTENANCE DUE REPORT (service due within %d days)", withinDays);

    status = printReportTitle(out, title, today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    printTableHeader(out);

    for (index = 0; index < sorted.count; ++index)
    {
        status = describeMachine(&sorted.items[index], today, &view);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }

        if (view.status != MAINT_UNDER_REPAIR && view.daysRemaining <= withinDays)
        {
            printTableRow(out, &sorted.items[index], &view);
            listed += 1;
        }
    }

    fputs(SEPARATOR_LINE, out);

    if (listed == 0)
    {
        fprintf(out, "No equipment due within %d days.\n", withinDays);
    }
    else
    {
        fprintf(out, "Machines listed: %d\n", listed);
    }

    return finishOutput(out);
}

Status_t generateOverdueReport(FILE *out, const EquipmentList_t *list,
                               const Date_t *today)
{
    EquipmentList_t sorted;
    MachineView_t view;
    Status_t status;
    int listed = 0;
    int index;

    if (out == NULL || list == NULL || today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = getSortedByNextDue(list, &sorted);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    status = printReportTitle(out, "OVERDUE EQUIPMENT REPORT", today);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    fprintf(out, "%-8s %-24s %-12s %-11s %9s\n", "ID", "Name",
            "Last Service", "Was Due", "Days Late");
    fputs(SEPARATOR_LINE, out);

    for (index = 0; index < sorted.count; ++index)
    {
        status = describeMachine(&sorted.items[index], today, &view);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }

        if (view.status == MAINT_OVERDUE)
        {
            fprintf(out, "%-8s %-24.24s %-12s %-11s %9ld\n",
                    sorted.items[index].id, sorted.items[index].name,
                    view.lastServiceText, view.nextDueText,
                    -view.daysRemaining);
            listed += 1;
        }
    }

    fputs(SEPARATOR_LINE, out);

    if (listed == 0)
    {
        fputs("No overdue equipment.\n", out);
    }
    else
    {
        fprintf(out, "Overdue machines: %d\n", listed);
    }

    return finishOutput(out);
}

Status_t generateFleetSummaryReport(FILE *out, const FleetSummary_t *summary)
{
    double total;

    if (out == NULL || summary == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (summary->total <= 0)
    {
        return STATUS_ERR_EMPTY;
    }

    total = (double)summary->total;

    fprintf(out, "\nFLEET HEALTH SUMMARY\n");
    fputs(SEPARATOR_LINE, out);
    fprintf(out, "Total equipment : %d\n", summary->total);
    fprintf(out, "OK              : %d (%.1f%%)\n", summary->okCount,
            summary->okCount * PERCENT_FACTOR / total);
    fprintf(out, "Due soon        : %d (%.1f%%)\n", summary->dueSoonCount,
            summary->dueSoonCount * PERCENT_FACTOR / total);
    fprintf(out, "Overdue         : %d (%.1f%%)\n", summary->overdueCount,
            summary->overduePercent);
    fprintf(out, "Under repair    : %d (%.1f%%)\n", summary->underRepairCount,
            summary->underRepairCount * PERCENT_FACTOR / total);

    return finishOutput(out);
}

Status_t generateServiceHistoryReport(FILE *out, const ServiceLog_t *log,
                                      const char *id)
{
    char dateText[DATE_TEXT_SIZE];
    double totalCost = 0.0;
    double average = 0.0;
    int matched = 0;
    int index;
    Status_t status;

    if (out == NULL || log == NULL || id == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    status = validateEquipmentId(id);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    if (!isCountValid(log->count, MAX_SERVICE_RECORDS))
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    for (index = 0; index < log->count; ++index)
    {
        if (strncmp(log->records[index].equipmentId, id, ID_LENGTH + 1) == 0)
        {
            matched += 1;
        }
    }

    if (matched == 0)
    {
        return STATUS_ERR_NOT_FOUND;
    }

    fprintf(out, "\nSERVICE HISTORY FOR %s\n", id);
    fputs(SEPARATOR_LINE, out);
    fprintf(out, "%-11s %-20s %12s  %s\n", "Date", "Technician", "Cost", "Remarks");
    fputs(SEPARATOR_LINE, out);

    for (index = 0; index < log->count; ++index)
    {
        const ServiceRecord_t *record = &log->records[index];

        if (strncmp(record->equipmentId, id, ID_LENGTH + 1) != 0)
        {
            continue;
        }

        status = formatDate(&record->serviceDate, dateText, sizeof dateText);
        if (status != STATUS_SUCCESS)
        {
            return status;
        }

        fprintf(out, "%-11s %-20.20s %12.2f  %s\n", dateText,
                record->technician, record->cost, record->remarks);
        totalCost += record->cost;
    }

    fputs(SEPARATOR_LINE, out);
    fprintf(out, "Services recorded : %d\n", matched);
    fprintf(out, "Total cost        : %.2f\n", totalCost);

    status = calculateAverageServiceInterval(log, id, &average);
    if (status == STATUS_SUCCESS)
    {
        fprintf(out, "Average interval  : %.1f days\n", average);
    }
    else if (status == STATUS_ERR_INSUFFICIENT_DATA)
    {
        fputs("Average interval  : not available (needs 2 or more services)\n",
              out);
    }
    else
    {
        return status;
    }

    return finishOutput(out);
}
