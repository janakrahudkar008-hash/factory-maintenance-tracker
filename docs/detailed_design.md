# Detailed Software Design: Factory Equipment Preventive Maintenance Tracker

| Field | Details |
|---|---|
| Student name | Janak Sharad Rahudkar |
| Roll number | 612509021 |
| Division / batch | 1 |
| Repository | https://github.com/janakrahudkar008-hash/factory-maintenance-tracker |
| Related document | `docs/project_proposal.md` |

## 0. Changes from the Proposal

During detailed design one refinement was made:

- **A new module `date_utils` was added.** Date arithmetic (leap years, days in month, adding days, comparing dates) is needed by both `validation` and `maintenance`. Keeping it in `maintenance` would make `validation` depend on `maintenance`, and `maintenance` already depends on `validation`, creating a circular dependency. A separate `date_utils` module removes the cycle and keeps date logic independently testable.
- A header-only file `common.h` holds the shared status enumeration and project-wide constants (it has no `.c` file).

---

## 1. Software Architecture

### 1.1 Layers

| Layer | Module(s) | Responsibility |
|---|---|---|
| Interface | `main.c` | Menu, reading user input, workflow control, final cleanup |
| Validation | `validation.c` | Validate and parse every input |
| Data management | `equipment.c` | Own equipment and service-log collections; add, find, update, delete |
| Business logic | `maintenance.c`, `date_utils.c` | Dates, next-due calculation, status decision, sort, filter, statistics |
| File storage | `file_handler.c` | Load and save text files |
| Reports | `report.c` | Format and print reports to any `FILE *` stream |

### 1.2 Module Dependency Diagram

```
                    main.c
     +--------+--------+--------+---------+--------+
     |        |        |        |         |        |
     v        v        v        v         v        v
 report   file_handler  maintenance   equipment  validation
   |  \        |  \          |  \         |           |
   |   \       |   \         |   \        v           v
   |    +------+----+--------+----+---> validation  date_utils
   |           |    |        |    |
   +-----------+----+--> equipment, maintenance, date_utils
                              |
                              v
                          date_utils            (common.h used by all)
```

Simplified rule: **lower layers never call upper layers.** `date_utils` and `validation` call no other project module except `date_utils`.

### 1.3 Design Principles

- No dynamic memory. All collections are fixed-size arrays inside structures.
- The maintenance status is **derived**, never stored in a file. It is always calculated from dates, which prevents stale or inconsistent data.
- "Today" is passed to functions as a parameter (`const Date_t *today`), so calculations are deterministic and unit-testable.
- Report functions write to a `FILE *`, so they print to the screen in the application and to a temporary file in tests.
- Every function that can fail returns `Status_t`; results are returned through output parameters.

---

## 2. Module Design

| Module | Source | Header | Purpose |
|---|---|---|---|
| Common definitions | none | `common.h` | `Status_t`, shared constants |
| Date utilities | `date_utils.c` | `date_utils.h` | Date structure and calendar arithmetic |
| Validation | `validation.c` | `validation.h` | Input validation and strict parsing |
| Equipment management | `equipment.c` | `equipment.h` | Equipment and service-log data model and operations |
| Maintenance logic | `maintenance.c` | `maintenance.h` | Next due, days remaining, status, sort, filter, statistics |
| File handling | `file_handler.c` | `file_handler.h` | Persist and restore data |
| Reports | `report.c` | `report.h` | Reports and tabular output |
| Main application | `main.c` | none | High-level control only |

### 2.1 `date_utils`

- **Purpose:** Calendar arithmetic independent of any maintenance concept.
- **Public functions:** `isLeapYear`, `getDaysInMonth`, `dateToDayNumber`, `dayNumberToDate`, `addDaysToDate`, `compareDates`, `getTodayDate`, `formatDate`
- **Data used:** `Date_t`
- **Depends on:** `common.h`
- **Status codes:** `STATUS_SUCCESS`, `STATUS_ERR_NULL_POINTER`, `STATUS_ERR_INVALID_DATE`, `STATUS_ERR_INVALID_ARGUMENT`
- **Year range:** accepts real calendar dates in years 1 to 9999. The business range 2000 to 2100 is enforced by `validation`.

### 2.2 `validation`

- **Purpose:** Decide whether an input is acceptable and convert text to numbers or dates safely.
- **Public functions:** `validateEquipmentId`, `validateDate`, `validateServiceInterval`, `validateRunningHours`, `validateCost`, `validateTextField`, `validateMenuChoice`, `parseInteger`, `parseDouble`, `parseDate`
- **Data used:** constants from `common.h` (record-level validation lives in `equipment`, which avoids a circular dependency)
- **Depends on:** `date_utils`, `common.h`
- **Status codes:** `STATUS_SUCCESS`, `STATUS_ERR_NULL_POINTER`, `STATUS_ERR_INVALID_ID`, `STATUS_ERR_INVALID_DATE`, `STATUS_ERR_INVALID_VALUE`, `STATUS_ERR_INVALID_ARGUMENT`

### 2.3 `equipment`

- **Purpose:** Own the collections of equipment and service records and keep them consistent.
- **Public functions:** `validateEquipmentRecord`, `validateServiceRecord`, `initEquipmentList`, `initServiceLog`, `addEquipment`, `findEquipmentById`, `updateEquipment`, `deleteEquipment`, `addServiceRecord`, `logServiceEvent`
- **Data owned:** `EquipmentList_t`, `ServiceLog_t` (allocated in `main`, passed by pointer)
- **Depends on:** `validation`, `date_utils`
- **Status codes:** `STATUS_SUCCESS`, `STATUS_ERR_NULL_POINTER`, `STATUS_ERR_DUPLICATE`, `STATUS_ERR_NOT_FOUND`, `STATUS_ERR_FULL`, plus validation codes

### 2.4 `maintenance`

- **Purpose:** All maintenance decisions and calculations.
- **Public functions:** `calculateNextServiceDate`, `calculateDaysRemaining`, `determineStatus`, `getStatusName`, `getSortedByNextDue`, `filterByStatus`, `calculateAverageServiceInterval`, `calculateFleetSummary`
- **Data used:** `Equipment_t`, `ServiceLog_t`, `MaintenanceStatus_t`, `FleetSummary_t`
- **Depends on:** `equipment`, `date_utils`
- **Status codes:** `STATUS_SUCCESS`, `STATUS_ERR_NULL_POINTER`, `STATUS_ERR_INVALID_ARGUMENT`, `STATUS_ERR_EMPTY`, `STATUS_ERR_INSUFFICIENT_DATA`

### 2.5 `file_handler`

- **Purpose:** Convert between in-memory records and text files, with full error checking.
- **Public functions:** `saveEquipmentToFile`, `loadEquipmentFromFile`, `saveServiceLogToFile`, `loadServiceLogFromFile`, `parseEquipmentLine`, `parseServiceRecordLine`
- **Depends on:** `equipment`, `validation`, `date_utils`
- **Status codes:** `STATUS_SUCCESS`, `STATUS_ERR_NULL_POINTER`, `STATUS_ERR_FILE_OPEN`, `STATUS_ERR_FILE_IO`, `STATUS_ERR_FILE_FORMAT`, `STATUS_ERR_FULL`

### 2.6 `report`

- **Purpose:** Present information. It performs no calculation that belongs to `maintenance`.
- **Public functions:** `getEquipmentTypeName`, `getStatusMessage`, `printEquipmentDetails`, `printEquipmentTable`, `generateDueReport`, `generateOverdueReport`, `generateFleetSummaryReport`, `generateServiceHistoryReport`
- **Depends on:** `equipment`, `maintenance`, `date_utils`
- **Status codes:** `STATUS_SUCCESS`, `STATUS_ERR_NULL_POINTER`, `STATUS_ERR_INVALID_ARGUMENT`, `STATUS_ERR_NOT_FOUND`, `STATUS_ERR_FILE_IO`

### 2.7 `main.c` responsibilities

