/**
 * @file date_utils.c
 * @brief Implementation of calendar arithmetic.
 *
 * Day numbers use the "days from civil" algorithm, which counts days since
 * 01-01-1970 without looping over months or years.
 */

#include <stdio.h>
#include <time.h>

#include "date_utils.h"

#define MONTHS_PER_YEAR         12
#define FEBRUARY                2
#define FEBRUARY_LEAP_DAYS      29
#define MIN_CALENDAR_YEAR       1
#define MAX_CALENDAR_YEAR       9999
#define TM_YEAR_BASE            1900
#define TM_MONTH_OFFSET         1

/* Constants of the day-number algorithm (400-year Gregorian cycle). */
#define DAYS_PER_YEAR           365L
#define DAYS_PER_400_YEARS      146097L
#define EPOCH_SHIFT_DAYS        719468L

static const int DAYS_IN_MONTH[MONTHS_PER_YEAR] =
{
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/* Returns 1 if the date is a real calendar date, 0 otherwise. */
static int isValidCalendarDate(const Date_t *date)
{
    int daysInMonth;

    if (date->year < MIN_CALENDAR_YEAR || date->year > MAX_CALENDAR_YEAR)
    {
        return 0;
    }

    daysInMonth = getDaysInMonth(date->month, date->year);

    return (daysInMonth > 0 && date->day >= 1 && date->day <= daysInMonth);
}

int isLeapYear(int year)
{
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

int getDaysInMonth(int month, int year)
{
    if (month < 1 || month > MONTHS_PER_YEAR)
    {
        return 0;
    }

    if (month == FEBRUARY && isLeapYear(year))
    {
        return FEBRUARY_LEAP_DAYS;
    }

    return DAYS_IN_MONTH[month - 1];
}

Status_t dateToDayNumber(const Date_t *date, long *dayNumber)
{
    long year;
    long month;
    long era;
    long yearOfEra;
    long dayOfYear;
    long dayOfEra;

    if (date == NULL || dayNumber == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (!isValidCalendarDate(date))
    {
        return STATUS_ERR_INVALID_DATE;
    }

    year = date->year;
    month = date->month;

    /* Treat January and February as months of the previous year. */
    if (month <= FEBRUARY)
    {
        year -= 1;
    }

    era = year / 400;
    yearOfEra = year - era * 400;
    dayOfYear = (153 * (month > FEBRUARY ? month - 3 : month + 9) + 2) / 5
                + date->day - 1;
    dayOfEra = yearOfEra * DAYS_PER_YEAR + yearOfEra / 4 - yearOfEra / 100
               + dayOfYear;

    *dayNumber = era * DAYS_PER_400_YEARS + dayOfEra - EPOCH_SHIFT_DAYS;

    return STATUS_SUCCESS;
}

Status_t dayNumberToDate(long dayNumber, Date_t *date)
{
    long shifted;
    long era;
    long dayOfEra;
    long yearOfEra;
    long year;
    long dayOfYear;
    long monthIndex;
    long day;
    long month;

    if (date == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    shifted = dayNumber + EPOCH_SHIFT_DAYS;
    era = (shifted >= 0 ? shifted : shifted - (DAYS_PER_400_YEARS - 1))
          / DAYS_PER_400_YEARS;
    dayOfEra = shifted - era * DAYS_PER_400_YEARS;
    yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524
                 - dayOfEra / 146096) / DAYS_PER_YEAR;
    year = yearOfEra + era * 400;
    dayOfYear = dayOfEra - (DAYS_PER_YEAR * yearOfEra + yearOfEra / 4
                            - yearOfEra / 100);
    monthIndex = (5 * dayOfYear + 2) / 153;
    day = dayOfYear - (153 * monthIndex + 2) / 5 + 1;
    month = (monthIndex < 10) ? monthIndex + 3 : monthIndex - 9;

    if (month <= FEBRUARY)
    {
        year += 1;
    }

    if (year < MIN_CALENDAR_YEAR || year > MAX_CALENDAR_YEAR)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    date->day = (int)day;
    date->month = (int)month;
    date->year = (int)year;

    return STATUS_SUCCESS;
}

Status_t addDaysToDate(const Date_t *start, int days, Date_t *result)
{
    long startNumber;
    Status_t status;

    if (start == NULL || result == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (days < 0 || days > MAX_INTERVAL_DAYS)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    status = dateToDayNumber(start, &startNumber);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    return dayNumberToDate(startNumber + days, result);
}

int compareDates(const Date_t *a, const Date_t *b)
{
    if (a == NULL || b == NULL)
    {
        return 0;
    }

    if (a->year != b->year)
    {
        return (a->year < b->year) ? -1 : 1;
    }

    if (a->month != b->month)
    {
        return (a->month < b->month) ? -1 : 1;
    }

    if (a->day != b->day)
    {
        return (a->day < b->day) ? -1 : 1;
    }

    return 0;
}

Status_t getTodayDate(Date_t *today)
{
    time_t now;
    const struct tm *localTime;

    if (today == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    now = time(NULL);
    if (now == (time_t)-1)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    localTime = localtime(&now);
    if (localTime == NULL)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    today->day = localTime->tm_mday;
    today->month = localTime->tm_mon + TM_MONTH_OFFSET;
    today->year = localTime->tm_year + TM_YEAR_BASE;

    return STATUS_SUCCESS;
}

Status_t formatDate(const Date_t *date, char *buffer, size_t size)
{
    int written;

    if (date == NULL || buffer == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (size < DATE_TEXT_SIZE)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    if (!isValidCalendarDate(date))
    {
        return STATUS_ERR_INVALID_DATE;
    }

    written = snprintf(buffer, size, "%02d-%02d-%04d",
                       date->day, date->month, date->year);
    if (written < 0 || (size_t)written >= size)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    return STATUS_SUCCESS;
}
