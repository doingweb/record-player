NFC Record Player
=================

The magic inside the box that plays records without taking them out.

TODO
----

- [X] Better libraries for managing HTTP requests and JSON?
- [X] Proper OAuth flow instead of relying on the temporary bearer token
- [X] Refresh access token and retry if it's expired
- [X] Show errors via web
  - [X] Last command/response from Spotify (+ date/time/etc.)
- [X] Read album ID from NFC tag
- [X] More emojis in log messages
- [X] If it's already playing, don't restart
- [ ] Move config to /data/config.json (see https://arduinojson.org/v6/example/config/)
  - [X] Write (refresh) token info there so maybe we don't have to re-authorize every time we unplug it
- [ ] Support changing devices via web (dropdown?)
- [ ] More web-based configuration in general?
