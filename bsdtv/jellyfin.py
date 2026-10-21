"""Jellyfin client for the home screen: continue watching, latest additions, search.

Uses the Jellyfin HTTP API with an API key (Dashboard → API Keys). Only reads
data; playback happens in the Jellyfin web app or mpv.
"""

import json
import urllib.error
import urllib.parse
import urllib.request

TICKS_PER_SECOND = 10_000_000


class JellyfinError(Exception):
    pass


class Client:
    def __init__(self, url, api_key, user="", timeout=8):
        self.url = url.rstrip("/")
        self.api_key = api_key
        self.user = user
        self.timeout = timeout
        self._user_id = None

    # --- HTTP ------------------------------------------------------------

    def get(self, path, **params):
        query = urllib.parse.urlencode({k: v for k, v in params.items() if v is not None})
        request = urllib.request.Request(
            f"{self.url}{path}" + (f"?{query}" if query else ""),
            headers={"X-Emby-Token": self.api_key, "Accept": "application/json"},
        )
        try:
            with urllib.request.urlopen(request, timeout=self.timeout) as response:
                return json.load(response)
        except (urllib.error.URLError, OSError, ValueError) as exc:
            raise JellyfinError(f"{path}: {exc}") from exc

    def user_id(self):
        """Id of the configured user, or of the first visible user."""
        if self._user_id is None:
            users = self.get("/Users")
            if not isinstance(users, list) or not users:
                raise JellyfinError("no users on the server")
            wanted = self.user.lower()
            match = next((u for u in users if wanted and u.get("Name", "").lower() == wanted), None)
            if match is None and wanted:
                raise JellyfinError(f"user {self.user!r} not found")
            match = match or next((u for u in users if not u.get("Policy", {}).get("IsHidden")), users[0])
            self._user_id = match["Id"]
        return self._user_id

    # --- queries ---------------------------------------------------------

    FIELDS = "Overview,ProductionYear,OfficialRating,CommunityRating,RunTimeTicks"

    def resume(self, limit=12):
        data = self.get(f"/Users/{self.user_id()}/Items/Resume", Limit=limit, MediaTypes="Video",
                        Fields=self.FIELDS, EnableImageTypes="Primary,Backdrop,Thumb")
        return [card(i, self) for i in data.get("Items", [])]

    def latest(self, limit=16):
        data = self.get(f"/Users/{self.user_id()}/Items/Latest", Limit=limit,
                        IncludeItemTypes="Movie,Series,Episode", Fields=self.FIELDS)
        return [card(i, self) for i in (data if isinstance(data, list) else [])]

    def search(self, term, limit=20):
        data = self.get(f"/Users/{self.user_id()}/Items", SearchTerm=term, Recursive="true", Limit=limit,
                        IncludeItemTypes="Movie,Series,Episode", Fields=self.FIELDS)
        return [card(i, self) for i in data.get("Items", [])]

    # --- URLs ------------------------------------------------------------

    def image_url(self, item_id, kind, max_width):
        return f"{self.url}/Items/{item_id}/Images/{kind}?maxWidth={max_width}&quality=90"

    def web_url(self, item_id):
        return f"{self.url}/web/#/details?id={item_id}"

    def stream_url(self, item_id):
        return f"{self.url}/Videos/{item_id}/stream?static=true&api_key={urllib.parse.quote(self.api_key)}"


def card(raw, client):
    """Turn a Jellyfin item into what the home screen shows."""
    item_id = raw.get("Id", "")
    kind = raw.get("Type", "")
    tags = raw.get("ImageTags") or {}
    user = raw.get("UserData") or {}

    if kind == "Episode":
        title = raw.get("SeriesName") or raw.get("Name", "")
        season, episode = raw.get("ParentIndexNumber"), raw.get("IndexNumber")
        code = f"S{season}:E{episode} " if season is not None and episode is not None else ""
        subtitle = f"{code}{raw.get('Name', '')}".strip()
    else:
        title = raw.get("Name", "")
        subtitle = str(raw.get("ProductionYear") or "")

    # Wide picture for the card: the item's own thumb or backdrop, then the series',
    # then an episode still (episode Primary images are 16:9).
    if tags.get("Thumb"):
        wide = client.image_url(item_id, "Thumb", 640)
    elif raw.get("BackdropImageTags"):
        wide = client.image_url(item_id, "Backdrop", 640)
    elif raw.get("ParentThumbItemId"):
        wide = client.image_url(raw["ParentThumbItemId"], "Thumb", 640)
    elif tags.get("Primary"):
        wide = client.image_url(item_id, "Primary", 640)
    else:
        wide = ""

    if raw.get("BackdropImageTags"):
        backdrop = client.image_url(item_id, "Backdrop", 1920)
    elif raw.get("ParentBackdropItemId"):
        backdrop = client.image_url(raw["ParentBackdropItemId"], "Backdrop", 1920)
    else:
        backdrop = ""

    ticks = raw.get("RunTimeTicks") or 0
    position = user.get("PlaybackPositionTicks") or 0
    return {
        "mediaId": item_id,
        "type": kind,
        "title": title,
        "subtitle": subtitle,
        "overview": raw.get("Overview") or "",
        "year": raw.get("ProductionYear") or 0,
        "rating": raw.get("OfficialRating") or "",
        "minutes": int(ticks / TICKS_PER_SECOND / 60),
        "progress": min(1.0, position / ticks) if ticks else 0.0,
        "resumeSeconds": int(position / TICKS_PER_SECOND),
        "image": wide,
        "backdrop": backdrop,
    }


def from_config(cfg):
    """A client when Jellyfin is configured with a URL and an API key, else None."""
    opts = cfg.get("jellyfin") or {}
    url = opts.get("url") or (cfg.get("vars") or {}).get("jellyfin_url")
    if not url or not opts.get("api_key"):
        return None
    return Client(url, opts["api_key"], opts.get("user", ""))


def slides(items, limit=5):
    """Banner slides for items that have a backdrop."""
    out = []
    for item in items:
        if item["backdrop"] and all(s["mediaId"] != item["mediaId"] for s in out):
            out.append({"appId": "jellyfin", "name": "Jellyfin", "title": item["title"],
                        "tagline": item["overview"] or item["subtitle"], "image": item["backdrop"],
                        "color": "#101018", "textColor": "#aa5cc3", "icon": "", "caption": "",
                        "mediaId": item["mediaId"]})
        if len(out) >= limit:
            break
    return out
