/**
 * @file test_file_handler.cpp
 * @brief Unit tests for the file_handler module.
 *
 * Temporary files are created in the working directory with names starting
 * "test_tmp_" and are removed after every test.
 */

#include <cstdio>
#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "common.h"
#include "date_utils.h"
#include "equipment.h"
#include "file_handler.h"

namespace
{

const char *const EQUIPMENT_TEST_FILE = "test_tmp_equipment.txt";
const char *const LOG_TEST_FILE = "test_tmp_service_log.txt";
const char *const TEMP_SUFFIX_TEXT = ".tmp";

/* Dates in these lines are all in the past relative to any real clock. */
const char *const LINE_ONE =
    "EQ-0001|CNC Lathe A|0|Shop Floor 1|15-03-2022|90|10-08-2024|4520.5|0";
const char *const LINE_TWO =
    "EQ-0002|Air Compressor|3|Utility Room|01-06-2021|30|05-09-2024|9800.0|1";
const char *const LINE_THREE =
    "EQ-0003|Main Conveyor|4|Packing Hall|20-01-2023|60|01-02-2024|120.25|0";
const char *const SERVICE_LINE_ONE =
    "EQ-0001|10-08-2024|R. Patil|2500.00|Oil change and belt check";
const char *const SERVICE_LINE_TWO =
    "EQ-0001|10-11-2024|S. Khan|1200.50|";

Date_t makeDate(int day, int month, int year)
{
    Date_t date = {day, month, year};
    return date;
}

void copyText(char *destination, size_t size, const char *source)
{
    std::snprintf(destination, size, "%s", source);
}

Equipment_t makeEquipment(const char *id, const char *name, int type,
                          double hours, int underRepair)
{
    Equipment_t item;

    std::memset(&item, 0, sizeof item);
    copyText(item.id, sizeof item.id, id);
    copyText(item.name, sizeof item.name, name);
    item.type = static_cast<EquipmentType_t>(type);
    copyText(item.location, sizeof item.location, "Shop Floor 1");
    item.installDate = makeDate(15, 3, 2022);
    item.serviceIntervalDays = 90;
    item.lastServiceDate = makeDate(10, 8, 2024);
    item.runningHours = hours;
    item.underRepair = underRepair;

    return item;
}

ServiceRecord_t makeRecord(const char *id, const Date_t &date,
                           const char *remarks)
{
    ServiceRecord_t record;

    std::memset(&record, 0, sizeof record);
    copyText(record.equipmentId, sizeof record.equipmentId, id);
    record.serviceDate = date;
    copyText(record.technician, sizeof record.technician, "R. Patil");
    record.cost = 2500.5;
    copyText(record.remarks, sizeof record.remarks, remarks);

    return record;
}

void writeFile(const char *path, const std::string &content)
{
    std::FILE *file = std::fopen(path, "wb");

    ASSERT_NE(nullptr, file);
    std::fwrite(content.data(), 1, content.size(), file);
    std::fclose(file);
}

std::string readFile(const char *path)
{
    std::string content;
    char buffer[256];
    size_t count;
    std::FILE *file = std::fopen(path, "rb");

    if (file == nullptr)
    {
        return content;
    }

    while ((count = std::fread(buffer, 1, sizeof buffer, file)) > 0)
    {
        content.append(buffer, count);
    }

    std::fclose(file);
    return content;
}

bool fileExists(const std::string &path)
{
    std::FILE *file = std::fopen(path.c_str(), "rb");

    if (file == nullptr)
    {
        return false;
    }

    std::fclose(file);
    return true;
}

void expectSameDate(const Date_t &expected, const Date_t &actual)
{
    EXPECT_EQ(expected.day, actual.day);
    EXPECT_EQ(expected.month, actual.month);
    EXPECT_EQ(expected.year, actual.year);
}

} // namespace

class FileHandlerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        today = makeDate(15, 6, 2025);
        removeTestFiles();
    }

    void TearDown() override
    {
        removeTestFiles();
    }

    static void removeTestFiles()
    {
        std::remove(EQUIPMENT_TEST_FILE);
        std::remove(LOG_TEST_FILE);
        std::remove((std::string(EQUIPMENT_TEST_FILE) + TEMP_SUFFIX_TEXT).c_str());
        std::remove((std::string(LOG_TEST_FILE) + TEMP_SUFFIX_TEXT).c_str());
    }

    Date_t today;
    EquipmentList_t list;
    ServiceLog_t log;
    int skipped = -1;
};

/* ---------- parseEquipmentLine ---------- */

TEST_F(FileHandlerTest, ParseEquipmentLineReadsAllFields)
{
    Equipment_t item;

    ASSERT_EQ(STATUS_SUCCESS, parseEquipmentLine(LINE_ONE, &item, &today));
    EXPECT_STREQ("EQ-0001", item.id);
    EXPECT_STREQ("CNC Lathe A", item.name);
    EXPECT_EQ(EQTYPE_LATHE, item.type);
    EXPECT_STREQ("Shop Floor 1", item.location);
    expectSameDate(makeDate(15, 3, 2022), item.installDate);
    EXPECT_EQ(90, item.serviceIntervalDays);
    expectSameDate(makeDate(10, 8, 2024), item.lastServiceDate);
    EXPECT_DOUBLE_EQ(4520.5, item.runningHours);
    EXPECT_EQ(UNDER_REPAIR_NO, item.underRepair);
}

TEST_F(FileHandlerTest, ParseEquipmentLineReadsRepairFlag)
{
    Equipment_t item;

    ASSERT_EQ(STATUS_SUCCESS, parseEquipmentLine(LINE_TWO, &item, &today));
    EXPECT_EQ(UNDER_REPAIR_YES, item.underRepair);
    EXPECT_EQ(EQTYPE_COMPRESSOR, item.type);
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsNullArguments)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseEquipmentLine(NULL, &item, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseEquipmentLine(LINE_ONE, NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseEquipmentLine(LINE_ONE, &item, NULL));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsWrongFieldCount)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|90|10-08-2024|5.0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|90|10-08-2024|5.0|0|x",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT, parseEquipmentLine("", &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsTextInNumericFields)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|abc|10-08-2024|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|90|10-08-2024|many|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|x|Hall|15-03-2022|90|10-08-2024|5.0|0",
                                 &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsPaddedNumbers)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022| 90|10-08-2024|5.0|0",
                                 &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsOutOfRangeType)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|7|Hall|15-03-2022|90|10-08-2024|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|-1|Hall|15-03-2022|90|10-08-2024|5.0|0",
                                 &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsInvalidFieldValues)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("eq-0001|Lathe|0|Hall|15-03-2022|90|10-08-2024|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001||0|Hall|15-03-2022|90|10-08-2024|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|31-02-2022|90|10-08-2024|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|0|10-08-2024|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|90|10-08-2024|-5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|90|10-08-2024|5.0|2",
                                 &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsNameOfThirtyOneCharacters)
{
    Equipment_t item;
    std::string tooLong = "EQ-0001|" + std::string(MAX_NAME_LEN + 1, 'a') +
                          "|0|Hall|15-03-2022|90|10-08-2024|5.0|0";
    std::string maxLength = "EQ-0001|" + std::string(MAX_NAME_LEN, 'a') +
                            "|0|Hall|15-03-2022|90|10-08-2024|5.0|0";

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT, parseEquipmentLine(tooLong.c_str(), &item, &today));
    EXPECT_EQ(STATUS_SUCCESS, parseEquipmentLine(maxLength.c_str(), &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsFutureAndMisorderedDates)
{
    Equipment_t item;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2100|90|15-03-2100|5.0|0",
                                 &item, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseEquipmentLine("EQ-0001|Lathe|0|Hall|15-03-2022|90|14-03-2022|5.0|0",
                                 &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineRejectsLineLongerThanBuffer)
{
    Equipment_t item;
    std::string tooLong(MAX_LINE_LEN, 'x');

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT, parseEquipmentLine(tooLong.c_str(), &item, &today));
}

TEST_F(FileHandlerTest, ParseEquipmentLineLeavesOutputUnchangedOnFailure)
{
    Equipment_t item = makeEquipment("EQ-0009", "Untouched", 1, 1.0, 0);

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT, parseEquipmentLine("garbage", &item, &today));
    EXPECT_STREQ("EQ-0009", item.id);
    EXPECT_STREQ("Untouched", item.name);
}

