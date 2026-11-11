/**
 * @file test_equipment.cpp
 * @brief Unit tests for the equipment module.
 */

#include <cmath>
#include <cstdio>
#include <cstring>

#include <gtest/gtest.h>

#include "common.h"
#include "date_utils.h"
#include "equipment.h"

namespace
{

Date_t makeDate(int day, int month, int year)
{
    Date_t date = {day, month, year};
    return date;
}

void copyText(char *destination, size_t size, const char *source)
{
    std::snprintf(destination, size, "%s", source);
}

Equipment_t makeEquipment(const char *id)
{
    Equipment_t item;

    std::memset(&item, 0, sizeof item);
    copyText(item.id, sizeof item.id, id);
    copyText(item.name, sizeof item.name, "Test Machine");
    item.type = EQTYPE_LATHE;
    copyText(item.location, sizeof item.location, "Shop Floor 1");
    item.installDate = makeDate(15, 3, 2022);
    item.serviceIntervalDays = 90;
    item.lastServiceDate = makeDate(10, 1, 2025);
    item.runningHours = 1000.0;
    item.underRepair = UNDER_REPAIR_NO;

    return item;
}

ServiceRecord_t makeRecord(const char *id, const Date_t &date)
{
    ServiceRecord_t record;

    std::memset(&record, 0, sizeof record);
    copyText(record.equipmentId, sizeof record.equipmentId, id);
    record.serviceDate = date;
    copyText(record.technician, sizeof record.technician, "R. Patil");
    record.cost = 2500.0;
    copyText(record.remarks, sizeof record.remarks, "Oil change");

    return record;
}

void expectSameDate(const Date_t &expected, const Date_t &actual)
{
    EXPECT_EQ(expected.day, actual.day);
    EXPECT_EQ(expected.month, actual.month);
    EXPECT_EQ(expected.year, actual.year);
}

} // namespace

class EquipmentTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        today = makeDate(15, 6, 2025);
        ASSERT_EQ(STATUS_SUCCESS, initEquipmentList(&list));
        ASSERT_EQ(STATUS_SUCCESS, initServiceLog(&log));
    }

    Status_t addUnit(const char *id)
    {
        Equipment_t item = makeEquipment(id);
        return addEquipment(&list, &item, &today);
    }

    EquipmentList_t list;
    ServiceLog_t log;
    Date_t today;
};

/* ---------- init ---------- */

TEST_F(EquipmentTest, InitEquipmentListResetsCount)
{
    list.count = 42;
    EXPECT_EQ(STATUS_SUCCESS, initEquipmentList(&list));
    EXPECT_EQ(0, list.count);
}

TEST_F(EquipmentTest, InitFunctionsRejectNull)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, initEquipmentList(NULL));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, initServiceLog(NULL));
}

TEST_F(EquipmentTest, InitServiceLogResetsCount)
{
    log.count = 7;
    EXPECT_EQ(STATUS_SUCCESS, initServiceLog(&log));
    EXPECT_EQ(0, log.count);
}

/* ---------- validateEquipmentRecord ---------- */

TEST_F(EquipmentTest, ValidRecordIsAccepted)
{
    Equipment_t item = makeEquipment("EQ-0001");
    EXPECT_EQ(STATUS_SUCCESS, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsNullArguments)
{
    Equipment_t item = makeEquipment("EQ-0001");

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, validateEquipmentRecord(NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, validateEquipmentRecord(&item, NULL));
}

TEST_F(EquipmentTest, ValidateRecordRejectsInvalidToday)
{
    Equipment_t item = makeEquipment("EQ-0001");
    Date_t badToday = makeDate(31, 2, 2025);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateEquipmentRecord(&item, &badToday));
}