1. Create the empty `EquipmentList_t` and `ServiceLog_t`.
2. Load existing data (a missing file means a fresh start, not an error).
3. Show the menu and read the choice.
4. Prompt for inputs, call the module functions, display the result or error message.
5. Track whether data was modified.
6. Save data on exit and finish cleanly.

`main.c` contains static helper functions for prompting (`handleAddEquipment`, `handleSearch`, and so on) but no validation rules, calculations or file formats.

---

## 3. Data Design

### 3.1 Constants (`common.h`)

| Constant | Value | Purpose |
|---|---|---|
| `MAX_EQUIPMENT` | 100 | Maximum equipment records |
| `MAX_SERVICE_RECORDS` | 500 | Maximum service-log entries |
| `ID_PREFIX` | `"EQ-"` | Required prefix of equipment ID |
| `ID_DIGITS` | 4 | Number of digits after the prefix |
| `ID_LENGTH` | 7 | Characters in a valid ID (`EQ-0001`) |
| `MAX_NAME_LEN` | 30 | Maximum equipment name length |
| `MAX_LOCATION_LEN` | 30 | Maximum location length |
| `MAX_TECHNICIAN_LEN` | 30 | Maximum technician name length |
| `MAX_REMARKS_LEN` | 60 | Maximum remarks length |
| `MIN_INTERVAL_DAYS` | 1 | Smallest service interval |
| `MAX_INTERVAL_DAYS` | 365 | Largest service interval |
| `MAX_RUNNING_HOURS` | 500000.0 | Largest running-hours value |
| `MAX_COST` | 10000000.0 | Largest service cost |
| `MIN_YEAR` | 2000 | Earliest accepted year |
| `MAX_YEAR` | 2100 | Latest accepted year |
| `DUE_SOON_DAYS` | 7 | Days before due date that count as "due soon" |
| `MAX_LINE_LEN` | 256 | Buffer size for reading a line |
| `DATE_TEXT_SIZE` | 11 | Buffer size for a formatted date `DD-MM-YYYY` (defined in `date_utils.h`) |
| `FIELD_DELIMITER` | `'|'` | Separator in data files |
| `EQUIPMENT_FILE_PATH` | `"data/equipment.txt"` | Equipment data file (relative path) |
| `SERVICE_LOG_FILE_PATH` | `"data/service_log.txt"` | Service-log data file (relative path) |

Buffers are sized as `MAX_x_LEN + 1` to hold the terminating null character.

### 3.2 Enumerations

**`Status_t`** (function result codes)

| Constant | Meaning |
|---|---|
| `STATUS_SUCCESS` (0) | Operation completed |
| `STATUS_ERR_NULL_POINTER` | A required pointer argument was NULL |
| `STATUS_ERR_INVALID_ARGUMENT` | Argument outside its allowed use (for example a bad index or size) |
| `STATUS_ERR_INVALID_ID` | Equipment ID has wrong format |
| `STATUS_ERR_INVALID_DATE` | Date does not exist or is out of range |
| `STATUS_ERR_INVALID_VALUE` | Number, text or choice outside allowed range |
| `STATUS_ERR_DUPLICATE` | Record with this ID already exists |
| `STATUS_ERR_NOT_FOUND` | No record with this ID |
| `STATUS_ERR_EMPTY` | Collection is empty where data is required |
| `STATUS_ERR_FULL` | Maximum storage capacity reached |
| `STATUS_ERR_INSUFFICIENT_DATA` | Too few records for the calculation |
| `STATUS_ERR_FILE_OPEN` | File could not be opened |
| `STATUS_ERR_FILE_IO` | Read or write operation failed |
| `STATUS_ERR_FILE_FORMAT` | File content is malformed |

**`MaintenanceStatus_t`** (derived equipment condition)

| Constant | Meaning |
|---|---|
| `MAINT_OK` | More than `DUE_SOON_DAYS` days until service |
| `MAINT_DUE_SOON` | 0 to `DUE_SOON_DAYS` days until service |
| `MAINT_OVERDUE` | Service date has passed |
| `MAINT_UNDER_REPAIR` | Equipment flagged as under repair (overrides the others) |
| `MAINT_COUNT` | Number of statuses (used for range checking, not a real status) |

**`EquipmentType_t`**

| Constant | Meaning |
|---|---|
| `EQTYPE_LATHE` | Lathe |
| `EQTYPE_DRILL` | Drilling machine |
| `EQTYPE_MILL` | Milling machine |
| `EQTYPE_COMPRESSOR` | Air compressor |
| `EQTYPE_CONVEYOR` | Conveyor |
| `EQTYPE_GENERATOR` | Generator |
| `EQTYPE_OTHER` | Other equipment |
| `EQTYPE_COUNT` | Number of types (used for range checking, not a real type) |

### 3.3 Structures

```c
typedef struct
{
    int day;
    int month;
    int year;
} Date_t;

typedef struct
{
    char id[ID_LENGTH + 1];
    char name[MAX_NAME_LEN + 1];
    EquipmentType_t type;
    char location[MAX_LOCATION_LEN + 1];
    Date_t installDate;
    int serviceIntervalDays;
    Date_t lastServiceDate;
    double runningHours;
    int underRepair;            /* 0 = no, 1 = yes */
} Equipment_t;

typedef struct
{
    char equipmentId[ID_LENGTH + 1];
    Date_t serviceDate;
    char technician[MAX_TECHNICIAN_LEN + 1];
    double cost;
    char remarks[MAX_REMARKS_LEN + 1];
} ServiceRecord_t;

typedef struct
{
    Equipment_t items[MAX_EQUIPMENT];
    int count;
} EquipmentList_t;

typedef struct
{
    ServiceRecord_t records[MAX_SERVICE_RECORDS];
    int count;
} ServiceLog_t;

typedef struct
{
    int total;
    int okCount;
    int dueSoonCount;
    int overdueCount;
    int underRepairCount;
    double overduePercent;
} FleetSummary_t;
```

**Relationship:** one `Equipment_t` has zero or more `ServiceRecord_t` entries. They are linked by `equipmentId`. Deleting equipment does not delete its service history (history is kept for audit); the history report for a deleted ID reports "not found" for the equipment but can still list the log lines.

### 3.4 Data Dictionary

| Data Item | C Type | Valid Range or Size | Validation Rule | Description |
|---|---|---|---|---|
| `id` | `char[8]` | `EQ-` + 4 digits, for example `EQ-0001` | Exact length 7, prefix match, remaining characters are digits, unique in list | Unique equipment identifier |
| `name` | `char[31]` | 1 to 30 characters | Not empty, no `\|`, not only spaces | Equipment name |
| `type` | `EquipmentType_t` | 0 to `EQTYPE_COUNT - 1` | In enumeration range | Equipment category |
| `location` | `char[31]` | 1 to 30 characters | Not empty, no `\|` | Where the machine is installed |
| `installDate` | `Date_t` | 01-01-2000 to 31-12-2100 | Real calendar date, not in the future | Date machine was installed |
| `serviceIntervalDays` | `int` | 1 to 365 | Within range | Days between services |
| `lastServiceDate` | `Date_t` | 01-01-2000 to 31-12-2100 | Real date, not before `installDate`, not in the future | Most recent service |
| `runningHours` | `double` | 0.0 to 500000.0 | Not negative, not above maximum, not NaN | Total operating hours |
| `underRepair` | `int` | 0 or 1 | Exactly 0 or 1 | Repair flag |
| `technician` | `char[31]` | 1 to 30 characters | Not empty, no `\|` | Person who serviced |
| `cost` | `double` | 0.0 to 10000000.0 | Not negative, not above maximum | Service cost |
| `remarks` | `char[61]` | 0 to 60 characters | No `\|`; may be empty | Notes on the service |
| `serviceDate` | `Date_t` | as for dates | Real date, not in the future | Date service was performed |
| menu choice | `int` | 0 to 9 | Integer only, within range | Main-menu selection |

---

## 4. Function Design

### 4.1 Detailed Specifications of Key Functions

#### `validateEquipmentId`

