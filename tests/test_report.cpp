/**
 * @file test_report.cpp
 * @brief Unit tests for the report module.
 *
 * Reports are written to a temporary file in the working directory (named
 * test_tmp_report.txt, which .gitignore excludes) and read back as text.
 */

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "common.h"
#include "date_utils.h"
#include "equipment.h"
#include "maintenance.h"
#include "report.h"

namespace
{

const char *const TEMP_FILE_NAME = "test_tmp_report.txt";

Date_t makeDate(int day, int month, int year)
{
    Date_t date = {day, month, year};
    return date;
}

void copyText(char *destination, size_t size, const char *source)
{
    std::snprintf(destination, size, "%s", source);
}

/* Builds a machine whose next service is daysLeft days after today. */
Equipment_t makeMachine(const char *id, const char *name, long daysLeft,
                        int underRepair, const Date_t &today)
{
    const int interval = 100;
    Equipment_t item;
    Date_t lastService;
    long todayNumber = 0;

    std::memset(&item, 0, sizeof item);
    copyText(item.id, sizeof item.id, id);
    copyText(item.name, sizeof item.name, name);
    item.type = EQTYPE_MILL;
    copyText(item.location, sizeof item.location, "Shop Floor");
    item.installDate = makeDate(1, 1, 2020);
    item.serviceIntervalDays = interval;
    item.runningHours = 500.0;
    item.underRepair = underRepair;

    dateToDayNumber(&today, &todayNumber);
    dayNumberToDate(todayNumber - (interval - daysLeft), &lastService);
    item.lastServiceDate = lastService;

    return item;
}

ServiceRecord_t makeRecord(const char *id, const Date_t &date, double cost,
                           const char *remarks)
{
    ServiceRecord_t record;

    std::memset(&record, 0, sizeof record);
    copyText(record.equipmentId, sizeof record.equipmentId, id);
    record.serviceDate = date;
    copyText(record.technician, sizeof record.technician, "R. Patil");
    record.cost = cost;
    copyText(record.remarks, sizeof record.remarks, remarks);

    return record;
}

} // namespace

class ReportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        today = makeDate(15, 6, 2025);
        initEquipmentList(&list);
        initServiceLog(&log);
        output = std::fopen(TEMP_FILE_NAME, "w+");
        ASSERT_NE(nullptr, output);
    }

    void TearDown() override
    {
        if (output != nullptr)
        {
            std::fclose(output);
        }
        std::remove(TEMP_FILE_NAME);
    }

    void add(const Equipment_t &item)
    {
        ASSERT_EQ(STATUS_SUCCESS, addEquipment(&list, &item, &today));
    }

    /* Everything written to the stream so far. */
    std::string written()
    {
        std::string text;
        char buffer[256];
        size_t count;

        std::fflush(output);
        std::rewind(output);
        while ((count = std::fread(buffer, 1, sizeof buffer, output)) > 0)
        {
            text.append(buffer, count);
        }

        return text;
    }

    bool contains(const std::string &needle)
    {
        return written().find(needle) != std::string::npos;
    }

    EquipmentList_t list;
    ServiceLog_t log;
    Date_t today;
    FILE *output = nullptr;
};

/* ---------- getEquipmentTypeName ---------- */

TEST_F(ReportTest, TypeNamesMatchTheEnumeration)
{
    EXPECT_STREQ("Lathe", getEquipmentTypeName(EQTYPE_LATHE));
    EXPECT_STREQ("Generator", getEquipmentTypeName(EQTYPE_GENERATOR));
    EXPECT_STREQ("Other", getEquipmentTypeName(EQTYPE_OTHER));
}

TEST_F(ReportTest, OutOfRangeTypeIsUnknown)
{
    EXPECT_STREQ("Unknown", getEquipmentTypeName(static_cast<EquipmentType_t>(EQTYPE_COUNT)));
}

/* ---------- getStatusMessage ---------- */

TEST_F(ReportTest, EveryStatusCodeHasADistinctMessage)
{
    std::string seen;

    for (int code = STATUS_SUCCESS; code <= STATUS_ERR_FILE_FORMAT; ++code)
    {
        const char *message = getStatusMessage(static_cast<Status_t>(code));

        ASSERT_NE(nullptr, message);
        EXPECT_GT(std::strlen(message), 0u);
        EXPECT_EQ(std::string::npos, seen.find(std::string("|") + message + "|"));
        seen += std::string("|") + message + "|";
    }
}

