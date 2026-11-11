/**
 * @file test_maintenance.cpp
 * @brief Unit tests for the maintenance module.
 */

#include <cstdio>
#include <cstring>

#include <gtest/gtest.h>

#include "common.h"
#include "date_utils.h"
#include "equipment.h"
#include "maintenance.h"

namespace
{

const int TEST_INTERVAL_DAYS = 30;

Date_t makeDate(int day, int month, int year)
{
    Date_t date = {day, month, year};
    return date;
}

void copyText(char *destination, size_t size, const char *source)
{
    std::snprintf(destination, size, "%s", source);
}

/* Date that is offset days after (or before, if negative) the given date. */
Date_t shiftDate(const Date_t &base, long offset)
{
    long number = 0;
    Date_t result = {0, 0, 0};

    dateToDayNumber(&base, &number);
    dayNumberToDate(number + offset, &result);

    return result;
}

Equipment_t makeEquipment(const char *id, int interval, const Date_t &last)
{
    Equipment_t item;

    std::memset(&item, 0, sizeof item);
    copyText(item.id, sizeof item.id, id);
    copyText(item.name, sizeof item.name, "Test Machine");
    item.type = EQTYPE_LATHE;
    copyText(item.location, sizeof item.location, "Shop Floor 1");
    item.installDate = makeDate(15, 3, 2022);
    item.serviceIntervalDays = interval;
    item.lastServiceDate = last;
    item.runningHours = 1000.0;
    item.underRepair = UNDER_REPAIR_NO;

    return item;
}

/* Machine whose next service falls daysUntilDue days after today. */
Equipment_t makeDueIn(const char *id, const Date_t &today, long daysUntilDue)
{
    Date_t last = shiftDate(today, daysUntilDue - TEST_INTERVAL_DAYS);
    return makeEquipment(id, TEST_INTERVAL_DAYS, last);
}

ServiceRecord_t makeRecord(const char *id, const Date_t &date)
{
    ServiceRecord_t record;

    std::memset(&record, 0, sizeof record);
    copyText(record.equipmentId, sizeof record.equipmentId, id);
    record.serviceDate = date;
    copyText(record.technician, sizeof record.technician, "R. Patil");
    record.cost = 100.0;
    copyText(record.remarks, sizeof record.remarks, "Routine");

    return record;
}

void expectSameDate(const Date_t &expected, const Date_t &actual)
{
    EXPECT_EQ(expected.day, actual.day);
    EXPECT_EQ(expected.month, actual.month);
    EXPECT_EQ(expected.year, actual.year);
}

} // namespace

class MaintenanceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        today = makeDate(15, 6, 2025);
        list.count = 0;
        log.count = 0;
    }

    void append(const Equipment_t &item)
    {
        list.items[list.count] = item;
        list.count += 1;
    }

    void appendLog(const ServiceRecord_t &record)
    {
        log.records[log.count] = record;
        log.count += 1;
    }

    Date_t today;
    EquipmentList_t list;
    ServiceLog_t log;
};

/* ---------- calculateNextServiceDate ---------- */

TEST_F(MaintenanceTest, NextServiceDateAddsInterval)
{
    Equipment_t item = makeEquipment("EQ-0001", 90, makeDate(1, 1, 2025));
    Date_t next = {0, 0, 0};

    EXPECT_EQ(STATUS_SUCCESS, calculateNextServiceDate(&item, &next));
    expectSameDate(makeDate(1, 4, 2025), next);
}

TEST_F(MaintenanceTest, NextServiceDateCrossesYearEnd)
{
    Equipment_t item = makeEquipment("EQ-0001", 30, makeDate(20, 12, 2025));
    Date_t next = {0, 0, 0};

    EXPECT_EQ(STATUS_SUCCESS, calculateNextServiceDate(&item, &next));
    expectSameDate(makeDate(19, 1, 2026), next);
}

TEST_F(MaintenanceTest, NextServiceDateHandlesLeapDay)
{
    Equipment_t item = makeEquipment("EQ-0001", 1, makeDate(28, 2, 2024));
    Date_t next = {0, 0, 0};

    EXPECT_EQ(STATUS_SUCCESS, calculateNextServiceDate(&item, &next));
    expectSameDate(makeDate(29, 2, 2024), next);
}