- **Module:** `validation`
- **Prototype:** `Status_t validateEquipmentId(const char *id);`
- **Purpose:** Check that an ID follows the format `EQ-NNNN`.
- **Parameters:** `id` (`const char *`, input): text to check; must not be NULL.
- **Return values:** `STATUS_SUCCESS` if valid; `STATUS_ERR_NULL_POINTER` if NULL; `STATUS_ERR_INVALID_ID` otherwise.
- **Preconditions:** If not NULL, `id` is a null-terminated string.
- **Postconditions:** No data is modified.
- **Errors handled:** NULL, wrong length, wrong prefix, lowercase prefix, non-digit characters.
- **Requirements:** FR-02 | **Tests:** UT-01 to UT-04

#### `validateDate`

- **Module:** `validation`
- **Prototype:** `Status_t validateDate(const Date_t *date);`
- **Purpose:** Check that a date is a real calendar date between `MIN_YEAR` and `MAX_YEAR`.
- **Parameters:** `date` (`const Date_t *`, input).
- **Return values:** `STATUS_SUCCESS`; `STATUS_ERR_NULL_POINTER`; `STATUS_ERR_INVALID_DATE`.
- **Preconditions:** None beyond a non-NULL pointer.
- **Postconditions:** No data is modified.
- **Errors handled:** Month outside 1 to 12, day outside the month's length (including 29 February in non-leap years), year outside range.
- **Requirements:** FR-04 | **Tests:** UT-08 to UT-11

#### `parseDate`

- **Module:** `validation`
- **Prototype:** `Status_t parseDate(const char *text, Date_t *date);`
- **Purpose:** Convert text in `DD-MM-YYYY` format to a validated `Date_t`.
- **Parameters:** `text` (input, null-terminated); `date` (output, written only on success).
- **Return values:** `STATUS_SUCCESS`; `STATUS_ERR_NULL_POINTER`; `STATUS_ERR_INVALID_DATE`.
- **Preconditions:** `text` and `date` non-NULL.
- **Postconditions:** On success `*date` holds a valid date. On failure `*date` is unchanged.
- **Errors handled:** Wrong separators, non-numeric parts, wrong digit counts, trailing characters, impossible dates.
- **Requirements:** FR-04, FR-20 | **Tests:** UT-12, UT-13

#### `addDaysToDate`

- **Module:** `date_utils`
- **Prototype:** `Status_t addDaysToDate(const Date_t *start, int days, Date_t *result);`
- **Purpose:** Compute the date that is `days` days after `start`.
- **Parameters:** `start` (input); `days` (input, 0 to `MAX_INTERVAL_DAYS`); `result` (output).
- **Return values:** `STATUS_SUCCESS`; `STATUS_ERR_NULL_POINTER`; `STATUS_ERR_INVALID_DATE` (start invalid); `STATUS_ERR_INVALID_ARGUMENT` (negative days).
- **Preconditions:** `start` is a valid date.
- **Postconditions:** `*result` is a valid date handling month ends, year ends and leap years.
- **Algorithm:** Convert to a day number, add days, convert back (civil-from-days algorithm), which avoids month-by-month loops.
- **Requirements:** FR-10 | **Tests:** UT-17 to UT-19

#### `determineStatus`

- **Module:** `maintenance`
- **Prototype:** `Status_t determineStatus(const Equipment_t *equipment, const Date_t *today, MaintenanceStatus_t *status);`
- **Purpose:** Decide the maintenance status of one machine.
- **Parameters:** `equipment` (input); `today` (input); `status` (output).
- **Decision rule:**
  1. If `underRepair` is 1, the result is `MAINT_UNDER_REPAIR`.
  2. Otherwise compute `days = nextServiceDate - today`.
  3. `days < 0` gives `MAINT_OVERDUE`.
  4. `0 <= days <= DUE_SOON_DAYS` gives `MAINT_DUE_SOON`.
  5. Otherwise `MAINT_OK`.
- **Return values:** `STATUS_SUCCESS`; `STATUS_ERR_NULL_POINTER`; `STATUS_ERR_INVALID_DATE`.
- **Postconditions:** `*status` is set only on success.
- **Boundaries:** days = 8 is OK, 7 is due soon, 0 is due soon, -1 is overdue.
- **Requirements:** FR-12 | **Tests:** UT-22 to UT-26

#### `addEquipment`

- **Module:** `equipment`
- **Prototype:** `Status_t addEquipment(EquipmentList_t *list, const Equipment_t *newItem, const Date_t *today);`
- **Purpose:** Append a validated, unique record to the list.
- **Parameters:** `list` (input-output); `newItem` (input); `today` (input, used for the "not in the future" date rules).
- **Return values:** `STATUS_SUCCESS`; `STATUS_ERR_NULL_POINTER`; any code from `validateEquipmentRecord`; `STATUS_ERR_DUPLICATE`; `STATUS_ERR_FULL`.
- **Preconditions:** `list` was initialised by `initEquipmentList`.
- **Postconditions:** On success `count` increases by 1 and the record is stored at the end. On any error the list is unchanged.
- **Requirements:** FR-01, FR-03 | **Tests:** UT-29 to UT-31

#### `logServiceEvent`

- **Module:** `equipment`
- **Prototype:** `Status_t logServiceEvent(EquipmentList_t *equipment, ServiceLog_t *log, const ServiceRecord_t *record, const Date_t *today);`
- **Purpose:** Record a service and update the machine's last service date.
- **Return values:** `STATUS_SUCCESS`; `STATUS_ERR_NULL_POINTER`; `STATUS_ERR_NOT_FOUND` (unknown equipment); `STATUS_ERR_FULL` (log full); validation codes for bad fields.
- **Preconditions:** Both collections initialised.
- **Postconditions:** On success the record is appended to the log, the equipment's `lastServiceDate` is set to the record date if it is later than the current one, and `underRepair` is cleared. On error neither collection changes.
- **Requirements:** FR-09 | **Tests:** UT-34 to UT-36

#### `loadEquipmentFromFile`

- **Module:** `file_handler`
- **Prototype:** `Status_t loadEquipmentFromFile(const char *path, EquipmentList_t *list, int *skippedLines);`
- **Purpose:** Read equipment records from a text file.
- **Parameters:** `path` (input); `list` (output, cleared first); `skippedLines` (output, count of malformed lines).
- **Return values:** `STATUS_SUCCESS` (all lines valid or file empty); `STATUS_ERR_NULL_POINTER`; `STATUS_ERR_FILE_OPEN`; `STATUS_ERR_FILE_FORMAT` (at least one invalid line, valid lines still loaded); `STATUS_ERR_FULL` (more lines than capacity); `STATUS_ERR_FILE_IO`.
- **Preconditions:** `list` is a valid pointer.
- **Postconditions:** The file is always closed. Lines starting with `#` and blank lines are ignored.
- **Requirements:** FR-18, FR-19 | **Tests:** UT-38 to UT-41

### 4.2 Function Catalogue (All Functions)

#### `date_utils`

| Function | Prototype | Purpose | Returns |
|---|---|---|---|
| `isLeapYear` | `int isLeapYear(int year);` | Leap-year test | 1 or 0 |
| `getDaysInMonth` | `int getDaysInMonth(int month, int year);` | Days in a month | 28 to 31, or 0 for an invalid month |
| `dateToDayNumber` | `Status_t dateToDayNumber(const Date_t *date, long *dayNumber);` | Convert date to a day count | SUCCESS, NULL_POINTER, INVALID_DATE |
| `dayNumberToDate` | `Status_t dayNumberToDate(long dayNumber, Date_t *date);` | Inverse conversion | SUCCESS, NULL_POINTER |
| `addDaysToDate` | see 4.1 | Date plus days | see 4.1 |
| `compareDates` | `int compareDates(const Date_t *a, const Date_t *b);` | Order two dates | -1, 0, 1 (NULL treated as equal, 0) |
| `getTodayDate` | `Status_t getTodayDate(Date_t *today);` | Read the system date | SUCCESS, NULL_POINTER, INVALID_DATE |
| `formatDate` | `Status_t formatDate(const Date_t *date, char *buffer, size_t size);` | Date to `DD-MM-YYYY` text | SUCCESS, NULL_POINTER, INVALID_ARGUMENT (buffer under 11 bytes) |

