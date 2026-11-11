# Project Proposal: Factory Equipment Preventive Maintenance Tracker

## 1. Project Information

### 1.1 Project Title

Factory Equipment Preventive Maintenance Tracker: A Command-Line Application in C

### 1.2 Engineering Domain

Manufacturing (Equipment Maintenance)

### 1.3 Student Details

| Field | Details |
|---|---|
| Student name | Janak Sharad Rahudkar |
| Roll number | 612509021 |
| Division / batch | 1 |
| Repository name | factory-maintenance-tracker |
| GitHub repository link | https://github.com/janakrahudkar008-hash/factory-maintenance-tracker |

### 1.4 Individual Project Declaration

I confirm that this project is my individual work. I understand the design, source code, unit tests, Makefile and documentation included in this repository, and I am prepared to explain or modify any part during evaluation.

Acknowledgement: Janak Sharad Rahudkar

---

## 2. Engineering Scenario

### 2.1 Scenario Description

**Where it occurs:** In small and medium manufacturing workshops and factory shop floors that operate machines such as lathes, drilling machines, milling machines, compressors, conveyors and generators.

**Who is involved:** Maintenance supervisors, maintenance technicians and the plant manager.

**Current working method:** Service dates and machine details are written in paper registers or scattered Excel sheets. Supervisors check them by hand to decide which machine needs servicing.

**Information that must be managed:**
- Machine identity (ID, name, type, location)
- Installation date
- Recommended service interval in days
- Last service date and running hours
- Current operating status
- History of service events (date, technician, cost, remarks)

**Problems in the current method:**
- Servicing is missed because nobody notices an approaching due date.
- Overdue machines are discovered only after a breakdown.
- Records contain typing errors, duplicate machine IDs and impossible dates.
- Producing a summary of machine health takes manual effort.

### 2.2 Problem Statement

In manufacturing workshops, maintenance supervisors currently face difficulty tracking service schedules for many machines. The current paper and spreadsheet method results in missed services, unexpected breakdowns, inconsistent records and slow report preparation. The proposed software will provide validated record management, automatic service-due calculation, status classification and maintenance reports so that supervisors can plan preventive maintenance on time and reduce unplanned downtime.

### 2.3 Proposed Solution

A menu-driven command-line application written in C that manages equipment maintenance records.

**What the application will do:**
- Store equipment records and service history in text files.
- Validate every input (IDs, dates, intervals, hours, menu choices).
- Calculate the next service date and the days remaining for each machine.
- Decide a maintenance status (`OK`, `DUE_SOON`, `OVERDUE`, `UNDER_REPAIR`) automatically.
- Support searching, sorting by next due date and filtering by status.

**Calculations and decisions:**
- Next service date = last service date + service interval.
- Days remaining = next service date - today's date.
- Status is determined from the days remaining and the repair flag.
- Average days between services, and percentage of equipment overdue.

**Outputs and reports:**
- Maintenance Due Report (equipment due within a chosen number of days)
- Overdue Equipment Report
- Fleet Health Summary (counts and percentages per status)
- Service History Report for a single machine

### 2.4 Target Users

| User Type | Responsibility | How the User Will Use the Application |
|---|---|---|
| Maintenance supervisor | Plans and monitors preventive maintenance | Views due and overdue reports, filters by status, sorts by next due date |
| Maintenance technician | Performs servicing | Logs service events and updates the last service date and running hours |
| Plant manager | Oversees equipment reliability | Reads the fleet health summary |
| Data entry operator | Maintains equipment master data | Adds, updates and deletes equipment records |

---

## 3. Project Scope

### 3.1 In-Scope Features

- Add, view, update and delete equipment records
- Search equipment by ID
- Log service events against equipment
- Automatic next-service-date and days-remaining calculation
- Automatic status determination
- Sort equipment by next due date
- Filter equipment by status
- Generate due, overdue, fleet-summary and service-history reports
- Save and load data using text files
- Input validation and error handling for all user and file input

### 3.2 Out-of-Scope Features

- Graphical user interface or web interface
- Multi-user login and access control
- Database integration
- Email or SMS reminders
- Spare-parts inventory and cost budgeting
- Real-time sensor or IoT integration

### 3.3 Assumptions

- The program is used by one person at a time on a single computer.
- Dates are entered in the format `DD-MM-YYYY`.
- The system date is used as "today", but "today" can also be passed as a parameter so calculations stay testable.
- Equipment IDs are unique and follow a fixed format.
- Data files are located in the `data/` folder relative to the program.