TEST_F(MaintenanceTest, NextServiceDateRejectsNullArguments)
{
    Equipment_t item = makeEquipment("EQ-0001", 90, makeDate(1, 1, 2025));
    Date_t next = {0, 0, 0};

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateNextServiceDate(NULL, &next));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateNextServiceDate(&item, NULL));
}

TEST_F(MaintenanceTest, NextServiceDateRejectsInvalidLastServiceDate)
{
    Equipment_t item = makeEquipment("EQ-0001", 90, makeDate(31, 2, 2025));
    Date_t next = {7, 7, 2007};

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, calculateNextServiceDate(&item, &next));
    expectSameDate(makeDate(7, 7, 2007), next);
}

TEST_F(MaintenanceTest, NextServiceDateRejectsIntervalBeyondLimit)
{
    Equipment_t item = makeEquipment("EQ-0001", MAX_INTERVAL_DAYS + 1,
                                     makeDate(1, 1, 2025));
    Date_t next = {0, 0, 0};

    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, calculateNextServiceDate(&item, &next));
}

/* ---------- calculateDaysRemaining ---------- */

TEST_F(MaintenanceTest, DaysRemainingIsPositiveBeforeDueDate)
{
    Equipment_t item = makeDueIn("EQ-0001", today, 10);
    long days = 0;

    EXPECT_EQ(STATUS_SUCCESS, calculateDaysRemaining(&item, &today, &days));
    EXPECT_EQ(10, days);
}

TEST_F(MaintenanceTest, DaysRemainingIsNegativeWhenOverdue)
{
    Equipment_t item = makeDueIn("EQ-0001", today, -5);
    long days = 0;

    EXPECT_EQ(STATUS_SUCCESS, calculateDaysRemaining(&item, &today, &days));
    EXPECT_EQ(-5, days);
}

TEST_F(MaintenanceTest, DaysRemainingIsZeroOnDueDate)
{
    Equipment_t item = makeDueIn("EQ-0001", today, 0);
    long days = 99;

    EXPECT_EQ(STATUS_SUCCESS, calculateDaysRemaining(&item, &today, &days));
    EXPECT_EQ(0, days);
}

TEST_F(MaintenanceTest, DaysRemainingRejectsNullArguments)
{
    Equipment_t item = makeDueIn("EQ-0001", today, 10);
    long days = 0;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateDaysRemaining(NULL, &today, &days));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateDaysRemaining(&item, NULL, &days));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateDaysRemaining(&item, &today, NULL));
}

TEST_F(MaintenanceTest, DaysRemainingRejectsInvalidTodayAndKeepsOutput)
{
    Equipment_t item = makeDueIn("EQ-0001", today, 10);
    Date_t badToday = makeDate(31, 4, 2025);
    long days = 1234;

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, calculateDaysRemaining(&item, &badToday, &days));
    EXPECT_EQ(1234, days);
}

/* ---------- determineStatus ---------- */

TEST_F(MaintenanceTest, StatusIsOkWhenEightDaysRemain)
{
    Equipment_t item = makeDueIn("EQ-0001", today, DUE_SOON_DAYS + 1);
    MaintenanceStatus_t status = MAINT_OVERDUE;

    EXPECT_EQ(STATUS_SUCCESS, determineStatus(&item, &today, &status));
    EXPECT_EQ(MAINT_OK, status);
}

TEST_F(MaintenanceTest, StatusIsDueSoonWhenSevenDaysRemain)
{
    Equipment_t item = makeDueIn("EQ-0001", today, DUE_SOON_DAYS);
    MaintenanceStatus_t status = MAINT_OK;

    EXPECT_EQ(STATUS_SUCCESS, determineStatus(&item, &today, &status));
    EXPECT_EQ(MAINT_DUE_SOON, status);
}

TEST_F(MaintenanceTest, StatusIsDueSoonOnTheDueDate)
{
    Equipment_t item = makeDueIn("EQ-0001", today, 0);
    MaintenanceStatus_t status = MAINT_OK;

    EXPECT_EQ(STATUS_SUCCESS, determineStatus(&item, &today, &status));
    EXPECT_EQ(MAINT_DUE_SOON, status);
}

