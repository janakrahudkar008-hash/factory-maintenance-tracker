/**
 * @file test_date_utils.cpp
 * @brief Unit tests for the date_utils module (design tests UT-18 to UT-23).
 */

#include <gtest/gtest.h>

#include "date_utils.h"

namespace
{

Date_t makeDate(int day, int month, int year)
{
    Date_t date = {day, month, year};
    return date;
}

void expectDate(const Date_t &actual, int day, int month, int year)
{
    EXPECT_EQ(actual.day, day);
    EXPECT_EQ(actual.month, month);
    EXPECT_EQ(actual.year, year);
}

}  // namespace

/* ---------- isLeapYear ---------- */

TEST(IsLeapYearTest, YearDivisibleByFourIsLeap)
{
    EXPECT_EQ(isLeapYear(2024), 1);
}

TEST(IsLeapYearTest, CenturyNotDivisibleByFourHundredIsNotLeap)
{
    EXPECT_EQ(isLeapYear(1900), 0);
}

TEST(IsLeapYearTest, YearDivisibleByFourHundredIsLeap)
{
    EXPECT_EQ(isLeapYear(2000), 1);
}

TEST(IsLeapYearTest, OrdinaryYearIsNotLeap)
{
    EXPECT_EQ(isLeapYear(2023), 0);
}

/* ---------- getDaysInMonth ---------- */

TEST(GetDaysInMonthTest, FebruaryInLeapYearHas29Days)
{
    EXPECT_EQ(getDaysInMonth(2, 2024), 29);
}

TEST(GetDaysInMonthTest, FebruaryInCommonYearHas28Days)
{
    EXPECT_EQ(getDaysInMonth(2, 2023), 28);
}

TEST(GetDaysInMonthTest, JanuaryHas31Days)
{
    EXPECT_EQ(getDaysInMonth(1, 2025), 31);
}

TEST(GetDaysInMonthTest, AprilHas30Days)
{
    EXPECT_EQ(getDaysInMonth(4, 2025), 30);
}

TEST(GetDaysInMonthTest, MonthOutsideRangeReturnsZero)
{
    EXPECT_EQ(getDaysInMonth(0, 2025), 0);
    EXPECT_EQ(getDaysInMonth(13, 2025), 0);
}

/* ---------- dateToDayNumber ---------- */

TEST(DateToDayNumberTest, EpochDateIsDayZero)
{
    Date_t epoch = makeDate(1, 1, 1970);
    long dayNumber = -1;

    EXPECT_EQ(dateToDayNumber(&epoch, &dayNumber), STATUS_SUCCESS);
    EXPECT_EQ(dayNumber, 0);
}

TEST(DateToDayNumberTest, LeapFebruaryContributes29Days)
{
    Date_t first = makeDate(1, 2, 2024);
    Date_t second = makeDate(1, 3, 2024);
    long firstNumber = 0;
    long secondNumber = 0;

    ASSERT_EQ(dateToDayNumber(&first, &firstNumber), STATUS_SUCCESS);
    ASSERT_EQ(dateToDayNumber(&second, &secondNumber), STATUS_SUCCESS);
    EXPECT_EQ(secondNumber - firstNumber, 29);
}

TEST(DateToDayNumberTest, InvalidDateIsRejected)
{
    Date_t invalid = makeDate(31, 4, 2025);
    long dayNumber = 123;

    EXPECT_EQ(dateToDayNumber(&invalid, &dayNumber), STATUS_ERR_INVALID_DATE);
    EXPECT_EQ(dayNumber, 123);
}

TEST(DateToDayNumberTest, NullPointersAreRejected)
{
    Date_t date = makeDate(1, 1, 2025);
    long dayNumber = 0;

    EXPECT_EQ(dateToDayNumber(NULL, &dayNumber), STATUS_ERR_NULL_POINTER);
    EXPECT_EQ(dateToDayNumber(&date, NULL), STATUS_ERR_NULL_POINTER);
}