### 3.4 Constraints

- Command-line interface only
- File-based storage (text format)
- Fixed maximum of 100 equipment records and 500 service log entries
- Application in C; unit tests in C++ using Google Test
- Individual project with limited development time

---

## 4. Requirements

### 4.1 Functional Requirements

| ID | Functional Requirement | Priority | Verification Method |
|---|---|---|---|
| FR-01 | The system shall add a new equipment record. | High | Test / Demonstration |
| FR-02 | The system shall validate the equipment identifier format. | High | Test |
| FR-03 | The system shall reject an equipment record whose identifier already exists. | High | Test |
| FR-04 | The system shall validate dates, service intervals and running hours. | High | Test |
| FR-05 | The system shall search for equipment using its identifier. | High | Test |
| FR-06 | The system shall display all equipment records. | Medium | Demonstration |
| FR-07 | The system shall update an existing equipment record. | Medium | Test / Demonstration |
| FR-08 | The system shall delete an existing equipment record. | Medium | Test |
| FR-09 | The system shall record a service event and update the equipment's last service date. | High | Test |
| FR-10 | The system shall calculate the next service date for each equipment. | High | Test |
| FR-11 | The system shall calculate the days remaining until the next service. | High | Test |
| FR-12 | The system shall determine the maintenance status of each equipment. | High | Test |
| FR-13 | The system shall sort equipment by next service date. | Medium | Test |
| FR-14 | The system shall filter equipment by maintenance status. | Medium | Test |
| FR-15 | The system shall generate a maintenance due report for a chosen number of days. | High | Demonstration |
| FR-16 | The system shall generate an overdue equipment report. | Medium | Demonstration |
| FR-17 | The system shall generate a fleet health summary with counts and percentages. | Medium | Test / Demonstration |
| FR-18 | The system shall save all records to files and load them at startup. | High | Test / Demonstration |
| FR-19 | The system shall report an error without crashing when a file is missing, unreadable or contains invalid data. | High | Test / Demonstration |
| FR-20 | The system shall reject unsupported menu choices and invalid input, and continue running. | High | Demonstration |

### 4.2 Non-Functional Requirements

| ID | Non-Functional Requirement | Verification Method |
|---|---|---|
| NFR-01 | The source code shall compile without warnings using `-Wall -Wextra -Wpedantic`. | Build output |
| NFR-02 | The project shall use a modular multi-file structure with separate headers. | Code review |
| NFR-03 | The application shall handle invalid input without crashing. | Negative testing |
| NFR-04 | Important functions shall be tested using Google Test. | Unit-test output |
| NFR-05 | The project shall build, run, test and clean using a Makefile. | Build demonstration |
| NFR-06 | The project shall follow a consistent naming and formatting standard. | Code review |
| NFR-07 | The project shall avoid magic numbers by using named constants. | Code review |
| NFR-08 | The project shall be maintained in Git with meaningful commits and published on GitHub. | Repository review |
| NFR-09 | The project shall include a README that lets another developer build and run it without help. | Clean-clone verification |

---

## 5. High-Level Architecture

### 5.1 Architecture Description

The software uses a layered modular design:

- **Interface layer (`main.c`):** shows the menu, reads the user's choice and controls the workflow. It contains no business logic.
- **Validation layer (`validation.c`):** checks all inputs before they are used.
- **Data-management layer (`equipment.c`):** owns the equipment and service-record collections and performs add, search, update and delete.
- **Business-logic layer (`maintenance.c`):** date arithmetic, next-due calculation, status determination, sorting and filtering.
- **File-storage layer (`file_handler.c`):** loads and saves records to text files.
- **Report layer (`report.c`):** produces the due, overdue, summary and history reports.

Each layer only calls the layers below it. Functions return status codes (an enumeration) so that errors flow back to `main.c`, which displays a clear message.

### 5.2 Architecture Diagram