TEST_F(ReportTest, UnrecognisedStatusCodeGivesUnknownError)
{
    EXPECT_STREQ("Unknown error.", getStatusMessage(static_cast<Status_t>(999)));
}

/* ---------- printEquipmentDetails ---------- */

TEST_F(ReportTest, DetailsShowFieldsDueDateAndStatus)
{
    Equipment_t item = makeMachine("EQ-0001", "Test Mill", 10, UNDER_REPAIR_NO, today);

    EXPECT_EQ(STATUS_SUCCESS, printEquipmentDetails(output, &item, &today));
    EXPECT_TRUE(contains("EQ-0001"));
    EXPECT_TRUE(contains("Test Mill"));
    EXPECT_TRUE(contains("Mill"));
    EXPECT_TRUE(contains("Days remaining   : 10"));
    EXPECT_TRUE(contains("Status           : OK"));
}

TEST_F(ReportTest, DetailsShowUnderRepairFlag)
{
    Equipment_t item = makeMachine("EQ-0001", "Test Mill", 10, UNDER_REPAIR_YES, today);

    EXPECT_EQ(STATUS_SUCCESS, printEquipmentDetails(output, &item, &today));
    EXPECT_TRUE(contains("Under repair     : Yes"));
    EXPECT_TRUE(contains("UNDER REPAIR"));
}

TEST_F(ReportTest, DetailsRejectNullArguments)
{
    Equipment_t item = makeMachine("EQ-0001", "Test Mill", 10, UNDER_REPAIR_NO, today);

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, printEquipmentDetails(nullptr, &item, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, printEquipmentDetails(output, nullptr, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, printEquipmentDetails(output, &item, nullptr));
}

/* ---------- printEquipmentTable ---------- */

TEST_F(ReportTest, TableListsEveryMachineWithItsStatus)
{
    add(makeMachine("EQ-0001", "Alpha", 40, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0002", "Bravo", -3, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, printEquipmentTable(output, &list, &today));
    EXPECT_TRUE(contains("EQ-0001"));
    EXPECT_TRUE(contains("EQ-0002"));
    EXPECT_TRUE(contains("OVERDUE"));
    EXPECT_TRUE(contains("Machines listed: 2"));
}

TEST_F(ReportTest, TablePrintsNegativeDaysForOverdueMachine)
{
    add(makeMachine("EQ-0002", "Bravo", -3, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, printEquipmentTable(output, &list, &today));
    EXPECT_TRUE(contains("-3"));
}

TEST_F(ReportTest, TableOfEmptyListPrintsNotice)
{
    EXPECT_EQ(STATUS_SUCCESS, printEquipmentTable(output, &list, &today));
    EXPECT_TRUE(contains("No equipment to display."));
}

TEST_F(ReportTest, TableTruncatesVeryLongNamesInsteadOfBreakingColumns)
{
    add(makeMachine("EQ-0001", "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234", 40, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, printEquipmentTable(output, &list, &today));
    EXPECT_TRUE(contains("ABCDEFGHIJKLMNOPQRSTUVWX "));
    EXPECT_FALSE(contains("ABCDEFGHIJKLMNOPQRSTUVWXY"));
}

TEST_F(ReportTest, TableRejectsNullArgumentsAndCorruptCount)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, printEquipmentTable(nullptr, &list, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, printEquipmentTable(output, nullptr, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, printEquipmentTable(output, &list, nullptr));

    list.count = -1;
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, printEquipmentTable(output, &list, &today));
}

/* ---------- generateDueReport ---------- */

TEST_F(ReportTest, DueReportIncludesMachinesInsideTheWindow)
{
    add(makeMachine("EQ-0001", "Soon", 5, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 30));
    EXPECT_TRUE(contains("EQ-0001"));
    EXPECT_TRUE(contains("Machines listed: 1"));
}

TEST_F(ReportTest, DueReportExcludesMachinesOutsideTheWindow)
{
    add(makeMachine("EQ-0001", "Later", 31, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 30));
    EXPECT_FALSE(contains("EQ-0001"));
    EXPECT_TRUE(contains("No equipment due within 30 days."));
}

TEST_F(ReportTest, DueReportIncludesMachineExactlyOnTheWindowEdge)
{
    add(makeMachine("EQ-0001", "Edge", 30, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 30));
    EXPECT_TRUE(contains("EQ-0001"));
}

TEST_F(ReportTest, DueReportIncludesOverdueMachines)
{
    add(makeMachine("EQ-0001", "Late", -20, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 7));
    EXPECT_TRUE(contains("EQ-0001"));
}

TEST_F(ReportTest, DueReportExcludesMachinesUnderRepair)
{
    add(makeMachine("EQ-0001", "Broken", 2, UNDER_REPAIR_YES, today));

    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 30));
    EXPECT_FALSE(contains("EQ-0001"));
}

TEST_F(ReportTest, DueReportListsEarliestDueFirst)
{
    add(makeMachine("EQ-0001", "Second", 20, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0002", "First", 3, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 30));

    const std::string text = written();
    EXPECT_LT(text.find("EQ-0002"), text.find("EQ-0001"));
}

TEST_F(ReportTest, DueReportAcceptsWindowBoundaries)
{
    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, 0));
    EXPECT_EQ(STATUS_SUCCESS, generateDueReport(output, &list, &today, MAX_INTERVAL_DAYS));
}

TEST_F(ReportTest, DueReportRejectsWindowOutsideRange)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, generateDueReport(output, &list, &today, -1));
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT,
              generateDueReport(output, &list, &today, MAX_INTERVAL_DAYS + 1));
}