/* ---------- dayNumberToDate ---------- */

TEST(DayNumberToDateTest, DayZeroIsEpochDate)
{
    Date_t date = makeDate(0, 0, 0);

    EXPECT_EQ(dayNumberToDate(0, &date), STATUS_SUCCESS);
    expectDate(date, 1, 1, 1970);
}

TEST(DayNumberToDateTest, ConversionRoundTripsAcrossManyYears)
{
    Date_t start = makeDate(1, 1, 2000);
    long startNumber = 0;
    bool allMatched = true;

    ASSERT_EQ(dateToDayNumber(&start, &startNumber), STATUS_SUCCESS);

    for (long offset = 0; offset < 4000; ++offset)
    {
        Date_t converted = makeDate(0, 0, 0);
        long backNumber = -1;

        if (dayNumberToDate(startNumber + offset, &converted) != STATUS_SUCCESS ||
            dateToDayNumber(&converted, &backNumber) != STATUS_SUCCESS ||
            backNumber != startNumber + offset)
        {
            allMatched = false;
            break;
        }
    }

    EXPECT_TRUE(allMatched);
}

TEST(DayNumberToDateTest, ExtremelyOldDayNumberIsRejected)
{
    Date_t date = makeDate(7, 7, 2007);

    EXPECT_EQ(dayNumberToDate(-1000000000L, &date), STATUS_ERR_INVALID_DATE);
    expectDate(date, 7, 7, 2007);
}

TEST(DayNumberToDateTest, NullPointerIsRejected)
{
    EXPECT_EQ(dayNumberToDate(0, NULL), STATUS_ERR_NULL_POINTER);
}

/* ---------- addDaysToDate ---------- */

TEST(AddDaysToDateTest, ThirtyDaysAfterJanuary31IsMarch2)
{
    Date_t start = makeDate(31, 1, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, 30, &result), STATUS_SUCCESS);
    expectDate(result, 2, 3, 2025);
}

TEST(AddDaysToDateTest, ZeroDaysReturnsSameDate)
{
    Date_t start = makeDate(15, 8, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, 0, &result), STATUS_SUCCESS);
    expectDate(result, 15, 8, 2025);
}

TEST(AddDaysToDateTest, CrossesYearBoundary)
{
    Date_t start = makeDate(31, 12, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, 1, &result), STATUS_SUCCESS);
    expectDate(result, 1, 1, 2026);
}

TEST(AddDaysToDateTest, LandsOnLeapDay)
{
    Date_t start = makeDate(28, 2, 2024);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, 1, &result), STATUS_SUCCESS);
    expectDate(result, 29, 2, 2024);
}

TEST(AddDaysToDateTest, SkipsLeapDayInCommonYear)
{
    Date_t start = makeDate(28, 2, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, 1, &result), STATUS_SUCCESS);
    expectDate(result, 1, 3, 2025);
}

TEST(AddDaysToDateTest, NegativeDaysAreRejected)
{
    Date_t start = makeDate(1, 1, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, -1, &result), STATUS_ERR_INVALID_ARGUMENT);
}

TEST(AddDaysToDateTest, DaysAboveMaximumAreRejected)
{
    Date_t start = makeDate(1, 1, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, MAX_INTERVAL_DAYS, &result), STATUS_SUCCESS);
    EXPECT_EQ(addDaysToDate(&start, MAX_INTERVAL_DAYS + 1, &result),
              STATUS_ERR_INVALID_ARGUMENT);
}

TEST(AddDaysToDateTest, InvalidStartDateIsRejected)
{
    Date_t start = makeDate(31, 4, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(&start, 5, &result), STATUS_ERR_INVALID_DATE);
}

TEST(AddDaysToDateTest, NullPointersAreRejected)
{
    Date_t start = makeDate(1, 1, 2025);
    Date_t result = makeDate(0, 0, 0);

    EXPECT_EQ(addDaysToDate(NULL, 5, &result), STATUS_ERR_NULL_POINTER);
    EXPECT_EQ(addDaysToDate(&start, 5, NULL), STATUS_ERR_NULL_POINTER);
}

