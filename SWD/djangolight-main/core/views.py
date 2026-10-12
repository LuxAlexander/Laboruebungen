import logging

from django.db import DatabaseError, connections
from django.http import HttpRequest, JsonResponse

logger = logging.getLogger(__name__)


def _database_answers(alias: str) -> bool:
    try:
        # the context manager closes the cursor; fetchone() proves a result came back, independent of what
        # the backend's execute() returns
        with connections[alias].cursor() as cursor:
            cursor.execute("SELECT 1")
            return cursor.fetchone() == (1,)
    except DatabaseError as e:
        logger.exception("health check: database %r unreachable: %s", alias, e)
        return False


def health_check(request: HttpRequest) -> JsonResponse:
    """Probe for the container healthcheck: 200 when every database answers, otherwise 503."""
    healthy = all(_database_answers(alias) for alias in connections)
    return JsonResponse({"status": "ok" if healthy else "unhealthy"}, status=200 if healthy else 503)
