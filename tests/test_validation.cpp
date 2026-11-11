/**
 * @file test_validation.cpp
 * @brief Unit tests for the validation module.
 */

#include <climits>
#include <cmath>
#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include "common.h"
#include "date_utils.h"
#include "validation.h"

namespace
{

Date_t makeDate(int day, int month, int year)
{
    Date_t date = {day, month, year};
    return date;
}

} // namespace

/* ---------- validateEquipmentId ---------- */

TEST(ValidateEquipmentIdTest, AcceptsCorrectFormat)
{
    EXPECT_EQ(STATUS_SUCCESS, validateEquipmentId("EQ-0001"));
    EXPECT_EQ(STATUS_SUCCESS, validateEquipmentId("EQ-9999"));
}

TEST(ValidateEquipmentIdTest, RejectsNullPointer)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, validateEquipmentId(NULL));
}

TEST(ValidateEquipmentIdTest, RejectsEmptyText)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId(""));
}

TEST(ValidateEquipmentIdTest, RejectsTooShortAndTooLong)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("EQ-001"));
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("EQ-00012"));
}

TEST(ValidateEquipmentIdTest, RejectsWrongPrefix)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("eq-0001"));
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("XX-0001"));
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("EQ_0001"));
}

TEST(ValidateEquipmentIdTest, RejectsNonDigitSuffix)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("EQ-00A1"));
    EXPECT_EQ(STATUS_ERR_INVALID_ID, validateEquipmentId("EQ-12 4"));
}

/* ---------- validateDate ---------- */

TEST(ValidateDateTest, AcceptsOrdinaryDate)
{
    Date_t date = makeDate(15, 3, 2024);
    EXPECT_EQ(STATUS_SUCCESS, validateDate(&date));
}

TEST(ValidateDateTest, RejectsNullPointer)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, validateDate(NULL));
}

TEST(ValidateDateTest, AcceptsFirstAndLastBusinessDay)
{
    Date_t first = makeDate(1, 1, MIN_YEAR);
    Date_t last = makeDate(31, 12, MAX_YEAR);

    EXPECT_EQ(STATUS_SUCCESS, validateDate(&first));
    EXPECT_EQ(STATUS_SUCCESS, validateDate(&last));
}

TEST(ValidateDateTest, RejectsYearOutsideBusinessRange)
{
    Date_t early = makeDate(31, 12, MIN_YEAR - 1);
    Date_t late = makeDate(1, 1, MAX_YEAR + 1);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&early));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&late));
}

TEST(ValidateDateTest, RejectsInvalidMonth)
{
    Date_t zeroMonth = makeDate(10, 0, 2024);
    Date_t bigMonth = makeDate(10, 13, 2024);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&zeroMonth));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&bigMonth));
}

TEST(ValidateDateTest, RejectsInvalidDay)
{
    Date_t zeroDay = makeDate(0, 5, 2024);
    Date_t april31 = makeDate(31, 4, 2024);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&zeroDay));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&april31));
}

TEST(ValidateDateTest, HandlesLeapYearFebruary)
{
    Date_t leapDay = makeDate(29, 2, 2024);
    Date_t notLeap = makeDate(29, 2, 2023);
    Date_t century = makeDate(29, 2, 2100);

    EXPECT_EQ(STATUS_SUCCESS, validateDate(&leapDay));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&notLeap));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, validateDate(&century));
}

/* ---------- validateServiceInterval ---------- */

TEST(ValidateServiceIntervalTest, AcceptsValueInsideRange)
{
    EXPECT_EQ(STATUS_SUCCESS, validateServiceInterval(30));
}

TEST(ValidateServiceIntervalTest, AcceptsBoundaryValues)
{
    EXPECT_EQ(STATUS_SUCCESS, validateServiceInterval(MIN_INTERVAL_DAYS));
    EXPECT_EQ(STATUS_SUCCESS, validateServiceInterval(MAX_INTERVAL_DAYS));
}

TEST(ValidateServiceIntervalTest, RejectsValuesJustOutsideRange)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateServiceInterval(MIN_INTERVAL_DAYS - 1));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateServiceInterval(MAX_INTERVAL_DAYS + 1));
}

TEST(ValidateServiceIntervalTest, RejectsNegativeValue)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateServiceInterval(-10));
}

/* ---------- validateRunningHours ---------- */

TEST(ValidateRunningHoursTest, AcceptsValueInsideRange)
{
    EXPECT_EQ(STATUS_SUCCESS, validateRunningHours(1234.5));
}

