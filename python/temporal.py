import mgp
import pytz
import datetime


@mgp.read_proc
def format(
    temporal: mgp.Any,
    format: str = "ISO_DATE",
) -> mgp.Record(formatted=str):

    if (not (isinstance(temporal, datetime.datetime) or isinstance(temporal, datetime.date) or isinstance(temporal, datetime.time) or isinstance(temporal, datetime.timedelta))):
        return mgp.Record(formatted=str(temporal))

    if (format == "ISO_DATE" and (isinstance(temporal, datetime.datetime) or isinstance(temporal, datetime.date))):
        return mgp.Record(formatted=temporal.isoformat())

    if ("%z" in format or "%Z" in format):
        raise Exception("We are working with UTC zone only.")

    if (isinstance(temporal, datetime.timedelta)):
        temporal = datetime.datetime(1900,1,1) + temporal

    return mgp.Record(formatted=temporal.strftime(format))