TEST_F(MaintenanceTest, StatusIsOverdueOneDayAfterDueDate)
{
    Equipment_t item = makeDueIn("EQ-0001", today, -1);
    MaintenanceStatus_t status = MAINT_OK;

    EXPECT_EQ(STATUS_SUCCESS, determineStatus(&item, &today, &status));
    EXPECT_EQ(MAINT_OVERDUE, status);
}

TEST_F(MaintenanceTest, UnderRepairOverridesOverdue)
{
    Equipment_t item = makeDueIn("EQ-0001", today, -40);
    MaintenanceStatus_t status = MAINT_OK;

    item.underRepair = UNDER_REPAIR_YES;

    EXPECT_EQ(STATUS_SUCCESS, determineStatus(&item, &today, &status));
    EXPECT_EQ(MAINT_UNDER_REPAIR, status);
}

TEST_F(MaintenanceTest, StatusRejectsNullArguments)
{
    Equipment_t item = makeDueIn("EQ-0001", today, 10);
    MaintenanceStatus_t status = MAINT_OK;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, determineStatus(NULL, &today, &status));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, determineStatus(&item, NULL, &status));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, determineStatus(&item, &today, NULL));
}

TEST_F(MaintenanceTest, StatusRejectsInvalidDateAndKeepsOutput)
{
    Equipment_t item = makeEquipment("EQ-0001", 30, makeDate(0, 1, 2025));
    MaintenanceStatus_t status = MAINT_DUE_SOON;

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, determineStatus(&item, &today, &status));
    EXPECT_EQ(MAINT_DUE_SOON, status);
}

/* ---------- getStatusName ---------- */

TEST_F(MaintenanceTest, StatusNamesAreReadable)
{
    EXPECT_STREQ("OK", getStatusName(MAINT_OK));
    EXPECT_STREQ("DUE SOON", getStatusName(MAINT_DUE_SOON));
    EXPECT_STREQ("OVERDUE", getStatusName(MAINT_OVERDUE));
    EXPECT_STREQ("UNDER REPAIR", getStatusName(MAINT_UNDER_REPAIR));
}

TEST_F(MaintenanceTest, OutOfRangeStatusGivesUnknown)
{
    EXPECT_STREQ("UNKNOWN", getStatusName(static_cast<MaintenanceStatus_t>(MAINT_COUNT)));
}

/* ---------- getSortedByNextDue ---------- */

TEST_F(MaintenanceTest, SortOrdersByNextDueDateEarliestFirst)
{
    EquipmentList_t sorted;

    append(makeDueIn("EQ-0001", today, 20));
    append(makeDueIn("EQ-0002", today, 5));
    append(makeDueIn("EQ-0003", today, 50));
    append(makeDueIn("EQ-0004", today, -4));

    ASSERT_EQ(STATUS_SUCCESS, getSortedByNextDue(&list, &sorted));
    ASSERT_EQ(4, sorted.count);
    EXPECT_STREQ("EQ-0004", sorted.items[0].id);
    EXPECT_STREQ("EQ-0002", sorted.items[1].id);
    EXPECT_STREQ("EQ-0001", sorted.items[2].id);
    EXPECT_STREQ("EQ-0003", sorted.items[3].id);
}

TEST_F(MaintenanceTest, SortDoesNotModifyTheSource)
{
    EquipmentList_t sorted;

    append(makeDueIn("EQ-0001", today, 20));
    append(makeDueIn("EQ-0002", today, 5));

    ASSERT_EQ(STATUS_SUCCESS, getSortedByNextDue(&list, &sorted));
    EXPECT_EQ(2, list.count);
    EXPECT_STREQ("EQ-0001", list.items[0].id);
    EXPECT_STREQ("EQ-0002", list.items[1].id);
}

TEST_F(MaintenanceTest, SortIsStableForEqualDueDates)
{
    EquipmentList_t sorted;

    append(makeDueIn("EQ-0001", today, 10));
    append(makeDueIn("EQ-0002", today, 3));
    append(makeDueIn("EQ-0003", today, 10));

    ASSERT_EQ(STATUS_SUCCESS, getSortedByNextDue(&list, &sorted));
    EXPECT_STREQ("EQ-0002", sorted.items[0].id);
    EXPECT_STREQ("EQ-0001", sorted.items[1].id);
    EXPECT_STREQ("EQ-0003", sorted.items[2].id);
}