/* ---------- parseServiceRecordLine ---------- */

TEST_F(FileHandlerTest, ParseServiceLineReadsAllFields)
{
    ServiceRecord_t record;

    ASSERT_EQ(STATUS_SUCCESS, parseServiceRecordLine(SERVICE_LINE_ONE, &record, &today));
    EXPECT_STREQ("EQ-0001", record.equipmentId);
    expectSameDate(makeDate(10, 8, 2024), record.serviceDate);
    EXPECT_STREQ("R. Patil", record.technician);
    EXPECT_DOUBLE_EQ(2500.0, record.cost);
    EXPECT_STREQ("Oil change and belt check", record.remarks);
}

TEST_F(FileHandlerTest, ParseServiceLineAcceptsEmptyRemarks)
{
    ServiceRecord_t record;

    ASSERT_EQ(STATUS_SUCCESS, parseServiceRecordLine(SERVICE_LINE_TWO, &record, &today));
    EXPECT_STREQ("", record.remarks);
}

TEST_F(FileHandlerTest, ParseServiceLineRejectsWrongFieldCount)
{
    ServiceRecord_t record;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|10-08-2024|R. Patil|2500.00", &record, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|10-08-2024|R. Patil|2500.00|a|b", &record, &today));
}

TEST_F(FileHandlerTest, ParseServiceLineRejectsInvalidFields)
{
    ServiceRecord_t record;

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("bad|10-08-2024|R. Patil|2500.00|x", &record, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|31-02-2024|R. Patil|2500.00|x", &record, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|10-08-2024||2500.00|x", &record, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|10-08-2024|R. Patil|cheap|x", &record, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|10-08-2024|R. Patil|-5.00|x", &record, &today));
    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              parseServiceRecordLine("EQ-0001|15-03-2100|R. Patil|5.00|x", &record, &today));
}

TEST_F(FileHandlerTest, ParseServiceLineRejectsNullArguments)
{
    ServiceRecord_t record;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseServiceRecordLine(NULL, &record, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseServiceRecordLine(SERVICE_LINE_ONE, NULL, &today));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseServiceRecordLine(SERVICE_LINE_ONE, &record, NULL));
}

TEST_F(FileHandlerTest, ParseServiceLineLeavesOutputUnchangedOnFailure)
{
    ServiceRecord_t record = makeRecord("EQ-0009", makeDate(1, 1, 2024), "Untouched");

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT, parseServiceRecordLine("garbage", &record, &today));
    EXPECT_STREQ("EQ-0009", record.equipmentId);
    EXPECT_STREQ("Untouched", record.remarks);
}

/* ---------- saveEquipmentToFile / loadEquipmentFromFile ---------- */