TEST_F(EquipmentTest, ValidateRecordRejectsBadId)
{
    Equipment_t item = makeEquipment("EQ-0001");
    copyText(item.id, sizeof item.id, "eq-1");

    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsBlankNameAndLocation)
{
    Equipment_t item = makeEquipment("EQ-0001");
    copyText(item.name, sizeof item.name, "   ");
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));

    item = makeEquipment("EQ-0001");
    copyText(item.location, sizeof item.location, "");
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsDelimiterInName)
{
    Equipment_t item = makeEquipment("EQ-0001");
    copyText(item.name, sizeof item.name, "Press|Line");

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsUnterminatedName)
{
    Equipment_t item = makeEquipment("EQ-0001");
    std::memset(item.name, 'a', sizeof item.name);

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsInvalidType)
{
    Equipment_t item = makeEquipment("EQ-0001");
    item.type = static_cast<EquipmentType_t>(EQTYPE_COUNT);

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsFutureInstallDate)
{
    Equipment_t item = makeEquipment("EQ-0001");
    item.installDate = makeDate(16, 6, 2025);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsFutureLastServiceDate)
{
    Equipment_t item = makeEquipment("EQ-0001");
    item.lastServiceDate = makeDate(16, 6, 2025);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsLastServiceBeforeInstall)
{
    Equipment_t item = makeEquipment("EQ-0001");
    item.lastServiceDate = makeDate(14, 3, 2022);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateEquipmentRecord(&item, &today));
}

TEST_F(EquipmentTest, ValidateRecordAcceptsDatesOnBoundaries)
{
    Equipment_t sameDay = makeEquipment("EQ-0001");
    sameDay.lastServiceDate = sameDay.installDate;
    EXPECT_EQ(STATUS_SUCCESS, validateEquipmentRecord(&sameDay, &today));

    Equipment_t allToday = makeEquipment("EQ-0002");
    allToday.installDate = today;
    allToday.lastServiceDate = today;
    EXPECT_EQ(STATUS_SUCCESS, validateEquipmentRecord(&allToday, &today));
}

TEST_F(EquipmentTest, ValidateRecordRejectsBadNumbers)
{
    Equipment_t item = makeEquipment("EQ-0001");
    item.serviceIntervalDays = 0;
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));

    item = makeEquipment("EQ-0001");
    item.runningHours = -1.0;
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));

    item = makeEquipment("EQ-0001");
    item.runningHours = std::nan("");
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));

    item = makeEquipment("EQ-0001");
    item.underRepair = 2;
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateEquipmentRecord(&item, &today));
}

/* ---------- addEquipment ---------- */

TEST_F(EquipmentTest, AddEquipmentStoresRecordInEmptyList)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(1, list.count);
    EXPECT_STREQ("EQ-0001", list.items[0].id);
    EXPECT_STREQ("Test Machine", list.items[0].name);
    EXPECT_EQ(90, list.items[0].serviceIntervalDays);
}

TEST_F(EquipmentTest, AddEquipmentKeepsInsertionOrder)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0003"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0002"));

    EXPECT_STREQ("EQ-0003", list.items[0].id);
    EXPECT_STREQ("EQ-0001", list.items[1].id);
    EXPECT_STREQ("EQ-0002", list.items[2].id);
}

TEST_F(EquipmentTest, AddEquipmentRejectsDuplicateId)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_ERR_DUPLICATE, addUnit("EQ-0001"));
    EXPECT_EQ(1, list.count);
}

TEST_F(EquipmentTest, AddEquipmentAcceptsHundredthAndRejectsHundredFirst)
{
    char id[ID_LENGTH + 1];
    int index;

    for (index = 0; index < MAX_EQUIPMENT; ++index)
    {
        std::snprintf(id, sizeof id, "EQ-%04d", index);
        ASSERT_EQ(STATUS_SUCCESS, addUnit(id));
    }
    EXPECT_EQ(MAX_EQUIPMENT, list.count);

    std::snprintf(id, sizeof id, "EQ-%04d", MAX_EQUIPMENT);
    EXPECT_EQ(STATUS_ERR_FULL, addUnit(id));
    EXPECT_EQ(MAX_EQUIPMENT, list.count);
}

TEST_F(EquipmentTest, AddEquipmentRejectsInvalidRecordAndKeepsListUnchanged)
{
    Equipment_t item = makeEquipment("EQ-0001");
    item.serviceIntervalDays = 366;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, addEquipment(&list, &item, &today));
    EXPECT_EQ(0, list.count);
}