TEST_F(MaintenanceTest, SortOfEmptyListGivesEmptyResult)
{
    EquipmentList_t sorted;
    sorted.count = 9;

    EXPECT_EQ(STATUS_SUCCESS, getSortedByNextDue(&list, &sorted));
    EXPECT_EQ(0, sorted.count);
}

TEST_F(MaintenanceTest, SortOfSingleRecordKeepsIt)
{
    EquipmentList_t sorted;

    append(makeDueIn("EQ-0001", today, 10));

    ASSERT_EQ(STATUS_SUCCESS, getSortedByNextDue(&list, &sorted));
    ASSERT_EQ(1, sorted.count);
    EXPECT_STREQ("EQ-0001", sorted.items[0].id);
}

TEST_F(MaintenanceTest, SortHandlesFullListInReverseOrder)
{
    EquipmentList_t sorted;
    char id[ID_LENGTH + 1];
    int index;

    for (index = 0; index < MAX_EQUIPMENT; ++index)
    {
        std::snprintf(id, sizeof id, "EQ-%04d", index);
        append(makeDueIn(id, today, MAX_EQUIPMENT - index));
    }

    ASSERT_EQ(STATUS_SUCCESS, getSortedByNextDue(&list, &sorted));
    ASSERT_EQ(MAX_EQUIPMENT, sorted.count);
    EXPECT_STREQ("EQ-0099", sorted.items[0].id);
    EXPECT_STREQ("EQ-0000", sorted.items[MAX_EQUIPMENT - 1].id);
}

TEST_F(MaintenanceTest, SortRejectsNullAndSameObject)
{
    EquipmentList_t sorted;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, getSortedByNextDue(NULL, &sorted));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, getSortedByNextDue(&list, NULL));
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, getSortedByNextDue(&list, &list));
}

TEST_F(MaintenanceTest, SortRejectsCorruptCount)
{
    EquipmentList_t sorted;
    list.count = MAX_EQUIPMENT + 1;

    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, getSortedByNextDue(&list, &sorted));
}

TEST_F(MaintenanceTest, SortWithBadRecordLeavesOutputUnchanged)
{
    EquipmentList_t sorted;
    sorted.count = 7;

    append(makeDueIn("EQ-0001", today, 10));
    append(makeEquipment("EQ-0002", 30, makeDate(31, 2, 2025)));

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, getSortedByNextDue(&list, &sorted));
    EXPECT_EQ(7, sorted.count);
}

/* ---------- filterByStatus ---------- */

TEST_F(MaintenanceTest, FilterReturnsOnlyMatchingMachinesInOrder)
{
    EquipmentList_t result;

    append(makeDueIn("EQ-0001", today, -3));
    append(makeDueIn("EQ-0002", today, 30));
    append(makeDueIn("EQ-0003", today, -10));
    append(makeDueIn("EQ-0004", today, 5));

    ASSERT_EQ(STATUS_SUCCESS, filterByStatus(&list, MAINT_OVERDUE, &today, &result));
    ASSERT_EQ(2, result.count);
    EXPECT_STREQ("EQ-0001", result.items[0].id);
    EXPECT_STREQ("EQ-0003", result.items[1].id);
}

TEST_F(MaintenanceTest, FilterWithNoMatchGivesEmptyResult)
{
    EquipmentList_t result;
    result.count = 5;

    append(makeDueIn("EQ-0001", today, 30));

    EXPECT_EQ(STATUS_SUCCESS, filterByStatus(&list, MAINT_OVERDUE, &today, &result));
    EXPECT_EQ(0, result.count);
}

TEST_F(MaintenanceTest, FilterFindsUnderRepairMachines)
{
    EquipmentList_t result;
    Equipment_t broken = makeDueIn("EQ-0002", today, -20);
    broken.underRepair = UNDER_REPAIR_YES;

    append(makeDueIn("EQ-0001", today, -20));
    append(broken);

    ASSERT_EQ(STATUS_SUCCESS,
              filterByStatus(&list, MAINT_UNDER_REPAIR, &today, &result));
    ASSERT_EQ(1, result.count);
    EXPECT_STREQ("EQ-0002", result.items[0].id);
}

