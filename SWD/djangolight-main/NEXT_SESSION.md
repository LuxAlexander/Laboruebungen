# Next session: handoff

Status at the end of the session on 2026-11-12, when work was stopped mid-way on the basic structure.
Everything below is **uncommitted** on `main`.

## 1. Done in this session

The shop starter has been converted into the ticketing structure. See `PROJECT_STRUCTURE.md` for the original
layout and `PROJECT.md` for requirements and the task table.

Decisions taken with the user:
- Customer types (fan club, half price) are Team(3) only and are **not** implemented.
- Anonymous online purchase is limited to **1 ticket**. More tickets need login. Staff users act as the ticket
  counter (anonymous sale, max. 4, cash allowed).
- Ticket numbers are random strings like `TCK-7F3K9Q`.
- Statistics: all of them (per match, per competition, period utilisation).
- Work happens directly on `main`.

### Removed
- `products` app (whole folder)
- old migrations of `customers` and `orders`
- `core/managers.py`
- old tests: `orders/test_basket.py`, `orders/test_service.py`, `orders/test_ui.py`,
  `customers/test_service.py`, `customers/test_concurrency.py`
- old templates `basket_overview.html` and `order_list.html` (both were then rewritten)
- `db.sqlite3`, which was recreated

### New or rewritten

| App | Files | Content |
|---|---|---|
| `core` | `enums.py` (new), `domain.py`, `interfaces.py`, `models.py`, `index_view.py`, `urls.py`, `tests.py`, `templates/core/base.html` | Enums (MatchStatus, CompetitionType, PriceCategory, TicketType/Status, SalesChannel, PaymentMethod, AccessAction, DenyReason); frozen records; Protocols `MatchCatalog`, `Tickets`, `Customers`, `Payments`, `Statistics`, `SecurityAlerts`; error types; `normalize_ticket_number`; nav and branding |
| `stadiums` (new) | `models.py`, `managers.py`, `services.py` (MatchCatalogService), `statistics.py` (StatisticsService), `seeding.py`, `data/stadium.json`, `management/commands/seed_stadium.py`, `admin.py`, `views.py`, `urls.py`, `templates/stadiums/statistics.html` | Stadium, Sector, Seat (32,000 rows, 50 per row), Gate, Team, Competition, Match (constraints: two different teams, one active match per stadium), MatchStatistics, CompetitionStatistics. Admin actions: activate / conduct / cancel. Staff statistics page `/stadium/statistics/` |
| `tickets` (new) | `models.py`, `managers.py`, `services.py` (TicketService), `seeding.py`, `data/tickets.json`, `management/commands/seed_tickets.py`, `admin.py` | PriceRule (category x competition type), Ticket (check constraint single/season, conditional unique constraints per seat). Seed: prices plus 20 season tickets for users `season01`..`season05` (password `season123`), sector B row 1 |
| `customers` | `models.py` (credit removed), `services.py`, `admin.py`, `views.py`, `urls.py`, `templates/customers/login.html`, `register.html` | Login and registration at `/accounts/login/` and `/accounts/register/` |
| `orders` | `models.py`, `managers.py`, `basket.py`, `payments.py` (new), `services.py`, `views.py`, `urls.py`, 5 templates | UC 4.1. Basket = seats of one match (max. 4). `OrderService.purchase` locks the seats, re-checks availability, issues tickets, pays (simulated), writes the order and statistics, all in one transaction. URLs under `/tickets/` |
| `access` (new) | `models.py`, `services.py`, `views.py`, `urls.py`, `admin.py`, `templates/access/gate.html`, `alerts.html` | UC 4.2. TicketUsage (unique open entry per ticket and match), AccessLog, SecurityAlert. `AccessService.enter/leave`; denied scans are logged outside the transaction. Gate simulation at `/gate/`, staff alerts at `/gate/alerts/` |
| project | `settings.py` (apps, `LOGIN_URL`), `containers.py`, `apps.py` (wiring), `urls.py`, `pyproject.toml`, `justfile` (`just seed` runs both seeds), `static/css/application.css` (`.seat-btn`) | |

### Verified
- `manage.py check`: no issues.
- `makemigrations` created a fresh `0001_initial` for every app (`access` also got a `0002`).
- `migrate`, `seed_stadium` and `seed_tickets` ran successfully, and a second run creates no duplicates.

## 2. NOT done or NOT verified yet

