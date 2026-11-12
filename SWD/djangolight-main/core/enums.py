from django.db import models

"""
Enumerations shared by all apps. They are Django TextChoices, so models can use them as `choices`
and the domain records can carry the same values.
"""


class MatchStatus(models.TextChoices):
    PLANNED = "planned", "Planned"
    ACTIVE = "active", "Active"  # spectator entry is permitted only in this status
    CONDUCTED = "conducted", "Conducted"
    CANCELLED = "cancelled", "Cancelled"


class CompetitionType(models.TextChoices):
    LEAGUE = "league", "Bundesliga"
    CUP = "cup", "Cup"
    EUROPA_LEAGUE = "europa_league", "UEFA Europa League"
    CHAMPIONS_LEAGUE = "champions_league", "UEFA Champions League"
    FRIENDLY = "friendly", "Friendly"


class PriceCategory(models.TextChoices):
    CATEGORY_1 = "cat1", "Category 1"
    CATEGORY_2 = "cat2", "Category 2"
    CATEGORY_3 = "cat3", "Category 3"
    FAN = "fan", "Fan sector"


class TicketType(models.TextChoices):
    SINGLE = "single", "Single-match ticket"
    SEASON = "season", "Season ticket"


class TicketStatus(models.TextChoices):
    SOLD = "sold", "Sold"
    RESERVED = "reserved", "Reserved"
    CANCELLED = "cancelled", "Cancelled"


class SalesChannel(models.TextChoices):
    ONLINE = "online", "Online"
    COUNTER = "counter", "Staffed ticket counter"


class PaymentMethod(models.TextChoices):
    CASH = "cash", "Cash"  # staffed counters only
    CREDIT_CARD = "card", "Credit card"


class AccessAction(models.TextChoices):
    ENTER = "enter", "Enter"
    LEAVE = "leave", "Leave"


class DenyReason(models.TextChoices):
    UNKNOWN_TICKET = "unknown_ticket", "Unknown ticket number"
    NO_ACTIVE_MATCH = "no_active_match", "No active match"
    NOT_VALID_FOR_MATCH = "not_valid_for_match", "Ticket not valid for this match"
    ALREADY_INSIDE = "already_inside", "Ticket already used for entry"
    NOT_INSIDE = "not_inside", "Leave without entry"
    TICKET_CANCELLED = "ticket_cancelled", "Ticket cancelled"