#### `validation`

| Function | Prototype | Purpose | Returns |
|---|---|---|---|
| `validateEquipmentId` | see 4.1 | ID format | see 4.1 |
| `validateDate` | see 4.1 | Real date and range | see 4.1 |
| `validateServiceInterval` | `Status_t validateServiceInterval(int days);` | Interval in range | SUCCESS, INVALID_VALUE |
| `validateRunningHours` | `Status_t validateRunningHours(double hours);` | Hours in range | SUCCESS, INVALID_VALUE |
| `validateCost` | `Status_t validateCost(double cost);` | Cost in range | SUCCESS, INVALID_VALUE |
| `validateTextField` | `Status_t validateTextField(const char *text, size_t maxLen, int allowEmpty);` | Length, delimiter, blank check | SUCCESS, NULL_POINTER, INVALID_VALUE |
| `validateMenuChoice` | `Status_t validateMenuChoice(int choice, int min, int max);` | Choice in range | SUCCESS, INVALID_VALUE, INVALID_ARGUMENT (min above max) |
| `parseInteger` | `Status_t parseInteger(const char *text, int *value);` | Strict text to int using `strtol` | SUCCESS, NULL_POINTER, INVALID_VALUE |
| `parseDouble` | `Status_t parseDouble(const char *text, double *value);` | Strict text to double using `strtod` | SUCCESS, NULL_POINTER, INVALID_VALUE |
| `parseDate` | see 4.1 | Text to date | see 4.1 |

#### `equipment`

| Function | Prototype | Purpose | Returns |
|---|---|---|---|
| `initEquipmentList` | `Status_t initEquipmentList(EquipmentList_t *list);` | Set count to 0 | SUCCESS, NULL_POINTER |
| `initServiceLog` | `Status_t initServiceLog(ServiceLog_t *log);` | Set count to 0 | SUCCESS, NULL_POINTER |
| `validateEquipmentRecord` | `Status_t validateEquipmentRecord(const Equipment_t *item, const Date_t *today);` | All fields plus cross-field rules (last service not before install, no future dates) | SUCCESS or the first error found |
| `validateServiceRecord` | `Status_t validateServiceRecord(const ServiceRecord_t *record, const Date_t *today);` | All fields of a service record, no future date | SUCCESS or the first error found |
| `addEquipment` | see 4.1 | Add record | see 4.1 |
| `findEquipmentById` | `Status_t findEquipmentById(const EquipmentList_t *list, const char *id, int *index);` | Linear search | SUCCESS, NULL_POINTER, INVALID_ID, NOT_FOUND |
| `updateEquipment` | `Status_t updateEquipment(EquipmentList_t *list, const char *id, const Equipment_t *newData, const Date_t *today);` | Replace fields of an existing record (ID cannot change) | SUCCESS, NULL_POINTER, NOT_FOUND, validation codes |
| `deleteEquipment` | `Status_t deleteEquipment(EquipmentList_t *list, const char *id);` | Remove record and close the gap | SUCCESS, NULL_POINTER, NOT_FOUND |
| `addServiceRecord` | `Status_t addServiceRecord(ServiceLog_t *log, const ServiceRecord_t *record);` | Append a log entry | SUCCESS, NULL_POINTER, FULL |
| `logServiceEvent` | see 4.1 | Log and update machine | see 4.1 |

#### `maintenance`

| Function | Prototype | Purpose | Returns |
|---|---|---|---|
| `calculateNextServiceDate` | `Status_t calculateNextServiceDate(const Equipment_t *equipment, Date_t *nextDate);` | Last service plus interval | SUCCESS, NULL_POINTER, INVALID_DATE |
| `calculateDaysRemaining` | `Status_t calculateDaysRemaining(const Equipment_t *equipment, const Date_t *today, long *days);` | Next due minus today (negative if overdue) | SUCCESS, NULL_POINTER, INVALID_DATE |
| `determineStatus` | see 4.1 | Status decision | see 4.1 |
| `getStatusName` | `const char *getStatusName(MaintenanceStatus_t status);` | Status to text | Text, or `"UNKNOWN"` for an out-of-range value |
| `getSortedByNextDue` | `Status_t getSortedByNextDue(const EquipmentList_t *source, EquipmentList_t *sorted);` | Copy and sort by next due date (stable insertion sort); source is not modified | SUCCESS (empty source gives count 0), NULL_POINTER |
| `filterByStatus` | `Status_t filterByStatus(const EquipmentList_t *source, MaintenanceStatus_t wanted, const Date_t *today, EquipmentList_t *result);` | Copy matching records | SUCCESS (no matches gives count 0), NULL_POINTER, INVALID_ARGUMENT |
| `calculateAverageServiceInterval` | `Status_t calculateAverageServiceInterval(const ServiceLog_t *log, const char *id, double *average);` | (Latest date - earliest date) / (n - 1) for one machine | SUCCESS, NULL_POINTER, INSUFFICIENT_DATA (fewer than 2 records) |
| `calculateFleetSummary` | `Status_t calculateFleetSummary(const EquipmentList_t *list, const Date_t *today, FleetSummary_t *summary);` | Count per status and overdue percentage | SUCCESS, NULL_POINTER, EMPTY (no equipment) |

#### `file_handler`

| Function | Prototype | Purpose | Returns |
|---|---|---|---|
| `saveEquipmentToFile` | `Status_t saveEquipmentToFile(const char *path, const EquipmentList_t *list);` | Write all records (to a temporary file, then rename) | SUCCESS, NULL_POINTER, FILE_OPEN, FILE_IO |
| `loadEquipmentFromFile` | see 4.1 | Read records | see 4.1 |
| `saveServiceLogToFile` | `Status_t saveServiceLogToFile(const char *path, const ServiceLog_t *log);` | Write all log entries | SUCCESS, NULL_POINTER, FILE_OPEN, FILE_IO |
| `loadServiceLogFromFile` | `Status_t loadServiceLogFromFile(const char *path, ServiceLog_t *log, int *skippedLines);` | Read log entries | as for equipment load |
| `parseEquipmentLine` | `Status_t parseEquipmentLine(const char *line, Equipment_t *item, const Date_t *today);` | One text line to a validated record | SUCCESS, NULL_POINTER, FILE_FORMAT |
| `parseServiceRecordLine` | `Status_t parseServiceRecordLine(const char *line, ServiceRecord_t *record, const Date_t *today);` | One text line to a validated record | SUCCESS, NULL_POINTER, FILE_FORMAT |

#### `report`

| Function | Prototype | Purpose | Returns |
|---|---|---|---|
| `getEquipmentTypeName` | `const char *getEquipmentTypeName(EquipmentType_t type);` | Equipment type to text | Text, or `"Unknown"` |
| `printEquipmentDetails` | `Status_t printEquipmentDetails(FILE *out, const Equipment_t *equipment, const Date_t *today);` | All fields of one machine plus due date and status | SUCCESS, NULL_POINTER, date errors, FILE_IO |
| `printEquipmentTable` | `Status_t printEquipmentTable(FILE *out, const EquipmentList_t *list, const Date_t *today);` | Table: ID, name, next due, days left, status | SUCCESS, NULL_POINTER |
| `generateDueReport` | `Status_t generateDueReport(FILE *out, const EquipmentList_t *list, const Date_t *today, int withinDays);` | Equipment due within N days, overdue machines included, earliest first (machines under repair excluded) | SUCCESS, NULL_POINTER, INVALID_ARGUMENT |
| `generateOverdueReport` | `Status_t generateOverdueReport(FILE *out, const EquipmentList_t *list, const Date_t *today);` | Overdue equipment with days late | SUCCESS, NULL_POINTER |
| `generateFleetSummaryReport` | `Status_t generateFleetSummaryReport(FILE *out, const FleetSummary_t *summary);` | Counts and percentages | SUCCESS, NULL_POINTER |
| `generateServiceHistoryReport` | `Status_t generateServiceHistoryReport(FILE *out, const ServiceLog_t *log, const char *id);` | History and average interval for one machine | SUCCESS, NULL_POINTER, NOT_FOUND (no records) |
| `getStatusMessage` | `const char *getStatusMessage(Status_t status);` | User-friendly text for a status code | Message text |