TEST(ValidateRunningHoursTest, AcceptsBoundaryValues)
{
    EXPECT_EQ(STATUS_SUCCESS, validateRunningHours(0.0));
    EXPECT_EQ(STATUS_SUCCESS, validateRunningHours(MAX_RUNNING_HOURS));
}

TEST(ValidateRunningHoursTest, RejectsValuesJustOutsideRange)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateRunningHours(-0.01));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateRunningHours(MAX_RUNNING_HOURS + 0.01));
}

TEST(ValidateRunningHoursTest, RejectsNanAndInfinity)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateRunningHours(std::nan("")));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateRunningHours(INFINITY));
}

/* ---------- validateCost ---------- */

TEST(ValidateCostTest, AcceptsValueInsideRange)
{
    EXPECT_EQ(STATUS_SUCCESS, validateCost(2500.75));
}

TEST(ValidateCostTest, AcceptsBoundaryValues)
{
    EXPECT_EQ(STATUS_SUCCESS, validateCost(0.0));
    EXPECT_EQ(STATUS_SUCCESS, validateCost(MAX_COST));
}

TEST(ValidateCostTest, RejectsNegativeAndTooLarge)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateCost(-1.0));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateCost(MAX_COST + 1.0));
}

TEST(ValidateCostTest, RejectsNan)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateCost(std::nan("")));
}

/* ---------- validateTextField ---------- */

TEST(ValidateTextFieldTest, AcceptsOrdinaryText)
{
    EXPECT_EQ(STATUS_SUCCESS,
              validateTextField("Hydraulic Press", MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, RejectsNullPointer)
{
    EXPECT_EQ(STATUS_ERR_NULL_POINTER,
              validateTextField(NULL, MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, AcceptsTextOfExactlyMaxLength)
{
    std::string text(MAX_NAME_LEN, 'a');
    EXPECT_EQ(STATUS_SUCCESS,
              validateTextField(text.c_str(), MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, RejectsTextOneCharacterTooLong)
{
    std::string text(MAX_NAME_LEN + 1, 'a');
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateTextField(text.c_str(), MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, RejectsFieldDelimiter)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateTextField("Press|Line", MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, RejectsControlCharacters)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateTextField("Press\tLine", MAX_NAME_LEN, 0));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateTextField("Press\nLine", MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, RejectsEmptyAndBlankWhenNotAllowed)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateTextField("", MAX_NAME_LEN, 0));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              validateTextField("   ", MAX_NAME_LEN, 0));
}

TEST(ValidateTextFieldTest, AcceptsEmptyAndBlankWhenAllowed)
{
    EXPECT_EQ(STATUS_SUCCESS, validateTextField("", MAX_REMARKS_LEN, 1));
    EXPECT_EQ(STATUS_SUCCESS, validateTextField("   ", MAX_REMARKS_LEN, 1));
}

/* ---------- validateMenuChoice ---------- */

TEST(ValidateMenuChoiceTest, AcceptsChoiceInsideRange)
{
    EXPECT_EQ(STATUS_SUCCESS, validateMenuChoice(3, 1, 8));
}

TEST(ValidateMenuChoiceTest, AcceptsBothBoundaries)
{
    EXPECT_EQ(STATUS_SUCCESS, validateMenuChoice(1, 1, 8));
    EXPECT_EQ(STATUS_SUCCESS, validateMenuChoice(8, 1, 8));
}

TEST(ValidateMenuChoiceTest, RejectsChoiceOutsideRange)
{
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateMenuChoice(0, 1, 8));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, validateMenuChoice(9, 1, 8));
}

TEST(ValidateMenuChoiceTest, RejectsReversedRange)
{
    EXPECT_EQ(STATUS_ERR_INVALID_ARGUMENT, validateMenuChoice(5, 8, 1));
}

TEST(ValidateMenuChoiceTest, AcceptsSingleValueRange)
{
    EXPECT_EQ(STATUS_SUCCESS, validateMenuChoice(4, 4, 4));
}

/* ---------- parseInteger ---------- */

TEST(ParseIntegerTest, ParsesPositiveAndNegativeNumbers)
{
    int value = 0;

    EXPECT_EQ(STATUS_SUCCESS, parseInteger("42", &value));
    EXPECT_EQ(42, value);

    EXPECT_EQ(STATUS_SUCCESS, parseInteger("-7", &value));
    EXPECT_EQ(-7, value);
}

TEST(ParseIntegerTest, RejectsNullArguments)
{
    int value = 0;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseInteger(NULL, &value));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseInteger("5", NULL));
}

TEST(ParseIntegerTest, RejectsEmptyTextAndSpaces)
{
    int value = 99;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger(" 5", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("5 ", &value));
}

