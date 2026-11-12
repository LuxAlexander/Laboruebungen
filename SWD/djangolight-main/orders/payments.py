import uuid

from core.domain import PaymentFailed, PaymentRequest
from core.enums import PaymentMethod, SalesChannel

# a test card number that the simulated provider always declines
DECLINED_CARD = "4000000000000002"


def _luhn_valid(number: str) -> bool:
    digits = [int(d) for d in number]
    checksum = 0
    for i, d in enumerate(reversed(digits)):
        if i % 2 == 1:
            d = d * 2 - 9 if d > 4 else d * 2
        checksum += d
    return checksum % 10 == 0


class SimulatedPaymentService:
    """
    Adapter for core.interfaces.Payments. No real payment provider is connected (non-goal):
    cash is accepted at staffed counters only, a credit card must be 13-19 digits and pass the Luhn check,
    and DECLINED_CARD is always declined (to demonstrate the rollback of a failed purchase).
    """

    def pay(self, request: PaymentRequest) -> str:
        if request.method == PaymentMethod.CASH:
            if request.channel != SalesChannel.COUNTER:
                raise PaymentFailed("cash is accepted at staffed ticket counters only")
            return f"CASH-{uuid.uuid4().hex[:12].upper()}"

        card = request.card_number.replace(" ", "")
        if not (card.isdigit() and 13 <= len(card) <= 19 and _luhn_valid(card)):
            raise PaymentFailed("invalid credit card number")
        if card == DECLINED_CARD:
            raise PaymentFailed("card declined")
        return f"CARD-{uuid.uuid4().hex[:12].upper()}"
