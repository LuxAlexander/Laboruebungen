# PROJECT: Ticketing (Soccer)

Software Design Lab, FH Salzburg. Team of two (Susanne Lastowicka, Alexander Lux).
Sources: `doc/lab_instructions/SWD-Projekt-Ticketing.pdf` (assignment, 3 Sep 2025) and `doc/lab_instructions/SWD-Project-Ticketing.docx` (our project report, Nov 2026).

## 1. Goal

A server-centric ticket shop for FC Red Bull Salzburg (32,000-seat, all-seater stadium, now also Champions League).
One central server manages stadiums, matches, seats, tickets, prices and statistics.
Independent clients (web shop, POS, kiosk, app, entry gate) talk to it.
Prototype: one stadium. Later: all Austrian Bundesliga stadiums, so capacity must scale to the largest one (the report sets an upper limit of 100,000 seats).

## 2. Scope: our two use cases

| UC | Name | Core behaviour |
|----|------|----------------|
| 4.1 | Single-Match Ticket Sale | Pick upcoming match, sector and available seat; see price; pay (cash only at staffed counters, or credit card); seat becomes `sold`. Max. 4 tickets per person. No double-selling (concurrency control). Anonymous sale allowed at the counter. |
| 4.2 | Access Control | Entry by ticket number. Granted only if the match is `active`, the ticket is valid for that match (single) or its competition/series (season), and not already inside. Log every entry, leave and denied attempt. Leave then re-entry with the same number is allowed. A repeated or double entry attempt notifies staff. |

Not ours: UC 4.3 Season Ticket Sale (team of 3 only). Season tickets come as seed data.

### Out of scope (per our report)
- Season ticket sale, ticket resale, premium tickets (EUR 200-500), stadiums over 100,000 seats
- Admin/CRUD UI for master data (use seed data, fixtures or migrations)
- Real payment provider or real gate hardware (payment is simulated, gate is an HTML page)
- Everything marked Team(3) in the assignment PDF: season ticket sale (4.3), explicit customer types (regular, reduced-price, fan club) and purchase rules based on them (half price, fan sector C restriction). Pricing depends only on sector and competition type.

## 3. Domain data

**Stadium sections**

| Section | Capacity | Price category |
|---------|----------|----------------|
| A, B | 8,000 each | Category 1 |
| C | 4,000 | Fan sector (FC ITS Fan Club members only) |
| D | 4,000 | Category 2 |
| E, F, G, H | 2,000 each | Category 3 |

Total: 32,000.

**Master data:** stadium (name, address), sectors with capacity, competition/series (Bundesliga, Cup, Europa League, Champions League, friendly; recurs per season), match (kickoff, stadium, series, exactly 2 teams).

**Transactional data:**
- Ticket: single-match or season, bound to exactly one fixed seat.
- Price: determined by sector and competition type.
- Match status: `planned` -> `active` -> `conducted` (or `cancelled`). Entry is allowed only when `active`.
- Entry/leave log (`TicketUsage`).
- Statistics per conducted match: tickets sold, tickets used, occupancy %. Series statistics are updated only after `conducted`.

**Business rules**
- Max. 4 tickets per person.
- Cash only at staffed stalls.
- Not enforced by us (Team(3) only): reduced price (50%) for children under 16 and persons with disabilities; fan sector C only for FC ITS Fan Club members.
- Season tickets require a customer account.
- A seat is available only if there is no single-match ticket for that match and no season ticket for that series.

## 4. Quality requirements (architecture drivers)

| ID | Attribute | Requirement |
|----|-----------|-------------|
| QR-01 | Integrity / concurrency | A seat is never sold or assigned twice, even under simultaneous requests. |
| QR-02 | Performance | Seat queries and entry checks respond in about 2 s (entry decision about 1 s). |
| QR-03 | Availability | Server stays up during sales peaks and stadium fill. |
| QR-04 | Scalability / extensibility | More stadiums, clients and competitions without major changes. |
| QR-05 | Security / privacy | GDPR; payment data protected; only authorised staff can cancel. |
| QR-06 | Traceability | Every entry, leave and denied attempt is stored permanently. |
| QR-07 | Usability | Simple purchase flow showing sectors, seats and prices clearly. |
| QR-08 | Testability | Unit tests for critical paths (isolation, AAA, mocks, boundary/error cases). |

## 5. Design decisions already made (report)

- **Components:** Data Management, Core, Customer, Ticket, Orders, Security Alert. External: Payments, Gate.
- **Concurrency for access control:**
  - One transaction per entry check.
  - Pessimistic row lock on `Ticket` (`SELECT ... FOR UPDATE`).
  - Safety net: `@Version` optimistic lock plus a unique constraint "max. one open ENTRY per ticket and match".
  - Denied attempts are logged in a separate transaction (`REQUIRES_NEW`).
  - Statistics are updated atomically (`used = used + 1`), only for a ticket's first entry.
  - Fixed lock order: Match -> Ticket -> TicketUsage -> Statistics.
- **Pricing:** by sector and competition type.

## 6. Deliverables (submit via Moodle, all names in every file)

1. Implementation: source code plus minimal setup/run instructions.
2. Concise documentation: architecture, data model, external API, enough for another team to run and test the system.
3. Screencast of about 5 min per use case showing the app working and explaining one important code fragment.
4. One consolidated project report, always submitted as **PDF**.

Required functional outputs:
- Student-facing web app (availability search and booking).
- Lightweight HTML page simulating the entry gate (enter a ticket number, request grant/deny, show status).
- Automated unit tests, documented how to run them.