TEST_F(FileHandlerTest, EquipmentSurvivesSaveAndLoad)
{
    EquipmentList_t saved;
    saved.count = 3;
    saved.items[0] = makeEquipment("EQ-0001", "CNC Lathe A", 0, 4520.5, 0);
    saved.items[1] = makeEquipment("EQ-0002", "Air Compressor", 3, 9800.0, 1);
    saved.items[2] = makeEquipment("EQ-0003", "Main Conveyor", 4, 0.0, 0);

    ASSERT_EQ(STATUS_SUCCESS, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));
    ASSERT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));

    EXPECT_EQ(0, skipped);
    ASSERT_EQ(3, list.count);
    for (int index = 0; index < 3; ++index)
    {
        EXPECT_STREQ(saved.items[index].id, list.items[index].id);
        EXPECT_STREQ(saved.items[index].name, list.items[index].name);
        EXPECT_EQ(saved.items[index].type, list.items[index].type);
        EXPECT_STREQ(saved.items[index].location, list.items[index].location);
        expectSameDate(saved.items[index].installDate, list.items[index].installDate);
        EXPECT_EQ(saved.items[index].serviceIntervalDays,
                  list.items[index].serviceIntervalDays);
        expectSameDate(saved.items[index].lastServiceDate,
                       list.items[index].lastServiceDate);
        EXPECT_DOUBLE_EQ(saved.items[index].runningHours, list.items[index].runningHours);
        EXPECT_EQ(saved.items[index].underRepair, list.items[index].underRepair);
    }
}

TEST_F(FileHandlerTest, SavedFileStartsWithHeaderComment)
{
    EquipmentList_t saved;
    saved.count = 0;

    ASSERT_EQ(STATUS_SUCCESS, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));
    EXPECT_EQ('#', readFile(EQUIPMENT_TEST_FILE)[0]);
}

TEST_F(FileHandlerTest, EmptyListSavesAndLoadsAsEmpty)
{
    EquipmentList_t saved;
    saved.count = 0;
    list.count = 5;

    ASSERT_EQ(STATUS_SUCCESS, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));
    EXPECT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(0, list.count);
    EXPECT_EQ(0, skipped);
}

TEST_F(FileHandlerTest, SaveReplacesExistingFileAndRemovesTemporaryFile)
{
    EquipmentList_t saved;
    saved.count = 3;
    saved.items[0] = makeEquipment("EQ-0001", "A", 0, 1.0, 0);
    saved.items[1] = makeEquipment("EQ-0002", "B", 1, 2.0, 0);
    saved.items[2] = makeEquipment("EQ-0003", "C", 2, 3.0, 0);
    ASSERT_EQ(STATUS_SUCCESS, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));

    saved.count = 1;
    ASSERT_EQ(STATUS_SUCCESS, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));

    ASSERT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(1, list.count);
    EXPECT_FALSE(fileExists(std::string(EQUIPMENT_TEST_FILE) + TEMP_SUFFIX_TEXT));
}

TEST_F(FileHandlerTest, SaveWithInvalidRecordKeepsExistingFile)
{
    EquipmentList_t saved;
    saved.count = 1;
    saved.items[0] = makeEquipment("EQ-0001", "A", 0, 1.0, 0);
    saved.items[0].lastServiceDate = makeDate(31, 2, 2024);
    writeFile(EQUIPMENT_TEST_FILE, "KEEP");

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));
    EXPECT_EQ("KEEP", readFile(EQUIPMENT_TEST_FILE));
    EXPECT_FALSE(fileExists(std::string(EQUIPMENT_TEST_FILE) + TEMP_SUFFIX_TEXT));
}

TEST_F(FileHandlerTest, SaveToMissingFolderReportsFileOpenError)
{
    EquipmentList_t saved;
    saved.count = 0;

    EXPECT_EQ(STATUS_ERR_FILE_OPEN,
              saveEquipmentToFile("test_tmp_no_such_folder/equipment.txt", &saved));
}

TEST_F(FileHandlerTest, SaveRejectsNullArgumentsCorruptCountAndLongPath)
{
    EquipmentList_t saved;
    saved.count = 0;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, saveEquipmentToFile(NULL, &saved));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, saveEquipmentToFile(EQUIPMENT_TEST_FILE, NULL));

    saved.count = MAX_EQUIPMENT + 1;
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, saveEquipmentToFile(EQUIPMENT_TEST_FILE, &saved));

    saved.count = 0;
    std::string longPath(400, 'p');
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, saveEquipmentToFile(longPath.c_str(), &saved));
}