TEST_F(EquipmentTest, AddEquipmentRejectsNullArguments)
{
    Equipment_t item = makeEquipment("EQ-0001");

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, addEquipment(NULL, &item, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, addEquipment(&list, NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, addEquipment(&list, &item, NULL));
}

TEST_F(EquipmentTest, AddEquipmentRejectsCorruptCount)
{
    Equipment_t item = makeEquipment("EQ-0001");
    list.count = -1;

    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, addEquipment(&list, &item, &today));
}

/* ---------- findEquipmentById ---------- */

TEST_F(EquipmentTest, FindReturnsIndexOfExistingId)
{
    int index = -1;

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0002"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0003"));

    EXPECT_EQ(STATUS_SUCCESS, findEquipmentById(&list, "EQ-0002", &index));
    EXPECT_EQ(1, index);
}

TEST_F(EquipmentTest, FindReportsNotFoundAndLeavesIndexUnchanged)
{
    int index = 99;

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_ERR_NOT_FOUND, findEquipmentById(&list, "EQ-9999", &index));
    EXPECT_EQ(99, index);
}

TEST_F(EquipmentTest, FindInEmptyListReportsNotFound)
{
    int index = 0;

    EXPECT_EQ(STATUS_ERR_NOT_FOUND, findEquipmentById(&list, "EQ-0001", &index));
}

TEST_F(EquipmentTest, FindRejectsBadIdFormat)
{
    int index = 0;

    EXPECT_EQ(STATUS_ERR_INVALID_ID, findEquipmentById(&list, "bad", &index));
}

TEST_F(EquipmentTest, FindRejectsNullArguments)
{
    int index = 0;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, findEquipmentById(NULL, "EQ-0001", &index));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, findEquipmentById(&list, NULL, &index));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, findEquipmentById(&list, "EQ-0001", NULL));
}

/* ---------- updateEquipment ---------- */

TEST_F(EquipmentTest, UpdateReplacesFieldsOfExistingRecord)
{
    Equipment_t newData = makeEquipment("EQ-0001");

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    copyText(newData.name, sizeof newData.name, "Renamed Lathe");
    newData.serviceIntervalDays = 30;
    newData.underRepair = UNDER_REPAIR_YES;

    EXPECT_EQ(STATUS_SUCCESS, updateEquipment(&list, "EQ-0001", &newData, &today));
    EXPECT_STREQ("Renamed Lathe", list.items[0].name);
    EXPECT_EQ(30, list.items[0].serviceIntervalDays);
    EXPECT_EQ(UNDER_REPAIR_YES, list.items[0].underRepair);
}

TEST_F(EquipmentTest, UpdateNeverChangesTheId)
{
    Equipment_t newData = makeEquipment("EQ-0999");
    int index = -1;

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_SUCCESS, updateEquipment(&list, "EQ-0001", &newData, &today));
    EXPECT_STREQ("EQ-0001", list.items[0].id);
    EXPECT_EQ(STATUS_ERR_NOT_FOUND, findEquipmentById(&list, "EQ-0999", &index));
}

TEST_F(EquipmentTest, UpdateReportsNotFoundForMissingId)
{
    Equipment_t newData = makeEquipment("EQ-0001");

    EXPECT_EQ(STATUS_ERR_NOT_FOUND,
              updateEquipment(&list, "EQ-0001", &newData, &today));
}

TEST_F(EquipmentTest, UpdateWithInvalidDataLeavesRecordUnchanged)
{
    Equipment_t newData = makeEquipment("EQ-0001");

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    copyText(newData.name, sizeof newData.name, "Should Not Be Saved");
    newData.runningHours = -5.0;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              updateEquipment(&list, "EQ-0001", &newData, &today));
    EXPECT_STREQ("Test Machine", list.items[0].name);
    EXPECT_DOUBLE_EQ(1000.0, list.items[0].runningHours);
}

