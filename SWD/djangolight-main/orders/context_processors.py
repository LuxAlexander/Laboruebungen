from orders.basket import Basket


def basket(request) -> dict:
    """Makes the basket item count available to every template (navigation badge)."""
    session = getattr(request, "session", None)
    return {"basket_count": len(Basket.load(session)) if session is not None else 0}