TEST_F(ReportTest, DueReportRejectsNullArguments)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateDueReport(nullptr, &list, &today, 30));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateDueReport(output, nullptr, &today, 30));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateDueReport(output, &list, nullptr, 30));
}

/* ---------- generateOverdueReport ---------- */

TEST_F(ReportTest, OverdueReportListsOnlyOverdueMachinesWithDaysLate)
{
    add(makeMachine("EQ-0001", "Fine", 40, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0002", "Late", -12, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateOverdueReport(output, &list, &today));
    EXPECT_FALSE(contains("EQ-0001"));
    EXPECT_TRUE(contains("EQ-0002"));
    EXPECT_TRUE(contains("12"));
    EXPECT_TRUE(contains("Overdue machines: 1"));
}

TEST_F(ReportTest, OverdueReportListsMostOverdueFirst)
{
    add(makeMachine("EQ-0001", "Slightly", -2, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0002", "Very", -50, UNDER_REPAIR_NO, today));

    EXPECT_EQ(STATUS_SUCCESS, generateOverdueReport(output, &list, &today));

    const std::string text = written();
    EXPECT_LT(text.find("EQ-0002"), text.find("EQ-0001"));
}

TEST_F(ReportTest, OverdueReportExcludesMachineDueTodayAndUnderRepair)
{
    add(makeMachine("EQ-0001", "Today", 0, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0002", "Broken", -30, UNDER_REPAIR_YES, today));

    EXPECT_EQ(STATUS_SUCCESS, generateOverdueReport(output, &list, &today));
    EXPECT_FALSE(contains("EQ-0001"));
    EXPECT_FALSE(contains("EQ-0002"));
    EXPECT_TRUE(contains("No overdue equipment."));
}

TEST_F(ReportTest, OverdueReportOfEmptyListSaysNoneOverdue)
{
    EXPECT_EQ(STATUS_SUCCESS, generateOverdueReport(output, &list, &today));
    EXPECT_TRUE(contains("No overdue equipment."));
}

TEST_F(ReportTest, OverdueReportRejectsNullArguments)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateOverdueReport(nullptr, &list, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateOverdueReport(output, nullptr, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateOverdueReport(output, &list, nullptr));
}

/* ---------- generateFleetSummaryReport ---------- */

TEST_F(ReportTest, FleetSummaryShowsCountsAndPercentages)
{
    FleetSummary_t summary = {4, 1, 1, 1, 1, 25.0};

    EXPECT_EQ(STATUS_SUCCESS, generateFleetSummaryReport(output, &summary));
    EXPECT_TRUE(contains("Total equipment : 4"));
    EXPECT_TRUE(contains("Overdue         : 1 (25.0%)"));
    EXPECT_TRUE(contains("OK              : 1 (25.0%)"));
}

TEST_F(ReportTest, FleetSummaryOfCalculatedFigures)
{
    FleetSummary_t summary;

    add(makeMachine("EQ-0001", "A", 40, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0002", "B", -5, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0003", "C", 2, UNDER_REPAIR_NO, today));
    add(makeMachine("EQ-0004", "D", 60, UNDER_REPAIR_NO, today));
    ASSERT_EQ(STATUS_SUCCESS, calculateFleetSummary(&list, &today, &summary));

    EXPECT_EQ(STATUS_SUCCESS, generateFleetSummaryReport(output, &summary));
    EXPECT_TRUE(contains("OK              : 2 (50.0%)"));
    EXPECT_TRUE(contains("Due soon        : 1 (25.0%)"));
    EXPECT_TRUE(contains("Overdue         : 1 (25.0%)"));
}

TEST_F(ReportTest, FleetSummaryRejectsEmptyTotal)
{
    FleetSummary_t summary = {0, 0, 0, 0, 0, 0.0};

    EXPECT_EQ(STATUS_ERR_EMPTY, generateFleetSummaryReport(output, &summary));
}

TEST_F(ReportTest, FleetSummaryRejectsNullArguments)
{
    FleetSummary_t summary = {1, 1, 0, 0, 0, 0.0};

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateFleetSummaryReport(nullptr, &summary));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateFleetSummaryReport(output, nullptr));
}

/* ---------- generateServiceHistoryReport ---------- */

TEST_F(ReportTest, HistoryListsEntriesTotalCostAndAverageInterval)
{
    ServiceRecord_t first = makeRecord("EQ-0001", makeDate(1, 1, 2025), 100.0, "First service");
    ServiceRecord_t second = makeRecord("EQ-0001", makeDate(31, 1, 2025), 250.5, "Second service");
    ServiceRecord_t third = makeRecord("EQ-0001", makeDate(2, 3, 2025), 49.5, "Third service");

    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &first));
    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &second));
    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &third));

    EXPECT_EQ(STATUS_SUCCESS, generateServiceHistoryReport(output, &log, "EQ-0001"));
    EXPECT_TRUE(contains("Second service"));
    EXPECT_TRUE(contains("Services recorded : 3"));
    EXPECT_TRUE(contains("Total cost        : 400.00"));
    EXPECT_TRUE(contains("Average interval  : 30.0 days"));
}