---

## 5. Input-Validation Plan

| Input | Source | Validation Rule | Invalid Example | Expected Error Behaviour |
|---|---|---|---|---|
| Menu choice | User | Whole number 0 to 9, nothing else on the line | `abc`, `12`, `3x`, empty | Message "Invalid menu choice", menu shown again |
| Equipment ID | User, file | `EQ-` followed by 4 digits | `eq-1`, `EQ-12345`, `1234` | "Invalid ID format (use EQ-0001)", re-prompt |
| Duplicate ID | User, file | Must not already exist when adding | Adding `EQ-0001` twice | `STATUS_ERR_DUPLICATE`, record rejected |
| Name, location, technician | User, file | Not empty, not blank, max length, no `\|` | Empty line, 40-character name | "Text must be 1 to 30 characters", re-prompt |
| Remarks | User, file | 0 to 60 characters, no `\|` | 80-character remark | Re-prompt |
| Equipment type | User, file | Integer 0 to 6 | `9`, `-1`, `lathe` | Re-prompt with the list of types |
| Dates | User, file | `DD-MM-YYYY`, real date, year 2000 to 2100, not in the future | `31-02-2025`, `2025-01-10`, `01-01-1999` | "Invalid date", re-prompt |
| Date order | User, file | Last service not before install date | Install 2024, last service 2023 | Record rejected with `STATUS_ERR_INVALID_DATE` |
| Service interval | User, file | Integer 1 to 365 | `0`, `366`, `30.5`, `abc` | Re-prompt |
| Running hours | User, file | Number 0.0 to 500000.0 | `-5`, `abc`, `1e99` | Re-prompt |
| Service cost | User, file | Number 0.0 to 10000000.0 | `-100` | Re-prompt |
| Repair flag | User, file | Exactly 0 or 1 | `2` | Re-prompt |
| Empty input | User | Required fields cannot be empty | Pressing Enter only | Re-prompt (unless the field allows empty) |
| Search/delete ID not present | User | Must exist | `EQ-9999` | "Equipment not found", return to menu |
| Capacity | User, file | Fewer than 100 equipment and 500 log entries | 101st record | `STATUS_ERR_FULL`, "Storage full" |
| File line | File | 9 fields (equipment) or 5 fields (log), each field valid | Missing field, text in a number field | Line skipped, counted, user warned at startup |

**Validation strategy:**

1. **Safe reading:** all keyboard input is read with `fgets` into a fixed buffer, the newline is removed, and over-long lines are discarded. `scanf` is not used for input. `gets` is never used.
2. **Parse, then validate:** text is converted using `parseInteger`, `parseDouble` and `parseDate` (based on `strtol`, `strtod`), which reject trailing characters and overflow.
3. **Validate before use:** modules that store data (`addEquipment`, `updateEquipment`, `logServiceEvent`) call the validation functions themselves, so invalid data cannot enter even if the interface layer forgets a check.
4. **Same rules for files and keyboard:** file lines pass through the same validators, so a hand-edited data file cannot corrupt the program state.
5. **Re-prompt on error:** the interface re-asks a field until the input is valid or the user types `q` to cancel the operation.

---

## 6. File-Handling Plan

### 6.1 File Summary

| File | Purpose | Format | Created By | Read By | Updated By |
|---|---|---|---|---|---|
| `data/equipment.txt` | Equipment master records | Text, one record per line | `saveEquipmentToFile` on first save | `loadEquipmentFromFile` at startup | Save on exit (rewritten completely) |
| `data/service_log.txt` | Service history | Text, one entry per line | `saveServiceLogToFile` on first save | `loadServiceLogFromFile` at startup | Save on exit (rewritten completely) |
| `data/report_<date>.txt` (optional) | Saved report | Text | Report generators with a file stream | User | Not updated |

A text format is used so the files are human-readable, easy to inspect in a demo, and easy to create in tests.

### 6.2 Record Format

Fields are separated by `|`. Lines beginning with `#` are comments. Blank lines are ignored. Dates are `DD-MM-YYYY`.

**`equipment.txt`** (9 fields)

```
# ID|Name|Type|Location|InstallDate|IntervalDays|LastServiceDate|RunningHours|UnderRepair
EQ-0001|CNC Lathe A|0|Shop Floor 1|15-03-2022|90|10-08-2025|4520.5|0
EQ-0002|Air Compressor|3|Utility Room|01-06-2021|30|05-09-2025|9800.0|0
```

**`service_log.txt`** (5 fields)

```
# ID|ServiceDate|Technician|Cost|Remarks
EQ-0001|10-08-2025|R. Patil|2500.00|Oil change and belt check
EQ-0002|05-09-2025|S. Khan|1200.50|Filter replaced
```

The delimiter `|` is forbidden inside text fields, so a line can always be split safely.

### 6.3 File Operations

| Operation | Function or Module | C Library Function | Failure Handling |
|---|---|---|---|
| Open for reading | `file_handler` load functions | `fopen(path, "r")` | NULL gives `STATUS_ERR_FILE_OPEN`; missing file at startup means start empty and show a notice |
| Read | `file_handler` load functions | `fgets` | Lines longer than the buffer count as format errors; `ferror` checked after the loop gives `STATUS_ERR_FILE_IO` |
| Parse | `parseEquipmentLine`, `parseServiceRecordLine` | `strchr`, `strncpy`, parse helpers | Wrong field count or invalid field gives `STATUS_ERR_FILE_FORMAT` |
| Open for writing | `file_handler` save functions | `fopen(tempPath, "w")` | NULL gives `STATUS_ERR_FILE_OPEN`; original file untouched |
| Write | `file_handler` save functions | `fprintf` | Negative return gives `STATUS_ERR_FILE_IO`; temporary file removed |
| Close | all file functions | `fclose` | Return value checked; non-zero gives `STATUS_ERR_FILE_IO` |
| Replace original | save functions | `rename(tempPath, path)` | Failure gives `STATUS_ERR_FILE_IO`; the user is told and the original remains |

Saving to a temporary file first means a failure halfway cannot destroy existing data.

### 6.4 File Error Conditions

| Error Condition | Detection Method | Expected Behaviour |
|---|---|---|
| File does not exist | `fopen` returns NULL on load | Start with empty data; "No existing data found, starting fresh" |
| File cannot be opened | `fopen` returns NULL on save | "Cannot write data file"; program stays open so the user can retry or exit |
| File is empty | No data lines read | `STATUS_SUCCESS` with 0 records |
| File contains invalid data | Line parser fails | Line skipped; at startup show "N invalid line(s) skipped" |
| Read or write fails | `ferror`, negative `fprintf`, `fclose` failure | `STATUS_ERR_FILE_IO`; message shown; data in memory is kept |
| File has more records than capacity | Count reaches `MAX_EQUIPMENT` | `STATUS_ERR_FULL`; remaining lines ignored; user warned |

---

## 7. Error-Handling Strategy

### 7.1 Principles

- Every function that can fail returns a `Status_t`. The caller **must** check it; return values are never ignored.
- Output parameters are written only on success.
- Functions that modify data are all-or-nothing: on error the data is unchanged.
- Only `main.c` and `report.c` print messages. Lower modules return codes. The text for a code comes from `getStatusMessage`.
- The program never calls `exit()` on a recoverable error. Fatal conditions end through the normal exit path so cleanup always runs.

### 7.2 Error Catalogue