```
                         +------------------+
                         |       USER       |
                         +--------+---------+
                                  |
                                  v
                  +-------------------------------+
                  |   APPLICATION INTERFACE       |
                  |   main.c  (menu + control)    |
                  +---------------+---------------+
                                  |
        +------------+------------+------------+-------------+
        |            |                         |             |
        v            v                         v             v
 +-------------+ +-------------+      +---------------+ +-------------+
 | validation  | | equipment   |      | maintenance   | |   report    |
 | (input      | | (records:   |      | (date math,   | | (due, over- |
 |  checks)    | |  add/find/  |      |  status, sort,| |  due, fleet |
 |             | |  update/del)|      |  filter)      | |  summary)   |
 +-------------+ +------+------+      +-------+-------+ +------+------+
                        |                     |                |
                        +----------+----------+----------------+
                                   |   DATA PROCESSING
                                   v
                        +----------------------+
                        |    file_handler      |
                        |  (load / save)       |
                        +----------+-----------+
                                   |
                                   v
                        +----------------------+
                        |    FILE STORAGE      |
                        |  data/equipment.txt  |
                        |  data/service_log.txt|
                        +----------------------+

   Reports and messages are printed to the screen (and may be saved to a
   report text file).
```

### 5.3 High-Level Workflow

```
START
  |
  v
Initialise application
  |
  v
Load equipment.txt and service_log.txt
  |   (if missing: start with empty data and inform the user)
  v
+--> Display main menu
|      |
|      v
|    Read user choice --> invalid? --> show error ---------+
|      |                                                    |
|      v                                                    |
|    Valid choice                                           |
|      |                                                    |
|      v                                                    |
|    Ask for required inputs and validate them              |
|      |                                                    |
|      v                                                    |
|    Call the matching module function                      |
|      |                                                    |
|      v                                                    |
|    Display success message, result or error status        |
|      |                                                    |
|      v                                                    |
|    Data changed? --> yes --> mark data as modified        |
|      |                                                    |
+------+----------------------------------------------------+
       |
  choice = Save & Exit
       v
Save all data to files
       |
       v
Cleanup and EXIT
```

**Main menu (planned):**

```
1. Add equipment            6. Log service event
2. View all equipment       7. Sort by next due date
3. Search by ID             8. Filter by status
4. Update equipment         9. Reports
5. Delete equipment         0. Save and exit
```

---

## 6. Module Design (Initial)

### 6.1 Module Summary

| Module Name | Source File | Header File | Responsibility |
|---|---|---|---|
| Main application | `main.c` | Not applicable | Initialise, load data, show menu, call modules, save and clean up |
| Equipment management | `equipment.c` | `equipment.h` | Defines equipment and service-record structures; add, find, update, delete |
| Validation | `validation.c` | `validation.h` | Validates IDs, dates, numbers, strings and menu choices |
| Maintenance logic | `maintenance.c` | `maintenance.h` | Date arithmetic, next-due date, days remaining, status, sorting, filtering |
| File handling | `file_handler.c` | `file_handler.h` | Loads and saves equipment and service-log files with error checks |
| Reports | `report.c` | `report.h` | Generates due, overdue, fleet-summary and history reports |

### 6.2 Planned Dependencies

| Module | Depends On |
|---|---|
| `main.c` | all modules |
| `equipment` | `validation` |
| `maintenance` | `equipment` |
| `file_handler` | `equipment`, `validation` |
| `report` | `equipment`, `maintenance` |
| `validation` | none (standalone, easiest to unit test) |

Detailed function specifications will be written in `docs/detailed_design.md` (Phase 2).

---

## 7. Planned C Concepts

| C Concept | Planned Use | Why It Is Needed |
|---|---|---|
| Functions | Every operation in its own function with clear parameters and return values | Single responsibility, testable code |
| Structures | `Equipment_t`, `ServiceRecord_t`, `Date_t` | Group related data of one machine or event |
| Enumerations | `MaintenanceStatus_t`, `Status_t` (error codes) | Named states and consistent error reporting |
| Arrays | Fixed-size arrays of equipment and service records | Simple, predictable storage within the 100 / 500 record limit |
| Strings | Names, locations, IDs, technician names | Text fields with fixed maximum lengths |
| Pointers | Passing structures and arrays to functions, output parameters | Efficient access and returning results |
| Constants (`#define` / `const`) | Max records, string sizes, interval limits, due-soon threshold | Avoid magic numbers |
| File handling | `fopen`, `fclose`, `fgets`, `fprintf`, `sscanf` | Persistent storage in text files |
| Error / status codes | Returned by all functions that can fail | Consistent error handling, no ignored failures |
| Header guards and `extern "C"` | All headers | Prevent double inclusion; allow C++ gTest files to link |
| Dynamic memory | Not planned | Fixed arrays are sufficient and safer for this scope |