TEST_F(FileHandlerTest, LoadOfMissingFileReportsFileOpenError)
{
    list.count = 5;

    EXPECT_EQ(STATUS_ERR_FILE_OPEN,
              loadEquipmentFromFile("test_tmp_does_not_exist.txt", &list, &skipped));
    EXPECT_EQ(0, list.count);
    EXPECT_EQ(0, skipped);
}

TEST_F(FileHandlerTest, LoadOfEmptyFileGivesNoRecords)
{
    writeFile(EQUIPMENT_TEST_FILE, "");

    EXPECT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(0, list.count);
    EXPECT_EQ(0, skipped);
}

TEST_F(FileHandlerTest, LoadIgnoresCommentsAndBlankLines)
{
    writeFile(EQUIPMENT_TEST_FILE,
              std::string("# comment\n\n   \n") + LINE_ONE + "\n# another\n" + LINE_TWO + "\n");

    EXPECT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(2, list.count);
    EXPECT_EQ(0, skipped);
}

TEST_F(FileHandlerTest, LoadSkipsMalformedLineButKeepsValidOnes)
{
    writeFile(EQUIPMENT_TEST_FILE,
              std::string(LINE_ONE) + "\nthis is not a record\n" + LINE_THREE + "\n");

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(1, skipped);
    ASSERT_EQ(2, list.count);
    EXPECT_STREQ("EQ-0001", list.items[0].id);
    EXPECT_STREQ("EQ-0003", list.items[1].id);
}

TEST_F(FileHandlerTest, LoadSkipsDuplicateIds)
{
    writeFile(EQUIPMENT_TEST_FILE,
              std::string(LINE_ONE) + "\n" + LINE_ONE + "\n" + LINE_TWO + "\n");

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(1, skipped);
    EXPECT_EQ(2, list.count);
}

TEST_F(FileHandlerTest, LoadSkipsFutureDatedRecords)
{
    writeFile(EQUIPMENT_TEST_FILE,
              std::string("EQ-0001|Lathe|0|Hall|15-03-2100|90|15-03-2100|5.0|0\n") +
                  LINE_TWO + "\n");

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(1, skipped);
    EXPECT_EQ(1, list.count);
}

TEST_F(FileHandlerTest, LoadAcceptsWindowsLineEndingsAndMissingFinalNewline)
{
    writeFile(EQUIPMENT_TEST_FILE,
              std::string(LINE_ONE) + "\r\n" + LINE_TWO + "\r\n" + LINE_THREE);

    EXPECT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(3, list.count);
    EXPECT_STREQ("Main Conveyor", list.items[2].name);
}

TEST_F(FileHandlerTest, LoadSkipsOverlongLineAndContinues)
{
    writeFile(EQUIPMENT_TEST_FILE,
              std::string(LINE_ONE) + "\n" + std::string(600, 'x') + "\n" + LINE_TWO + "\n");

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT,
              loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(1, skipped);
    ASSERT_EQ(2, list.count);
    EXPECT_STREQ("EQ-0002", list.items[1].id);
}

TEST_F(FileHandlerTest, LoadStopsWithFullErrorAtCapacity)
{
    std::string content;
    char id[ID_LENGTH + 1];

    for (int index = 0; index <= MAX_EQUIPMENT; ++index)
    {
        std::snprintf(id, sizeof id, "EQ-%04d", index);
        content += std::string(id) + "|Machine|0|Hall|15-03-2022|90|10-08-2024|5.0|0\n";
    }
    writeFile(EQUIPMENT_TEST_FILE, content);

    EXPECT_EQ(STATUS_ERR_FULL, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(MAX_EQUIPMENT, list.count);
}

TEST_F(FileHandlerTest, LoadEmptiesTheListBeforeReading)
{
    list.count = 50;
    writeFile(EQUIPMENT_TEST_FILE, std::string(LINE_ONE) + "\n");

    ASSERT_EQ(STATUS_SUCCESS, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, &skipped));
    EXPECT_EQ(1, list.count);
}