| Error ID | Error Condition | Detection Location | Returned Status | User-Facing Behaviour |
|---|---|---|---|---|
| ERR-01 | NULL pointer argument | Start of every function | `STATUS_ERR_NULL_POINTER` | "Internal error" (indicates a programming defect) |
| ERR-02 | Invalid ID format | `validateEquipmentId` | `STATUS_ERR_INVALID_ID` | "Invalid ID, use format EQ-0001" |
| ERR-03 | Invalid date | `validateDate`, `parseDate` | `STATUS_ERR_INVALID_DATE` | "Invalid date, use DD-MM-YYYY" |
| ERR-04 | Value out of range or invalid text | `validate*` functions | `STATUS_ERR_INVALID_VALUE` | Field-specific range message |
| ERR-05 | Duplicate equipment ID | `addEquipment` | `STATUS_ERR_DUPLICATE` | "Equipment ID already exists" |
| ERR-06 | Equipment not found | `findEquipmentById` and callers | `STATUS_ERR_NOT_FOUND` | "No equipment with that ID" |
| ERR-07 | No equipment stored | `calculateFleetSummary` | `STATUS_ERR_EMPTY` | "No equipment records yet" |
| ERR-08 | Equipment list full | `addEquipment`, `loadEquipmentFromFile` | `STATUS_ERR_FULL` | "Equipment storage full (100 records)" |
| ERR-09 | Service log full | `addServiceRecord` | `STATUS_ERR_FULL` | "Service log full (500 records)" |
| ERR-10 | Too few records for average | `calculateAverageServiceInterval` | `STATUS_ERR_INSUFFICIENT_DATA` | "At least 2 service records needed" |
| ERR-11 | File cannot be opened | `file_handler` | `STATUS_ERR_FILE_OPEN` | Notice at startup; error on save |
| ERR-12 | File read/write failure | `file_handler` | `STATUS_ERR_FILE_IO` | "File operation failed, data kept in memory" |
| ERR-13 | Malformed file content | `parseEquipmentLine`, `parseServiceRecordLine` | `STATUS_ERR_FILE_FORMAT` | "N invalid line(s) skipped" |
| ERR-14 | Invalid menu choice or unparsable input | `validateMenuChoice`, `parseInteger` | `STATUS_ERR_INVALID_VALUE` | "Invalid choice, enter 0 to 9" |
| ERR-15 | Invalid index or argument | Functions with index or size parameters | `STATUS_ERR_INVALID_ARGUMENT` | "Internal error" |

### 7.3 Recovery Strategy

- **Bad user input:** show the message, re-prompt or return to the menu. The program continues.
- **Missing data file at startup:** start with empty collections.
- **Corrupt lines:** skip them, load the valid rest, warn the user.
- **Save failure on exit:** report the failure and ask whether to retry or quit without saving.
- **Full storage:** refuse new records, keep existing data safe.
- **Internal errors (NULL pointers):** print a message and return to the menu without modifying data.

---

## 8. Coding Standards Plan

### 8.1 Naming Conventions

| Item | Convention | Example |
|---|---|---|
| Source files | lowercase with underscores | `file_handler.c` |
| Header files | same name as source | `file_handler.h` |
| Functions | `camelCase`, starting with a verb | `findEquipmentById()` |
| Local variables and parameters | `camelCase` | `daysRemaining` |
| Structures and enumerations (types) | `PascalCase_t` | `Equipment_t` |
| Structure members | `camelCase` | `lastServiceDate` |
| Constants and macros | `UPPER_SNAKE_CASE` | `MAX_EQUIPMENT` |
| Enumeration constants | `UPPER_SNAKE_CASE` with a group prefix | `MAINT_OVERDUE`, `STATUS_ERR_FULL` |
| Header guards | `FILENAME_H` | `FILE_HANDLER_H` |

### 8.2 Formatting Rules

- Indentation: 4 spaces, no tabs
- Brace style: Allman (opening brace on its own line)
- Maximum line length: 100 characters
- One statement per line; spaces around operators; a blank line between logical blocks
- Compiler flags: `-Wall -Wextra -Wpedantic -std=c11`
- Comments: a short block comment above each function in the header (purpose, parameters, return values); inline comments only for non-obvious logic

### 8.3 Function Rules

- One responsibility per function; aim for under 40 lines
- Check every pointer argument at the start
- Validate inputs before using them
- Return `Status_t`; output through pointer parameters
- Mark read-only pointer parameters `const`
- Avoid global variables; state is passed by pointer
- Use `snprintf` and bounded copies, never `strcpy`, `sprintf`, `gets`

### 8.4 Prohibited Practices

- Application logic in `main.c`
- Ignoring return values or compiler warnings
- Magic numbers
- Duplicate logic
- Unused variables, functions or commented-out code
- Machine-specific absolute paths
- `scanf` for string input

---

## 9. Application Flow

### 9.1 Main Application Flowchart

```
            +------------------+
            |      START       |
            +--------+---------+
                     v
            +------------------+
            | Initialise lists |
            +--------+---------+
                     v
            +------------------+      file missing
            | Load data files  |-----> start empty, notify
            +--------+---------+
                     v
            +------------------+
    +------>|  Display menu    |
    |       +--------+---------+
    |                v
    |       +------------------+   no
    |       | Read and validate|-------> show error ---+
    |       | choice (0-9)     |                       |
    |       +--------+---------+                       |
    |                | yes                             |
    |                v                                 |
    |       +------------------+                       |
    |       | choice == 0 ?    |--- yes --> Save data  |
    |       +--------+---------+            |          |
    |                | no                    v          |
    |                v                 Save OK? --no--> offer retry
    |       +------------------+            | yes       |
    |       | Prompt and       |            v           |
    |       | validate inputs  |          EXIT          |
    |       +--------+---------+                        |
    |                v                                  |
    |       +------------------+                        |
    |       | Call module      |                        |
    |       | function         |                        |
    |       +--------+---------+                        |
    |                v                                  |
    |       +------------------+                        |
    |       | Show success or  |                        |
    |       | error message    |                        |
    |       +--------+---------+                        |
    |                |                                  |
    +----------------+----------------------------------+
```

### 9.2 Operation Flow: Add Equipment

```
Start
  v
Prompt ID --> validateEquipmentId --invalid--> show error, re-prompt
  v valid
Prompt name, type, location, dates, interval, hours
  (each validated; invalid input re-prompts that field only)
  v
Build Equipment_t
  v
addEquipment(list, item)
  |-- DUPLICATE --> "ID already exists"
  |-- FULL      --> "Storage full"
  |-- other err --> show message from getStatusMessage
  v SUCCESS
Mark data modified, show "Equipment added"
  v
Return to menu
```

### 9.3 Operation Flow: Determine Status

```
equipment, today
     v
underRepair == 1 ? --yes--> MAINT_UNDER_REPAIR
     | no
     v
nextDue = lastService + interval
days = nextDue - today
     v
days < 0 ? ---------yes--> MAINT_OVERDUE
     | no
     v
days <= DUE_SOON_DAYS ? --yes--> MAINT_DUE_SOON
     | no
     v
MAINT_OK
```

---

## 10. Unit-Test Plan

### 10.1 Functions Selected for Testing

| Function | Module | Reason for Testing |
|---|---|---|
| `validateEquipmentId` | validation | Core input rule with many edge cases |
| `validateDate`, `parseDate` | validation | Leap years and format handling |
| `validateServiceInterval`, `validateTextField`, `validateMenuChoice` | validation | Boundary-value rules |
| `parseInteger` | validation | Safe parsing of untrusted text |
| `isLeapYear`, `getDaysInMonth`, `addDaysToDate`, `compareDates` | date_utils | Calendar correctness |
| `calculateNextServiceDate`, `calculateDaysRemaining` | maintenance | Core calculation |
| `determineStatus` | maintenance | Key decision logic with boundaries |
| `getSortedByNextDue`, `filterByStatus` | maintenance | Sorting and filtering |
| `calculateFleetSummary`, `calculateAverageServiceInterval` | maintenance | Report calculations |
| `addEquipment`, `findEquipmentById`, `deleteEquipment`, `logServiceEvent` | equipment | Record insertion, search and deletion |
| `parseEquipmentLine`, `saveEquipmentToFile`, `loadEquipmentFromFile` | file_handler | Persistence and invalid-file handling |