TEST_F(MaintenanceTest, FilterOfEmptyListGivesEmptyResult)
{
    EquipmentList_t result;

    EXPECT_EQ(STATUS_SUCCESS, filterByStatus(&list, MAINT_OK, &today, &result));
    EXPECT_EQ(0, result.count);
}

TEST_F(MaintenanceTest, FilterRejectsInvalidStatusValue)
{
    EquipmentList_t result;

    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT,
              filterByStatus(&list, static_cast<MaintenanceStatus_t>(MAINT_COUNT),
                             &today, &result));
}

TEST_F(MaintenanceTest, FilterRejectsNullAndSameObject)
{
    EquipmentList_t result;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, filterByStatus(NULL, MAINT_OK, &today, &result));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, filterByStatus(&list, MAINT_OK, NULL, &result));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, filterByStatus(&list, MAINT_OK, &today, NULL));
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, filterByStatus(&list, MAINT_OK, &today, &list));
}

TEST_F(MaintenanceTest, FilterWithBadRecordLeavesResultUnchanged)
{
    EquipmentList_t result;
    result.count = 4;

    append(makeDueIn("EQ-0001", today, -5));
    append(makeEquipment("EQ-0002", 30, makeDate(31, 2, 2025)));

    EXPECT_EQ(STATUS_ERR_INVALID_DATE,
              filterByStatus(&list, MAINT_OVERDUE, &today, &result));
    EXPECT_EQ(4, result.count);
}

/* ---------- calculateFleetSummary ---------- */

TEST_F(MaintenanceTest, FleetSummaryCountsEachStatus)
{
    FleetSummary_t summary;
    Equipment_t broken = makeDueIn("EQ-0004", today, -50);
    broken.underRepair = UNDER_REPAIR_YES;

    append(makeDueIn("EQ-0001", today, 30));
    append(makeDueIn("EQ-0002", today, 5));
    append(makeDueIn("EQ-0003", today, -3));
    append(broken);

    ASSERT_EQ(STATUS_SUCCESS, calculateFleetSummary(&list, &today, &summary));
    EXPECT_EQ(4, summary.total);
    EXPECT_EQ(1, summary.okCount);
    EXPECT_EQ(1, summary.dueSoonCount);
    EXPECT_EQ(1, summary.overdueCount);
    EXPECT_EQ(1, summary.underRepairCount);
    EXPECT_DOUBLE_EQ(25.0, summary.overduePercent);
}

TEST_F(MaintenanceTest, FleetSummaryOfAllOverdueIsHundredPercent)
{
    FleetSummary_t summary;

    append(makeDueIn("EQ-0001", today, -1));
    append(makeDueIn("EQ-0002", today, -9));

    ASSERT_EQ(STATUS_SUCCESS, calculateFleetSummary(&list, &today, &summary));
    EXPECT_DOUBLE_EQ(100.0, summary.overduePercent);
}

TEST_F(MaintenanceTest, FleetSummaryOfNoneOverdueIsZeroPercent)
{
    FleetSummary_t summary;

    append(makeDueIn("EQ-0001", today, 40));

    ASSERT_EQ(STATUS_SUCCESS, calculateFleetSummary(&list, &today, &summary));
    EXPECT_DOUBLE_EQ(0.0, summary.overduePercent);
}

TEST_F(MaintenanceTest, FleetSummaryOfEmptyListReportsEmpty)
{
    FleetSummary_t summary;

    EXPECT_EQ(STATUS_ERR_EMPTY, calculateFleetSummary(&list, &today, &summary));
}

TEST_F(MaintenanceTest, FleetSummaryRejectsNullArguments)
{
    FleetSummary_t summary;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateFleetSummary(NULL, &today, &summary));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateFleetSummary(&list, NULL, &summary));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateFleetSummary(&list, &today, NULL));
}