TEST(AddDaysToDateTest, ResultIsUnchangedOnFailure)
{
    Date_t start = makeDate(31, 4, 2025);
    Date_t result = makeDate(9, 9, 2009);

    EXPECT_NE(addDaysToDate(&start, 5, &result), STATUS_SUCCESS);
    expectDate(result, 9, 9, 2009);
}

/* ---------- compareDates ---------- */

TEST(CompareDatesTest, EarlierYearReturnsMinusOne)
{
    Date_t earlier = makeDate(31, 12, 2024);
    Date_t later = makeDate(1, 1, 2025);

    EXPECT_EQ(compareDates(&earlier, &later), -1);
}

TEST(CompareDatesTest, EarlierMonthReturnsMinusOne)
{
    Date_t earlier = makeDate(30, 3, 2025);
    Date_t later = makeDate(1, 4, 2025);

    EXPECT_EQ(compareDates(&earlier, &later), -1);
}

TEST(CompareDatesTest, EarlierDayReturnsMinusOne)
{
    Date_t earlier = makeDate(9, 6, 2025);
    Date_t later = makeDate(10, 6, 2025);

    EXPECT_EQ(compareDates(&earlier, &later), -1);
}

TEST(CompareDatesTest, EqualDatesReturnZero)
{
    Date_t first = makeDate(15, 8, 2025);
    Date_t second = makeDate(15, 8, 2025);

    EXPECT_EQ(compareDates(&first, &second), 0);
}

TEST(CompareDatesTest, LaterDateReturnsOne)
{
    Date_t later = makeDate(2, 1, 2025);
    Date_t earlier = makeDate(1, 1, 2025);

    EXPECT_EQ(compareDates(&later, &earlier), 1);
}

TEST(CompareDatesTest, NullPointerReturnsZero)
{
    Date_t date = makeDate(1, 1, 2025);

    EXPECT_EQ(compareDates(NULL, &date), 0);
    EXPECT_EQ(compareDates(&date, NULL), 0);
}

/* ---------- formatDate ---------- */

TEST(FormatDateTest, UsesLeadingZeros)
{
    Date_t date = makeDate(5, 3, 2025);
    char text[DATE_TEXT_SIZE];

    EXPECT_EQ(formatDate(&date, text, sizeof(text)), STATUS_SUCCESS);
    EXPECT_STREQ(text, "05-03-2025");
}

TEST(FormatDateTest, TooSmallBufferIsRejected)
{
    Date_t date = makeDate(5, 3, 2025);
    char text[DATE_TEXT_SIZE - 1];

    EXPECT_EQ(formatDate(&date, text, sizeof(text)), STATUS_ERR_INVALID_ARGUMENT);
}

TEST(FormatDateTest, InvalidDateIsRejected)
{
    Date_t date = makeDate(30, 2, 2025);
    char text[DATE_TEXT_SIZE];

    EXPECT_EQ(formatDate(&date, text, sizeof(text)), STATUS_ERR_INVALID_DATE);
}

/* ---------- getTodayDate ---------- */

TEST(GetTodayDateTest, ReturnsARealCalendarDate)
{
    Date_t today = makeDate(0, 0, 0);

    ASSERT_EQ(getTodayDate(&today), STATUS_SUCCESS);
    EXPECT_GE(today.year, MIN_YEAR);
    EXPECT_GE(today.month, 1);
    EXPECT_LE(today.month, 12);
    EXPECT_GE(today.day, 1);
    EXPECT_LE(today.day, getDaysInMonth(today.month, today.year));
}

TEST(GetTodayDateTest, NullPointerIsRejected)
{
    EXPECT_EQ(getTodayDate(NULL), STATUS_ERR_NULL_POINTER);
}