## 7. Open questions / inconsistencies to resolve

- ~~Customer types vs. UC 4.1 alternative flows 4.1.1 / 4.1.2.~~ **Resolved:** customer types and the purchase rules based on them are marked Team(3) in the assignment PDF, so we (team of 2) skip them. Remove alternative flows 4.1.1 (fan club check for sector C) and 4.1.2 (half price) from the UC 4.1 description in the report.
- Entry-decision time is about 2 s in QR-02 but about 1 s in UC 4.2. Pick one target.
- The report describes the concurrency design with Java/JPA terms (`@Version`, `REQUIRES_NEW`). This repo is Django, so map to `select_for_update()`, a version field, `transaction.atomic` and DB constraints.
- Reservation and cancellation (FR-13) and period statistics (FR-16) are listed as requirements but are not part of either use case. Decide whether they are in scope.

## 8. Task tracker

Status values: `TODO` | `IN PROGRESS` | `DONE` | `BLOCKED` | `DROPPED`. Edit the Status column as work proceeds.
Design items are marked DONE based on the sections present in the report (Nov 2026). Implementation items are TODO until checked against the code.

### Documentation and design

| # | Task | Status | Notes |
|---|------|--------|-------|
| D1 | Project setting, goals, non-goals, stakeholders | DONE | Report ch. 1 |
| D2 | Context diagram | DONE | |
| D3 | Functional and non-functional requirements (FR-01..17, QR-01..08) | DONE | |
| D4 | Use case overview diagram | DONE | |
| D5 | UC 4.1 description and activity diagrams | DONE | |
| D6 | UC 4.2 description and activity diagram | DONE | |
| D7 | Component diagram | DONE | |
| D8 | Class diagrams (master, services, enums) | DONE | |
| D9 | Sequence diagrams (access control) and concurrency measures | DONE | |
| D10 | User stories | DONE | |
| D11 | Design patterns write-up | DONE | Separate PDF in `Soccer_Ticketing/` |
| D12 | Resolve the open questions in section 7; remove flows 4.1.1 / 4.1.2 from UC 4.1 in the report | TODO | |
| D13 | Final consolidated report as PDF, names on cover | TODO | |

### Implementation: master data

| # | Task | Status | Notes |
|---|------|--------|-------|
| M1 | Models: Stadium, Sector, Seat | TODO | Sector capacities as in section 3 |
| M2 | Models: Team, Competition/Series, Match (status enum) | TODO | Exactly 2 teams per match |
| M3 | Models: Ticket (single/season, fixed seat), Customer | TODO | |
| M4 | Models: TicketUsage (entry/leave log), Statistics | TODO | |
| M5 | Seed data / fixtures / migrations (32,000 seats, matches, season tickets) | TODO | No admin UI needed |

### Implementation: UC 4.1 Single-Match Ticket Sale

| # | Task | Status | Notes |
|---|------|--------|-------|
| S1 | List upcoming matches | TODO | |
| S2 | Seat availability per match/sector (excludes sold single and season seats) | TODO | FR-03, FR-14 |
| S3 | Price calculation by sector and competition type | TODO | FR-04 |
| S4 | Purchase service with concurrency control (no double sale) | TODO | QR-01, FR-05 |
| S5 | Max. 4 tickets per person | TODO | FR-06 |
| S6 | Payment simulation (cash at counter, credit card), rollback on failure | TODO | FR-07 |
| S7 | Anonymous sale at counter | TODO | |
| S8 | ~~Fan sector C membership check / half price~~ | DROPPED | Team(3) only, not in our scope |
| S9 | Statistics update on sale | TODO | |
| S10 | Customer-facing web UI for the purchase flow | TODO | QR-07 |

### Implementation: UC 4.2 Access Control

| # | Task | Status | Notes |
|---|------|--------|-------|
| A1 | Entry check service (match active, ticket valid, not already inside) | TODO | FR-08..10 |
| A2 | Season ticket validation against the competition/series | TODO | |
| A3 | Entry/leave logging; re-entry after leave | TODO | FR-11 |
| A4 | Atomic transaction with row lock, version and unique constraint | TODO | Section 5 |
| A5 | Denied-attempt logging in a separate transaction | TODO | QR-06 |
| A6 | Double-entry / repeated-attempt notification to staff | TODO | FR-12 |
| A7 | Match statistics update (used, occupancy) | TODO | FR-15 |
| A8 | HTML gate simulation page | TODO | Enter number -> grant/deny, show status |
| A9 | Error handling: unknown ticket, no active match, wrong match, server error | TODO | Exceptions 4.2.0.E.1-E.5, 4.2.1.E.1 |

### Testing

| # | Task | Status | Notes |
|---|------|--------|-------|
| T1 | Unit tests UC 4.1: purchase, limit of 4, price, boundary/error cases | TODO | |
| T2 | Concurrency test: two buyers, same seat | TODO | |
| T3 | Unit tests UC 4.2: valid/invalid ticket, double entry, leave/re-entry | TODO | |
| T4 | Concurrency test: same ticket at two gates | TODO | |
| T5 | Document how to run the tests | TODO | |

### Submission

| # | Task | Status | Notes |
|---|------|--------|-------|
| X1 | Setup/run instructions (README) | TODO | |
| X2 | Architecture, data model and external API documentation | TODO | |
| X3 | Screencast UC 4.1 (about 5 min) | TODO | Include a code fragment explanation |
| X4 | Screencast UC 4.2 (about 5 min) | TODO | |
| X5 | Upload all to Moodle (names in every file) | TODO | |