TEST_F(EquipmentTest, UpdateRejectsNullArguments)
{
    Equipment_t newData = makeEquipment("EQ-0001");

    EXPECT_EQ(STATUS_ERR_NULL_POINTER,
              updateEquipment(NULL, "EQ-0001", &newData, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER,
              updateEquipment(&list, NULL, &newData, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER,
              updateEquipment(&list, "EQ-0001", NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER,
              updateEquipment(&list, "EQ-0001", &newData, NULL));
}

/* ---------- deleteEquipment ---------- */

TEST_F(EquipmentTest, DeleteMiddleRecordKeepsOrderOfTheRest)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0002"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0003"));

    EXPECT_EQ(STATUS_SUCCESS, deleteEquipment(&list, "EQ-0002"));
    EXPECT_EQ(2, list.count);
    EXPECT_STREQ("EQ-0001", list.items[0].id);
    EXPECT_STREQ("EQ-0003", list.items[1].id);
}

TEST_F(EquipmentTest, DeleteFirstAndLastRecords)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0002"));
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0003"));

    EXPECT_EQ(STATUS_SUCCESS, deleteEquipment(&list, "EQ-0001"));
    EXPECT_EQ(STATUS_SUCCESS, deleteEquipment(&list, "EQ-0003"));
    EXPECT_EQ(1, list.count);
    EXPECT_STREQ("EQ-0002", list.items[0].id);
}

TEST_F(EquipmentTest, DeleteOnlyRecordEmptiesList)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_SUCCESS, deleteEquipment(&list, "EQ-0001"));
    EXPECT_EQ(0, list.count);
}

TEST_F(EquipmentTest, DeleteMissingIdLeavesListUnchanged)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_ERR_NOT_FOUND, deleteEquipment(&list, "EQ-0002"));
    EXPECT_EQ(1, list.count);
}

TEST_F(EquipmentTest, DeleteFromEmptyListReportsNotFound)
{
    EXPECT_EQ(STATUS_ERR_NOT_FOUND, deleteEquipment(&list, "EQ-0001"));
}

TEST_F(EquipmentTest, DeleteRejectsNullAndBadId)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, deleteEquipment(NULL, "EQ-0001"));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, deleteEquipment(&list, NULL));
    EXPECT_EQ(STATUS_ERR_INVALID_ID, deleteEquipment(&list, "bad"));
}

TEST_F(EquipmentTest, DeletedIdCanBeAddedAgain)
{
    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));
    ASSERT_EQ(STATUS_SUCCESS, deleteEquipment(&list, "EQ-0001"));

    EXPECT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));
    EXPECT_EQ(1, list.count);
}

/* ---------- addServiceRecord ---------- */

TEST_F(EquipmentTest, AddServiceRecordAppendsEntry)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));

    EXPECT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &record));
    EXPECT_EQ(1, log.count);
    EXPECT_STREQ("EQ-0001", log.records[0].equipmentId);
}

TEST_F(EquipmentTest, AddServiceRecordRejectsWhenLogIsFull)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    int index;

    for (index = 0; index < MAX_SERVICE_RECORDS; ++index)
    {
        ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &record));
    }

    EXPECT_EQ(STATUS_ERR_FULL, addServiceRecord(&log, &record));
    EXPECT_EQ(MAX_SERVICE_RECORDS, log.count);
}

TEST_F(EquipmentTest, AddServiceRecordRejectsNullArguments)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, addServiceRecord(NULL, &record));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, addServiceRecord(&log, NULL));
}

/* ---------- validateServiceRecord ---------- */

TEST_F(EquipmentTest, ValidateServiceRecordAcceptsValidAndEmptyRemarks)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    EXPECT_EQ(STATUS_SUCCESS, validateServiceRecord(&record, &today));

    copyText(record.remarks, sizeof record.remarks, "");
    EXPECT_EQ(STATUS_SUCCESS, validateServiceRecord(&record, &today));
}

TEST_F(EquipmentTest, ValidateServiceRecordRejectsBadFields)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    copyText(record.equipmentId, sizeof record.equipmentId, "bad");
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateServiceRecord(&record, &today));

    record = makeRecord("EQ-0001", makeDate(16, 6, 2025));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateServiceRecord(&record, &today));

    record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    copyText(record.technician, sizeof record.technician, "");
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateServiceRecord(&record, &today));

    record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    record.cost = -1.0;
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateServiceRecord(&record, &today));
}