TEST_F(FileHandlerTest, LoadRejectsNullArguments)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, loadEquipmentFromFile(NULL, &list, &skipped));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, NULL, &skipped));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, loadEquipmentFromFile(EQUIPMENT_TEST_FILE, &list, NULL));
}

/* ---------- service log ---------- */

TEST_F(FileHandlerTest, ServiceLogSurvivesSaveAndLoad)
{
    ServiceLog_t saved;
    saved.count = 2;
    saved.records[0] = makeRecord("EQ-0001", makeDate(10, 8, 2024), "Oil change");
    saved.records[1] = makeRecord("EQ-0002", makeDate(1, 2, 2024), "");

    ASSERT_EQ(STATUS_SUCCESS, saveServiceLogToFile(LOG_TEST_FILE, &saved));
    ASSERT_EQ(STATUS_SUCCESS, loadServiceLogFromFile(LOG_TEST_FILE, &log, &skipped));

    EXPECT_EQ(0, skipped);
    ASSERT_EQ(2, log.count);
    for (int index = 0; index < 2; ++index)
    {
        EXPECT_STREQ(saved.records[index].equipmentId, log.records[index].equipmentId);
        expectSameDate(saved.records[index].serviceDate, log.records[index].serviceDate);
        EXPECT_STREQ(saved.records[index].technician, log.records[index].technician);
        EXPECT_DOUBLE_EQ(saved.records[index].cost, log.records[index].cost);
        EXPECT_STREQ(saved.records[index].remarks, log.records[index].remarks);
    }
}

TEST_F(FileHandlerTest, ServiceLogLoadSkipsMalformedLines)
{
    writeFile(LOG_TEST_FILE,
              std::string(SERVICE_LINE_ONE) + "\nEQ-0001|bad-date|X|1.0|y\n" +
                  SERVICE_LINE_TWO + "\n");

    EXPECT_EQ(STATUS_ERR_FILE_FORMAT, loadServiceLogFromFile(LOG_TEST_FILE, &log, &skipped));
    EXPECT_EQ(1, skipped);
    EXPECT_EQ(2, log.count);
}

TEST_F(FileHandlerTest, ServiceLogLoadOfMissingFileReportsFileOpenError)
{
    log.count = 3;

    EXPECT_EQ(STATUS_ERR_FILE_OPEN,
              loadServiceLogFromFile("test_tmp_does_not_exist.txt", &log, &skipped));
    EXPECT_EQ(0, log.count);
}

TEST_F(FileHandlerTest, ServiceLogLoadStopsWithFullErrorAtCapacity)
{
    std::string content;

    for (int index = 0; index <= MAX_SERVICE_RECORDS; ++index)
    {
        content += std::string(SERVICE_LINE_ONE) + "\n";
    }
    writeFile(LOG_TEST_FILE, content);

    EXPECT_EQ(STATUS_ERR_FULL, loadServiceLogFromFile(LOG_TEST_FILE, &log, &skipped));
    EXPECT_EQ(MAX_SERVICE_RECORDS, log.count);
}

TEST_F(FileHandlerTest, ServiceLogSaveRejectsNullArgumentsAndInvalidDate)
{
    ServiceLog_t saved;
    saved.count = 1;
    saved.records[0] = makeRecord("EQ-0001", makeDate(31, 2, 2024), "x");
    writeFile(LOG_TEST_FILE, "KEEP");

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, saveServiceLogToFile(NULL, &saved));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, saveServiceLogToFile(LOG_TEST_FILE, NULL));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, saveServiceLogToFile(LOG_TEST_FILE, &saved));
    EXPECT_EQ("KEEP", readFile(LOG_TEST_FILE));
}

TEST_F(FileHandlerTest, ServiceLogLoadRejectsNullArguments)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, loadServiceLogFromFile(NULL, &log, &skipped));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, loadServiceLogFromFile(LOG_TEST_FILE, NULL, &skipped));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, loadServiceLogFromFile(LOG_TEST_FILE, &log, NULL));
}
