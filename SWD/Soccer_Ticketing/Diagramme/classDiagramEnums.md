```mermaid

classDiagram
direction LR

class MatchStatus {
    <<enumeration>>
    PLANNED
    ACTIVE
    CONDUCTED
    CANCELLED
}
class CustomerType {
    <<enumeration>>
    REGULAR
    REDUCED
    FAN_CLUB_MEMBER
}
class PriceCategory {
    <<enumeration>>
    CAT_1
    CAT_2
    CAT_3
    FAN
}
class CompetitionType {
    <<enumeration>>
    BUNDESLIGA
    CUP
    EUROPA_LEAGUE
    CHAMPIONS_LEAGUE
    FRIENDLY
}
class PaymentMethod {
    <<enumeration>>
    CASH
    CARD
}
class UsageDirection {
    <<enumeration>>
    ENTRY
    EXIT
}
class ReservationStatus {
    <<enumeration>>
    ACTIVE
    CANCELLED
}
```
```
