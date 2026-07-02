# Idea: Crowd Song Requests over the deck's WiFi (v0.3+)

**Status:** idea captured 2026-07-02 — researching prior art + feasibility before design.

## Vision

The ESP32-S3 already has WiFi (currently only used for optional config, deck runs fine offline).
Reuse it as a **guest request system**:

1. Party guests connect to the deck's WiFi (SoftAP) — a captive portal pops up automatically.
2. The portal shows the DJ's **library / playlist** — browsable/searchable.
3. Guests **select tracks they'd like to hear later**.
4. Those requests surface to the DJ as a **special "Requests" playlist** they can pull from during the set.

Think "digital jukebox / request box" but built into the CDJ itself, feeding Traktor Pro 4.

## Why it fits this build

- The S3 (N16R8) already has WiFi/BLE and 8 MB PSRAM — enough to host a SoftAP + captive portal + small web server.
- The deck is already the crowd-facing object; putting the request UI "on the decks" is a natural, on-brand touch.
- Resilient-WiFi firmware design means this can't take down the USB-MIDI controller function.

## The hard part — three distinct problems

1. **Where does the library come from?** The S3 is a USB-MIDI controller; it does **not** hold the music or the Traktor collection. The real library lives in **Traktor on the DJ's laptop** (`collection.nml`). So either:
   - (A) export/sync a track list to the S3 (names only — S3 shows a searchable list), or
   - (B) a small **companion app on the DJ laptop** serves the library + owns requests, and the S3 just hosts the WiFi/portal (or even just advertises the app's URL), or
   - (C) hybrid: S3 hosts the portal, companion app is the backend.
2. **Guest UI at a party** — captive portal on the SoftAP, must handle many phones, be dead-simple, and resist spam/abuse (rate-limit, dedupe, profanity/troll filter, DJ approval).
3. **Requests → the DJ inside Traktor** — Traktor Pro has **no live "add to playlist" API**. Getting requests into a real Traktor **Requests playlist** likely needs a companion app writing to the collection/playlist, or the DJ reading a separate list on a phone/second screen. This is the crux to validate.

## Open research questions (feed the deep-research pass)

- Who has built crowd/audience **song-request systems** for DJs/parties/bars — commercial apps, open-source, and DIY (ESP32/Pi captive-portal jukeboxes)? What worked, what failed, what UX/anti-abuse patterns emerged?
- Does any DJ software (**Traktor**, rekordbox, Serato, VirtualDJ) expose crowd requests or an ingest path into a playlist? Any API, watched folder, or bridge (Bome, OSC, MIDI, `.nml`/`.m3u`)?
- Is a captive-portal request server on an **ESP32** realistic for party-scale concurrency, or does the backend belong on the laptop/Pi with the S3 as AP only?
- How is the **library exposed** without shipping the audio (metadata-only list, search, cover art)?
- Prior art on getting a list **back into Traktor** as a live "requests" crate/playlist.

## Provisional architecture options (to be confirmed by research)

| | Library source | Backend | S3 role | Requests → DJ |
|--|--|--|--|--|
| A | list synced to S3 | ESP32 | full (AP + web + store) | export `.m3u`/`.nml` the DJ imports |
| B | Traktor `collection.nml` | laptop companion app | AP + captive redirect only | app writes Traktor Requests playlist |
| C | companion app | laptop | AP + serve portal shell | app is source of truth |

**Leaning B/C** on first principles (library + Traktor integration live on the laptop), but the research pass will confirm and surface anything smarter.

---

## Research findings (deep-research pass, 2026-07-02)

*5 angles, 22 sources fetched, 25 claims adversarially verified (23 confirmed, 2 killed).*

**Verdict: feasible, but the ESP32-S3 must be the WiFi front-end ONLY, not the backend** — SoftAP
defaults to **4 simultaneous clients**, caps at **~10** (firmware macro `ESP_WIFI_MAX_CONN_NUM`, not
silicon); excess phones get deauth'd (`max connection, deauth!`). Party scale (dozens) needs the
request server on the **laptop** (already running Traktor); the S3 provides the SoftAP + captive
portal so guests still "join the deck's WiFi." → **Architecture C (hybrid) confirmed.**

### Prior art (concept validated; UX + anti-abuse already solved)
- **BeatTribe** (QR→browser, no app/account; every request reviewed, do-not-play lists, dedupe), **DJFY**
  (browser, personal QR, DJ accept/reject — but *paid bidding* model), **RequestBox** (native, drop+vote),
  **mubo** (vote-to-play), **Jukestar** (Spotify jukebox, fair-play interleaving — *can't play since Sept
  2022 Spotify API change; UX pattern only*).
- DIY analog: **`bschuetze/catalyst-jukebox`** — guests request from phone, pager-style ack, queue.
- Anti-abuse patterns to copy: **approval gate before the DJ sees it**, dedupe, do-not-play list,
  fair-play interleaving (no single guest hogs).

### Traktor ingest (the hardest link — no live "add to crate" API)
Three paths, each with a caveat:
1. **MIDI `Append` / `Add As Next To Preparation List`** — officially MIDI-mappable browser controls.
   Natural fit (deck is already USB-MIDI): MIDI-navigate to the track, append. Caveat: appends the
   track the browser cursor is *on* → also need MIDI scroll-to-select.
2. **M3U import** — Traktor imports `.m3u` natively; companion app writes a `Requests.m3u` to re-import.
   Caveat: not real-time; may need re-analysis (verified 2-1; early TP3 briefly broke M3U import).
3. **Direct `collection.nml` edit.**
   *No source found a supported live Traktor Pro 4 API — this is the least-certain part of the design.*

### Library exposure (solved + safe)
`collection.nml` = plain XML, metadata + file paths, **NO audio** → safe to expose. Export via
right-click → *Export Collection*. Parsers exist: `iond2v/NML-parser`, `wolkenarchitekt/traktor-nml-utils`,
SetFlow, TraCoConverter. **beets `web` plugin** = ready metadata-over-HTTP browse pattern.

### Captive portal caveat
Buildable on S3 with stock ESP-IDF (`Nordesems/esp-captive-portal` targets esp32s3, zero deps) but
**auto-popup is flaky** (no popup macOS post-Big Sur; some Android stall on IP w/ cellular; old devices
need manual browser open — modern OSes want RFC 8910 signaling). **Printed QR → fixed IP is the robust
fallback.**

### Next steps
1. Laptop **companion app**: watch exported `collection.nml`, serve searchable metadata list.
2. **Request queue**: approval gate + dedupe + do-not-play (mirror BeatTribe/DJFY).
3. **Ingest**: start M3U re-import (simplest) → evaluate MIDI Preparation-List append (most live).
4. **Test early** on real iOS/Android (SoftAP cap + portal onboarding).
5. Decide if S3 hosts anything beyond the AP.

### Open questions
- Any newer Traktor Pro 4 (2024+) scripting/OSC/plugin API for live crate manipulation? (none found)
- Can Traktor reload an updated `.nml`/M3U mid-set without disrupting loaded decks / full re-analysis?
- Real S3 concurrency/memory headroom as captive-redirect-only vs hosting the server — crossover point?
- For a *private party* (not a paid bar), is a full manual approval gate needed or does fair-play + dedupe suffice?

### Key sources
BeatTribe, DJFY, RequestBox, Jukestar, mubo (prior art) · NI Traktor manual (MIDI Preparation List) ·
`convert.guru`/`iond2v/NML-parser`/SetFlow (NML) · Espressif ESP-IDF WiFi docs + issue #10511 (SoftAP cap) ·
`CDFER/Captive-Portal-ESP32`, `Nordesems/esp-captive-portal` (portal) · `bschuetze/catalyst-jukebox` (DIY) · beets.io.