### Must do first
1. **Run the test suite.** It was not run. `swd_django_demo/test_dependency.py` still asks for `product_service`
   and will fail; rewrite it for `match_catalog`, `ticket_service`, `order_service`, `access_service`.
2. **Click through the app.** No view has been opened in a browser yet:
   - matches, then match detail, then seat selection, basket, payment and confirmation
   - "My tickets", login and register
   - gate enter, leave and re-entry, and the double-entry alert
   - the statistics page
   - the admin actions

   Template errors are possible.
3. **uv is not on the PATH** in this environment. I used `.venv/Scripts/python.exe manage.py ...` instead of
   `uv run` / `just`. Check whether `just`/`uv` work in your own terminal.
4. **Commit** once the tests are green.

### Tests to write (PROJECT.md T1–T5)
- `stadiums`: seeding (use a small config dict), statistics (`record_sale`, `record_entry`, `conduct_match`
  adds to competition, `utilisation` periods).
- `tickets`: `taken_seat_ids` (single plus season), price, `issue_single_tickets` raises `SeatNotAvailable`,
  ticket number format.
- `orders`: basket (pure unit test, max. 4, switching match resets), payments (cash online fails, Luhn, declined
  card), purchase happy path, anonymous online >1 rejected, 4-per-person limit across orders, rollback on payment
  failure (no tickets left behind), view tests with the Django client.
- `access`: granted entry, unknown ticket, no active match, wrong match, season ticket valid for its competition,
  double entry gives an alert with an increasing `attempt_count`, leave then re-entry (counted once in the
  statistics), leave without entry, denied scans are still logged.
- Protocol conformance test per adapter (`isinstance(service, Protocol)`).
- Concurrency (threads, `TransactionTestCase`): two buyers for the same seat, and the same ticket at two gates.
  Only meaningful on MariaDB via `just test-mariadb`.
- Document how to run the tests (README).

### Known weak spots to review
- `StatisticsService.record_sale/record_entry` use `get_or_create` followed by an `F()` update. Under heavy
  concurrency, `get_or_create` itself can race. Better: create the `MatchStatistics` row when the match is created
  (seed or signal) and only do the update.
- Payment is simulated **inside** the purchase transaction. That is fine for a simulation; with a real provider it
  would need a reservation step.
- `stadiums/admin.py` gets `statistics_service` from `swd_django_demo.get_container()` inside the action.
- `stadiums/views.py` reads the `Competition` model directly for the dropdown (its own app, so OK).
- `tickets/seeding.py` imports `stadiums` models and `StatisticsService` (seeding only, acceptable).
- Season tickets count into `tickets_sold` of every open match of the competition (done in the seed only).
- `MatchManager.upcoming()` still shows a match until 3 h after kickoff.
- The gate page has no authentication (it simulates a device).
- `lock_ticket` locks only the ticket row (the report also mentions a version field; not added).

### Not started
- Reservation by season-ticket holders and cancellation by customer/staff (FR-13). The `TicketStatus.RESERVED` and
  `CANCELLED` values exist, but there are no services or views.
- REST/external API documentation (the deliverable asks for "external API" docs). `djangorestframework` is
  installed but unused.
- Update docs: `README.md`, `CONTEXT.md` (still the shop glossary), `PROJECT_OVERVIEW.md` (shop diagrams),
  `PROJECT_STRUCTURE.md` (describes the old layout).
- `containers/initial_data.sql` still holds the old shop schema. Regenerate it (see its header), or `just compose`
  will not match the new models.
- Update the statuses in the `PROJECT.md` task table (M1–M5, S1–S10, A1–A9 are now implemented but untested).
- Open questions still in `PROJECT.md` section 7: entry response-time target (1 s vs 2 s), scope of FR-13/FR-16
  (FR-16 is now implemented).

## 3. How to get going

```bash
cd djangolight-main
.venv/Scripts/python.exe manage.py migrate
.venv/Scripts/python.exe manage.py seed_stadium
.venv/Scripts/python.exe manage.py seed_tickets      # prints the season ticket numbers
.venv/Scripts/python.exe manage.py createsuperuser   # staff = ticket counter, alerts, statistics
.venv/Scripts/python.exe manage.py runserver
.venv/Scripts/python.exe manage.py test -v 2
```

Demo flow:
- Buy a ticket at `/tickets/` (card `4111 1111 1111 1111`).
- Enter it at `/gate/`. One match is seeded as `active` for today.
- Scan it again to see the double-entry alert.
- Leave, then enter again.
- Season ticket numbers from `seed_tickets` are valid for the active Bundesliga match.
