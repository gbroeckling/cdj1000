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