TEST_F(ReportTest, HistoryShowsOnlyTheRequestedMachine)
{
    ServiceRecord_t mine = makeRecord("EQ-0001", makeDate(1, 1, 2025), 10.0, "Mine");
    ServiceRecord_t other = makeRecord("EQ-0002", makeDate(1, 1, 2025), 10.0, "Other one");

    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &mine));
    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &other));

    EXPECT_EQ(STATUS_SUCCESS, generateServiceHistoryReport(output, &log, "EQ-0001"));
    EXPECT_TRUE(contains("Mine"));
    EXPECT_FALSE(contains("Other one"));
}

TEST_F(ReportTest, HistoryWithSingleEntryReportsAverageNotAvailable)
{
    ServiceRecord_t only = makeRecord("EQ-0001", makeDate(1, 1, 2025), 10.0, "Only");

    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &only));

    EXPECT_EQ(STATUS_SUCCESS, generateServiceHistoryReport(output, &log, "EQ-0001"));
    EXPECT_TRUE(contains("not available"));
}

TEST_F(ReportTest, HistoryForMachineWithoutEntriesIsNotFoundAndPrintsNothing)
{
    ServiceRecord_t other = makeRecord("EQ-0002", makeDate(1, 1, 2025), 10.0, "Other");

    ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &other));

    EXPECT_EQ(STATUS_ERR_NOT_FOUND, generateServiceHistoryReport(output, &log, "EQ-0001"));
    EXPECT_EQ("", written());
}

TEST_F(ReportTest, HistoryRejectsBadIdAndNullArguments)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ID, generateServiceHistoryReport(output, &log, "bad"));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateServiceHistoryReport(nullptr, &log, "EQ-0001"));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateServiceHistoryReport(output, nullptr, "EQ-0001"));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, generateServiceHistoryReport(output, &log, nullptr));
}

TEST_F(ReportTest, HistoryRejectsCorruptLogCount)
{
    log.count = -1;

    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, generateServiceHistoryReport(output, &log, "EQ-0001"));
}
