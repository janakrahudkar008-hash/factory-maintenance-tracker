/**
 * @file validation.c
 * @brief Implementation of input validation and strict parsing.
 */

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "validation.h"

#define MONTHS_PER_YEAR     12
#define DECIMAL_BASE        10
#define DATE_SEPARATOR      '-'
#define DAY_OFFSET          0
#define DAY_DIGITS          2
#define FIRST_SEPARATOR     2
#define MONTH_OFFSET        3
#define MONTH_DIGITS        2
#define SECOND_SEPARATOR    5
#define YEAR_OFFSET         6
#define YEAR_DIGITS         4

/* Returns 1 if the first count characters of text are all digits. */
static int hasOnlyDigits(const char *text, int count)
{
    int index;

    for (index = 0; index < count; ++index)
    {
        if (!isdigit((unsigned char)text[index]))
        {
            return 0;
        }
    }

    return 1;
}

/* Converts count digit characters to a number. Digits must be pre-checked. */
static int readNumber(const char *text, int count)
{
    int index;
    int number = 0;

    for (index = 0; index < count; ++index)
    {
        number = number * DECIMAL_BASE + (text[index] - '0');
    }

    return number;
}

Status_t validateEquipmentId(const char *id)
{
    size_t index;

    if (id == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (strlen(id) != (size_t)ID_LENGTH)
    {
        return STATUS_ERR_INVALID_ID;
    }

    if (strncmp(id, ID_PREFIX, (size_t)ID_PREFIX_LENGTH) != 0)
    {
        return STATUS_ERR_INVALID_ID;
    }

    for (index = (size_t)ID_PREFIX_LENGTH; index < (size_t)ID_LENGTH; ++index)
    {
        if (!isdigit((unsigned char)id[index]))
        {
            return STATUS_ERR_INVALID_ID;
        }
    }

    return STATUS_SUCCESS;
}

Status_t validateDate(const Date_t *date)
{
    int daysInMonth;

    if (date == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (date->year < MIN_YEAR || date->year > MAX_YEAR)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    if (date->month < 1 || date->month > MONTHS_PER_YEAR)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    daysInMonth = getDaysInMonth(date->month, date->year);
    if (date->day < 1 || date->day > daysInMonth)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    return STATUS_SUCCESS;
}

Status_t validateServiceInterval(int days)
{
    if (days < MIN_INTERVAL_DAYS || days > MAX_INTERVAL_DAYS)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return STATUS_SUCCESS;
}

Status_t validateRunningHours(double hours)
{
    /* Written as a positive range test so that NaN is rejected too. */
    if (!(hours >= 0.0 && hours <= MAX_RUNNING_HOURS))
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return STATUS_SUCCESS;
}

Status_t validateCost(double cost)
{
    if (!(cost >= 0.0 && cost <= MAX_COST))
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return STATUS_SUCCESS;
}

Status_t validateTextField(const char *text, size_t maxLen, int allowEmpty)
{
    size_t length;
    size_t index;
    int hasVisibleCharacter = 0;

    if (text == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    length = strlen(text);
    if (length > maxLen)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    for (index = 0; index < length; ++index)
    {
        unsigned char current = (unsigned char)text[index];

        if (current == (unsigned char)FIELD_DELIMITER || iscntrl(current))
        {
            return STATUS_ERR_INVALID_VALUE;
        }

        if (!isspace(current))
        {
            hasVisibleCharacter = 1;
        }
    }

    if (!allowEmpty && !hasVisibleCharacter)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return STATUS_SUCCESS;
}

Status_t validateMenuChoice(int choice, int min, int max)
{
    if (min > max)
    {
        return STATUS_ERR_INVALID_ARGUMENT;
    }

    if (choice < min || choice > max)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    return STATUS_SUCCESS;
}

Status_t parseInteger(const char *text, int *value)
{
    char *end = NULL;
    long parsed;

    if (text == NULL || value == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    /* strtol would silently skip leading spaces, so reject them here. */
    if (text[0] == '\0' || isspace((unsigned char)text[0]))
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    errno = 0;
    parsed = strtol(text, &end, DECIMAL_BASE);

    if (end == text || *end != '\0' || errno == ERANGE)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    if (parsed < INT_MIN || parsed > INT_MAX)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    *value = (int)parsed;

    return STATUS_SUCCESS;
}

Status_t parseDouble(const char *text, double *value)
{
    char *end = NULL;
    double parsed;

    if (text == NULL || value == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (text[0] == '\0' || isspace((unsigned char)text[0]))
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    /* strtod also accepts hexadecimal floats such as "0x10"; refuse them. */
    if (strpbrk(text, "xX") != NULL)
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    errno = 0;
    parsed = strtod(text, &end);

    if (end == text || *end != '\0' || errno == ERANGE || !isfinite(parsed))
    {
        return STATUS_ERR_INVALID_VALUE;
    }

    *value = parsed;

    return STATUS_SUCCESS;
}

Status_t parseDate(const char *text, Date_t *date)
{
    Date_t parsed;

    if (text == NULL || date == NULL)
    {
        return STATUS_ERR_NULL_POINTER;
    }

    if (strlen(text) != (size_t)(DATE_TEXT_SIZE - 1))
    {
        return STATUS_ERR_INVALID_DATE;
    }

    if (text[FIRST_SEPARATOR] != DATE_SEPARATOR ||
        text[SECOND_SEPARATOR] != DATE_SEPARATOR)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    if (!hasOnlyDigits(text + DAY_OFFSET, DAY_DIGITS) ||
        !hasOnlyDigits(text + MONTH_OFFSET, MONTH_DIGITS) ||
        !hasOnlyDigits(text + YEAR_OFFSET, YEAR_DIGITS))
    {
        return STATUS_ERR_INVALID_DATE;
    }

    parsed.day = readNumber(text + DAY_OFFSET, DAY_DIGITS);
    parsed.month = readNumber(text + MONTH_OFFSET, MONTH_DIGITS);
    parsed.year = readNumber(text + YEAR_OFFSET, YEAR_DIGITS);

    if (validateDate(&parsed) != STATUS_SUCCESS)
    {
        return STATUS_ERR_INVALID_DATE;
    }

    *date = parsed;

    return STATUS_SUCCESS;
}