TEST(ParseIntegerTest, RejectsNonNumericAndTrailingCharacters)
{
    int value = 99;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("abc", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("12abc", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("4.5", &value));
}

TEST(ParseIntegerTest, AcceptsIntLimits)
{
    int value = 0;

    EXPECT_EQ(STATUS_SUCCESS, parseInteger("2147483647", &value));
    EXPECT_EQ(INT_MAX, value);

    EXPECT_EQ(STATUS_SUCCESS, parseInteger("-2147483648", &value));
    EXPECT_EQ(INT_MIN, value);
}

TEST(ParseIntegerTest, RejectsValuesBeyondIntRange)
{
    int value = 99;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("2147483648", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("-2147483649", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE,
              parseInteger("99999999999999999999", &value));
}

TEST(ParseIntegerTest, LeavesOutputUnchangedOnFailure)
{
    int value = 99;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseInteger("bad", &value));
    EXPECT_EQ(99, value);
}

/* ---------- parseDouble ---------- */

TEST(ParseDoubleTest, ParsesDecimalAndExponentForms)
{
    double value = 0.0;

    EXPECT_EQ(STATUS_SUCCESS, parseDouble("12.5", &value));
    EXPECT_DOUBLE_EQ(12.5, value);

    EXPECT_EQ(STATUS_SUCCESS, parseDouble("-3.25", &value));
    EXPECT_DOUBLE_EQ(-3.25, value);

    EXPECT_EQ(STATUS_SUCCESS, parseDouble("1e3", &value));
    EXPECT_DOUBLE_EQ(1000.0, value);
}

TEST(ParseDoubleTest, RejectsNullArguments)
{
    double value = 0.0;

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseDouble(NULL, &value));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseDouble("1.0", NULL));
}

TEST(ParseDoubleTest, RejectsEmptyTextSpacesAndTrailingCharacters)
{
    double value = 7.0;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble(" 1.5", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("1.5 ", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("1.5abc", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("abc", &value));
}

TEST(ParseDoubleTest, RejectsHexadecimalNanAndInfinity)
{
    double value = 7.0;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("0x10", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("nan", &value));
    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("inf", &value));
}

TEST(ParseDoubleTest, RejectsOverflow)
{
    double value = 7.0;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("1e999", &value));
}

TEST(ParseDoubleTest, LeavesOutputUnchangedOnFailure)
{
    double value = 7.0;

    EXPECT_EQ(STATUS_ERR_INVALID_VALUE, parseDouble("bad", &value));
    EXPECT_DOUBLE_EQ(7.0, value);
}

/* ---------- parseDate ---------- */

TEST(ParseDateTest, ParsesValidDate)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_SUCCESS, parseDate("15-03-2024", &date));
    EXPECT_EQ(15, date.day);
    EXPECT_EQ(3, date.month);
    EXPECT_EQ(2024, date.year);
}

TEST(ParseDateTest, RejectsNullArguments)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseDate(NULL, &date));
    EXPECT_EQ(STATUS_ERR_NULL_POINTER, parseDate("15-03-2024", NULL));
}

TEST(ParseDateTest, RejectsWrongLength)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("5-3-2024", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("15-03-24", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("15-03-20244", &date));
}

TEST(ParseDateTest, RejectsWrongSeparatorsAndNonDigits)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("15/03/2024", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("aa-03-2024", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("15-0x-2024", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("15-03-20a4", &date));
}

TEST(ParseDateTest, RejectsImpossibleCalendarDates)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("31-04-2024", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("29-02-2023", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("00-01-2024", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("10-13-2024", &date));
}

TEST(ParseDateTest, AcceptsLeapDayAndYearBoundaries)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_SUCCESS, parseDate("29-02-2024", &date));
    EXPECT_EQ(STATUS_SUCCESS, parseDate("01-01-2000", &date));
    EXPECT_EQ(STATUS_SUCCESS, parseDate("31-12-2100", &date));
    EXPECT_EQ(31, date.day);
    EXPECT_EQ(12, date.month);
    EXPECT_EQ(2100, date.year);
}

TEST(ParseDateTest, RejectsYearOutsideBusinessRange)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("31-12-1999", &date));
    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("01-01-2101", &date));
}

TEST(ParseDateTest, LeavesOutputUnchangedOnFailure)
{
    Date_t date = makeDate(1, 2, 2003);

    EXPECT_EQ(STATUS_ERR_INVALID_DATE, parseDate("31-04-2024", &date));
    EXPECT_EQ(1, date.day);
    EXPECT_EQ(2, date.month);
    EXPECT_EQ(2003, date.year);
}