Menu display and print functions are not unit-test targets (one report function writing to a temporary file is tested to confirm output).

### 10.2 Test Cases

Test file names: `tests/test_validation.cpp`, `tests/test_date_utils.cpp`, `tests/test_maintenance.cpp`, `tests/test_equipment.cpp`, `tests/test_file_handler.cpp`.

#### Validation (`test_validation.cpp`)

| Test ID | Function | Category | Input or Condition | Expected Result | Requirement |
|---|---|---|---|---|---|
| UT-01 | `validateEquipmentId` | Positive | `"EQ-0001"` | `STATUS_SUCCESS` | FR-02 |
| UT-02 | `validateEquipmentId` | Negative | `"eq-0001"`, `"XX-0001"`, `"EQ-00A1"` | `STATUS_ERR_INVALID_ID` | FR-02 |
| UT-03 | `validateEquipmentId` | Negative | `NULL` | `STATUS_ERR_NULL_POINTER` | FR-02 |
| UT-04 | `validateEquipmentId` | Boundary | `"EQ-123"` (too short), `"EQ-12345"` (too long), `"EQ-9999"` (max) | invalid, invalid, success | FR-02 |
| UT-05 | `validateServiceInterval` | Positive | 90 | `STATUS_SUCCESS` | FR-04 |
| UT-06 | `validateServiceInterval` | Negative | 0, -5 | `STATUS_ERR_INVALID_VALUE` | FR-04 |
| UT-07 | `validateServiceInterval` | Boundary | 1 and 365 accepted; 0 and 366 rejected | as stated | FR-04 |
| UT-08 | `validateDate` | Positive | 29-02-2024 (leap year) | `STATUS_SUCCESS` | FR-04 |
| UT-09 | `validateDate` | Negative | 29-02-2023, 31-04-2025, month 13 | `STATUS_ERR_INVALID_DATE` | FR-04 |
| UT-10 | `validateDate` | Boundary | 01-01-2000 and 31-12-2100 accepted; 31-12-1999 and 01-01-2101 rejected | as stated | FR-04 |
| UT-11 | `validateDate` | Negative | `NULL` | `STATUS_ERR_NULL_POINTER` | FR-04 |
| UT-12 | `parseDate` | Positive | `"15-08-2025"` | success; day 15, month 8, year 2025 | FR-04 |
| UT-13 | `parseDate` | Negative | `"15/08/2025"`, `"2025-08-15"`, `"abc"`, `""`, `"15-08-2025x"` | `STATUS_ERR_INVALID_DATE`, output unchanged | FR-20 |
| UT-14 | `parseInteger` | Negative | `"4x2"`, `""`, `"99999999999"` | `STATUS_ERR_INVALID_VALUE` | FR-20 |
| UT-15 | `validateTextField` | Boundary | 30-character text accepted, 31 rejected (max 30) | success, then `STATUS_ERR_INVALID_VALUE` | FR-01 |
| UT-16 | `validateTextField` | Negative | `""`, `"   "`, `"a\|b"` | `STATUS_ERR_INVALID_VALUE` | FR-01 |
| UT-17 | `validateMenuChoice` | Boundary | 0 and 9 accepted; -1 and 10 rejected | as stated | FR-20 |

#### Date utilities (`test_date_utils.cpp`)

| Test ID | Function | Category | Input or Condition | Expected Result | Requirement |
|---|---|---|---|---|---|
| UT-18 | `isLeapYear` | Positive/Boundary | 2024 and 2000 true; 1900 and 2023 false | as stated | FR-04 |
| UT-19 | `getDaysInMonth` | Boundary | February 2024 gives 29; February 2023 gives 28; month 13 gives 0 | as stated | FR-04 |
| UT-20 | `addDaysToDate` | Positive | 31-01-2025 plus 30 days | 02-03-2025 | FR-10 |
| UT-21 | `addDaysToDate` | Boundary | 31-12-2025 plus 1; 28-02-2024 plus 1 | 01-01-2026; 29-02-2024 | FR-10 |
| UT-22 | `addDaysToDate` | Negative | negative days, invalid start date, `NULL` | `INVALID_ARGUMENT`, `INVALID_DATE`, `NULL_POINTER` | FR-10 |
| UT-23 | `compareDates` | Positive | earlier, equal, later pairs | -1, 0, 1 | FR-13 |

#### Maintenance (`test_maintenance.cpp`)

| Test ID | Function | Category | Input or Condition | Expected Result | Requirement |
|---|---|---|---|---|---|
| UT-24 | `calculateNextServiceDate` | Positive | last service 01-01-2025, interval 90 | 01-04-2025 | FR-10 |
| UT-25 | `calculateDaysRemaining` | Positive | next due 10 days after today | 10 | FR-11 |
| UT-26 | `calculateDaysRemaining` | Negative-case value | next due 5 days before today | -5 | FR-11 |
| UT-27 | `determineStatus` | Boundary | days remaining 8 | `MAINT_OK` | FR-12 |
| UT-28 | `determineStatus` | Boundary | days remaining 7 and 0 | `MAINT_DUE_SOON` | FR-12 |
| UT-29 | `determineStatus` | Boundary | days remaining -1 | `MAINT_OVERDUE` | FR-12 |
| UT-30 | `determineStatus` | Positive | overdue machine with `underRepair = 1` | `MAINT_UNDER_REPAIR` | FR-12 |
| UT-31 | `determineStatus` | Negative | `NULL` equipment or today | `STATUS_ERR_NULL_POINTER` | FR-12 |
| UT-32 | `getSortedByNextDue` | Positive | 3 machines in random order | Output ordered earliest due first; source unchanged | FR-13 |
| UT-33 | `getSortedByNextDue` | Boundary | empty list | `STATUS_SUCCESS`, count 0 | FR-13 |
| UT-34 | `filterByStatus` | Positive | list with 2 overdue of 4 | 2 records, all overdue | FR-14 |
| UT-35 | `filterByStatus` | Negative-case value | no machine has requested status | `STATUS_SUCCESS`, count 0 | FR-14 |
| UT-36 | `calculateFleetSummary` | Positive | 4 machines, 1 overdue | counts correct; overduePercent 25.0 | FR-17 |
| UT-37 | `calculateFleetSummary` | Negative | empty list | `STATUS_ERR_EMPTY` | FR-17 |
| UT-38 | `calculateAverageServiceInterval` | Positive | services on 01-01, 31-01, 02-03-2025 for one ID | 30.0 | FR-17 |
| UT-39 | `calculateAverageServiceInterval` | Boundary | only one record | `STATUS_ERR_INSUFFICIENT_DATA` | FR-17 |

#### Equipment management (`test_equipment.cpp`)

| Test ID | Function | Category | Input or Condition | Expected Result | Requirement |
|---|---|---|---|---|---|
| UT-40 | `addEquipment` | Positive | valid record into empty list | success; count 1 | FR-01 |
| UT-41 | `addEquipment` | Negative | same ID added twice | `STATUS_ERR_DUPLICATE`; count stays 1 | FR-03 |
| UT-42 | `addEquipment` | Boundary | 100th record accepted; 101st rejected | success, then `STATUS_ERR_FULL` | FR-01 |
| UT-43 | `addEquipment` | Negative | record with invalid interval | `STATUS_ERR_INVALID_VALUE`; list unchanged | FR-04 |
| UT-44 | `findEquipmentById` | Positive | existing ID in list of 3 | success; correct index | FR-05 |
| UT-45 | `findEquipmentById` | Negative | ID not in list | `STATUS_ERR_NOT_FOUND` | FR-05 |
| UT-46 | `deleteEquipment` | Positive | delete middle of 3 | count 2; remaining order preserved | FR-08 |
| UT-47 | `deleteEquipment` | Negative | missing ID | `STATUS_ERR_NOT_FOUND`; list unchanged | FR-08 |
| UT-48 | `logServiceEvent` | Positive | service later than last service | log count +1; machine's last service date updated | FR-09 |
| UT-49 | `logServiceEvent` | Negative | unknown equipment ID | `STATUS_ERR_NOT_FOUND`; log unchanged | FR-09 |

