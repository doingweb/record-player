NFC Record Player
=================

The magic inside the box that plays records without taking them out.

TODO
----

- [X] Better libraries for managing HTTP requests and JSON?
- [X] Proper OAuth flow instead of relying on the temporary bearer token
- [X] Refresh access token and retry if it's expired
- [ ] Read album ID from NFC tag
- [ ] If it's already playing, don't restart
- [ ] Show errors via web
  - [ ] Last command/response from Spotify (+ date/time/etc.)
- [ ] Move more error handling to `recordPlayer`
  - [ ] WiFi connection
  - [ ] TLS
- [ ] Support changing devices via web (dropdown?)
- [ ] More web-based configuration in general?