TEST_F(MaintenanceTest, FleetSummaryWithBadRecordLeavesOutputUnchanged)
{
    FleetSummary_t summary;
    summary.total = 77;

    append(makeDueIn("EQ-0001", today, 10));
    append(makeEquipment("EQ-0002", 30, makeDate(31, 2, 2025)));

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, calculateFleetSummary(&list, &today, &summary));
    EXPECT_EQ(77, summary.total);
}

/* ---------- calculateAverageServiceInterval ---------- */

TEST_F(MaintenanceTest, AverageIntervalIsSpanDividedByGaps)
{
    double average = 0.0;

    appendLog(makeRecord("EQ-0001", makeDate(1, 1, 2025)));
    appendLog(makeRecord("EQ-0001", makeDate(31, 1, 2025)));
    appendLog(makeRecord("EQ-0001", makeDate(2, 3, 2025)));

    ASSERT_EQ(STATUS_SUCCESS, calculateAverageServiceInterval(&log, "EQ-0001", &average));
    EXPECT_DOUBLE_EQ(30.0, average);
}

TEST_F(MaintenanceTest, AverageIntervalIgnoresOtherMachinesAndOrder)
{
    double average = 0.0;

    appendLog(makeRecord("EQ-0001", makeDate(11, 1, 2025)));
    appendLog(makeRecord("EQ-0002", makeDate(1, 6, 2025)));
    appendLog(makeRecord("EQ-0001", makeDate(1, 1, 2025)));

    ASSERT_EQ(STATUS_SUCCESS, calculateAverageServiceInterval(&log, "EQ-0001", &average));
    EXPECT_DOUBLE_EQ(10.0, average);
}

TEST_F(MaintenanceTest, AverageIntervalOfTwoServicesOnSameDayIsZero)
{
    double average = 5.0;

    appendLog(makeRecord("EQ-0001", makeDate(1, 1, 2025)));
    appendLog(makeRecord("EQ-0001", makeDate(1, 1, 2025)));

    ASSERT_EQ(STATUS_SUCCESS, calculateAverageServiceInterval(&log, "EQ-0001", &average));
    EXPECT_DOUBLE_EQ(0.0, average);
}

TEST_F(MaintenanceTest, AverageIntervalNeedsAtLeastTwoRecords)
{
    double average = 5.0;

    appendLog(makeRecord("EQ-0001", makeDate(1, 1, 2025)));
    appendLog(makeRecord("EQ-0002", makeDate(1, 2, 2025)));

    EXPECT_EQ(STATUS_ERR_INSUFFICIENT_DATA,
              calculateAverageServiceInterval(&log, "EQ-0001", &average));
    EXPECT_DOUBLE_EQ(5.0, average);
}

TEST_F(MaintenanceTest, AverageIntervalWithNoRecordsIsInsufficientData)
{
    double average = 0.0;

    EXPECT_EQ(STATUS_ERR_INSUFFICIENT_DATA,
              calculateAverageServiceInterval(&log, "EQ-0001", &average));
}

TEST_F(MaintenanceTest, AverageIntervalRejectsBadIdAndNullArguments)
{
    double average = 0.0;

    EXPECT_EQ(STATUS_ERR_INVALID_ID, calculateAverageServiceInterval(&log, "bad", &average));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateAverageServiceInterval(NULL, "EQ-0001", &average));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateAverageServiceInterval(&log, NULL, &average));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, calculateAverageServiceInterval(&log, "EQ-0001", NULL));
}

TEST_F(MaintenanceTest, AverageIntervalRejectsInvalidRecordDate)
{
    double average = 5.0;

    appendLog(makeRecord("EQ-0001", makeDate(1, 1, 2025)));
    appendLog(makeRecord("EQ-0001", makeDate(31, 2, 2025)));

    EXPECT_EQ(STATUS_ERR_INVALID_DATE,
              calculateAverageServiceInterval(&log, "EQ-0001", &average));
    EXPECT_DOUBLE_EQ(5.0, average);
}

TEST_F(MaintenanceTest, AverageIntervalRejectsCorruptCount)
{
    double average = 0.0;
    log.count = MAX_SERVICE_RECORDS + 1;

    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT,
              calculateAverageServiceInterval(&log, "EQ-0001", &average));
}