TEST_F(EquipmentTest, ValidateServiceRecordRejectsNullArguments)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, validateServiceRecord(NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, validateServiceRecord(&record, NULL));
}

/* ---------- logServiceEvent ---------- */

TEST_F(EquipmentTest, LogServiceEventUpdatesLastServiceAndClearsRepairFlag)
{
    Equipment_t item = makeEquipment("EQ-0001");
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));

    item.underRepair = UNDER_REPAIR_YES;
    ASSERT_EQ(STATUS_SUCCESS, addEquipment(&list, &item, &today));

    EXPECT_EQ(STATUS_SUCCESS, logServiceEvent(&list, &log, &record, &today));
    EXPECT_EQ(1, log.count);
    expectSameDate(makeDate(1, 6, 2025), list.items[0].lastServiceDate);
    EXPECT_EQ(UNDER_REPAIR_NO, list.items[0].underRepair);
}

TEST_F(EquipmentTest, LogServiceEventKeepsLaterLastServiceDate)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 1, 2024));

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_SUCCESS, logServiceEvent(&list, &log, &record, &today));
    EXPECT_EQ(1, log.count);
    expectSameDate(makeDate(10, 1, 2025), list.items[0].lastServiceDate);
}

TEST_F(EquipmentTest, LogServiceEventAcceptsServiceDateOfToday)
{
    ServiceRecord_t record = makeRecord("EQ-0001", today);

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_SUCCESS, logServiceEvent(&list, &log, &record, &today));
    expectSameDate(today, list.items[0].lastServiceDate);
}

TEST_F(EquipmentTest, LogServiceEventRejectsUnknownEquipment)
{
    ServiceRecord_t record = makeRecord("EQ-0042", makeDate(1, 6, 2025));

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_ERR_NOT_FOUND, logServiceEvent(&list, &log, &record, &today));
    EXPECT_EQ(0, log.count);
}

TEST_F(EquipmentTest, LogServiceEventRejectsInvalidRecordAndChangesNothing)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    record.cost = -50.0;

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              logServiceEvent(&list, &log, &record, &today));
    EXPECT_EQ(0, log.count);
    expectSameDate(makeDate(10, 1, 2025), list.items[0].lastServiceDate);
}

TEST_F(EquipmentTest, LogServiceEventRejectsFutureServiceDate)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(16, 6, 2025));

    ASSERT_EQ(STATUS_SUCCESS, addUnit("EQ-0001"));

    EXPECT_EQ(STATUS_ERR_INVALID_DATE,
              logServiceEvent(&list, &log, &record, &today));
    EXPECT_EQ(0, log.count);
}

TEST_F(EquipmentTest, LogServiceEventWithFullLogLeavesMachineUnchanged)
{
    Equipment_t item = makeEquipment("EQ-0001");
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));
    int index;

    item.underRepair = UNDER_REPAIR_YES;
    ASSERT_EQ(STATUS_SUCCESS, addEquipment(&list, &item, &today));

    for (index = 0; index < MAX_SERVICE_RECORDS; ++index)
    {
        ASSERT_EQ(STATUS_SUCCESS, addServiceRecord(&log, &record));
    }

    EXPECT_EQ(STATUS_ERR_FULL, logServiceEvent(&list, &log, &record, &today));
    EXPECT_EQ(MAX_SERVICE_RECORDS, log.count);
    expectSameDate(makeDate(10, 1, 2025), list.items[0].lastServiceDate);
    EXPECT_EQ(UNDER_REPAIR_YES, list.items[0].underRepair);
}

TEST_F(EquipmentTest, LogServiceEventRejectsNullArguments)
{
    ServiceRecord_t record = makeRecord("EQ-0001", makeDate(1, 6, 2025));

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, logServiceEvent(NULL, &log, &record, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, logServiceEvent(&list, NULL, &record, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, logServiceEvent(&list, &log, NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, logServiceEvent(&list, &log, &record, NULL));
}