#### File handling (`test_file_handler.cpp`)

| Test ID | Function | Category | Input or Condition | Expected Result | Requirement |
|---|---|---|---|---|---|
| UT-50 | `parseEquipmentLine` | Positive | valid 9-field line | success; fields match | FR-18 |
| UT-51 | `parseEquipmentLine` | Negative | 8 fields; text in a numeric field | `STATUS_ERR_FILE_FORMAT` | FR-19 |
| UT-52 | save then load | Positive | 3 records saved to a temp file, then loaded | loaded records equal saved | FR-18 |
| UT-53 | `loadEquipmentFromFile` | Negative | path does not exist | `STATUS_ERR_FILE_OPEN` | FR-19 |
| UT-54 | `loadEquipmentFromFile` | Boundary | empty file | `STATUS_SUCCESS`, count 0 | FR-19 |
| UT-55 | `loadEquipmentFromFile` | Negative | 3 lines, one malformed | `STATUS_ERR_FILE_FORMAT`; 2 loaded; skipped = 1 | FR-19 |

**Planned: 55 representative tests across 5 test files. Implemented: 243 tests** in the `tests/` folder (the minimum required is 10 tests on 3 functions). The implemented suite is larger because each function also has null-pointer and "data unchanged after an error" tests.

### 10.3 Test Data and Setup

- Test data is built inside each test using helper functions such as `makeEquipment(id, interval, lastService)`, so no test depends on a shared data file.
- "Today" is a fixed date (for example 15-06-2025) passed into each function, so results never change with the real date.
- File tests create temporary files in the working directory (for example `test_tmp_equipment.txt`) and remove them in a `TearDown()` method or at the end of the test, even after failures.
- Each test uses its own fresh lists (gTest fixtures), so tests are independent and can run in any order.

### 10.4 Expected Test Command

```
make test
```

---

## 11. Makefile Design

### 11.1 Targets

| Target | Purpose | Expected Output |
|---|---|---|
| `all` | Compile all sources to object files and link the application | `build/maintenance_tracker` |
| `run` | Build if needed, then run the application | Menu appears |
| `test` | Build the gTest executable and run it | gTest report; non-zero exit status on failure |
| `clean` | Delete the `build/` directory | Generated files removed |

### 11.2 Build Configuration

| Item | Planned Value |
|---|---|
| C compiler | `gcc` |
| C++ compiler | `g++` |
| C compiler flags | `-std=c11 -Wall -Wextra -Wpedantic -g` |
| C++ compiler flags | `-std=c++17 -Wall -Wextra -g` |
| Include paths | `-Iinclude` |
| gTest libraries | `-lgtest -lgtest_main -lpthread` |
| Application executable | `build/maintenance_tracker` |
| Test executable | `build/run_tests` |
| Object files | `build/*.o` (C sources compiled separately; test objects separate) |
| Dependency tracking | `-MMD -MP` so changing a header rebuilds only affected files |

The test executable links all application object files **except** `main.o`, so `main()` does not clash with gTest's own.

All C headers include `#ifdef __cplusplus extern "C" { ... } #endif` so the C++ tests can link to the C functions.

---

## 12. Repository Structure

```
factory-maintenance-tracker/
|
|-- src/
|   |-- main.c
|   |-- date_utils.c
|   |-- validation.c
|   |-- equipment.c
|   |-- maintenance.c
|   |-- file_handler.c
|   `-- report.c
|
|-- include/
|   |-- common.h
|   |-- date_utils.h
|   |-- validation.h
|   |-- equipment.h
|   |-- maintenance.h
|   |-- file_handler.h
|   `-- report.h
|
|-- tests/
|   |-- test_validation.cpp
|   |-- test_date_utils.cpp
|   |-- test_maintenance.cpp
|   |-- test_equipment.cpp
|   `-- test_file_handler.cpp
|
|-- data/
|   |-- equipment.txt
|   `-- service_log.txt
|
|-- docs/
|   |-- project_proposal.md
|   |-- detailed_design.md
|   `-- architecture.png
|
|-- Makefile
|-- README.md
|-- .gitignore
`-- LICENSE
```

---

## 13. Requirement Traceability

| Requirement ID | Module | Function or Feature | Test ID |
|---|---|---|---|
| FR-01 | equipment | `addEquipment` | UT-15, UT-16, UT-40, UT-42 |
| FR-02 | validation | `validateEquipmentId` | UT-01 to UT-04 |
| FR-03 | equipment | duplicate check in `addEquipment` | UT-41 |
| FR-04 | validation, date_utils | `validateDate`, `parseDate`, `validateServiceInterval`, `validateRunningHours` | UT-05 to UT-13, UT-18, UT-19, UT-43 |
| FR-05 | equipment | `findEquipmentById` | UT-44, UT-45 |
| FR-06 | report | `printEquipmentTable` | Demonstration |
| FR-07 | equipment | `updateEquipment` | Demonstration (and optional test) |
| FR-08 | equipment | `deleteEquipment` | UT-46, UT-47 |
| FR-09 | equipment | `logServiceEvent` | UT-48, UT-49 |
| FR-10 | maintenance, date_utils | `calculateNextServiceDate`, `addDaysToDate` | UT-20 to UT-22, UT-24 |
| FR-11 | maintenance | `calculateDaysRemaining` | UT-25, UT-26 |
| FR-12 | maintenance | `determineStatus` | UT-27 to UT-31 |
| FR-13 | maintenance | `getSortedByNextDue`, `compareDates` | UT-23, UT-32, UT-33 |
| FR-14 | maintenance | `filterByStatus` | UT-34, UT-35 |
| FR-15 | report | `generateDueReport` | Demonstration |
| FR-16 | report | `generateOverdueReport` | Demonstration |
| FR-17 | maintenance, report | `calculateFleetSummary`, `calculateAverageServiceInterval`, `generateFleetSummaryReport` | UT-36 to UT-39 |
| FR-18 | file_handler | save and load functions, `parseEquipmentLine` | UT-50, UT-52 |
| FR-19 | file_handler | error handling in load functions | UT-51, UT-53 to UT-55 |
| FR-20 | main, validation | `validateMenuChoice`, `parseInteger`, `parseDate` | UT-13, UT-14, UT-17 |

---

## 14. Git and GitHub Plan

### 14.1 Repository Information

| Field | Value |
|---|---|
| Repository name | factory-maintenance-tracker |
| Visibility | Public |
| Main branch | main |
| GitHub link | https://github.com/janakrahudkar008-hash/factory-maintenance-tracker |

### 14.2 Planned Commit Milestones

| Milestone | Planned Commit Message | Status |
|---|---|---|
| Repository creation | Create initial project structure | Complete |
| Proposal | Add project proposal | Complete |
| Detailed design | Add detailed software design document | In progress |
| Common definitions | Add common status codes and constants | Planned |
| Date module | Implement date utilities | Planned |
| Validation | Implement input validation module | Planned |
| Equipment data model | Add equipment data model and status enumeration | Planned |
| Equipment operations | Implement equipment record management | Planned |
| Maintenance logic | Implement maintenance status calculation | Planned |
| Sorting and filtering | Add sorting and filtering by status | Planned |
| File handling | Add file-storage module | Planned |
| Reports | Add report generation module | Planned |
| Main program | Implement menu-driven main application | Planned |
| Unit tests | Add validation, date and maintenance unit tests | Planned |
| More tests | Add equipment and file-handler unit tests | Planned |
| Makefile | Add Makefile with all, run, test and clean targets | Planned |
| Sample data | Add sample data files | Planned |
| Documentation | Write README with build and run steps | Planned |
| Final check | Fix issues found during clean-clone verification | Planned |

### 14.3 Files to Ignore

```
build/
*.o
*.d
*.out
*.exe
*.log
core
core.*
test_tmp_*
```
